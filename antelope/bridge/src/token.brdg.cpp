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
    ACTION tokenbridge::bridge(name caller, name token_account, uint256_t amount)
    {
        // Check auth
        auth(caller);
        // TODO: check the token has an active pair
        // TODO: lock the tokens of user
        // TODO: prepare EVM Bridge call
        // Send it back to EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, request.gas + BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };

    // Trustless bridge from EVM
    ACTION tokenbridge::reqnotify()
    {
        // TODO: parse EVM Bridge request, unlock & send tokens to receiver
        // TODO: setup success callback call
        // Send success callback call back to EVM using eosio.evm
        action(
           permission_level {get_self(), "active"_n},
           EVM_SYSTEM_CONTRACT,
           "raw"_n,
           std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, request.gas + BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };


    // Verify token & sign EVM registration request
    ACTION tokenbridge::registerToken(uint256_t evm_request, name token_account, name token_symbol, eosio::checksum160 evm_address)
    {
        // Check auth
        auth(token_account);

        // Open config singleton
        auto conf = config_bridge.get();

        // Define EVM Account State table with EVM register contract scope
        account_state_table account_states(EVM_SYSTEM_CONTRACT, conf.evm_register_scope);
        auto account_states_bykey = account_states.get_index<"bykey"_n>();

        // Get array slot to find Token tokens[] array length
        auto token_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_TOKEN_INDEX));
        auto token_array_length = account_states_bykey.find(storage_key);
        auto token_array_slot = checksum256ToValue(keccak_256(storage_key.extract_as_byte_array()));

        // Get array slot to find Request requests[] array length
        auto request_storage_key = toChecksum256(uint256_t(STORAGE_REGISTER_TOKEN_INDEX));
        auto request_array_length = account_states_bykey.find(storage_key);
        auto request_array_slot = checksum256ToValue(keccak_256(storage_key.extract_as_byte_array()));

        // TODO: Check token doesn't already exist in EVM Register
        bool exists = false;
        for(uint256_t i = 0; i < token_array_length; i++){
            // Get each member of that Token tokens[] array's antelope_account and compare
        }
        for(uint256_t i = 0; i < request_array_length; i++){
            // Get each member of that Request requests[] array's antelope_account and compare.
        }
        if(exists){
            check(false, "The token is already registered or awaiting approval")
        }

        // TODO: Get token infos (decimals, ...)
        // TODO:  Check token is compatible (TBD)
        // TODO:  Prepare signRegistrationRequest call on EVM

        // Send signRegistrationRequest call to EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, request.gas + BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };
}
