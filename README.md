# DWeb

This repo contains an ndnSIM simulation to simulate DWeb, scripts for running the required Ethereum blockchain in docker containers, and the modifications required to implement the MAGIC caching algorithm.

## Instructions for running
To run the simulation, a local installation of ndnSIM is required in a top level folder called ```ndnSIM```. ndnSIM/ns-3 should be installed directly in the folder such that it contains the ```ns-3``` folder at the top level.

### Creating the Ethereum network
Prior to running the simulation, the Ethereum network must be created and running. To create the network, run the ```start_here.sh``` script in the ```scripts``` folder. Once the containers have been started along with their respective Ethereum client instances, connect to the ```multi_gateway``` container and start the miner as show in the commands below.

```bash
docker exec -it multi_gateway bin/sh
geth attach
miner.start(1)
admin.startRPC(host='localhost', port=8545, cors="", apis="db,eth,net,web3,personal")
web3.personal.unlockAccount(web3.personal.listAccounts[0], "multi_gateway", 3600)
```

Note: the Ethereum account associated with the ```multi_gateway``` container requires a non-zero account balance in order to interact with the Ethereum contract. The miner is used to provide the account balance.

### Launching the Python Server
After the Ethereum network has been created or the ```multi_gateway``` container has been launched directly from the ```gateway_node.sh``` script, the Python server can be launched in the container as shown below. 

```bash
docker exec -it multi_gateway bin/sh
cd eth
python3 server.py
```

### Running the simulation
Prior to running the simulation, ensure that the ```repoPath``` in ```dweb_simulation.cpp``` is set to the correct path to the repo. Once the correct path has been set, running the ```run_simulation.sh``` script from the ```scripts``` folder will copy the required modifications to the ndnSIM installation and run the simulation for ```dweb_simulation.cpp```. Note: the modifications will overwrite existing files in ndnSIM. When running the script, an argument must be provided to specify the simulation type. The following arguments are supported:

* -v - runs the simulation with parameters specified in ```dweb_simulation.cpp`` and uses the ndnSIM visualiser
* -d - runs the simulation with parameters specified in ```dweb_simulation.cpp`` with gdb
* -l <log_folder_name> - runs the simulation with parameters specified in ```dweb_simulation.cpp``` and adds the output logs to the folder ```scripts/logs/<log_folder_name>```
* -run_test - runs a number of repeat simulations with both the LRU-based LCE and MAGIC caching strategies across a range of cache budgets. The corresponding logs for the simulations will be output to ```scripts/logs/```
* -run_test_alpha - runs a number of repeat simulations with both the LRU-based LCE and MAGIC caching strategies across a range of $\alpha$ (or $s$) power values in the Zipf-Mandelbrot request distribution
* run_test_baseline - runs a number of repeat simulations with both the popularity-based LCE and DWeb broadcast caching strategies across a range of cache budgets