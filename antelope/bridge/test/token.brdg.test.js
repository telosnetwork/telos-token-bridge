const { Chain } = require("qtest-js");
const { expectAction, expectThrow } = require("qtest-js");
const EVM_BRIDGE = "6f989daff4f485aba94583110d555e7af36e531a";
const EVM_REGISTER = "5f989daff4f485aba94583110d555e7af36e531a";

describe("token.brdg.cpp test", () => {
    let chain;
    let account, account2, bridgeAccount;
    let chainName = process.env.CHAIN_NAME || 'TLOS';
    let bridge;

    beforeAll(async () => {
        chain = await Chain.setupChain(chainName);
        account = await chain.system.createAccount("testaccount1");
        account2 = await chain.system.createAccount("testaccount2");
        bridgeAccount = await chain.system.createAccount("token.brdg");
        bridge = await bridgeAccount.setContract({
            abi: "./build/token.brdg.abi",
            wasm: "./build/token.brdg.wasm",
        });
    }, 60000);

    describe(":: Deployment", function () {
        it("Should not let random accounts init configuration", async () => {
            await expectThrow(
                bridge.action.init(
                    { "bridge_address" : EVM_BRIDGE, "register_address" :EVM_REGISTER, "version": "1", "admin": account.name },
                    [{ actor: account.name, permission: "active" }]
                ),
                "missing authority of token.brdg"
            );
        });
        it("Should let token.brdg account init configuration", async () => {
            bridge.action.init(
                { "bridge_address" : EVM_BRIDGE, "register_address" :EVM_REGISTER, "version": "1", "admin": bridge.name },
                [{ actor: bridge.name, permission: "active" }]
            );
        });
    });
    describe(":: Setters", function () {
        it("Should let admin set new evm contracts addresses", async () => {

        });
        it("Should let admin set the admin", async () => {

        });
        it("Should not let random accounts set the admin", async () => {

        });
        it("Should let admin set the version", async () => {

        });
        it("Should not let random accounts set the version", async () => {

        });
    });
    describe(":: Sign EVM registration request", function () {
        it("Should let token owners sign registration requests from Antelope", async () => {

        });
    });
    describe(":: Bridge to EVM", function () {
        it("Should let users bridge a eosio.token token with a registered pair to its paired token on EVM", async () => {

        });
    });
    describe(":: Bridge from EVM", function () {
        it("Should let user bridge a ERC20Bridgeable token with a registered pair to its paired token on Antelope", async () => {

        });
    });
});