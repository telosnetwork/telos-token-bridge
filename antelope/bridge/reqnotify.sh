#! /bin/bash
echo ">>> Calling reqnotify() ..."
if [ "$1" == "mainnet" ]
then
  url="https://mainnet.telos.caleos.io"
else
  url="https://testnet.telos.caleos.io"
fi
cleos --url "$url" push action token.brdg reqnotify '{}' -p token.brdg
