// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) public virtual;
 function mint(address _recipient, uint256 _amount);
}

contract TokenBridge is Ownable {
    event  Sent(address indexed sender, address indexed token, uint amount, string recipient);
    event  Received(address indexed recipient, address indexed token, uint amount);

    uint fee;
    uint8 _max_requests;

    address bridge_evm_address;

    struct Request {
        uint id
        address sender
        address token
        uint amount
        timestamp requested_at
        string receiver
    }

    Request[] requests;

    mapping(address => uint) request_counts
    uint count;

    constructor(uint _fee, uint _max_requests, address _bridge_evm_address) {
        fee = _fee;
        max_requests = _max_requests;
        bridge_evm_address = _bridge_evm_address;
        count = 0;
    }

    modifier onlyBridge() {
        require(
            bridge_evm_address == msg.sender,
            "TokenBridge: only the Antelope bridge can trigger this method !"
        );
        _;
    }

     // SETTERS  ================================================================ >
     function setFee(uint _fee) external onlyOwner returns(bool) {
        fee = _fee;
        return true;
     }

     function setMaxRequests(uint8 _max_requests) external onlyOwner returns(bool) {
        max_requests = _max_requests;
        return true;
     }

     function setOracleEvmAddress(address _oracle_evm_address) external onlyOwner returns(bool) {
        oracle_evm_address = _oracle_evm_address;
        return true;
     }

     // MAIN   ================================================================ >
     // REMOVE A REQUEST
     function removeRequest(uint id) external onlyBridge returns (bool) {
        for(uint i = 0; i < requests.length; i++){
            if(requests[i].id == id){
                require(msg.sender == owner(), "Only the owner can delete a request by id");
                address sender = requests[i].sender;
                requests[i] = requests[requests.length - 1];
                requests.pop();
                requests_count[sender]--;
                return true;
            }
        }
        return false;
     }

     // FROM ANTELOPE BRIDGE
     function bridgeTo(IERC20Bridgeable token, address receiver) external onlyBridge {
        // TODO: Check token registered
        try token.mint(receiver, amount){

        } catch {
            revert('Tokens could not be minted');
        }
     }

     // TO ANTELOPE
     function bridgeFrom(IERC20Burnable token, uint amount, string calldata receiver) external payable {
        check(msg.value >= fee, "Needs TLOS fee passed");
        check(requests_count[caller] < max_requests, "Maximum requests reached. Please wait for them to complete before trying again.");
        // TODO: Check token registered
        // TODO: ALLOWANCE
        // BURN TOKENS
        try token.burnFrom(msg.sender, amount){
            // ADD REQUEST (TOKENS ALREADY BURNED)
            requests.push(Request (count, msg.sender, token, amount, block.timestamp, receiver));
            requests_count[msg.sender]++;
            // INCREMENT COUNT FOR ID
            count++;
        } catch {
            revert('Tokens could not be burned...');
        }
     }
}