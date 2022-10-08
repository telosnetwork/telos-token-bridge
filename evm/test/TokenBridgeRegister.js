const { ethers } = require("hardhat");
const { expect } = require("chai");
const ONE_TLOS = ethers.utils.parseEther("1.0");
const HALF_TLOS = ethers.utils.parseEther("0.5");
const HUNDRED_TLOS = ethers.utils.parseEther('200.0');
const TOKEN_NAME = "My Bridgeable Token";
const TOKEN_SYMBOL = "MBT";
const MAX_REQUESTS = 10;

describe("TokenBridgeRegister Contract", function () {
    let antelope_bridge, evm_bridge, user, erc20bridgeable, register;
    beforeEach(async () => {
        [antelope_bridge, user] = await ethers.getSigners();
        let ERC20Bridgeable = await ethers.getContractFactory("ERC20Bridgeable");
        erc20bridgeable = await ERC20Bridgeable.deploy(antelope_bridge.address,  TOKEN_NAME, TOKEN_SYMBOL);
        let TokenRegister = await ethers.getContractFactory("TokenBridgeRegister");
        register = await TokenRegister.deploy();
        let EVMBridge = await ethers.getContractFactory("TokenBridge");
        evm_bridge = await EVMBridge.deploy(antelope_bridge.address, register.address, MAX_REQUESTS, HALF_TLOS);
    })
    describe(":: Deployment", async function () {

    });
    describe(":: Token CRUD", async function () {
        it("Should let owner add a token" , async function () {
        });
        it("Should not let other addresses add a token" , async function () {
        });
        it("Should let owner remove a token" , async function () {
        });
        it("Should not let other addresses remove a token" , async function () {
        });
        it("Should let owner pause a token" , async function () {
        });
        it("Should let token owner pause a token" , async function () {
        });
        it("Should not let other addresses pause a token" , async function () {
        });
    });
});
