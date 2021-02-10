module.exports = {
  networks: {
      development: {
          host: "127.0.0.1",
          port: 8545,
          network_id: "*",
          gasPrice: 0,
          from: "0x3f93fdc9c946e63ee50966559b2bce63f1c03084"
      },
      local: {
        host: "127.0.0.1",     
        port: 8545,            
        network_id: "4", // Rinkeby's id          
        from: "0x3f93fdc9c946e63ee50966559b2bce63f1c03084" 
      },
  },
  // compilers: {
  //     solc: {
  //       settings: {
  //         optimizer: {
  //           enabled: true,
  //           runs: 200   // Optimize for how many times you intend to run the code
  //         },
  //       }
  //     }
  // }
}