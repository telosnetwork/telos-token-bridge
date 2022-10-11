# Telos Antelope <> EVM Token Bridge

This repository contains the smart contracts of our trustless Telos Token Bridge divided in two parts: EVM & Antelope.

Refer to each folder for specific documentation on deploying each side of the token bridge:
- [EVM](https://github.com/telosnetwork/telos-token-bridge/tree/master/evm)
- [Antelope](https://github.com/telosnetwork/telos-token-bridge/tree/master/antelope)

Refer to the following Telos docs relative to the bridge for help using it in your smart contract:
- [Bridge Antelope >> EVM](https://doc.telos.net)
- [Bridge EVM >> Antelope](https://doc.telos.net)

Or our [Telos help guide](https://help.telos.net) to simply bridge your tokens manually

_To create a bridgeable erc20 token that you can register on the bridge, paired with an Antelope eosio.token token, refer to our [ERC20Bridgeable](https://github.com/telosnetwork/erc20-bridgeable) repository._

## How it works

On Telos, the EVM runs inside one eosio.evm smart contract on our Antelope native chain. This means any Antelope smart contract can read EVM storage and call EVM functions, making it the most trustless token bridge there can be ! We do not use liquidity pools or other solutions that bring complexity as well as their own problems, everything happens directly inside the smart contracts and only the native Telos BPs & community can modify its settings and pause/unpause tokens. 
