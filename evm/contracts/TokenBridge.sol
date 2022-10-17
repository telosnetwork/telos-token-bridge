// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import {Ownable} from "@openzeppelin/contracts/access/Ownable.sol";
import {IERC20Bridgeable} from "./IERC20Bridgeable.sol";

interface IPairBridgeRegister {
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
 function getPair(address _token) external view returns(Pair memory);
}

contract TokenBridge is Ownable {

    event  BridgeToAntelopeRequested(uint request_id, address indexed sender, address indexed token, uint amount, string recipient);
    event  BridgeToAntelopeSucceeded(uint request_id, address indexed sender, address indexed token, uint amount, string recipient);
    event  BridgeFromAntelopeSucceeded(address indexed recipient, address indexed token, uint amount);
    event  BridgeFromAntelopeFailed(address indexed recipient, address indexed token, uint amount, string refund_account);
    event  BridgeFromAntelopeRefunded(uint refund_id);

    uint public fee;
    uint8 public max_requests_per_requestor;

    address public antelope_bridge_evm_address;
    IPairBridgeRegister public pair_register;

    struct Request {
        uint id;
        address sender;
        address token;
        uint amount;
        uint requested_at;
        string receiver;
        string memo;
    }

    Request[] public requests;

    struct Refund {
        uint id;
        uint amount;
        string token;
        string receiver;
    }

    Refund[] refunds;

    mapping(address => uint) public request_counts;
    uint request_id;
    uint refund_id;
    uint public min_amount;

    constructor(address _antelope_bridge_evm_address, IPairBridgeRegister _pair_register,  uint8 _max_requests_per_requestor, uint _fee, uint _min_amount) {
        fee = _fee;
        min_amount = _min_amount;
        pair_register = _pair_register;
        max_requests_per_requestor = _max_requests_per_requestor;
        antelope_bridge_evm_address = _antelope_bridge_evm_address;
        request_id = 0;
    }

    modifier onlyAntelopeBridge() {
        require(
            antelope_bridge_evm_address == msg.sender,
            "Only the Antelope bridge EVM address can trigger this method !"
        );
        _;
    }

     // SETTERS  ================================================================ >
     function setFee(uint _fee) external onlyOwner {
        fee = _fee;
     }

     function setMaxRequestsPerRequestor(uint8 _max_requests_per_requestor) external onlyOwner {
        max_requests_per_requestor = _max_requests_per_requestor;
     }

     function setPairRegister(IPairBridgeRegister _pair_register) external onlyOwner {
        pair_register = _pair_register;
     }

     function setAntelopeBridgeEvmAddress(address _antelope_bridge_evm_address) external onlyOwner {
        antelope_bridge_evm_address = _antelope_bridge_evm_address;
     }

     // MAIN   ================================================================ >
     // SUCCESS ANTELOPE CALLBACK
     function requestSuccessful(uint id) external onlyAntelopeBridge {
        for(uint i = 0; i < requests.length; i++){
            if(requests[i].id == id){
                emit BridgeToAntelopeSucceeded(id, requests[i].sender, requests[i].token, requests[i].amount, requests[i].receiver);
                _removeRequest(i);
            }
        }
     }

     // REFUND ANTELOPE CALLBACK
     function refundSuccessful(uint id) external onlyAntelopeBridge {
        for(uint i = 0; i < requests.length; i++){
            if(refunds[i].id == id){
                refunds[i] = refunds[requests.length - 1];
                emit BridgeFromAntelopeRefunded(id);
                refunds.pop();
            }
        }
     }

     function _removeRequest(uint i) internal {
        address sender = requests[i].sender;
        requests[i] = requests[requests.length - 1];
        requests.pop();
        request_counts[sender]--;
     }

     function removeRequest(uint id) external onlyAntelopeBridge returns (bool) {
        for(uint i = 0; i < requests.length; i++){
            if(requests[i].id == id){
                _removeRequest(i);
                return true;
            }
        }
        return false;
     }

     // FROM ANTELOPE BRIDGE
     function bridgeTo(IERC20Bridgeable token, address receiver, uint amount, string calldata sender) external onlyAntelopeBridge {
        // Get pair & check it is not paused
        IPairBridgeRegister.Pair memory pairData = pair_register.getPair(address(token));
        require(pairData.active, "Bridging is paused for pair");

        try token.mint(receiver, amount) {
            emit BridgeFromAntelopeSucceeded(receiver, address(token), amount);
        } catch {
            // Could not mint... Refund the Antelope tokens
            emit BridgeFromAntelopeFailed(receiver, address(token), amount, sender);
            refunds.push(Refund(refund_id, amount, pairData.antelope_account_name, sender));
        }
     }

     // TO ANTELOPE
     function bridge(IERC20Bridgeable token, uint amount, string calldata receiver, string calldata memo) external payable {
        // Checks
        require(msg.value >= fee, "Needs TLOS fee passed");
        require(request_counts[msg.sender] < max_requests_per_requestor, "Maximum requests reached. Please wait for them to complete before trying again.");
        require(bytes(receiver).length <= 12, "Receiver name cannot be over 12 characters");
        require(bytes(receiver).length > 2, "Receiver name cannot be less than 3 characters");
        require(bytes(memo).length < 257, "Memo cannot be over 256 characters");
        require(amount >= min_amount, "Minimum amount is not reached");

        // Check token has bridge address
        require(token.bridge() == address(this), "The token bridge address must be set to this contract");

        // Check token is registered & active
        IPairBridgeRegister.Pair memory pairData = pair_register.getPair(address(token));
        require(pairData.active, "Bridging is paused for token");

        // Check allowance is ok
        uint remaining = token.allowance(msg.sender, address(this));
        require(remaining >= amount, "Allowance is too low");

        // Todo: determine needed TLOS for max operations and if we add fee for reserves
        // Send fee to the antelope bridge evm address (for the callback, removal of request, ...)
        payable(antelope_bridge_evm_address).transfer(msg.value);

        // Burn it <(;;)>
        try token.burnFrom(msg.sender, amount){
            // Add a request to be picked up and processed by the Antelope side
            requests.push(Request (request_id, msg.sender, address(token), amount, block.timestamp, receiver, memo));
            emit BridgeToAntelopeRequested(request_id, msg.sender, address(token), amount, receiver);
            request_id++;
            request_counts[msg.sender]++;
        } catch {
            // Burning failed... Nothing to do but revert...
            revert('Tokens could not be burned...');
        }
     }
}