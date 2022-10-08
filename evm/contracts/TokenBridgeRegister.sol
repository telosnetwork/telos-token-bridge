// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
}

contract TokenBridgeRegister is Ownable {
    event  Registered(address indexed token, string antelope_account, string symbol, string name);
    event  Paused(address indexed token, string symbol);
    event  Unpaused(address indexed token, string symbol);
    event  Deleted(address indexed token, string symbol);

    uint count;

    struct Token {
        bool active;
        uint id;
        address evm_address;
        uint8 evm_decimals;
        uint8 antelope_decimals;
        string antelope_account;
        string symbol;
        string name;
    }

    Token[] public tokens;

    constructor() {
        count = 0;
    }

    // GETTERS
    function getToken(address token) external view returns (Token memory) {
        for(uint i = 0; i < tokens.length;i++){
            if(token == tokens[i].evm_address){
                return tokens[i];
            }
        }
        revert('Token not found');
    }

     // MAIN   ================================================================ >
    function registerToken () external {
        // TODO: LET USERS ADD TOKENS
        // NEED TO CHECK ERC20 BRIDGEABLE COMPLIANCE
    }

    function addToken () external onlyOwner {
        // TODO: Let prods.evm add a token
    }

    function pauseToken (uint id) external onlyOwner {
        // TODO: Let prods.evm pause a token (maybe let token owner too ??)
    }

    function removeToken (uint id) external onlyOwner {
        // TODO: Let prods.evm remove a token
    }
}