# Telos Antelope <> EVM Token Bridge (WIP)

This repository contains the smart contracts of our trustless Telos Token Bridge divided in two parts: EVM & Antelope.

Refer to each folder for specific documentation on deploying each side of the token bridge:
- [EVM](https://github.com/telosnetwork/telos-token-bridge/tree/master/evm)
- [Antelope](https://github.com/telosnetwork/telos-token-bridge/tree/master/antelope)

Refer to the following Telos docs relative to the bridge for help using it in your smart contract:
- [Bridge Antelope >> EVM](https://doc.telos.net)
- [Bridge EVM >> Antelope](https://doc.telos.net)

Or our [Telos help guide](https://help.telos.net) to simply bridge your tokens manually on our [Telos Wallet](https://wallet.telos.net) frontend

_To create a bridgeable erc20 token that you can register on the bridge and pair with an Antelope token, refer to our [ERC20Bridgeable](https://github.com/telosnetwork/erc20-bridgeable) repository._

## How it works

On Telos, the EVM runs inside one eosio.evm smart contract on our Antelope native chain. This means any Antelope smart contract can read EVM storage and call EVM functions, making a 100% trustless token bridge possible ! We do not use liquidity pools or other solutions that bring complexity as well as their own problems, everything happens directly inside the smart contracts and only the native Telos BPs & community can modify its settings and pause/unpause tokens. 

### 1. Pair registration

[ERC20Bridgeable](https://github.com/telosnetwork/erc20-bridgeable) token owners can use our PairRegister to request a pair registration on EVM

```
const { BigNumber, ethers, utils } = require("ethers");
const address = "REGISTER EVM CONTRACT";
const token = "ERC20Bridgeable TOKEN ADDRESS"
const abi = [
    "function requestRegistration (IERC20Bridgeable token) external returns(uint)",
];

(async function() {

})();
```

Once the request has been created you can sign it from Antelope , using the token account with eosjs cleos or a block explorer.

```
  const { eosjs } = require("eosjs");
```

### 2. Pair approval

The community / BPs can approve that request via an Antelope multisig sending a transaction to EVM via the prods.evm account and adding the new pair to the bridge.

### 3. Bridge from Antelope to EVM

```
  const { eosjs } = require("eosjs");
```


![antelope2evmb](https://user-images.githubusercontent.com/5913758/195126884-1cc95bcf-d318-465c-8d1f-6ba603e37126.jpg)

### 4. Bridge from EVM to Antelope

You can use the `TokenBridge` contract `bridge(address token, uint amount, string receiver, string memo)` function to bridge a registered ERC20Bridgeable token its paired token on Antelope ! You will need to pass in the fee that you can query with the `fee()` function.

```
  const { ethers } = require("ethers");
```

You will find a breakdown below of how it works:

