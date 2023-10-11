# Telos Token Bridge :: Antelope

Currently running on

**TESTNET:** token.brdg

**MAINNET:** TBD

## Install

### Requirements
- [EOSIO](https://developers.eos.io/manuals/eos/v2.2/install/install-prebuilt-binaries) for access to `cleos` *
- [EOSIO CDT](https://developers.eos.io/welcome/latest/getting-started-guide/local-development-environment/installing-eosiocdt)
- A wallet setup for `cleos`

<sub>* If you are trying to install EOSIO on Ubuntu > 22.04 you may be missing libssl1.1, see [this workaround](https://askubuntu.com/questions/1403619/mongodb-install-fails-on-ubuntu-22-04-depends-on-libssl1-1-but-it-is-not-insta)</sub>

### Build

`bash build.sh`

### Test

`npm install`
followed by
`npm run test`

### Deploy 

First create an EOSIO account (**YOUR_CONTRACT_ACCOUNT**) for your contract and setup the key in your Cleos wallet.
Then use the following command:

`bash deploy.sh testnet **YOUR_CONTRACT_ACCOUNT**`
