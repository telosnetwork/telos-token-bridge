// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
 function decimals() external returns(uint8);
 function symbol() external returns(string memory);
 function name() external returns(string memory);
}

contract TokenBridgeRegister is Ownable {
    event  RegistrationRequested(address indexed token, string antelope_account, string symbol, string name);
    event  Approved(address indexed token, string symbol, string name);
    event  Paused(address indexed token, string symbol, string name);
    event  Unpaused(address indexed token, string symbol, string name);
    event  Deleted(address indexed token, string symbol, string name);

    uint request_id;
    uint token_id;
    uint public request_validity_seconds;
    uint8 public max_requests_per_requestor;
    address public antelope_bridge_evm_address;

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
        uint id;
        address sender;
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
    mapping(address => uint) public request_counts;

    constructor(address _antelope_bridge_evm_address, uint8 _max_requests_per_requestor, uint _request_validity_seconds) {
        token_id = 0;
        request_id = 0;
        antelope_bridge_evm_address = _antelope_bridge_evm_address;
        max_requests_per_requestor = _max_requests_per_requestor;
        request_validity_seconds = _request_validity_seconds;
    }

    modifier onlyBridge() {
        require(
            antelope_bridge_evm_address == msg.sender,
            "Only the Antelope bridge EVM address can trigger this method !"
        );
        _;
    }

    // SETTERS
    function setMaxRequestsPerRequestor(uint8 _max_requests_per_requestor) external onlyOwner {
        max_requests_per_requestor = _max_requests_per_requestor;
    }
    function setRequestValiditySeconds(uint _request_validity_seconds) external onlyOwner {
        request_validity_seconds = _request_validity_seconds;
    }
    function setAntelopeBridgeEvmAddress(address _antelope_bridge_evm_address) external onlyOwner {
        antelope_bridge_evm_address = _antelope_bridge_evm_address;
    }

    // GETTERS
    function _getToken(uint id) internal view returns (Token storage) {
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

    // REQUEST   ================================================================ >

    // Let token owners ask for registration of their tokens
    // returns the uint request id
    function requestRegistration (IERC20Bridgeable token, string calldata antelope_account_name) external returns(uint) {
        require(request_counts[msg.sender] >= max_requests_per_requestor, "Maximum request reached, please wait until approved or delete one");

        _removeOutdatedRegistrationRequests(); // Remove outdated registrations as new one are added...

        // TODO: NEED TO CHECK ERC20 BRIDGEABLE COMPLIANCE, etc...

        // Get token data
        uint8 evm_decimals = token.decimals();
        string memory symbol = token.symbol();
        string memory name = token.name();

        // Add token request
        requests.push(Request(request_id, msg.sender, address(token), evm_decimals, uint8(0), block.timestamp, antelope_account_name, symbol, name ));
        request_id++;
        request_counts[msg.sender]++;
        emit RegistrationRequested(address(token), antelope_account_name, symbol, name);
        return (request_id - 1); // Owner needs that ID to sign from Antelope next
    }

    // Let Antelope bridge sign request (after verification of the eosio.token token there)
    function signRegistrationRequest (uint id, uint8 _antelope_decimals, string calldata _antelope_account_name) external onlyBridge {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].id == id){
               requests[i].antelope_account_name = _antelope_account_name;
               requests[i].antelope_decimals = _antelope_decimals;
            }
        }
        revert('Request not found');
    }

    // Let owner or registration sender delete requests
    function removeRegistrationRequest (uint id) external {

    }

    function _removeRegistrationRequest (uint i) internal {
       address sender = requests[i].sender;
       requests[i] = requests[requests.length];
       requests.pop();
       request_counts[sender]--;
    }

    function _removeOutdatedRegistrationRequests () internal {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].timestamp < block.timestamp - request_validity_seconds){
                _removeRegistrationRequest(i);
            }
        }
    }

    // Let owner, the prods.evm EVM address, approve tokens, adding them to the registry
    // returns the uint token id
    function approveRegistrationRequest (uint id) external onlyOwner returns(uint) {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].id == id){
               require(requests[i].antelope_decimals > 0, "Request not signed by Antelope");
               requests[i] = requests[requests.length];
               requests.pop();
               tokens.push(Token(true, token_id, requests[i].evm_address, requests[i].evm_decimals, requests[i].antelope_decimals, requests[i].antelope_account_name, requests[i].symbol, requests[i].name));
               token_id++;
               return (token_id - 1);
            }
        }
        revert('Request not found');
    }

    // TOKEN   ================================================================ >

    // Let owner, the prods.evm EVM address, add tokens
    function addToken () external onlyOwner {
        // TODO: Let prods.evm add a token
        token_id++;
    }

    // Let owner, the prods.evm EVM address, un-pause tokens
    function unpauseToken (uint id) external onlyOwner {
        Token storage token = _getToken(id);
        token.active = true;
        emit Unpaused(token.evm_address, token.symbol, token.name);
    }

    // Let owner, the prods.evm EVM address, pause tokens
    function pauseToken (uint id) external onlyOwner {
       Token storage token = _getToken(id);
       token.active = false;
       emit Paused(token.evm_address, token.symbol, token.name);
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