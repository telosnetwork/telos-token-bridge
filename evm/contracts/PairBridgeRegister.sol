// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
 function decimals() external returns(uint8);
 function symbol() external returns(string memory);
 function name() external returns(string memory);
 function owner() external returns(address);
}

contract PairBridgeRegister is Ownable {
    event  RegistrationRequested(uint request_id, address requestor, address indexed token, string symbol, string name);
    event  RegistrationRequestSigned(uint request_id, address indexed token, string antelope_account, string antelope_name, string symbol, string name);
    event  RegistrationRequestApproved(uint request_id, address indexed token, string antelope_account, string antelope_name, string symbol, string name);
    event  RegistrationRequestDeleted(uint request_id, address indexed token, string antelope_account);
    event  PairPaused(uint pair_id, address indexed token, string symbol, string name, string antelope_account);
    event  PairAdded(uint pair_id, address indexed token, string symbol, string name, string antelope_account);
    event  PairUnpaused(uint pair_id, address indexed token, string symbol, string name, string antelope_account);
    event  PairDeleted(uint pair_id, address indexed token, string symbol, string name, string antelope_account);

    uint request_id;
    uint pair_id;

    struct Pair {
        bool active;
        uint id;
        address evm_address;
        uint8 evm_decimals;
        uint8 antelope_decimals;
        string antelope_account_name;
        string antelope_name;
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
        string antelope_name;
        string antelope_symbol;
        string symbol;
        string name;
    }

    Pair[] public pairs;
    Request[] public requests;
    mapping(address => uint) public request_counts;
    uint public request_validity_seconds;
    uint8 public max_requests_per_requestor;
    address public antelope_bridge_evm_address;

    constructor(address _antelope_bridge_evm_address, uint8 _max_requests_per_requestor, uint _request_validity_seconds) {
        pair_id = 0;
        request_id = 0;
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

        // Check sender is owner
        address owner = token.owner();
        require(msg.sender == owner, "Sender must be token owner");

        // Check pair for this token does not already exist
        require(_tokenPairExists(address(token)) == false, "Token has pair already registered");
        require(_tokenPairRegistrationExists(address(token)) == false, "Token has pair already being registered");

        // TODO: NEED TO CHECK ERC20 BRIDGEABLE COMPLIANCE, etc...

        // Get the token data
        uint8 evm_decimals = token.decimals();
        string memory symbol = token.symbol();
        string memory name = token.name();

        // Add a token pair registration request
        requests.push(Request(request_id, msg.sender, address(token), evm_decimals, uint8(0), block.timestamp, "", "", "", symbol, name ));
        emit RegistrationRequested(request_id, msg.sender, address(token), symbol, name);
        request_id++;
        request_counts[msg.sender]++;
        return (request_id - 1); // Owner needs that ID to sign from Antelope next
    }

    // Let Antelope bridge sign request (after verification of the eosio.token token there)
    function signRegistrationRequest (uint id, uint8 _antelope_decimals, string calldata _antelope_account_name, string calldata _antelope_name, string calldata _antelope_symbol) external onlyBridge {
        _removeOutdatedRegistrationRequests();
        require(_antelopeTokenPairExists(_antelope_account_name) == false, "Antelope token already in a registered pair");
        for(uint i = 0;i<requests.length;i++){
            if(requests[i].id == id){
               requests[i].antelope_account_name = _antelope_account_name;
               requests[i].antelope_symbol = _antelope_symbol;
               requests[i].antelope_name = _antelope_name;
               requests[i].antelope_decimals = _antelope_decimals;
               emit RegistrationRequestSigned(requests[i].id, requests[i].evm_address,  _antelope_account_name, _antelope_name, requests[i].symbol, requests[i].name);
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
               pairs.push(Pair(true, pair_id, requests[i].evm_address, requests[i].evm_decimals, requests[i].antelope_decimals, requests[i].antelope_account_name, requests[i].antelope_name, requests[i].symbol, requests[i].name));
               emit PairAdded(pair_id, requests[i].evm_address, requests[i].symbol, requests[i].name, requests[i].antelope_account_name);
               requests[i] = requests[requests.length - 1];
               requests.pop();
               pair_id++;
               return (pair_id - 1);
            }
        }
        revert('Request not found');
    }

    // TOKEN   ================================================================ >

    // Let owner, the prods.evm EVM address, add pairs
    function addPair (IERC20Bridgeable evm_token, uint8 antelope_decimals, string calldata antelope_account_name, string calldata antelope_name) external onlyOwner returns(uint) {
        uint8 evm_decimals = evm_token.decimals();
        string memory symbol = evm_token.symbol();
        string memory name = evm_token.name();
        pairs.push(Pair(true, pair_id, address(evm_token), evm_decimals, antelope_decimals, antelope_account_name, antelope_name, symbol, name));
        emit PairAdded(pair_id, address(evm_token), symbol, name, antelope_account_name);
        pair_id++;
        return (pair_id - 1);
    }

    // Let owner, the prods.evm EVM address, un-pause pairs
    function unpausePair (uint id) external onlyOwner {
        Pair storage token = _getPair(id);
        token.active = true;
        emit PairUnpaused(id, token.evm_address, token.symbol, token.name, token.antelope_account_name);
    }

    // Let owner, the prods.evm EVM address, pause pairs
    function pausePair (uint id) external onlyOwner {
       Pair storage token = _getPair(id);
       token.active = false;
       emit PairPaused(id, token.evm_address, token.symbol, token.name, token.antelope_account_name);
    }

    function removePair (uint id) external onlyOwner {
        for(uint i; i < pairs.length;i++){
            if(pairs[i].id == id){
               emit PairDeleted(pairs[i].id, pairs[i].evm_address, pairs[i].symbol, pairs[i].name, pairs[i].antelope_account_name);
               pairs[i] = pairs[pairs.length - 1];
               pairs.pop();
               return;
            }
        }
        revert('Pair not found');
    }

    // UTILS   ================================================================ >
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