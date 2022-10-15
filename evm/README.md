# Telos Token Bridge :: EVM

This is the EVM part of our Token Bridge repository. It is currently running on:

**TESTNET**
- PairBridgeRegister: TBD
- TokenBridge: TBD

**MAINNET**
- PairBridgeRegister: TBD
- TokenBridge: TBD

## Contracts

### TokenBridge.sol

This is the main EVM contract for the token bridge.

#### Events

- `event  BridgeToAntelopeRequested(address indexed sender, address indexed token, uint amount, string recipient);`
- `event  BridgeToAntelopeSucceeded(address indexed sender, address indexed token, uint amount, string recipient);`
- `event  BridgeFromAntelopeSucceeded(address indexed recipient, address indexed token, uint amount);`

#### Public functions

- `function bridge(address token, uint amount, string receiver)` _note that you need ERC20 allowance for the bridge address_
- `function fee()`
- `function max_requests_per_requestor()`

### PairBridgeRegister.sol

This is the token pairs register for the bridge. Token owners can request registration and sign the request from Antelope, the prods.evm owner evm address can invoke CRUD operations on the pairs & approve requests.

#### Events

- `event  RegistrationRequested(uint request_id, address requestor, address indexed token, string symbol);`
- `event  RegistrationRequestSigned(uint request_id, address indexed token, string antelope_account, string symbol);`
- `event  RegistrationRequestApproved(uint request_id, address indexed token, string antelope_account, string symbol);`
- `event  RegistrationRequestDeleted(uint request_id, address indexed token, string antelope_account);`
- `event  PairPaused(uint pair_id, address indexed evm_token, string symbol, string name, string antelope_account);`
- `event  PairAdded(uint pair_id, address indexed evm_token, string symbol, string antelope_account);`
- `event  PairUnpaused(uint pair_id, address indexed evm_token, string symbol);`
- `event  PairDeleted(uint pair_id, address indexed evm_token, string symbol);`

#### Public functions

Get the pair data from the EVM token address or from the Antelope token account name

- `function getPair(address evm_token_address)`
- `function getPairByAntelopeAccount(string antelope_account)`

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
