// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
}

contract TokenBridgeRegister is Ownable {
    event  Registered(address indexed token, string antelope_account, string symbol, string name);
    event  Approved(address indexed token, string symbol, string name);
    event  Paused(address indexed token, string symbol, string name);
    event  Unpaused(address indexed token, string symbol, string name);
    event  Deleted(address indexed token, string symbol, string name);

    uint count;
    uint8 max_requests_per_requestor;

    struct Token {
        bool active;
        uint id;
        address evm_address;
        uint8 evm_decimals;
        uint8 antelope_decimals;
        string antelope_account_name;
        string symbol;
        string name;
    }

    struct Request {
        bool antelope_signed;
        bool active;
        uint id;
        address evm_address;
        uint8 evm_decimals;
        uint8 antelope_decimals;
        uint timestamp;
        string antelope_account_name;
        string symbol;
        string name;
    }

    Token[] public tokens;
    Request[] public requests;

    constructor(uint8 _max_requests_per_requestor) {
        count = 0;
        max_requests_per_requestor = _max_requests_per_requestor;
    }

    // SETTERS
    function setMaxRequestsPerRequestor(uint8 _max_requests_per_requestor) external onlyOwner {
        max_requests_per_requestor = _max_requests_per_requestor;
    }

    // GETTERS
    function _getToken(uint id) internal returns (Token storage) {
        for(uint i = 0; i < tokens.length;i++){
            if(id == tokens[i].id){
                return tokens[i];
            }
        }
        revert('Token not found');
    }
    function getToken(address token) external view returns (Token memory) {
        for(uint i = 0; i < tokens.length;i++){
            if(token == tokens[i].evm_address){
                return tokens[i];
            }
        }
        revert('Token not found');
    }

     // MAIN   ================================================================ >
    function requestRegistration (address token_address, string calldata token_name, string calldata token_symbol, string calldata antelope_account_name) external {
        _removeOutdatedRegistrationRequests();
        // TODO: LET USERS ADD TOKENS to requests
        // NEED TO CHECK ERC20 BRIDGEABLE COMPLIANCE, etc...
    }

    function _removeRegistrationRequest (uint id) internal {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].id == id){
               requests[i] = requests[requests.length];
               requests.pop();
            }
        }
    }

    function approveToken (uint id) external onlyOwner {
        // TODO: Let prods.evm approve a token
        _removeRegistrationRequest(id);
    }

    function addToken () external onlyOwner {
        _removeOutdatedRegistrationRequests();
        // TODO: Let prods.evm add a token
    }

    function unpauseToken (uint id) external onlyOwner {
        Token storage token = _getToken(id);
        token.active = true;
        emit Unpaused(token.evm_address, token.symbol, token.name);
    }

    function pauseToken (uint id) external onlyOwner {
       Token storage token = _getToken(id);
       token.active = false;
       emit Paused(token.evm_address, token.symbol, token.name);
    }

    function _removeOutdatedRegistrationRequests () internal {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].timestamp < block.timestamp - 8640000){

            }
        }
    }

    function removeToken (uint id) external onlyOwner {
        for(uint i; i < tokens.length;i++){
            if(tokens[i].id == id){
               tokens[i] = tokens[tokens.length];
               tokens.pop();
               emit Deleted(tokens[i].evm_address, tokens[i].symbol, tokens[i].name);
            }
        }
        revert('Token not found');
    }
}