const { ethers } = require("hardhat");
const { expect } = require("chai");
const ONE_TLOS = ethers.utils.parseEther("1.0");
const HALF_TLOS = ethers.utils.parseEther("0.5");
const HUNDRED_TLOS = ethers.utils.parseEther('200.0');
const TOKEN_NAME = "My Bridgeable Token";
const TOKEN_SYMBOL = "MBT";
const MAX_REQUESTS = 10;
const REQUEST_VALIDITY = 30; // 30s
const ANTELOPE_ACCOUNT_NAME = "mytoken";
const ANTELOPE_DECIMALS = 14;

describe("TokenBridge Contract", function () {
    let antelope_bridge, evm_bridge, user, token, token2, register;
    beforeEach(async () => {
        [antelope_bridge, user] = await ethers.getSigners();
        let ERC20Bridgeable = await ethers.getContractFactory("ERC20Bridgeable");
        token = await ERC20Bridgeable.deploy(antelope_bridge.address,  TOKEN_NAME, TOKEN_SYMBOL);
        token2 = await ERC20Bridgeable.deploy(antelope_bridge.address,  TOKEN_NAME + " 2", TOKEN_SYMBOL + "2");
        let TokenRegister = await ethers.getContractFactory("TokenBridgeRegister");
        register = await TokenRegister.deploy(antelope_bridge.address, MAX_REQUESTS, REQUEST_VALIDITY);
        let EVMBridge = await ethers.getContractFactory("TokenBridge");
        evm_bridge = await EVMBridge.deploy(antelope_bridge.address, register.address, MAX_REQUESTS, HALF_TLOS);
    })
    describe(":: Deployment", async function () {
        it("Should have correct max requests" , async function () {
            expect(await evm_bridge.max_requests_per_requestor()).to.equal(MAX_REQUESTS);
        });
        it("Should have correct token register address" , async function () {
            expect(await evm_bridge.token_register()).to.equal(register.address);
        });
        it("Should have correct fee" , async function () {
            expect(await evm_bridge.fee()).to.equal(HALF_TLOS);
        });
        it("Should have correct antelope bridge evm address" , async function () {
            expect(await evm_bridge.antelope_bridge_evm_address()).to.equal(antelope_bridge.address);
        });
    });
    describe(":: Setters", async function () {
        it("Should let owner set max requests" , async function () {
            await expect(evm_bridge.setMaxRequestsPerRequestor(MAX_REQUESTS + 1)).to.not.be.reverted;
            expect(await evm_bridge.max_requests_per_requestor()).to.equal(MAX_REQUESTS + 1);
        });
        it("Should let owner set token register address" , async function () {
            await expect(evm_bridge.setTokenRegister(user.address)).to.not.be.reverted;
            expect(await evm_bridge.token_register()).to.equal(user.address);
        });
        it("Should let owner set fee" , async function () {
            await expect(evm_bridge.setFee(ONE_TLOS)).to.not.be.reverted;
            expect(await evm_bridge.fee()).to.equal(ONE_TLOS);
        });
        it("Should let owner set antelope bridge evm address" , async function () {
            await expect(evm_bridge.setAntelopeBridgeEvmAddress(user.address)).to.not.be.reverted;
            expect(await evm_bridge.antelope_bridge_evm_address()).to.equal(user.address);
        });
    });
    describe(":: Registration Request", async function () {

    });
    describe(":: Token CRUD", async function () {

    });
});
