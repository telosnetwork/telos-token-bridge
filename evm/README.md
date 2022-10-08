# Telos Token Bridge :: EVM

This is the EVM part of our Token Bridge repository

## Contracts

### TokenBridge.sol

#### Events

- `event  BridgeToAntelopeRequested(address indexed sender, address indexed token, uint amount, string recipient);`
- `event  BridgeToAntelopeSucceeded(address indexed sender, address indexed token, uint amount, string recipient);`
- `event  BridgeFromAntelopeSucceeded(address indexed recipient, address indexed token, uint amount);`

### TokenBridgeRegister.sol

#### Events

- `event  Registered(address indexed token, string antelope_account, string symbol, string name);`
- `event  Paused(address indexed token, string symbol);`
- `event  Unpaused(address indexed token, string symbol);`
- `event  Deleted(address indexed token, string symbol);`

### ERC20Bridgeable.sol

_For test purposes only, does not get deployed_

## Install

`npm install`

## Test

Use the following command to test it:

`npx hardhat test`

## Deploy

Use the following command to deploy it:

`npx hardhat deploy --network [testnet|mainnet]`
