// SPDX-License-Identifier: MIT
// OpenZeppelin Contracts v4.4.1 (utils/introspection/IERC165.sol)

pragma solidity ^0.8.4;

interface IERC20Bridgeable {
    function mint(address _recipient, uint256 _amount) external;
    function burnFrom(address _account, uint256 _amount) external;
    function bridge() external view returns(address);
    function allowance(address owner, address spender) external view returns(uint);
    function owner() external view returns(address);
    function decimals() external view returns(uint8);
    function symbol() external view returns(string memory);
    function name() external view returns(string memory);
    function supportsInterface(bytes4 interfaceId) external view returns (bool);
}