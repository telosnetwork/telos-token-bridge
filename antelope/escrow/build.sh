#! /bin/bash
echo ">>> Building contract..."
if [ ! -d "$PWD/build" ]
then
  mkdir build
fi
eosio-cpp -I="./include/"  -I="./external/"  -o="./build/token.bridge.wasm" -contract="tokenbridge" -abigen ./src/token.bridge.cpp