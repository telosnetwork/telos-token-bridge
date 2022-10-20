#pragma once

using namespace std;
using namespace eosio;
using namespace evm_bridge;

namespace evm_bridge {
    //======================== eosio.token tables ========================
     struct [[eosio::table, eosio::contract("eosio.token")]] currency_stats {
        asset    supply;
        asset    max_supply;
        name     issuer;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
     };
     typedef eosio::multi_index< "stat"_n, currency_stats > eosio_tokens;

    //======================== Tables ========================
    // Requests
    struct [[eosio::table, eosio::contract("token.brdg")]] requests {
        uint64_t request_id;
        eosio::checksum256 call_id;
        time_point timestamp;

        uint64_t primary_key() const { return request_id; };
        eosio::checksum256 by_call_id() const { return call_id; };
        uint64_t by_timestamp() const {return timestamp.elapsed.to_seconds();}

        EOSLIB_SERIALIZE(requests, (request_id)(call_id)(timestamp));
    };
    typedef multi_index<name("requests"), requests,
       eosio::indexed_by<eosio::name("bycallid"), eosio::const_mem_fun<requests, eosio::checksum256, &requests::by_call_id >>
    >  requests_table;

    // Config
    struct [[eosio::table, eosio::contract("token.brdg")]] bridgeconfig {
        eosio::checksum160 evm_bridge_address;
        eosio::checksum160 evm_register_address;
        uint64_t evm_bridge_scope;
        uint64_t evm_register_scope;
        name admin;
        string version;

        EOSLIB_SERIALIZE(bridgeconfig, (evm_bridge_address)(evm_register_address)(evm_bridge_scope)(evm_register_scope)(admin)(version));
    } config_row;

    typedef singleton<"bridgeconfig"_n, bridgeconfig> config_singleton_bridge;

    // Refunds
}