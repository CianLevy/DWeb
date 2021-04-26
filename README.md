# DWeb

This repository contains an ndnSIM simulation to simulate DWeb, scripts for running the required Ethereum blockchain in docker containers, and the modifications required to implement the MAGIC caching algorithm. The repository is hosted at: https://github.com/CianLevy/DWeb.

## Instructions for running
To run the simulation, a local installation of ndnSIM is required in a top level folder called ```ndnSIM```. ndnSIM/ns-3 should be installed directly in the folder such that it contains the ```ns-3``` folder at the top level. The installation is provided as a submodule which can be obtained by recursively cloning the repository. Alternatively, the installation can be created manually be following the steps provided here: https://ndnsim.net/current/getting-started.html
Note: the simulation is based on ndnSIM 2.8 and involves direct modification of this ndnSIM version and therefore may not be compatible with other versions.

After creating the local ns-3 and ndnSIM installation, run the ```configure_waf.sh``` in the scripts folder to initialise the environment.

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

* -v - runs the simulation with parameters specified in ```dweb_simulation.cpp``` and uses the ndnSIM visualiser
* -d - runs the simulation with parameters specified in ```dweb_simulation.cpp``` with gdb
* -l <log_folder_name> - runs the simulation with parameters specified in ```dweb_simulation.cpp``` and adds the output logs to the folder ```scripts/logs/<log_folder_name>```
* -run_test - runs a number of repeat simulations with both the LRU-based LCE and MAGIC caching strategies across a range of cache budgets. The corresponding logs for the simulations will be output to ```scripts/logs/```
* -run_test_alpha - runs a number of repeat simulations with both the LRU-based LCE and MAGIC caching strategies across a range of ![\alpha](https://latex.codecogs.com/svg.latex?\Large&space;\alpha)  (or ![s](https://latex.codecogs.com/svg.latex?\Large&space;s)) power values in the Zipf-Mandelbrot request distribution
* run_test_baseline - runs a number of repeat simulations with both the popularity-based LCE and DWeb broadcast caching strategies across a range of cache budgets


### Analysing the simulation results
When run with the -l or -run_test* flags, the logs produced in the ```scripts/logs/``` directory are automatically parsed using the ```read_logs.py``` script. The script produces additionally summary files within the corresponding log folder for the cache behaviour, consumer request history, producer production history, along with plots of the request distribution and cache hit ratio vs object popularity. The relevant files contain key performance metrics such as the average cache hit ratio, number of caching operations, and average hop count. 

For a test run over a range of values (run with a -run_test* flag), the output directories for each individual simulation can be copied to a single directory for further analysis. e.g. the output from the test can be copied to a single directory ```test_1``` which contains all the results associated with the test. Running ```python summarise_test.py <test_dir_name>```  will produce a csv with a name corresponding to the test directory name in the ```scripts/test_summaries``` directory. The resulting csv will contain columns for the caching strategy, cache budget, average hop count, hit ratio, cache insertion count, and eviction count for each simulation in the test.


Finally, running ```python plot_results.py <topology_size> <test_dir_name>``` (where topology_size is the number of nodes in the topology) will produce a set of plots in the ```images/<test_dir_name>``` directory showing the average hop count, cache hit ratio, and number of caching operations across the range of test values.

The test summaries associated with the results presented in my dissertation are provided in the ```summary_100_nodes.csv``` and ```summary_50_nodes.csv``` files along with their respective plots in the ```images``` directory. The raw logs used to generate the summaries are available upon request, but are not provided here due to their combined size exceeding 50 GB.
