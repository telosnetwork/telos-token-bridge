# Telos Token Bridge :: Antelope

Currently running on

**TESTNET:** token.brdg

**MAINNET:** TBD

## Install

### Requirements
- [EOSIO CDT](https://developers.eos.io/welcome/latest/getting-started-guide/local-development-environment/installing-eosiocdt)
- [Cleos](https://developers.eos.io/manuals/eos/v2.2/install/install-prebuilt-binaries) *
- A wallet setup for `Cleos`

If you are trying to install EOSIO on Ubuntu > 22.04 you may be missing libssl1.1, see [this workaround](https://askubuntu.com/questions/1403619/mongodb-install-fails-on-ubuntu-22-04-depends-on-libssl1-1-but-it-is-not-insta)

### Build

`bash build.sh`

### Test

`npm install`
followed by
`npm run test`

### Deploy 

`bash deploy.sh`
