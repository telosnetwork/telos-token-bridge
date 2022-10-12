#! /bin/bash
echo ">>> Building contract..."
if [ ! -d "$PWD/build" ]
then
  mkdir build
fi
eosio-cpp -I="./include/"  -I="./external/"  -o="./build/token.brdg.wasm" -contract="token.brdg" -abigen ./src/token.brdg.cpp