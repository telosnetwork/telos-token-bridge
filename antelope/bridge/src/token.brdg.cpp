#include "../include/token.brdg.hpp";

namespace evm_bridge
{
    //======================== Admin actions ==========================
    // Initialize the contract
    ACTION tokenbridge::init(eosio::checksum160 evm_contract, string version, name admin){
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
        auto account = accounts_byaddress.require_find(pad160(evm_contract), "EVM bridge contract not found in eosio.evm accounts");

        stored.evm_contract_scope = account->index;

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

    // Set the bridge evm address
    ACTION tokenbridge::setevmctc(eosio::checksum160 new_contract){
        // Authenticate
        require_auth(config_bridge.get().admin);

        // Get the scope for accountstates
        account_table accounts(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value);
        auto accounts_byaddress = accounts.get_index<"byaddress"_n>();
        auto account = accounts_byaddress.require_find(pad160(new_contract), "EVM bridge contract not found in eosio.evm accounts");

        // Save
        auto stored = config_bridge.get();
        stored.evm_contract = new_contract;
        stored.evm_contract_scope = account->index;

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
    // Bridge to EVM
    ACTION tokenbridge::bridge()
    {
        // Send it back to EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, request.gas + BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };

    };

    // Verify token & sign EVM registration request
    ACTION tokenbridge::registerToken()
    {
        // Send it back to EVM using eosio.evm
        action(
            permission_level {get_self(), "active"_n},
            EVM_SYSTEM_CONTRACT,
            "raw"_n,
            std::make_tuple(get_self(), rlp::encode(account->nonce, evm_conf.gas_price, request.gas + BASE_GAS, to, uint256_t(0), data, 41, 0, 0),  false, std::optional<eosio::checksum160>(account->address))
        ).send();
    };
}
