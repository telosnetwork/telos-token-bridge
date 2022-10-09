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
    let antelope_bridge, evm_bridge, user, token, register;
    beforeEach(async () => {
        [antelope_bridge, user] = await ethers.getSigners();
        let TokenRegister = await ethers.getContractFactory("TokenBridgeRegister");
        register = await TokenRegister.deploy(antelope_bridge.address, MAX_REQUESTS, REQUEST_VALIDITY);
        let EVMBridge = await ethers.getContractFactory("TokenBridge");
        evm_bridge = await EVMBridge.deploy(antelope_bridge.address, register.address, MAX_REQUESTS, HALF_TLOS);
        let ERC20Bridgeable = await ethers.getContractFactory("ERC20Bridgeable");
        token = await ERC20Bridgeable.deploy(evm_bridge.address,  TOKEN_NAME, TOKEN_SYMBOL);
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
    describe(":: Brige to Antelope", async function () {
        beforeEach(async () => {
            // Add a registered token
            expect(await register.addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit("TokenAdded");
            // Add tokens to user (fake bridging from Antelope)
            expect(await evm_bridge.connect(antelope_bridge).bridgeTo(token.address, user.address, ONE_TLOS)).to.emit('Transfer');
            // User tries to bridge
            await expect(token.connect(user).approve(evm_bridge.address, ONE_TLOS)).to.not.be.reverted;
        })
        it("Should let any sender request bridging of a registered ERC20Bridgeable token to Antelope" , async function () {
            expect(await evm_bridge.connect(user).bridge(token.address, HALF_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.emit('BridgeToAntelopeRequested');
        });
        it("Should not let senders send request if allowance is too low" , async function () {
            await expect(evm_bridge.connect(user).bridge(token.address, ONE_TLOS.mul(2), "exrsrv.tf", {value: HALF_TLOS})).to.be.revertedWith('Allowance is too low');
        });
        it("Should not let senders request more than " + MAX_REQUESTS + " requests" , async function () {
            // Add a registered token
            expect(await register.addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit("TokenAdded");
            expect(await token.connect(user).approve(evm_bridge.address, ONE_TLOS.mul(MAX_REQUESTS + 1)));
            // Add tokens to user (fake bridging from Antelope)
            expect(await evm_bridge.connect(antelope_bridge).bridgeTo(token.address, user.address, ONE_TLOS.mul(MAX_REQUESTS + 1))).to.emit('Transfer');
            for(var i = 0; i < MAX_REQUESTS; i++){
                expect(await evm_bridge.connect(user).bridge(token.address, ONE_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.emit('BridgeToAntelopeRequested');
            }
            await expect(evm_bridge.connect(user).bridge(token.address, ONE_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.be.reverted;
        });
        it("Should let antelope bridge set request as successful" , async function () {
            expect(await evm_bridge.connect(user).bridge(token.address, HALF_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.emit('BridgeToAntelopeRequested');
            expect(await evm_bridge.connect(antelope_bridge).requestSuccessful(0)).to.emit('BridgeToAntelopeSucceeded');
        });
        it("Should not let a random address set request as successful" , async function () {
            expect(await evm_bridge.connect(user).bridge(token.address, HALF_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.emit('BridgeToAntelopeRequested');
            await expect(evm_bridge.connect(user).requestSuccessful(0)).to.be.revertedWith('Only the Antelope bridge EVM address can trigger this method !');
        });
        it("Should let antelope bridge remove a request" , async function () {
            expect(await evm_bridge.connect(user).bridge(token.address, HALF_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.emit('BridgeToAntelopeRequested');
            await expect(evm_bridge.connect(antelope_bridge).removeRequest(0)).to.not.be.reverted;
        });
        it("Should not let a random address remove a request" , async function () {
            // Add a registered token
            expect(await register.addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit("TokenAdded");
            // Add tokens to user (fake bridging from Antelope)
            expect(await evm_bridge.connect(antelope_bridge).bridgeTo(token.address, user.address, ONE_TLOS)).to.emit('Transfer');
            // User tries to bridge
            await expect(token.connect(user).approve(evm_bridge.address, ONE_TLOS)).to.not.be.reverted;
            expect(await evm_bridge.connect(user).bridge(token.address, HALF_TLOS, "exrsrv.tf", {value: HALF_TLOS})).to.emit('BridgeToAntelopeRequested');
            await expect(evm_bridge.connect(user).removeRequest(0)).to.be.revertedWith('Only the Antelope bridge EVM address can trigger this method !');
        });
    });
    describe(":: Brige from Antelope", async function () {
        it("Should let antelope bridge mint & send a registered ERC20Bridgeable token" , async function () {
            expect(await register.addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit("TokenAdded");
            expect(await evm_bridge.connect(antelope_bridge).bridgeTo(token.address, user.address, ONE_TLOS)).to.emit('Transfer');
            expect(await token.balanceOf(user.address)).to.equal(ONE_TLOS);
        });
        it("Should not let random addresses mint & send a registered ERC20Bridgeable token" , async function () {
            expect(await register.addToken(token.address, ANTELOPE_DECIMALS, ANTELOPE_ACCOUNT_NAME, TOKEN_NAME)).to.emit("TokenAdded");
            await expect(evm_bridge.connect(user).bridgeTo(token.address, user.address, ONE_TLOS)).to.be.revertedWith('Only the Antelope bridge EVM address can trigger this method !');
        });
    });
});
