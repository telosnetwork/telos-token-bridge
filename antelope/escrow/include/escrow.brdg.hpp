// @author Thomas Cuvillier
// @contract rngorcbrdg
// @version v0.1.0

// EOSIO
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>

// EXTERNAL
#include <intx/base.hpp>
#include <rlp/rlp.hpp>
#include <ecc/uECC.c>
#include <keccak256/k.c>
#include <boost/multiprecision/cpp_int.hpp>

// TELOS EVM
#include <escrow_constants.hpp>
#include <evm_util.hpp>
#include <datastream.hpp>
#include <evm_bridge_tables.hpp>
#include <escrow_tables.hpp>

#define EVM_SYSTEM_CONTRACT name("eosio.evm")

using namespace std;
using namespace eosio;
using namespace evm_bridge;
using namespace evm_util;

namespace evm_bridge
{
    class [[eosio::contract("escrow.brdg")]] escrowbridge : public contract {
        public:
            using contract::contract;
            escrowbridge(name self, name code, datastream<const char*> ds) : contract(self, code, ds), config_bridge(self, self.value) { };
            ~escrowbridge() {};

            //======================== Admin actions ========================
            // intialize the contract
            ACTION init(eosio::checksum160 evm_contract, string version, name admin);

            //set the contract version
            ACTION setversion(string new_version);

            //set the bridge evm address
            ACTION setevmctc(eosio::checksum160 new_contract);

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

            config_singleton_bridge config_bridge;
    };
}
