// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
 function allowance(address owner, address spender) external returns(uint);
}

interface ITokenBridgeRegister {
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
 function getToken(address _token) external view returns(Token memory);
}

contract TokenBridge is Ownable {
    event  BridgeToAntelopeRequested(address indexed sender, address indexed token, uint amount, string recipient);
    event  BridgeToAntelopeSucceeded(address indexed sender, address indexed token, uint amount, string recipient);
    event  BridgeFromAntelopeSucceeded(address indexed recipient, address indexed token, uint amount);

    uint fee;
    uint8 max_requests;

    address bridge_evm_address;
    ITokenBridgeRegister token_register;

    struct Request {
        uint id;
        address sender;
        address token;
        uint amount;
        uint requested_at;
        string receiver;
    }

    Request[] requests;

    mapping(address => uint) request_counts;
    uint id_count;

    constructor(address _bridge_evm_address, ITokenBridgeRegister _token_register,  uint8 _max_requests, uint _fee) {
        fee = _fee;
        token_register = _token_register;
        max_requests = _max_requests;
        bridge_evm_address = _bridge_evm_address;
        id_count = 0;
    }

    modifier onlyBridge() {
        require(
            bridge_evm_address == msg.sender,
            "Only the Antelope bridge EVM address can trigger this method !"
        );
        _;
    }

     // SETTERS  ================================================================ >
     function setFee(uint _fee) external onlyOwner {
        fee = _fee;
     }

     function setMaxRequests(uint8 _max_requests) external onlyOwner {
        max_requests = _max_requests;
     }

     function setBridgeEvmAddress(address _bridge_evm_address) external onlyOwner {
        bridge_evm_address = _bridge_evm_address;
     }

     // MAIN   ================================================================ >
     // SUCCESS ANTELOPE CALLBACK
     function requestSuccessful(uint id) external onlyBridge {
        for(uint i = 0; i < requests.length; i++){
            if(requests[i].id == id){
                _removeRequest(i);
                emit BridgeToAntelopeSucceeded(requests[i].sender, requests[i].token, requests[i].amount, requests[i].receiver);
            }
        }
     }

     function _removeRequest(uint i) internal {
        address sender = requests[i].sender;
        requests[i] = requests[requests.length - 1];
        requests.pop();
        request_counts[sender]--;
     }

     function removeRequest(uint id) external onlyBridge returns (bool) {
        for(uint i = 0; i < requests.length; i++){
            if(requests[i].id == id){
                _removeRequest(i);
                return true;
            }
        }
        return false;
     }

     // FROM ANTELOPE BRIDGE
     function bridgeTo(IERC20Bridgeable token, address receiver, uint amount) external onlyBridge {
        ITokenBridgeRegister.Token memory tokenData = token_register.getToken(address(token));
        require(tokenData.active, "Bridging is paused for token.");
        try token.mint(receiver, amount) {
            emit BridgeFromAntelopeSucceeded(receiver, address(token), amount);
        } catch {
            revert('Token cannot be minted');
        }
     }

     // TO ANTELOPE
     function bridge(IERC20Bridgeable token, uint amount, string calldata receiver) external payable {
        require(msg.value >= fee, "Needs TLOS fee passed");
        require(request_counts[msg.sender] < max_requests, "Maximum requests reached. Please wait for them to complete before trying again.");

        // Check token is registered
        ITokenBridgeRegister.Token memory tokenData = token_register.getToken(address(token));
        require(tokenData.active, "Bridging is paused for token.");

        // Check allowance is ok
        uint remaining = token.allowance(msg.sender, address(this));
        require(remaining >= amount, "Allowance is too low");

        // Burn it <(;;)>
        try token.burnFrom(msg.sender, amount){
            // ADD REQUEST (TOKENS ALREADY BURNED)
            requests.push(Request (id_count, msg.sender, address(token), amount, block.timestamp, receiver));
            request_counts[msg.sender]++;
            // Increment id count
            id_count++;
            emit BridgeToAntelopeRequested(msg.sender, address(token), amount, receiver);
        } catch {
            revert('Tokens could not be burned...');
        }
     }
}