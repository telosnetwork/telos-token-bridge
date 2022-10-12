// SPDX-License-Identifier: MIT
pragma solidity ^0.8.4;

import {ERC20} from "@openzeppelin/contracts/token/ERC20/ERC20.sol";
import {ERC165Storage} from "@openzeppelin/contracts/utils/introspection/ERC165Storage.sol";
import {ERC20Burnable} from "@openzeppelin/contracts/token/ERC20/extensions/ERC20Burnable.sol";
import {IERC20Bridgeable} from "./IERC20Bridgeable.sol";
import {Ownable} from "@openzeppelin/contracts/access/Ownable.sol";

contract ERC20Bridgeable is ERC20, ERC165Storage, ERC20Burnable, Ownable {
    address public bridge;

    constructor(address _bridge, string memory _name, string memory _symbol) ERC20(_name, _symbol) {
        bridge = _bridge;
        _registerInterface(type(IERC20Bridgeable).interfaceId);
    }

    modifier onlyBridge() {
        require(
            bridge == msg.sender,
            "BridgeableERC20: only the bridge can trigger this method !"
        );
        _;
    }

    // @dev called from the bridge when tokens are locked on Antelope side
    function mint(address _recipient, uint256 _amount)
        public
        virtual
        onlyBridge
    {
        _mint(_recipient, _amount);
    }

    // @dev called from the bridge when tokens are received on EVM side
    function burnFrom(address _account, uint256 _amount)
        public
        virtual
        override(ERC20Burnable)
        onlyBridge
    {
        super.burnFrom(_account, _amount);
    }
}