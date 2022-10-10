// @author Thomas Cuvillier
// @organization Telos Foundation
// @contract token.brdg
// @version v1.0

#include <vector>

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
#include <token_bridge_constants.hpp>
#include <evm_util.hpp>
#include <datastream.hpp>
#include <evm_bridge_tables.hpp>
#include <token_bridge_tables.hpp>

#define EVM_SYSTEM_CONTRACT name("eosio.evm")

using namespace std;
using namespace eosio;
using namespace evm_bridge;
using namespace evm_util;

namespace evm_bridge
{
    class [[eosio::contract("token.brdg")]] tokenbridge : public contract {
        public:
            using contract::contract;
            tokenbridge(name self, name code, datastream<const char*> ds) : contract(self, code, ds), config_bridge(self, self.value) { };
            ~tokenbridge() {};

            //======================== Admin actions ========================

            // intialize the contract
            ACTION init(eosio::checksum160 evm_contract, string version, name admin);

            //set the contract version
            ACTION setversion(string new_version);

            //set the bridge evm address
            ACTION setevmctc(eosio::checksum160 new_contract);

            //set new contract admin
            ACTION setadmin(name new_admin);

            //======================== Token bridge actions ========================

            ACTION reqnotify();
            ACTION bridge(name token_account, uint256_t amount);
            ACTION tokenbridge::registerToken(name user, name token_account, name token_symbol, eosio::checksum160 evm_address);

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
