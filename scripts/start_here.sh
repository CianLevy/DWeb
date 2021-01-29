#!/bin/bash
nLocality=10

# Start the bootnode node

./bootnode.sh

# Create the ehtereum nodes

ip_c_temp=1
ip_c1_temp=1
for i in $(seq $nLocality); do
  if [ $i -gt 32 ]; then
    if (($i % 32 == 1)); then
      ip_c1_temp=1
      ip_c_temp=$(($ip_c_temp + 1))
    fi

  fi
  ./setting_blocks_network.sh $i $nLocality $ip_c_temp $ip_c1_temp

  ip_c1_temp=$(($ip_c1_temp + 1))

done

# Start the miner node here
# ./runminer.sh
