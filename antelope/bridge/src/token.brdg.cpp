// @author Thomas Cuvillier
// @organization Telos Foundation
// @contract token.brdg
// @version v1.0

#include "../include/token.brdg.hpp";

namespace evm_bridge
{
    //======================== Admin actions ==========================
    // Initialize the contract
    ACTION tokenbridge::init(eosio::checksum160 bridge_address, eosio::checksum160 register_address, string version, name admin){
        // Authenticate
        require_auth(get_self());

        // Validate
        check(!config_bridge.exists(), "contract already initialized");
        check(is_account(admin), "initial admin account doesn't exist");

        // Initialize
        auto stored = config_bridge.get_or_create(get_self(), config_row);

        stored.version            = version;
        stored.admin              = admin;
        stored.evm_contract       = evm_contract;

        // Get the scope
        account_table accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaddress = accounts.get_index<"byaddress"_n>();
        auto account_bridge = accounts_byaddress.require_find(pad160(bridge_address), "EVM Bridge not found in eosio.evm accounts");
        auto account_register = accounts_byaddress.require_find(pad160(register_address), "EVM Register not found in eosio.evm accounts");

        stored.evm_bridge_scope = account_bridge->index;
        stored.evm_register_scope = account_register->index;

        config_bridge.set(stored, get_self());
    };

    // Set the contract version
    ACTION tokenbridge::setversion(string new_version){
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
        auto account_bridge = accounts_byaddress.require_find(pad160(bridge_address), "EVM Bridge not found in eosio.evm accounts");
        auto account_register = accounts_byaddress.require_find(pad160(register_address), "EVM Register not found in eosio.evm accounts");

        // Save
        auto stored = config_bridge.get();
        stored.evm_bridge_address = bridge_address;
        stored.evm_bridge_scope = account_bridge->index;
        stored.evm_register_address = register_address;
        stored.evm_register_scope = account_register->index;

        config_bridge.set(stored, get_self());
    };

    // Set new contract admin
    ACTION tokenbridge::setadmin(name new_admin){
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
    ACTION tokenbridge::bridge(name caller, name token_account, uint256_t amount, string receiver)
    {
        // Check auth
        auth(caller);

        // Open config singleton
        auto conf = config_bridge.get();

        // TODO: check the token has an active pair
        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find Token tokens[] array length
        auto token_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_TOKEN_INDEX));
        auto token_array_length = register_account_states_bykey.find(storage_key);
        auto token_array_slot = checksum256ToValue(keccak_256(storage_key.extract_as_byte_array()));

        // Get each member of the Token tokens[] array's antelope_account and compare
        for(uint256_t i = 0; i < token_array_length; i++){
            const auto account_name = register_account_states_bykey.find(getArrayMemberSlot(register_array_slot, 4, 8, position));
            if(account_name == token_account){
                const auto token_active = register_account_states_bykey.find(getArrayMemberSlot(register_array_slot, 4, 8, position));
                check(token_active, "The related token pair is paused");
            }
        }

        // TODO: lock the tokens of user

        // Prepare address for EVM Bridge call
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(),  evm_contract.begin(), evm_contract.end());

        // Prepare EVM function signature & arguments
        std::vector<uint8_t> data;
        auto fnsig = toBin(EVM_BRIDGE_SIGNATURE);
        data.insert(data.end(), fnsig.begin(), fnsig.end());
        // TODO: insert token EVM address
        // TODO: insert receiver EVM address
        auto amount_bs = intx::to_byte_string(amount);
        data.insert(to.end(),  amount_bs.begin(), amount_bs.end());

        // call TokenBridge.bridgeTo(address token, address receiver, uint amount) on EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };

    // Trustless bridge from EVM
    ACTION tokenbridge::reqnotify()
    {
        // Open config singleton
        auto conf = config_bridge.get();

        // Define EVM Account State table with EVM bridge contract scope
        account_state_table bridge_account_states(EVM_SYSTEM_CONTRACT, conf.evm_bridge_scope);
        auto bridge_account_states_bykey = bridge_account_states.get_index<"bykey"_n>();

        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_BRIDGE_REQUEST_INDEX));
        auto request_array_length = register_account_states_bykey.find(storage_key);
        auto request_array_slot = checksum256ToValue(keccak_256(storage_key.extract_as_byte_array()));

        // Prepare address for callback
        auto evm_contract = conf.evm_bridge_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(), evm_contract.begin(), evm_contract.end());
        auto fnsig = toBin(EVM_SUCCESS_CALLBACK_SIGNATURE);

        for(uint256_t i = 0; i < request_array_length; i++){
            // TODO: parse EVM Bridge request
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
                   std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, request.gas + BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
                ).send();
            }
        }
    };


    // Verify token & sign EVM registration request
    ACTION tokenbridge::signregreq(uint256_t evm_request, name token_account, name token_symbol, eosio::checksum160 evm_address)
    {
        // Check auth
        auth(token_account);

        // Open config singleton
        auto conf = config_bridge.get();

        // Define EVM Account State table with EVM register contract scope
        account_state_table register_account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto register_account_states_bykey = register_account_states.get_index<"bykey"_n>();

        // Get array slot to find Token tokens[] array length
        auto token_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_TOKEN_INDEX));
        auto token_array_length = register_account_states_bykey.find(storage_key);
        auto token_array_slot = checksum256ToValue(keccak_256(storage_key.extract_as_byte_array()));
        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_TOKEN_INDEX));
        auto request_array_length = register_account_states_bykey.find(storage_key);
        auto request_array_slot = checksum256ToValue(keccak_256(storage_key.extract_as_byte_array()));

        // Check token doesn't already exist in EVM Register
        // Get each member of the Token tokens[] array's antelope_account and compare
        for(uint256_t i = 0; i < token_array_length; i++){
            const auto account_name = register_account_states_bykey.find(getArrayMemberSlot(register_array_slot, 4, 8, position));
            if(account_name == token_account){
                check(false, "The token is already registered");
            }
        }
        // Get each member of the Request requests[] array's antelope_account and compare.
        for(uint256_t i = 0; i < request_array_length; i++){
            const auto account_name = register_account_states_bykey.find(getArrayMemberSlot(register_array_slot, 4, 8, position));
            if(account_name == token_account){
                check(false, "The token is already awaiting approval");
            }
        }

        // TODO: Get token infos (decimals, ...)
        uint256_t decimals = 14;
        // TODO:  Check token is compatible (TBD)(???)

        // Prepare address
        auto evm_contract = conf.evm_register_address.extract_as_byte_array();
        std::vector<uint8_t> to;
        to.insert(to.end(),  evm_contract.begin(), evm_contract.end());

        // Prepare solidity function parameters (function signature + arguments)
        std::vector<uint8_t> data;
        std::vector<uint8_t> request_id_bs = intx::to_byte_string(request_id);
        std::vector<uint8_t> decimals_bs = pad(intx::to_byte_string(decimals));
        request_id_bs.insert(request_id_bs.begin(),(16 - request_id_bs.size()), 0);
        auto fnsig = toBin(EVM_SIGN_REGISTRATION_SIGNATURE);
        data.insert(data.end(), fnsig.begin(), fnsig.end());
        data.insert(data.end(), request_id_bs.begin(), request_id_bs.end());
        data.insert(data.end(), decimals_bs.begin(), decimals_bs.end());
        // TODO: Convert & add antelope symbol & account name

        // Send signRegistrationRequest call to EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };
}
