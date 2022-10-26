#pragma once

// Adds superpower testing functions (required for running cpp tests/clearing data in contract)
#define TESTING true

// Crypto
#define MBEDTLS_ASN1_OCTET_STRING 0x04

namespace evm_bridge
{
  struct ChainIDs
  {
    static constexpr size_t TELOS_MAINNET        = 40;
    static constexpr size_t TELOS_TESTNET        = 41;
  };

  static constexpr auto WORD_SIZE       = 32u;
  // Constant chain ID determined at COMPILE time
  static constexpr size_t CURRENT_CHAIN_ID = ChainIDs::TELOS_TESTNET;
  static constexpr eosio::name ESCROW = eosio::name("escrow.brdg");
  static constexpr eosio::name EVM_SYSTEM_CONTRACT = eosio::name("eosio.evm");
  static constexpr eosio::name TOKEN_CONTRACT = eosio::name("eosio.token");
  static constexpr uint256_t BASE_GAS = 250000;
  static constexpr auto EVM_SUCCESS_CALLBACK_SIGNATURE = "0fbc79cd";
  static constexpr auto EVM_REFUND_CALLBACK_SIGNATURE = "dc2fdf9f";
  static constexpr auto EVM_BRIDGE_SIGNATURE = "7d056de7";
  static constexpr auto EVM_SIGN_REGISTRATION_SIGNATURE = "a1d22913";
  static constexpr uint256_t STORAGE_BRIDGE_REQUEST_INDEX = 4;
  static constexpr uint256_t STORAGE_BRIDGE_REFUND_INDEX = 5;
  static constexpr uint256_t STORAGE_REGISTER_REQUEST_INDEX = 4;
  static constexpr uint256_t STORAGE_REGISTER_PAIR_INDEX = 3;
}