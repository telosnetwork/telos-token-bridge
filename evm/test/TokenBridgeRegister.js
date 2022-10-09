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
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.requests(0)).to.not.be.reverted;
        });
        // What happens if someone adds tokens they do not own ??? Maybe need to add antelope account on antelope sign only....
        it("Should not allow two registration requests for same token" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME + "2")).to.be.revertedWith('Token already being registered');
            await expect(register.requestRegistration(token2.address, ANTELOPE_ACCOUNT_NAME)).to.be.revertedWith('Token already being registered');
        });
        it("Should remove request when new request after " + REQUEST_VALIDITY + " seconds" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.requests(0)).to.not.be.reverted;
            await ethers.provider.send('evm_increaseTime', [31]);
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.requests(1)).to.be.reverted;
        });
        it("Should not let a random address add a registration request" , async function () {
            await expect(register.connect(user).requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.be.revertedWith('Sender must be token owner');
        });
        it("Should let the antelope bridge sign a request" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            expect(await register.connect(antelope_bridge).signRegistrationRequest(0, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit('RegistrationRequestSigned');
        });
        it("Should not let a random address sign a request" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.connect(user).signRegistrationRequest(0, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.be.revertedWith('Only the Antelope bridge EVM address can trigger this method !');
        });
        it("Should let the requestor or owner remove a request" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            expect(await register.removeRegistrationRequest(0)).to.emit('RegistrationRequestRemoved');
        });
        it("Should not let a random address remove a request" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.connect(user).removeRegistrationRequest(0)).to.be.revertedWith('Only the requestor or contract owner can invoke this method');
        });
        it("Should let the owner approve a request" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            expect(await register.connect(antelope_bridge).signRegistrationRequest(0, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit('RegistrationRequestSigned');
            expect(await register.connect(antelope_bridge).approveRegistrationRequest(0)).to.emit('RegistrationRequestApproved');
        });
        it("Should not let an unsigned request be approved" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.connect(antelope_bridge).approveRegistrationRequest(0)).to.be.revertedWith('Request not signed by Antelope');
        });
        it("Should not let a random address approve a request" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            await expect(register.connect(user).approveRegistrationRequest(0)).to.be.revertedWith('Ownable: caller is not the owner');
        });
        it("Should not allow a registration request if token is registered" , async function () {
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            expect(await register.connect(antelope_bridge).signRegistrationRequest(0, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit('RegistrationRequestSigned');
            expect(await register.connect(antelope_bridge).approveRegistrationRequest(0)).to.emit('RegistrationRequestApproved');
            await expect(register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.be.revertedWith('Token already registered');
        });
    });
    describe(":: Token CRUD", async function () {
        it("Should let owner add a token" , async function () {
            expect(await register.addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit("TokenRegistered");
        });
        it("Should not let other addresses add a token" , async function () {
            await expect(register.connect(user).addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.be.revertedWith('Ownable: caller is not the owner');
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
            expect(await register.requestRegistration(token.address, ANTELOPE_ACCOUNT_NAME)).to.emit('RegistrationRequested');
            expect(await register.connect(antelope_bridge).signRegistrationRequest(0, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit('RegistrationRequestSigned');
            expect(await register.connect(antelope_bridge).approveRegistrationRequest(0)).to.emit('RegistrationRequestApproved');
            await expect(register.getToken(token.address)).to.not.be.reverted;
        });
    });
});
