// @author Thomas Cuvillier
// @organization Telos Foundation
// @contract token.brdg
// @version v1.0

#include "../include/token.brdg.hpp";

namespace evm_bridge
{
    //======================== Admin actions ==========================
    // Initialize the contract
    [[eosio::action]]
    void tokenbridge::init(eosio::checksum160 bridge_address, eosio::checksum160 register_address, std::string version, eosio::name admin){
        // Authenticate
        require_auth(get_self());

        // Validate
        check(!config_bridge.exists(), "contract already initialized");
        check(is_account(admin), "initial admin account doesn't exist");

        // Initialize
        auto stored = config_bridge.get_or_create(get_self(), config_row);

        stored.version              = version;
        stored.admin                = admin;
        stored.evm_bridge_address   = bridge_address;
        stored.evm_register_address = register_address;

        // Get the scope
        account_table accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaddress = accounts.get_index<"byaddress"_n>();
        auto account_bridge = accounts_byaddress.find(pad160(bridge_address));
        auto account_register = accounts_byaddress.find(pad160(register_address));

        stored.evm_bridge_scope = (account_bridge != accounts_byaddress.end()) ? account_bridge->index : 0;
        stored.evm_register_scope = (account_register != accounts_byaddress.end()) ? account_register->index : 0;

        config_bridge.set(stored, get_self());
    };

    // Set the contract version
    [[eosio::action]]
    void tokenbridge::setversion(std::string new_version){
        // Authenticate
        require_auth(config_bridge.get().admin);

        auto stored = config_bridge.get();
        stored.version = new_version;

        // Modify
        config_bridge.set(stored, get_self());
    };

    // Set the  evm bridge & evm register addresses
    [[eosio::action]]
    void tokenbridge::setevmctc(eosio::checksum160 bridge_address, eosio::checksum160 register_address){
        // Authenticate
        require_auth(config_bridge.get().admin);

        // Get the relevant accounts for eosio.evm accountstates table
        account_table accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaddress = accounts.get_index<"byaddress"_n>();
        auto account_bridge = accounts_byaddress.find(pad160(bridge_address));
        auto account_register = accounts_byaddress.find(pad160(register_address));

        // Save
        auto stored = config_bridge.get();
        stored.evm_bridge_address = bridge_address;
        stored.evm_bridge_scope = (account_bridge != accounts_byaddress.end()) ? account_bridge->index : 0;
        stored.evm_register_address = register_address;
        stored.evm_register_scope = (account_register != accounts_byaddress.end()) ? account_register->index : 0;

        config_bridge.set(stored, get_self());
    };

    // Set new contract admin
    [[eosio::action]]
    void tokenbridge::setadmin(eosio::name new_admin){
        // Authenticate
        require_auth(config_bridge.get().admin);

        // Check account exists
        check(is_account(new_admin), "New admin account does not exist, please verify the account name provided");

        auto stored = config_bridge.get();
        stored.admin = new_admin;
        // Modify
        config_bridge.set(stored, get_self());
    };

    //======================== Token Bridge actions ========================
    // Trustless bridge to EVM
    [[eosio::on_notify("*::transfer")]]
    void tokenbridge::bridge(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo)
    {
        if(from == get_self()) return; // Return so we don't stop sending from this contract when bridging from EVM
        check(to == get_self(), "Recipient is not this contract");
        check(memo.length() == 42, "Memo needs to contain the 42 character EVM recipient address");
        // Check amount
        uint256_t amount = uint256_t(quantity.amount);
        check(amount >= 1, "Minimum amount is not reached");

        // Open config singleton
        auto conf = config_bridge.get();
        auto evm_conf = config.get();

        // Find the EVM account of this contract
        account_table _accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaccount = _accounts.get_index<"byaccount"_n>();
        auto evm_account = accounts_byaccount.require_find(get_self().value, "EVM account not found for token.brdg");

        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find Pair pairs[] array length
        auto pair_storage_key = toChecksum256(STORAGE_REGISTER_PAIR_INDEX);
        auto pair_array_length = register_account_states_bykey.require_find(pair_storage_key, "No pairs have been found in the EVM register");
        auto pair_array_slot = checksum256ToValue(keccak_256(pair_storage_key.extract_as_byte_array()));
        auto pair_property_count = 10;

        // Get each member of the Pair pairs[] array's antelope_account and compare to get the EVM address
        std::string pair_evm_address = "";
        vector<uint8_t> pair_evm_address_bs;
        for(uint256_t i = 0; i < pair_array_length->value; i=i+1){
            // Get the account name string from EVM Storage, this works only for < 32bytes string which any EOSIO name should be (< 13 chars)
            const auto account_name_checksum = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 6, pair_property_count, i));
            eosio::name account_name = parseNameFromStorage(account_name_checksum->value);
            if(account_name.value  == get_first_receiver().value){
                const auto pair_active = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 0, pair_property_count, i));
                check(pair_active->value == uint256_t(1), "This token's pair is paused");
                const auto pair_evm_address_stored = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 2, pair_property_count, i));
                pair_evm_address_bs = parseAddressFromStorage(pair_evm_address_stored->value);
                const auto pair_evm_decimals = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 3, pair_property_count, i))->value;
            }
        }
        check(pair_evm_address_bs.size() > 0, "This token has no pair registered on this bridge");

        // Prepare address for EVM Bridge call
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> evm_to;
        evm_to.insert(evm_to.end(),  evm_contract.begin(), evm_contract.end());

        // Prepare EVM function signature & arguments
        std::vector<uint8_t> data;
        auto fnsig = checksum256ToValue(eosio::checksum256(toBin(EVM_BRIDGE_SIGNATURE)));
        vector<uint8_t> fnsig_bs = intx::to_byte_string(fnsig);
        fnsig_bs.resize(16);
        data.insert(data.end(), fnsig_bs.begin(), fnsig_bs.end());
        data.insert(data.end(), pair_evm_address_bs.begin(), pair_evm_address_bs.end());

        // Receiver EVM address from memo
        memo.replace(0, 2, ""); // remove the Ox
        auto receiver_ba = pad160(eosio::checksum160( toBin(memo))).extract_as_byte_array();
        std::vector<uint8_t> receiver(receiver_ba.begin(), receiver_ba.end());
        receiver = pad(receiver, 32, true);
        data.insert(data.end(),  receiver.begin(), receiver.end());

        // Amount
        vector<uint8_t> amount_bs = pad(intx::to_byte_string(amount * pow (10, 18)), 32, true);
        data.insert(data.end(),  amount_bs.begin(), amount_bs.end());

        // Sender
        std::string sender = from.to_string();
        insertElementPositions(&data, 128); // Our string position
        insertString(&data, sender, sender.length());

        // call TokenBridge.bridgeTo(address token, address receiver, uint amount) on EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(evm_account->nonce, evm_conf.gas_price, BASE_GAS, evm_to, uint256_t(0), data, CURRENT_CHAIN_ID, 0, 0),  false, std::optional<eosio::checksum160>(evm_account->address))
        ).send();
    };

    [[eosio::action]]
    void tokenbridge::refundnotify()
    {
        // Open config singletons
        auto conf = config_bridge.get();
        auto evm_conf = config.get();

        // Find the EVM account of this contract
        account_table _accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaccount = _accounts.get_index<"byaccount"_n>();
        auto account = accounts_byaccount.require_find(get_self().value, "Account not found");

        // Clean out old processed refunds
        refunds_table refunds(get_self(), get_self().value);
        auto refunds_by_timestamp = refunds.get_index<"timestamp"_n>();
        auto upper = refunds_by_timestamp.upper_bound(current_time_point().sec_since_epoch() - 60); // remove 60s so we get only requests that are at least 1mn old
        uint64_t count = 15; // max 15 refunds
        for(auto itr = refunds_by_timestamp.begin(); count > 0 && itr != upper; count--) {
            itr = refunds_by_timestamp.erase(itr);
        }

        // Define EVM Account State table with EVM bridge contract scope
        account_state_table bridge_account_states(EVM_SYSTEM_CONTRACT, conf.evm_bridge_scope);
        auto bridge_account_states_bykey = bridge_account_states.get_index<"bykey"_n>();

        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find Refund refunds[] array length
        auto refund_storage_key = toChecksum256(STORAGE_BRIDGE_REFUND_INDEX);
        auto refund_array_length = bridge_account_states_bykey.require_find(refund_storage_key, "No refunds found");
        auto refund_array_slot = checksum256ToValue(keccak_256(refund_storage_key.extract_as_byte_array()));
        uint8_t refund_property_count = 6;

        // Prepare address for callback
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(), evm_contract.begin(), evm_contract.end());

        const auto fnsig = toBin(EVM_REFUND_CALLBACK_SIGNATURE);
        const std::string memo = "Bridge refund";

        uint64_t max_refunds = 2;
        for(uint64_t i = 0; i < refund_array_length->value && i < max_refunds; i++){
            const auto refund_id_checksum = bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 0, refund_property_count, i));
            const uint256_t refund_id = (refund_id_checksum != bridge_account_states_bykey.end()) ? refund_id_checksum->value : uint256_t(0); // Needed because row is not set at all if the value is 0
            const auto refund_id_bs = pad(intx::to_byte_string(refund_id), 16, true);
            const eosio::name receiver = parseNameFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 4, refund_property_count, i))->value);
            const eosio::name token_account_name = parseNameFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 2, refund_property_count, i))->value);
            const eosio::symbol_code antelope_symbol = parseSymbolCodeFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 3, refund_property_count, i))->value);
            const uint256_t evm_decimals = bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 5, refund_property_count, i))->value;

            // Get token from token stat table (and not EVM Register, in case the token issuer changes precision)
            eosio_tokens token_row(token_account_name, antelope_symbol.raw());
            const auto antelope_token = token_row.require_find(antelope_symbol.raw(), "Token not found. Make sure the symbol is correct.");
            // Todo: what happens if there is wei > precision ??? Maybe make sure on EVM side that the max precision for refunds matches antelope ? This is not finished v
            // Todo: make sure amount isn't > uint64_t max on EVM
            const double exponent = ((static_cast<uint64_t>(evm_decimals) - antelope_token->supply.symbol.precision()) * 1.0);
            const uint256_t amount = bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 1, refund_property_count, i))->value / pow(10.0, exponent);
            const uint64_t amount_64 = static_cast<uint64_t>(amount);
            const eosio::asset quantity = asset(amount_64, antelope_token->supply.symbol);

            // Check refund not already being processed
            auto refunds_by_call_id = refunds.get_index<"callid"_n>();
            auto exists = refunds_by_call_id.find(toChecksum256(refund_id));
            if(exists != refunds_by_call_id.end()){
                continue;
            }

            // Add refund
            refunds.emplace(get_self(), [&](auto& r) {
                r.refund_id = refunds.available_primary_key();
                r.call_id = toChecksum256(refund_id);
                r.timestamp = current_time_point();
            });

            // Send tokens to receiver
            action(
                permission_level{ get_self(), "active"_n },
                    token_account_name,
                    "transfer"_n,
                    std::make_tuple(get_self(), receiver, quantity, memo)
            ).send();

            std::vector<uint8_t> data;
            data.insert(data.end(), fnsig.begin(), fnsig.end());
            data.insert(data.end(), refund_id_bs.begin(), refund_id_bs.end());

            // Send refundSuccessful call to EVM using eosio.evm
            action(
                permission_level {get_self(), "active"_n},
                EVM_SYSTEM_CONTRACT,
                "raw"_n,
                std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
            ).send();
        }

    }
    // Trustless bridge from EVM
    [[eosio::action]]
    void tokenbridge::reqnotify()
    {
        // Open config singleton
        auto conf = config_bridge.get();
        auto evm_conf = config.get();

        // Find the EVM account of this contract
        account_table _accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaccount = _accounts.get_index<"byaccount"_n>();
        auto evm_account = accounts_byaccount.require_find(get_self().value, "EVM account not found for token.brdg");

        // Erase old requests
        requests_table requests(get_self(), get_self().value);
        auto requests_by_timestamp = requests.get_index<"timestamp"_n>();
        auto upper = requests_by_timestamp.upper_bound(current_time_point().sec_since_epoch() - 60); // remove 60s so we get only requests that are at least 1mn old
        uint64_t count = 15; // max 15 requests
        for(auto itr = requests_by_timestamp.begin(); count > 0 && itr != upper; count--) {
            itr = requests_by_timestamp.erase(itr);
        }

        // Define EVM Account State table with EVM bridge contract scope
        account_state_table bridge_account_states(EVM_SYSTEM_CONTRACT, conf.evm_bridge_scope);
        auto bridge_account_states_bykey = bridge_account_states.get_index<"bykey"_n>();

        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find the TokenBridge Request[] requests array length
        auto request_storage_key = toChecksum256(STORAGE_BRIDGE_REQUEST_INDEX);
        auto request_array_length = bridge_account_states_bykey.require_find(request_storage_key, "No requests found");
        auto request_array_slot = checksum256ToValue(keccak_256(request_storage_key.extract_as_byte_array()));
        uint8_t request_property_count = 8;

        // Prepare address & function signature for callback
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> evm_to;
        evm_to.insert(evm_to.end(), evm_contract.begin(), evm_contract.end());
        auto fnsig = toBin(EVM_SUCCESS_CALLBACK_SIGNATURE);

        uint64_t max_requests = 2;
        // Loop over the requests
        for(uint64_t i = 0; i < request_array_length->value && i < max_requests; i++){
            const auto call_id_checksum = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 0, request_property_count, i));
            const uint256_t call_id = (call_id_checksum != bridge_account_states_bykey.end()) ? call_id_checksum->value : uint256_t(0); // Needed because row is not set at all if the value is 0
            const vector<uint8_t> call_id_bs = pad(intx::to_byte_string(call_id), 16, true);
            const eosio::name token_account_name = parseNameFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 4, request_property_count, i))->value);
            const uint256_t evm_decimals = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 7, request_property_count, i))->value;
            const eosio::name receiver = parseNameFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 6, request_property_count, i))->value);
            const eosio::symbol_code antelope_symbol = parseSymbolCodeFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 5, request_property_count, i))->value);
            const auto sender_address_checksum = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 1, request_property_count, i));
            std::string sender_address = bin2hex(parseAddressFromStorage(sender_address_checksum->value));
            const std::string memo = "Sent from tEVM by 0x" + sender_address;

            // Get token from token stat table (and not EVM Register, in case the token issuer changes precision)
            eosio_tokens token_row(token_account_name, antelope_symbol.raw());
            auto antelope_token = token_row.require_find(antelope_symbol.raw(), "Token not found. Make sure the symbol is correct.");
            // Todo: what happens if there is wei > precision ??? Maybe make sure on EVM side that the max precision for bridging matches antelope ? This is not finished v
            // Todo: make sure amount isn't > uint64_t max on EVM
            const double exponent = ((static_cast<uint64_t>(evm_decimals) - antelope_token->supply.symbol.precision()) * 1.0);
            const uint256_t amount = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 2, request_property_count, i))->value / pow(10.0, exponent);
            uint64_t amount_64 = static_cast<uint64_t>(amount);
            const eosio::asset quantity = asset(amount_64, antelope_token->supply.symbol);

            // Check request not already being processed
            auto requests_by_call_id = requests.get_index<"callid"_n>();
            auto exists = requests_by_call_id.find(toChecksum256(call_id));
            if(exists != requests_by_call_id.end()){
                continue;
            }

            // Add request
            requests.emplace(get_self(), [&](auto& r) {
                r.request_id = requests.available_primary_key();
                r.call_id = toChecksum256(call_id);
                r.timestamp = current_time_point();
            });

            // Send tokens to receiver
            action(
                permission_level{ get_self(), "active"_n },
                    token_account_name,
                    "transfer"_n,
                    std::make_tuple(get_self(), receiver, quantity, memo)
            ).send();

            // Setup success callback call so request get deleted on tEVM
            std::vector<uint8_t> data;
            data.insert(data.end(), fnsig.begin(), fnsig.end());
            data.insert(data.end(), call_id_bs.begin(), call_id_bs.end());

            // Call success callback on tEVM using eosio.evm
            action(
               permission_level {get_self(), "active"_n},
               EVM_SYSTEM_CONTRACT,
               "raw"_n,
               std::make_tuple(get_self(), rlp::encode(evm_account->nonce + i, evm_conf.gas_price, BASE_GAS, evm_to, uint256_t(0), data, CURRENT_CHAIN_ID, 0, 0),  false, std::optional<eosio::checksum160>(evm_account->address))
            ).send();
        }
    };

    // Verify token & sign tEVM registration request
    // Todo:: replace uint64_t by uint256_t request_id
    [[eosio::action]]
    void tokenbridge::signregpair(eosio::checksum160 evm_address, eosio::name account, eosio::symbol symbol, uint64_t request_id)
    {

        // Open config singleton
        auto conf = config_bridge.get();
        auto evm_conf = config.get();

        // Find the EVM account of this contract
        account_table _accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaccount = _accounts.get_index<"byaccount"_n>();
        auto evm_account = accounts_byaccount.require_find(get_self().value, "No EVM account found for token.brdg");

        // Get token info from eosio.token stat table
        eosio_tokens token_row(account, symbol.code().raw());
        auto token = token_row.require_find(symbol.code().raw(), "Token not found. Make sure the symbol is correct.");

        // Check auth
        require_auth(token->issuer);

        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find Token tokens[] array length
        auto pair_storage_key = toChecksum256(STORAGE_REGISTER_PAIR_INDEX);
        auto pair_array_length = register_account_states_bykey.find(pair_storage_key);
        auto pair_array_slot = checksum256ToValue(keccak_256(pair_storage_key.extract_as_byte_array()));

        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(STORAGE_REGISTER_REQUEST_INDEX);
        auto request_array_length = register_account_states_bykey.find(request_storage_key);
        auto request_array_slot = checksum256ToValue(keccak_256(request_storage_key.extract_as_byte_array()));

        // Check token doesn't already exist in EVM Register
        // Get each member of the Pair pairs[] array's antelope_account and compare
        for(uint256_t i = 0; i < pair_array_length->value; i=i+1){
            const auto symbol_name = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 5, 10, i));
            if(parseSymbolCodeFromStorage(symbol_name->value).raw() == symbol.code().raw()){
                check(false, "The token is already registered");
            }
        }
        // Get each member of the Request requests[] array's antelope_account and compare.
        for(uint256_t k = 0; k < request_array_length->value; k=k+1){
            const auto symbol_name = register_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 8, 11, k));
            if(parseSymbolCodeFromStorage(symbol_name->value).raw() == symbol.code().raw()){
                check(false, "The token is already awaiting approval");
            }
        }

        // Prepare EVM contract address
        auto evm_contract = conf.evm_register_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(),  evm_contract.begin(), evm_contract.end());

        // Prepare Solidity function call (function signature + arguments)
        std::vector<uint8_t> data;

        // Add function signature
        auto fnsig = toBin(EVM_SIGN_REGISTRATION_SIGNATURE);
        data.insert(data.end(), fnsig.begin(), fnsig.end());

        // Add integers argument
        std::vector<uint8_t> decimals_bs = pad(intx::to_byte_string(uint256_t(symbol.precision())), 32, true);
        std::vector<uint8_t> request_id_bs = pad(intx::to_byte_string(request_id), 16, true);
        data.insert(data.end(), request_id_bs.begin(), request_id_bs.end());
        data.insert(data.end(), decimals_bs.begin(), decimals_bs.end());

        // Add strings argument
        insertElementPositions(&data, 160, 224, 288); // Our 3 strings positions
        insertString(&data, account.to_string(), account.to_string().length());
        insertString(&data, token->issuer.to_string(), token->issuer.to_string().length());
        insertString(&data, symbol.code().to_string(), symbol.code().to_string().length());

        // Send signRegistrationRequest call to EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(evm_account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, CURRENT_CHAIN_ID, 0, 0),  false, std::optional<eosio::checksum160>(evm_account->address))
        ).send();
    };
}
