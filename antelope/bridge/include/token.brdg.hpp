// @author Thomas Cuvillier
// @organization Telos Foundation
// @contract token.brdg
// @version v1.0

// EOSIO
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>

// EXTERNAL
#include <intx/base.hpp>
#include <rlp/rlp.hpp>
#include <ecc/uECC.c>
#include <keccak256/k.c>
#include <boost/multiprecision/cpp_int.hpp>

// TELOS EVM
#include <constants.hpp>
#include <evm_util.hpp>
#include <datastream.hpp>
#include <evm_tables.hpp>
#include <tables.hpp>

using namespace std;
using namespace eosio;
using namespace evm_bridge;
using namespace intx;

namespace evm_bridge
{
    class [[eosio::contract("token.brdg")]] tokenbridge : public contract {
        public:
            using contract::contract;
            tokenbridge(name self, name code, datastream<const char*> ds) : contract(self, code, ds), config_bridge(self, self.value), config(EVM_SYSTEM_CONTRACT, EVM_SYSTEM_CONTRACT.value) { };
            ~tokenbridge() {};

            //======================== Admin actions ========================
            // intialize the contract
            [[eosio::action]] void init(eosio::checksum160 bridge_address, eosio::checksum160 register_address, std::string version, eosio::name admin);

            // set the contract version
            [[eosio::action]] void setversion(std::string new_version);

            // set the bridge & register evm addresses
            [[eosio::action]] void setevmctc(eosio::checksum160 bridge_address, eosio::checksum160 register_address);

            // set new contract admin
            [[eosio::action]] void setadmin(eosio::name new_admin);

            //======================== Token bridge actions ========================

            [[eosio::action]] void refundnotify();
            [[eosio::action]] void reqnotify();
            [[eosio::action]] void signregpair(eosio::checksum160 evm_address, eosio::name account, eosio::symbol symbol, uint64_t request_id);
            [[eosio::on_notify("*::transfer")]] void bridge(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo);

            config_singleton_bridge config_bridge;
            config_singleton_evm config;
    };
}
