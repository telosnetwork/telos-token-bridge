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
  static constexpr std::string EVM_SUCCESS_CALLBACK_SIGNATURE = "eb5bb2f6";
  static constexpr std::string EVM_BRIDGE_SIGNATURE = "eb5bb2f6";
  static constexpr std::string EVM_SIGN_REGISTRATION_SIGNATURE = "eb5bb2f6";
  static constexpr uint STORAGE_INDEX = 2;
}