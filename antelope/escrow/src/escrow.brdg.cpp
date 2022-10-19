// @author Thomas Cuvillier
// @organization Telos Foundation
// @contract token.brdg
// @version v1.0

#include "../include/token.brdg.hpp";

namespace escrow
{
    //======================== Admin actions ==========================
    // Initialize the contract
    ACTION escrow::init(eosio::name bridge std::string version, eosio::name admin){
        // Authenticate
        require_auth(get_self());

        // Validate
        check(!config.exists(), "contract already initialized");
        check(is_account(admin), "initial admin account doesn't exist");
        check(is_account(bridge), "initial bridge account doesn't exist");

        // Initialize
        auto stored = config_bridge.get_or_create(get_self(), config_row);

        stored.version              = version;
        stored.admin                = admin;
        stored.bridge               = bridge;

        config_bridge.set(stored, get_self());
    };

    // Set the contract version
    ACTION escrow::setversion(std::string new_version){
        // Authenticate
        require_auth(config.get().admin);

        auto stored = config_bridge.get();
        stored.version = new_version;

        // Modify
        config_bridge.set(stored, get_self());
    };

    // Set the bridge
    ACTION escrow::setbridge(name _bridge){
        // Authenticate
        require_auth(config.get().admin);
        check(is_account(_bridge), "initial bridge account doesn't exist");
        bridge = _bridge;
    };

    // Set new contract admin
    ACTION escrow::setadmin(eosio::name new_admin){
        // Authenticate
        require_auth(config_bridge.get().admin);

        // Check account exists
        check(is_account(new_admin), "New admin account does not exist, please verify the account name provided");

        auto stored = config_bridge.get();
        stored.admin = new_admin;
        // Modify
        config_bridge.set(stored, get_self());
    };

    //======================== Token Escrow actions ========================

}

