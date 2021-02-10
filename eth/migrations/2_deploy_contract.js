const Objects = artifacts.require("./publishobject.sol");

module.exports = function (deployer) {
    deployer.deploy(Objects);
}; 