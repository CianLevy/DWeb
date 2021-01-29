# DWeb

This repo contains an ndnSIM simulation to simulate DWeb, scripts for running the required Ethereum blockchain in docker containers, and the modifications required to implement the MAGIC caching algorithm.

## Instructions for running
To run the simulation, a local installation of ndnSIM is required in a top level folder called ```ndnSIM```. ndnSIM/ns-3 should be installed directly in the folder such that it contains the ```ns-3``` folder at the top level.

Prior to running the simulation, ensure that the ```repoPath``` in ```ndn-dweb_both.cpp``` is set to the correct path to the repo. Once the correct path has been set, running the ```run_simulation.sh``` script from the ```scripts``` folder will copy the required modifications to the ndnSIM installation and run the simulation for ```ndn-dweb_both.cpp```. Note: the modifications will overwrite existing files in ndnSIM.
