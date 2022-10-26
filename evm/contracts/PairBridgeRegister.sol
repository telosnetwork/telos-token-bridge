// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

import {IERC20Bridgeable} from "./IERC20Bridgeable.sol";

contract PairBridgeRegister is Ownable {
    event  RegistrationRequested(uint request_id, address requestor, address indexed token, string symbol, string name);
    event  RegistrationRequestSigned(uint request_id, address indexed token, string antelope_account, string antelope_symbol, string evm_symbol, string evm_name);
    event  RegistrationRequestApproved(uint request_id, address indexed token, string antelope_account, string antelope_name, string symbol, string name);
    event  RegistrationRequestDeleted(uint request_id, address indexed token, string antelope_account);
    event  PairPaused(uint pair_id, address indexed evm_token, string evm_symbol, string evm_name, string antelope_account);
    event  PairAdded(uint pair_id, address indexed evm_token, string evm_symbol, string evm_name, string antelope_account, string antelope_symbol);
    event  PairUnpaused(uint pair_id, address indexed evm_token, string evm_symbol, string evm_name, string antelope_account);
    event  PairDeleted(uint pair_id, address indexed evm_token, string evm_symbol, string evm_name, string antelope_account);

    uint request_id;
    uint pair_id;

    struct Pair {
        bool active;
        uint id;
        address evm_address;
        uint evm_decimals; // Needs to be uint256 because tight packing makes uint8 storage not reliably readable from Antelope
        uint antelope_decimals; // Needs to be uint256 because tight packing makes uint8 storage not reliably readable from Antelope
        string antelope_issuer_name;
        string antelope_account_name;
        string antelope_symbol_name;
        string evm_symbol;
        string evm_name;
    }

    struct Request {
        uint id;
        address sender;
        address evm_address;
        uint evm_decimals; // Needs to be uint256 because tight packing makes uint8 storage not reliably readable from Antelope
        uint timestamp;
        uint antelope_decimals; // Needs to be uint256 because tight packing makes uint8 storage not reliably readable from Antelope
        string antelope_issuer_name;
        string antelope_account_name;
        string antelope_symbol_name;
        string evm_symbol;
        string evm_name;
    }

    Pair[] public pairs;
    Request[] public requests;
    mapping(address => uint) public request_counts;
    uint public request_validity_seconds;
    uint8 public max_requests_per_requestor;
    address public antelope_bridge_evm_address;

    constructor(address _antelope_bridge_evm_address, uint8 _max_requests_per_requestor, uint _request_validity_seconds) {
        pair_id = 1;
        request_id = 1;
        antelope_bridge_evm_address = _antelope_bridge_evm_address;
        max_requests_per_requestor = _max_requests_per_requestor;
        request_validity_seconds = _request_validity_seconds;
    }

    fallback() external payable {
        payable(antelope_bridge_evm_address).transfer(msg.value);
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
    function _getPair(uint id) internal view returns (Pair storage) {
        for(uint i = 0; i < pairs.length;i++){
            if(id == pairs[i].id){
                return pairs[i];
            }
        }
        revert('Pair not found');
    }
    function getPair(address token) external view returns (Pair memory) {
        for(uint i = 0; i < pairs.length;i++){
            if(token == pairs[i].evm_address){
                return pairs[i];
            }
        }
        revert('Pair not found');
    }
    function getPairByAntelopeAccount(string calldata _antelope_account_name) external view returns (Pair memory) {
        bytes32 account_name = keccak256(abi.encodePacked(_antelope_account_name));
        for(uint i = 0; i < pairs.length;i++){
            if(account_name == keccak256(abi.encodePacked(pairs[i].antelope_account_name))){
                return pairs[i];
            }
        }
        revert('Pair not found');
    }

    // REQUEST   ================================================================ >

    // Let token owners ask for registration of their pairs
    // returns the uint request id
    function requestRegistration (IERC20Bridgeable token) external returns(uint) {

        require(request_counts[msg.sender] < max_requests_per_requestor, "Maximum request reached, please wait until approved or delete one");

        _removeOutdatedRegistrationRequests(); // Remove outdated registrations as new one are added...

        // Check pair for this token does not already exist
        require(_tokenPairExists(address(token)) == false, "Token has pair already registered");
        require(_tokenPairRegistrationExists(address(token)) == false, "Token has pair already being registered");

        // Check ERC20Bridgeable compliance
        require(_isERC20Bridgeable(token), "Token is not ERC20Bridgeable");

        // Check sender is owner (after ERC20Brigeable check to make sure the token is ownable)
        address owner = token.owner();
        require(msg.sender == owner, "Sender must be token owner");

        // Get the token data
        uint evm_decimals = token.decimals();
        string memory evm_symbol = token.symbol();
        string memory evm_name = token.name();

        // Add a token pair registration request
        requests.push(Request(request_id, msg.sender, address(token), evm_decimals, block.timestamp, uint(0), "", "", "", evm_symbol, evm_name ));
        emit RegistrationRequested(request_id, msg.sender, address(token), evm_symbol, evm_name);
        request_id++;
        request_counts[msg.sender]++;
        return (request_id - 1); // Owner needs that ID to sign from Antelope next
    }

    // Let Antelope bridge sign request (after verification of the eosio.token token there)
    function signRegistrationRequest (uint id, uint _antelope_decimals, string calldata _antelope_account_name, string calldata _antelope_issuer_name, string calldata _antelope_symbol) external onlyBridge {
        _removeOutdatedRegistrationRequests();
        require(_antelopeTokenPairExists(_antelope_account_name) == false, "Antelope token already in a pair");
        require(_isEosioName(_antelope_symbol), "Symbol must be an eosio name");
        require(_isEosioName(_antelope_account_name), "Account must be an eosio name");
        for(uint i = 0; i < requests.length ;i++){
            if(requests[i].id == id){
               requests[i].antelope_account_name = _antelope_account_name;
               requests[i].antelope_symbol_name = _antelope_symbol;
               requests[i].antelope_issuer_name = _antelope_issuer_name;
               requests[i].antelope_decimals = _antelope_decimals;
               emit RegistrationRequestSigned(requests[i].id, requests[i].evm_address,  _antelope_account_name, _antelope_symbol, requests[i].evm_symbol, requests[i].evm_name);
               return;
            }
        }
        revert('Request not found');
    }

    // Let owner or registration sender delete requests
    function removeRegistrationRequest (uint id) external {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].id == id){
                require(msg.sender == owner() || msg.sender == requests[i].sender, 'Only the requestor or contract owner can invoke this method');
                _removeRegistrationRequest(i);
            }
        }
    }

    function _removeRegistrationRequest (uint i) internal {
       address sender = requests[i].sender;
       emit RegistrationRequestDeleted(requests[i].id, requests[i].evm_address,  requests[i].antelope_account_name);
       requests[i] = requests[requests.length-1];
       requests.pop();
       request_counts[sender]--;
    }

    function _removeOutdatedRegistrationRequests () internal {
        uint i = 0;
        while(i<requests.length){
            if(requests[i].timestamp < block.timestamp - request_validity_seconds){
                _removeRegistrationRequest(i);
            } else {
                i++;
            }
        }
    }

    // Let owner, the prods.evm EVM address, approve pairs, adding them to the registry
    // returns the uint token id
    function approveRegistrationRequest (uint id) external onlyOwner returns(uint) {
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].id == id){
               require(requests[i].antelope_decimals > 0, "Request not signed by Antelope");
               pairs.push(Pair(true, pair_id, requests[i].evm_address, requests[i].evm_decimals, requests[i].antelope_decimals, requests[i].antelope_issuer_name, requests[i].antelope_account_name, requests[i].antelope_symbol_name,  requests[i].evm_symbol, requests[i].evm_name));
               emit PairAdded(pair_id, requests[i].evm_address, requests[i].evm_symbol, requests[i].evm_name, requests[i].antelope_account_name, requests[i].antelope_symbol_name);
               requests[i] = requests[requests.length - 1];
               requests.pop();
               pair_id++;
               return (pair_id - 1);
            }
        }
        revert('Request not found');
    }

    // TOKEN   ================================================================ >
    function addPair (IERC20Bridgeable evm_token, uint antelope_decimals, string calldata antelope_issuer_name, string calldata antelope_account_name, string calldata antelope_symbol_name) external onlyOwner returns(uint) {
        uint evm_decimals = evm_token.decimals();
        string memory evm_symbol = evm_token.symbol();
        string memory evm_name = evm_token.name();
        pairs.push(Pair(true, pair_id, address(evm_token), evm_decimals, antelope_decimals, antelope_issuer_name, antelope_account_name, antelope_symbol_name, evm_symbol, evm_name));
        emit PairAdded(pair_id, address(evm_token), evm_symbol, evm_name, antelope_account_name, antelope_symbol_name);
        pair_id++;
        return (pair_id - 1);
    }

    function unpausePair (uint id) external onlyOwner {
        Pair storage token = _getPair(id);
        token.active = true;
        emit PairUnpaused(id, token.evm_address, token.evm_symbol, token.evm_name, token.antelope_account_name);
    }

    function pausePair (uint id) external onlyOwner {
       Pair storage token = _getPair(id);
       token.active = false;
       emit PairPaused(id, token.evm_address, token.evm_symbol, token.evm_name, token.antelope_account_name);
    }

    function removePair (uint id) external onlyOwner {
        for(uint i; i < pairs.length;i++){
            if(pairs[i].id == id){
               emit PairDeleted(pairs[i].id, pairs[i].evm_address, pairs[i].evm_symbol, pairs[i].evm_name, pairs[i].antelope_account_name);
               pairs[i] = pairs[pairs.length - 1];
               pairs.pop();
               return;
            }
        }
        revert('Pair not found');
    }

    // UTILS   ================================================================ >
    function _isEosioName(string calldata text) internal view returns(bool) {
        if(bytes(text).length > 12){
            return false;
        }
        // Todo: check admitted characters
        return true;
    }
    function _isERC20Bridgeable(IERC20Bridgeable token) internal view returns(bool) {
        try token.supportsInterface(0x01ffc9a7) {
            return true;
        } catch {
            return false;
        }
    }
    function _antelopeTokenPairExists(string calldata account_name) internal view returns (bool) {
        bytes32 account =  keccak256(abi.encodePacked(account_name));
        for(uint i = 0; i < pairs.length;i++){
            if(account == keccak256(abi.encodePacked(pairs[i].antelope_account_name)) ){
                return true;
            }
        }
        for(uint k = 0; k < requests.length;k++){
            if(account == keccak256(abi.encodePacked(requests[k].antelope_account_name)) ){
                return true;
            }
        }
        return false;
    }
    function _tokenPairExists(address token) internal view returns (bool) {
        for(uint i = 0; i < pairs.length;i++){
            if(token == pairs[i].evm_address){
                return true;
            }
        }
        return false;
    }
    function _tokenPairRegistrationExists(address token) internal view returns (bool) {
        for(uint i = 0; i < requests.length;i++){
            if(token == requests[i].evm_address){
                return true;
            }
        }
        return false;
    }
}