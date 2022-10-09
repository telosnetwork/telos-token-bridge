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

- `event  RegistrationRequested(uint request_id, address requestor, address indexed token, string antelope_name, string symbol, string name);`
- `event  RegistrationRequestSigned(uint request_id, address indexed token, string antelope_account, string antelope_name, string symbol, string name);`
- `event  RegistrationRequestApproved(uint request_id, address indexed token, string antelope_account, string antelope_name, string symbol, string name);`
- `event  RegistrationRequestDeleted(uint request_id, address indexed token, string antelope_account);`
- `event  TokenPaused(uint token_id, address indexed token, string symbol, string name, string antelope_account);`
- `event  TokenAdded(uint token_id, address indexed token, string symbol, string antelope_account);`
- `event  TokenUnpaused(uint token_id, address indexed token, string symbol);`
- `event  TokenDeleted(uint token_id, address indexed token, string symbol);`

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
