require("@nomiclabs/hardhat-waffle");
require("solidity-coverage");
require('hardhat-deploy');

/**
 * @type import('hardhat/config').HardhatUserConfig
 */
module.exports = {
    solidity: "0.8.9",
    settings: {
        optimizer: {
            enabled: true,
            runs: 200,
        },
    },
    namedAccounts: {
        deployer: 'privatekey://0x87ef69a835f8cd0c44ab99b7609a20b2ca7f1c8470af4f0e5b44db927d542084'
    },
    networks: {
        testnet: {
            chainId: 41,
            url: "https://testnet.telos.net/evm",
            accounts: ['0x87ef69a835f8cd0c44ab99b7609a20b2ca7f1c8470af4f0e5b44db927d542084'],
        },
        local: {
            chainId: 41,
            url: "http://localhost:7000/evm",
            accounts: ['0x8dd3ec4846cecac347a830b758bf7e438c4d9b36a396b189610c90b57a70163d'],
        }
    },
};