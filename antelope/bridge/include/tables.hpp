#pragma once

using namespace std;
using namespace eosio;
using namespace evm_bridge;

namespace evm_bridge {
    //======================== Tables ========================
    // Config
    struct [[eosio::table, eosio::contract("token.brdg")]] bridgeconfig {
        eosio::checksum160 evm_contract;
        name admin;
        string version;
        uint64_t evm_contract_scope;

        EOSLIB_SERIALIZE(bridgeconfig, (evm_contract)(admin)(version)(evm_contract_scope));
    } config_row;

    typedef singleton<"bridgeconfig"_n, bridgeconfig> config_singleton_bridge;

}