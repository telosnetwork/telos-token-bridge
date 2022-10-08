// SPDX-License-Identifier: MIT

pragma solidity ^0.8.4;

import "@openzeppelin/contracts/access/Ownable.sol";

interface IERC20Bridgeable {
 function burnFrom(address _account, uint256 _amount) external;
 function mint(address _recipient, uint256 _amount) external;
}

contract TokenBridgeRegister is Ownable {

    address public bridge;
    address public antelope_bridge;

    uint count;

    struct Token {
        address evm_address;
        uint8 evm_decimals;
        uint8 antelope_decimals;
        string antelope_account;
        string symbol;
        string name;
    }

    Token[] public tokens;

    constructor(address _bridge, address _antelope_bridge) {
        bridge = _bridge;
        antelope_bridge = _antelope_bridge;
        count = 0;
    }

    modifier onlyBridges() {
        require(
            bridge == msg.sender || antelope_bridge == msg.sender,
            "Only the EVM or Antelope bridges can trigger this method !"
        );
        _;
    }

     // SETTERS  ================================================================ >
     function setBridge(address _bridge) external onlyOwner {
        bridge = _bridge;
     }
     function setAntelopeBridge(address _antelope_bridge) external onlyOwner {
        antelope_bridge = _antelope_bridge;
     }

     // MAIN   ================================================================ >
    function addToken () external onlyBridges {

    }
    function pauseToken () external onlyBridges {

    }
    function removeToken () external onlyBridges {

    }
}