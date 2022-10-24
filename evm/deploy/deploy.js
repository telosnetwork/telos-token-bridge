module.exports = async ({getNamedAccounts, deployments}) => {
    const {deploy} = deployments;
    const {deployer} = await getNamedAccounts();
    const ANTELOPE_BRIDGE_EVM_ADDRESS = "0x9893808bfd47d19cc8800f77c44d2f34f4ef1d26";
    const MAX_REG_REQUEST_PER_REQUESTOR = 10;
    const MAX_BRIDGE_REQUEST_PER_REQUESTOR = 10;
    const MAX_REQUEST_VALIDITY = 1200;

     const register = await deploy('PairBridgeRegister', {
         from: deployer,
         args: [ANTELOPE_BRIDGE_EVM_ADDRESS, MAX_REG_REQUEST_PER_REQUESTOR, MAX_REQUEST_VALIDITY],
     });
     console.log("Register deployed to:", register.address);

    const bridge = await deploy('TokenBridge', {
        from: deployer,
        args: [ANTELOPE_BRIDGE_EVM_ADDRESS, register.address,  MAX_BRIDGE_REQUEST_PER_REQUESTOR, "500000000000000000", "1000000000000000000"],
    });

    console.log("Bridge deployed to:", bridge.address);

};
module.exports.tags = ['TokenBridge'];