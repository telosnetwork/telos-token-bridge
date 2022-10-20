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
        // Check transfer is ok for bridging
        check(from != get_self(), "Sender is this contract");
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

        // Get each member of the Pair pairs[] array's antelope_account and compare to get the EVM address
        std::string pair_evm_address = "";
        vector<uint8_t> pair_evm_address_bs;
        for(uint256_t i = 0; i < pair_array_length->value; i=i+1){
            // Get the account name string from EVM Storage, this works only for < 32bytes string which any EOSIO name should be (< 13 chars)
            const auto account_name_checksum = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 4, 10, i));
            eosio::name account_name = parseNameFromStorage(account_name_checksum->value);
            if(account_name.value  == get_first_receiver().value){
                const auto pair_active = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 0, 10, i));
                check(pair_active->value == uint256_t(1), "This token's pair is paused");
                const auto pair_evm_address_stored = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 2, 10, i));
                pair_evm_address_bs = intx::to_byte_string(pair_evm_address_stored->value);
                reverse(pair_evm_address_bs.begin(),pair_evm_address_bs.end());
                pair_evm_address_bs.resize(20);
                reverse(pair_evm_address_bs.begin(),pair_evm_address_bs.end());
                // TODO: get EVM decimals to use below for amount calculation
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
        auto receiver_ba = pad160(eosio::checksum160( toBin((memo)))).extract_as_byte_array();
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

        // Todo: clean out old processed refunds

        // Define EVM Account State table with EVM bridge contract scope
        account_state_table bridge_account_states(EVM_SYSTEM_CONTRACT, conf.evm_bridge_scope);
        auto bridge_account_states_bykey = bridge_account_states.get_index<"bykey"_n>();

        // Get array slot to find Refund refunds[] array length
        auto refund_storage_key = toChecksum256(uint256_t(STORAGE_BRIDGE_REFUND_INDEX));
        auto refund_array_length = bridge_account_states_bykey.require_find(refund_storage_key, "No refunds found");
        auto refund_array_slot = checksum256ToValue(keccak_256(refund_storage_key.extract_as_byte_array()));

        // Prepare address for callback
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(), evm_contract.begin(), evm_contract.end());

        auto fnsig = toBin(EVM_REFUND_CALLBACK_SIGNATURE);

        for(uint256_t i = 0; i < refund_array_length->value; i=i+1){
            const uint256_t position = refund_array_length->value - i;
            // TODO: parse EVM Bridge refund
            const auto refund_id = bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 0, 8, position));
            const auto recipient = bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 2, 8, position));
            const auto token = bridge_account_states_bykey.find(getArrayMemberSlot(refund_array_slot, 3, 8, position));

            // TODO: unlock & send tokens to recipient
            // TODO: add refund to processed (???)

            // TODO: setup success callback call so refund get deleted on EVM
            std::vector<uint8_t> data;
            data.insert(data.end(), fnsig.begin(), fnsig.end());

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

        // Todo: clean out old processed requests
        requests_table requests(get_self(), get_self().value);

        // Define EVM Account State table with EVM bridge contract scope
        account_state_table bridge_account_states(EVM_SYSTEM_CONTRACT, conf.evm_bridge_scope);
        auto bridge_account_states_bykey = bridge_account_states.get_index<"bykey"_n>();

        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_BRIDGE_REQUEST_INDEX));
        auto request_array_length = bridge_account_states_bykey.require_find(request_storage_key, "No requests found");
        auto request_array_slot = checksum256ToValue(keccak_256(request_storage_key.extract_as_byte_array()));

        // Prepare address for callback
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> evm_to;
        evm_to.insert(evm_to.end(), evm_contract.begin(), evm_contract.end());
        auto fnsig = toBin(EVM_SUCCESS_CALLBACK_SIGNATURE);

        for(uint256_t i = 0; i < request_array_length->value; i=i+1){
            const auto call_id_checksum = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 0, 8, i));
            const uint256_t call_id = (call_id_checksum != bridge_account_states_bykey.end()) ? call_id_checksum->value : uint256_t(0); // Needed because row is not set at all if the value is 0
            const vector<uint8_t> call_id_bs = intx::to_byte_string(call_id);
            const eosio::name token_account_name = parseNameFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 2, 8, i))->value);
            const eosio::name receiver = parseNameFromStorage(bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 3, 8, i))->value);
            const auto sender_address_checksum = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 3, 8, i));
            std::string sender_address = "";
            const std::string memo = "Sent from tEVM via the TokenBridge by " + sender_address;
            const uint256_t amount = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 4, 8, i))->value;
            const std::string quantity = decodeHex(bin2hex(intx::to_byte_string(amount))) + " " + token_account_name.to_string();

            // Check request not already processing
            auto requests_by_call_id = requests.get_index<"bycallid"_n>();
            auto exists = requests_by_call_id.find(toChecksum256(call_id));
            if(exists != requests_by_call_id.end()){
                continue;
            }

            // Add request to processed
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

            // TODO: setup success callback call so request get deleted on EVM
            std::vector<uint8_t> data;
            data.insert(data.end(), fnsig.begin(), fnsig.end());
            data.insert(data.end(), fnsig.begin(), fnsig.end());
            data.insert(data.end(), call_id_bs.begin(), call_id_bs.end());

            // Print it
            auto rlp_encoded = rlp::encode(evm_account->nonce, evm_conf.gas_price, BASE_GAS, evm_to, uint256_t(0), data, CURRENT_CHAIN_ID, 0, 0);
            std::vector<uint8_t> raw;
            raw.insert(raw.end(), std::begin(rlp_encoded), std::end(rlp_encoded));
            print(bin2hex(raw));
            // Send success callback call back to EVM using eosio.evm
            //action(
            //   permission_level {get_self(), "active"_n},
            //   EVM_SYSTEM_CONTRACT,
            //   "raw"_n,
            //   std::make_tuple(get_self(), rlp::encode(evm_account->nonce, evm_conf.gas_price, BASE_GAS, evm_to, uint256_t(0), data, CURRENT_CHAIN_ID, 0, 0),  false, std::optional<eosio::checksum160>(evm_account->address))
            //).send();
        }
    };

    // Verify token & sign EVM registration request
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
        auto pair_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_PAIR_INDEX));
        auto pair_array_length = register_account_states_bykey.find(pair_storage_key);
        auto pair_array_slot = checksum256ToValue(keccak_256(pair_storage_key.extract_as_byte_array()));
        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_REQUEST_INDEX));
        auto request_array_length = register_account_states_bykey.find(request_storage_key);
        auto request_array_slot = checksum256ToValue(keccak_256(request_storage_key.extract_as_byte_array()));

        // Check token doesn't already exist in EVM Register
        // Get each member of the Token tokens[] array's antelope_account and compare
        for(uint256_t i = 0; i < pair_array_length->value; i=i+1){
            const auto symbol_name = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 7, 10, pair_array_length->value - i));
            // Todo: check antelope account too ?
            print(name(decodeHex(bin2hex(intx::to_byte_string(symbol_name->value)))).to_string());
            if(name(decodeHex(bin2hex(intx::to_byte_string(symbol_name->value)))).value == symbol.code().raw()){
                check(false, "The token is already registered");
            }
        }
        // Get each member of the Request requests[] array's antelope_account and compare.
        for(uint256_t k = 0; k < request_array_length->value; k=k+1){
            const auto symbol_name = register_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 8, 11, request_array_length->value - k));
            // Todo: check antelope account too ?
            if(name(decodeHex(bin2hex(intx::to_byte_string(symbol_name->value)))).value == symbol.code().raw()){
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
