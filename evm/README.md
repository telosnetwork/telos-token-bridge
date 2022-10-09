# Telos Token Bridge :: EVM

This is the EVM part of our Token Bridge repository

## Contracts

### TokenBridge.sol

This is the main EVM contract for the token bridge.

#### Events

- `event  BridgeToAntelopeRequested(address indexed sender, address indexed token, uint amount, string recipient);`
- `event  BridgeToAntelopeSucceeded(address indexed sender, address indexed token, uint amount, string recipient);`
- `event  BridgeFromAntelopeSucceeded(address indexed recipient, address indexed token, uint amount);`

### TokenBridgeRegister.sol

This is the token register for the bridge. Token owner can request registration, the prods.evm owner evm address can invoke CRUD operations on tokens & approve requests.

#### Events

- `event  RegistrationRequested(uint request_id, address requestor, address indexed token, string symbol);`
- `event  RegistrationRequestSigned(uint request_id, address indexed token, string antelope_account, string symbol);`
- `event  RegistrationRequestApproved(uint request_id, address indexed token, string antelope_account, string symbol);`
- `event  RegistrationRequestDeleted(uint request_id, address indexed token, string antelope_account);`
- `event  TokenPaused(uint token_id, address indexed token, string symbol, string name, string antelope_account);`
- `event  TokenAdded(uint token_id, address indexed token, string symbol, string antelope_account);`
- `event  TokenUnpaused(uint token_id, address indexed token, string symbol);`
- `event  TokenDeleted(uint token_id, address indexed token, string symbol);`

### ERC20Bridgeable.sol

This is an example ERC20 token compatible with our bridge, developers can extend it to write their own !

_For test purposes only, does not get deployed_

## Install

`npm install`

## Test

Use the following command to test it:

`npx hardhat test`

## Deploy

Use the following command to deploy it:

`npx hardhat deploy --network [testnet|mainnet]`
