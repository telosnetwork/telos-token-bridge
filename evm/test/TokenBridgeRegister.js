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

describe("TokenBridgeRegister Contract", function () {
    let antelope_bridge, evm_bridge, user, token, register;
    beforeEach(async () => {
        [antelope_bridge, user] = await ethers.getSigners();
        let ERC20Bridgeable = await ethers.getContractFactory("ERC20Bridgeable");
        token = await ERC20Bridgeable.deploy(antelope_bridge.address,  TOKEN_NAME, TOKEN_SYMBOL);
        let TokenRegister = await ethers.getContractFactory("TokenBridgeRegister");
        register = await TokenRegister.deploy(antelope_bridge.address, MAX_REQUESTS, REQUEST_VALIDITY);
        let EVMBridge = await ethers.getContractFactory("TokenBridge");
        evm_bridge = await EVMBridge.deploy(antelope_bridge.address, register.address, MAX_REQUESTS, HALF_TLOS);
    })
    describe(":: Deployment", async function () {
        it("Should have correct max requests" , async function () {
            expect(await register.max_requests_per_requestor()).to.equal(MAX_REQUESTS);
        });
        it("Should have correct request validity" , async function () {
            expect(await register.request_validity_seconds()).to.equal(REQUEST_VALIDITY);
        });
        it("Should have correct antelope bridge evm address" , async function () {
            expect(await register.antelope_bridge_evm_address()).to.equal(antelope_bridge.address);
        });
    });
    describe(":: Setters", async function () {
        it("Should let owner set max requests" , async function () {
            await expect(register.setMaxRequestsPerRequestor(MAX_REQUESTS + 10)).to.not.be.reverted;
            expect(await register.max_requests_per_requestor()).to.equal(MAX_REQUESTS + 10);
        });
        it("Should let owner set request validity" , async function () {
            await expect(register.setRequestValiditySeconds(REQUEST_VALIDITY + 10)).to.not.be.reverted;
            expect(await register.request_validity_seconds()).to.equal(REQUEST_VALIDITY + 10);
        });
        it("Should let owner set antelope bridge evm address" , async function () {
            await expect(register.setAntelopeBridgeEvmAddress(user.address)).to.not.be.reverted;
            expect(await register.antelope_bridge_evm_address()).to.equal(user.address);
        });
    });
    describe(":: Registration Request", async function () {
        it("Should let a token owner add a registration request" , async function () {
            await expect(register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
        });
        it("Should not allow two registration requests for same token" , async function () {
            await expect(register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.be.reverted;
        });
        it("Should not let a random address add a registration request" , async function () {
            await expect(register.connect(user).requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.be.reverted;
        });
        it("Should let the antelope bridge sign a request" , async function () {
            await expect(register.connect(antelope_bridge).signRequest(id, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequestSigned');
        });
        it("Should not let a random address sign a request" , async function () {
        });
        it("Should let the antelope bridge approve a request" , async function () {
        });
        it("Should not let a random address approve a request" , async function () {
        });
    });
    describe(":: Token CRUD", async function () {
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
    describe(":: Getters", async function () {
        it("Should return an existing token by address" , async function () {
        });
    });
});
