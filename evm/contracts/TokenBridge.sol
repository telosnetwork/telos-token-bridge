// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
 function allowance(address owner, address spender) external returns(uint);
}

interface IPairBridgeRegister {
 struct Pair {
    bool active;
    uint id;
    address evm_address;
    uint8 evm_decimals;
    uint8 antelope_decimals;
    string antelope_account;
    string symbol;
    string name;
 }
 function getPair(address _token) external view returns(Pair memory);
}

contract TokenBridge is Ownable {
    event  BridgeToAntelopeRequested(address indexed sender, address indexed token, uint amount, string recipient);
    event  BridgeToAntelopeSucceeded(address indexed sender, address indexed token, uint amount, string recipient);
    event  BridgeFromAntelopeSucceeded(address indexed recipient, address indexed token, uint amount);

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
    }

    Request[] public requests;

    mapping(address => uint) request_counts;
    uint id_count;

    constructor(address _antelope_bridge_evm_address, IPairBridgeRegister _pair_register,  uint8 _max_requests_per_requestor, uint _fee) {
        fee = _fee;
        pair_register = _pair_register;
        max_requests_per_requestor = _max_requests_per_requestor;
        antelope_bridge_evm_address = _antelope_bridge_evm_address;
        id_count = 0;
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
                emit BridgeToAntelopeSucceeded(requests[i].sender, requests[i].token, requests[i].amount, requests[i].receiver);
                _removeRequest(i);
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
     function bridgeTo(IERC20Bridgeable token, address receiver, uint amount) external onlyAntelopeBridge {
        IPairBridgeRegister.Pair memory pairData = pair_register.getPair(address(token));
        require(pairData.active, "Bridging is paused for pair");
        try token.mint(receiver, amount) {
            emit BridgeFromAntelopeSucceeded(receiver, address(token), amount);
        } catch {
            // Todo: refund Antelope token ??
            revert('Token cannot be minted');
        }
     }

     // TO ANTELOPE
     function bridge(IERC20Bridgeable token, uint amount, string calldata receiver) external payable {
        require(msg.value >= fee, "Needs TLOS fee passed");
        require(request_counts[msg.sender] < max_requests_per_requestor, "Maximum requests reached. Please wait for them to complete before trying again.");

        // Check token is registered
        IPairBridgeRegister.Pair memory pairData = pair_register.getPair(address(token));
        require(pairData.active, "Bridging is paused for token");

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