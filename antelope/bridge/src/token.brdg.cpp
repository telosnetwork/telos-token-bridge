// @author Thomas Cuvillier
// @organization Telos Foundation
// @contract token.brdg
// @version v1.0

#include "../include/token.brdg.hpp";

namespace evm_bridge
{
    //======================== Admin actions ==========================
    // Initialize the contract
    ACTION tokenbridge::init(eosio::checksum160 bridge_address, eosio::checksum160 register_address, std::string version, eosio::name admin){
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
    ACTION tokenbridge::setversion(std::string new_version){
        // Authenticate
        require_auth(config_bridge.get().admin);

        auto stored = config_bridge.get();
        stored.version = new_version;

        // Modify
        config_bridge.set(stored, get_self());
    };

    // Set the  evm bridge & evm register addresses
    ACTION tokenbridge::setevmctc(eosio::checksum160 bridge_address, eosio::checksum160 register_address){
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
    ACTION tokenbridge::setadmin(eosio::name new_admin){
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
    [[eosio::on_notify("eosio.token::transfer")]]
    ACTION tokenbridge::bridge(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo)
    {
        // Check auth (???)
        require_auth(from);

        // Open config singleton
        auto conf = config_bridge.get();
        auto evm_conf = config.get();

        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find Pair pairs[] array length
        auto pair_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_PAIR_INDEX));
        auto pair_array_length = register_account_states_bykey.require_find(pair_storage_key, "No pairs found");
        auto pair_array_slot = checksum256ToValue(keccak_256(pair_storage_key.extract_as_byte_array()));

        // Get each member of the Pair pairs[] array's antelope_account and compare to get the EVM address
        uint256_t pair_evm_address = 0;
        for(uint256_t i = 0; i < pair_array_length->value; i=i+1){
            const uint256_t position = pair_array_length->value - i;
            const auto account_name = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 5, 8, position));
            // TODO: convert account_name to eosio name
            if(name(decodeHex(bin2hex(intx::to_byte_string(account_name->value))))  == get_first_receiver()){
                const auto pair_active = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 0, 8, position));
                // TODO: convert pair_active to boolean
                check(pair_active->value == uint256_t(0), "The related token pair is paused");
                const auto pair_evm_address_stored = register_account_states_bykey.find(getArrayMemberSlot(pair_array_slot, 1, 8, position));
                pair_evm_address = pair_evm_address_stored->value;
            }
        }
        check(pair_evm_address > 0, "this eosio.token has no pair registered on this bridge");

        // TODO: lock the eosio.token of user (??? >>> escrow utility ?)

        // Find the EVM account of this contract
        account_table _accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaccount = _accounts.get_index<"byaccount"_n>();
        auto account = accounts_byaccount.require_find(get_self().value, "Account not found");

        // Prepare address for EVM Bridge call
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> evm_to;
        evm_to.insert(evm_to.end(),  evm_contract.begin(), evm_contract.end());

        // Prepare EVM function signature & arguments
        std::vector<uint8_t> data;
        auto fnsig = toBin(EVM_BRIDGE_SIGNATURE);
        data.insert(data.end(), fnsig.begin(), fnsig.end());
        // TODO: insert token EVM address
        // TODO: insert receiver EVM address
        auto amount_bs = intx::to_byte_string(uint256_t(quantity.amount));
        data.insert(data.end(),  amount_bs.begin(), amount_bs.end());

        // call TokenBridge.bridgeTo(address token, address receiver, uint amount) on EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, BASE_GAS, evm_to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };

    ACTION tokenbridge::refundnotify()
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
    ACTION tokenbridge::reqnotify()
    {
        // Open config singleton
        auto conf = config_bridge.get();
        auto evm_conf = config.get();

        // Find the EVM account of this contract
        account_table _accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaccount = _accounts.get_index<"byaccount"_n>();
        auto account = accounts_byaccount.require_find(get_self().value, "Account not found");

        // Todo: clean out old processed requests

        // Define EVM Account State table with EVM bridge contract scope
        account_state_table bridge_account_states(EVM_SYSTEM_CONTRACT, conf.evm_bridge_scope);
        auto bridge_account_states_bykey = bridge_account_states.get_index<"bykey"_n>();

        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_BRIDGE_REQUEST_INDEX));
        auto request_array_length = bridge_account_states_bykey.require_find(request_storage_key, "No requests found");
        auto request_array_slot = checksum256ToValue(keccak_256(request_storage_key.extract_as_byte_array()));

        // Prepare address for callback
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(), evm_contract.begin(), evm_contract.end());
        auto fnsig = toBin(EVM_SUCCESS_CALLBACK_SIGNATURE);

        for(uint256_t i = 0; i < request_array_length->value; i=i+1){
            const uint256_t position = request_array_length->value - i;
            // TODO: parse EVM Bridge request
            const auto request_id = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 0, 8, position));
            const auto account_name = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 2, 8, position));
            const auto recipient = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 3, 8, position));
            const auto memo = bridge_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 4, 8, position));

            bool success = false;
            // TODO: check valid
            // TODO: add request to processed (???)

            // TODO:: unlock & send tokens to receiver

            if(success){
                // TODO: setup success callback call so request get deleted on EVM
                std::vector<uint8_t> data;
                data.insert(data.end(), fnsig.begin(), fnsig.end());
                // TODO: insert the request id

                // Send success callback call back to EVM using eosio.evm
                action(
                   permission_level {get_self(), "active"_n},
                   EVM_SYSTEM_CONTRACT,
                   "raw"_n,
                   std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
                ).send();
            }
        }
    };

    // Verify token & sign EVM registration request
    ACTION tokenbridge::signregpair(eosio::checksum160 evm_address, eosio::name account, eosio::symbol symbol, bigint::checksum256 request_id)
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
        auto token_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_PAIR_INDEX));
        auto token_array_length = register_account_states_bykey.find(token_storage_key);
        auto token_array_slot = checksum256ToValue(keccak_256(token_storage_key.extract_as_byte_array()));
        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_REQUEST_INDEX));
        auto request_array_length = register_account_states_bykey.find(request_storage_key);
        auto request_array_slot = checksum256ToValue(keccak_256(request_storage_key.extract_as_byte_array()));

        // Check token doesn't already exist in EVM Register
        // Get each member of the Token tokens[] array's antelope_account and compare
        for(uint256_t i = 0; i < token_array_length->value; i=i+1){
            const auto symbol_name = register_account_states_bykey.find(getArrayMemberSlot(token_array_slot, 4, 8, token_array_length->value - i));
            // Todo: check antelope account too ?
            if(name(decodeHex(bin2hex(intx::to_byte_string(symbol_name->value)))).value == symbol.code().raw()){
                check(false, "The token is already registered");
            }
        }
        // Get each member of the Request requests[] array's antelope_account and compare.
        for(uint256_t k = 0; k < request_array_length->value; k=k+1){
            const auto symbol_name = register_account_states_bykey.find(getArrayMemberSlot(request_array_slot, 4, 8, request_array_length->value - k));
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
        auto fnsig = toBin(EVM_SIGN_REGISTRATION_SIGNATURE);
        std::vector<uint8_t> data;
        std::vector<uint8_t> request_id_bs = pad(intx::to_byte_string(request_id), 16, true);
        std::vector<uint8_t> decimals_bs = pad(intx::to_byte_string(uint256_t(symbol.precision())), 32, true);
        data.insert(data.end(), fnsig.begin(), fnsig.end());
        // Add argument integers
        data.insert(data.end(), request_id_bs.begin(), request_id_bs.end());
        data.insert(data.end(), decimals_bs.begin(), decimals_bs.end());
        // Add argument strings
        insertElementPosition(&data, 160); // base position (depends on previous arguments ^ but factor of 32)
        insertElementPosition(&data, 224); // +64b (= length prefix + string)
        insertElementPosition(&data, 288); // +64b (= length prefix + string)
        insertString(&data, account.value, account.to_string().length());
        insertString(&data, token->issuer.value, token->issuer.to_string().length());
        insertString(&data, symbol.code().raw(), symbol.code().to_string().length());

        // Print it
        auto rlp_encoded = rlp::encode(evm_account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, CURRENT_CHAIN_ID, 0, 0);
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), std::begin(rlp_encoded), std::end(rlp_encoded));
        print(bin2hex(raw));
        // Send signRegistrationRequest call to EVM using eosio.evm
        //action(
        //    permission_level {get_self(), "active"_n},
        //    EVM_SYSTEM_CONTRACT,
        //    "raw"_n,
        //    std::make_tuple(get_self(), rlp::encode(evm_account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(evm_account->address))
        //).send();
    };
}
