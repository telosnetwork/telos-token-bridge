#! /bin/bash
echo ">>> Deploying contract..."
if [ "$1" == "mainnet" ]
then
  url="https://mainnet.telos.caleos.io"
else
  url="https://testnet.telos.caleos.io"
fi
if [ -z "$2" ]
then
  contract="token.brdg"
else
  contract=$2
fi
cleos --url "$url" set contract "$contract" "$PWD/build" token.brdg.wasm token.brdg.abi