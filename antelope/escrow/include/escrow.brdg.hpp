// @author Thomas Cuvillier
// @contract rngorcbrdg
// @version v0.1.0

// EOSIO
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>

// TELOS EVM
#include <constants.hpp>
#include <tables.hpp>

using namespace std;
using namespace eosio;
using namespace escrow;

namespace escrow
{
    class [[eosio::contract("escrow.brdg")]] escrow : public contract {
        public:
            using contract::contract;
            escrow(name self, name code, datastream<const char*> ds) : contract(self, code, ds), config(self, self.value) { };
            ~escrow() {};

            //======================== Admin actions ========================
            // intialize the contract
            ACTION init(eosio::name bridge, string version, name admin);

            //set the contract version
            ACTION setversion(string new_version);

            //set the bridge
            ACTION setbridge(eosio::name bridge);

            //set new contract admin
            ACTION setadmin(name new_admin);

            //======================== Escrow actions ========================

            //======================= Testing action =============================
            #if (TESTING == true)
                ACTION clearall()
                {
                    config_bridge.remove();
                }
            #endif

            config_singleton config;
    };
}
