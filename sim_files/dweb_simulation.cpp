/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.

Modified by: Raman Singh <rasingh@tcd.ie>,<raman.singh@thapar.edu> Post Doctoral Fellow, School of Computer Science and Statistics, Trinity College Dublin, University of Dublin, Ireland.


 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/command-line.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/qos-txop.h"
#include "ns3/node.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/csma-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/random-variable-stream.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>

#include <stdlib.h>  
#include "ns3/ptr.h"
#include "dweb_simulation/dweb-producer.hpp"

#include <chrono>
#include <thread>


// Simulation parameters
uint32_t node_count = 100;  // Number of gateway routers
double simulation_duration = 60;
std::string topology = "100_topology.txt";

// Producer parameters
int initial_object_count = 5;
double producer_proportion = 0.2;
double initial_publish_split = 0.8;
int total_objects = 200;
double produce_rate;
bool object_broadcast_enabled = false;

// Consumer parameters
double consumer_proportion = 0.8;
int request_rate = 2;
double request_alpha = 0.7;

// Cache parameters
std::string cs_policy = "nfd::cs::popularity_priority_queue";
double cache_budget = 0.01;
bool magic_enabled = true;


namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("DWeb");
 
  NodeContainer randomNodeSample(double proportion){
    int target_count = (int)(node_count * proportion);
    Ptr<UniformRandomVariable> distribution = CreateObject<UniformRandomVariable>();
    std::vector<int> nodes;

    NodeContainer result;

    for (int i = 0; i < node_count; ++i)
      nodes.push_back(i);

    for (int i = 0; i < target_count; ++i){
      uint32_t index = rand() % (nodes.size() - 1);
      std::string node_name = "Node" + std::to_string(nodes.at(index));
      result.Add(Names::Find<Node>(node_name));

      nodes.erase(nodes.begin() + index);
    }
    return result;
  }

  void printNodeContainer(NodeContainer nodes){
    for (int i = 0; i < nodes.size(); ++i){
      std::cout << Names::FindName(nodes.Get(i)) << " ";
    }
    std::cout << std::endl;
  }

  int main(int argc, char *argv[])
  {

    // These two instructions will make sure that ns-3 simulation work in real time and accept data from Docker Containers.
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));

    CommandLine cmd;
    cmd.Parse(argc, argv);

    for (int i = 0; i < argc; ++i)
      std::cout << "Arg" << i << ": " << argv[i] << std::endl;

    if (argc >= 4){
      cs_policy = argv[1];
      magic_enabled = boost::lexical_cast<bool>(argv[2]);
      cache_budget = std::stod(argv[3]);
    }
    if (argc == 5){
      request_alpha = std::stod(argv[4]);
    }


    if (cs_policy == "dweb_broadcast"){
      cs_policy = "nfd::cs::popularity_priority_queue";
      object_broadcast_enabled = true;
    }

    srand(0);

    // Reading topology
    AnnotatedTopologyReader topologyReader("", 25);
    topologyReader.SetFileName("src/ndnSIM/examples/topologies/" + topology);
    topologyReader.Read();

    // Install NDN stack on all ndn nodes
    ndn::StackHelper ndnHelper;
    NodeContainer nodes;

    for (uint32_t i = 0; i < node_count; ++i)
      nodes.Add(Names::Find<Node>("Node" + std::to_string(i)));


    NodeContainer producers = randomNodeSample(producer_proportion);
    std::cout << "[NodeContainer] Producer nodes: ";
    printNodeContainer(producers);
    int producer_count = producers.size();

    // total_objects = initial_object_count * producer_count + producer_count * (simulation_duration * std::stod(produce_rate));
    initial_object_count = int(total_objects * initial_publish_split / producer_count);
    produce_rate = (total_objects * (1 - initial_publish_split) / producer_count) / simulation_duration;

    std::cout << "Initial object count: " << initial_object_count << std::endl;
    std::cout << "Produce rate: " << produce_rate << std::endl;
    std::cout << "Alpha: " << request_alpha << std::endl;

    ndnHelper.setPolicy(cs_policy); 
    ndnHelper.setMAGICEnabled(magic_enabled);

    if (object_broadcast_enabled)
      ndnHelper.setDWebObjectBroadcastEnabled(true);
  
    int cs_size = (int)(total_objects * cache_budget);
    std::cout << "CS budget " << cache_budget << std::endl;
    std::cout << "Setting CS size to " << cs_size << std::endl;

    ndnHelper.setCsSize(cs_size);  
    ndnHelper.Install(nodes);


    std::string broadcast_prefix = "/broadcast";

    ndn::StrategyChoiceHelper::Install(nodes, broadcast_prefix, "/localhost/nfd/strategy/multicast");
  	// Uncomment to use the broadcast strategy for all requests, as described in DWeb
    // ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/multicast");

    

    // Installing global routing interface on all ndn nodes
    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    ndnGlobalRoutingHelper.Install(nodes);
    ndnGlobalRoutingHelper.AddOrigins(broadcast_prefix, nodes);

    boost::asio::io_service io_service;
    UDPClient::instance().connect("127.0.0.1", 3000, io_service);
    std::thread thread([&io_service]() { io_service.run(); });
    

    // Consumers
    ndn::AppHelper consumerHelper("DWebConsumer");
    consumerHelper.SetAttribute("Frequency", StringValue(std::to_string(request_rate)));
    consumerHelper.SetAttribute("MaxSeq", IntegerValue(0));
    consumerHelper.SetAttribute("ProduceRate", StringValue(std::to_string(produce_rate)));
    consumerHelper.SetAttribute("TotalProducerCount", UintegerValue(producer_count));
    consumerHelper.SetAttribute("StartingMetadataCap", UintegerValue(initial_object_count * producer_count));
    consumerHelper.SetAttribute("s", StringValue(std::to_string(request_alpha)));

    NodeContainer consumers = randomNodeSample(consumer_proportion);
    std::cout << "[NodeContainer] Consumer nodes: ";
    printNodeContainer(consumers);
    consumerHelper.Install(consumers); 

    // Producers
    ndn::AppHelper producerHelper("DWebProducer");  
    producerHelper.SetAttribute("PayloadSize", StringValue("256"));
    producerHelper.SetAttribute("ProduceRate", StringValue(std::to_string(produce_rate)));
    producerHelper.SetAttribute("TotalProducerCount", UintegerValue(producer_count));
    producerHelper.SetAttribute("InitialObjects", UintegerValue(initial_object_count));
    if (object_broadcast_enabled)
      producerHelper.SetAttribute("EnableObjectBroadcast", StringValue("enabled"));

    for (int i = 0; i < producer_count; ++i){
      producerHelper.SetAttribute("ProducerNumber", UintegerValue(i));
      producerHelper.Install(producers.Get(i));
    }

    ndn::GlobalRoutingHelper::CalculateRoutes();
    ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
    Simulator::Stop(Seconds(simulation_duration));
    Simulator::Run();
    Simulator::Destroy();

    io_service.stop();
    thread.join();
  
    return 0;
  }

} // namespace ns3

int main(int argc, char *argv[])
{
  return ns3::main(argc, argv);
}
