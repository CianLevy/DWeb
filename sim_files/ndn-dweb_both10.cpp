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


#include "ns3/ptr.h"
#include "ndn-dweb_both10/dweb-producer.hpp"

#include <chrono>
#include <thread>

uint32_t node_count = 10;  //Number of gateway routers
std::string repoPath = "/home/cian/Documents/GitHub/DWeb";
std::ifstream file(repoPath + "/topologies/topology10.csv");  // Topology file for both ns-3 and ndn network
std::ifstream file1(repoPath + "/topologies/topology10.csv"); // Topology file for ndn network static route. We can use above pointer also but I kept both separately.
std::ifstream myfile(repoPath + "/txts/requests.txt");        // This is basically names of ndn prefix. ndnSIM require these prefix to identify requests from different consumers. We can aim for one prefix for one published object and can use OID as a prefix also. Need to study how ndn consumer/producer works to integrate it better with blockchain.

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("DWeb");
 
  NodeContainer randomNodeSample(double proportion){
      int target_count = (int)(node_count * proportion);
      NormalRandomVariable distribution;

  }


  int main(int argc, char *argv[])
  {

    // These two instructions will make sure that ns-3 simulation work in real time and accept data from Docker Containers.
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // These instructions set the default value of P2P network parameters like bandwidth, latency and Queue size.
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("200Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("0ms"));
    Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("200p"));

    //Enable packet metadata, this will be used for tracing purpose.
    Packet::EnablePrinting();

    // Here we are creating (1) number of nodes (named as ghost_nodes) for creation of ns-3 network.

    // These Temp nodes are created many places. Actually we can not connect ns-3 node with Docker COntainer directly as it has p2p network.
    // So we created these two intermediary for connecting Docker Container with ns-3 nodes.
    // (* ns-3 node) ----- Temp node 0----Temp node 1------Docker Container
    NodeContainer tempNodes0;
    tempNodes0.Create(2);

    CsmaHelper csma0;
    csma0.SetChannelAttribute("DataRate", StringValue("200Mbps"));
    csma0.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    NetDeviceContainer csmadevices0 = csma0.Install(tempNodes0); // Since Docker Container can not be connected with P2P nodes, we installed CSMA on intermediary nodes.

    InternetStackHelper internetTempN;
    internetTempN.Install(tempNodes0); // Internet stack is installed with intermediary nodes
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.248");                   // IP network is set to install IP address to interfaces of nodes
    Ipv4InterfaceContainer interfacesgen = address.Assign(csmadevices0); // Actual address from network 192.168.0.0/28 is assigned to CSMA interface

    // Now, tapBridge is defined. The tapBride is used to connect ns-3 node with Docker Container. To read more about how a Docker Container is connected to ns-3 node, you can follow my blog at:

    // https://sites.google.com/thapar.edu/ramansinghtechpages/home

    //In this blog I have explained procedures needs to follow for Docker COntainer to ns-3 interaction

    TapBridgeHelper tapBridge(interfacesgen.GetAddress(0));
    tapBridge.SetAttribute("Mode", StringValue("UseBridge"));
    tapBridge.SetAttribute("DeviceName", StringValue("tap_gate"));
    tapBridge.Install(tempNodes0.Get(0), csmadevices0.Get(0));
    std::cout << "NS-3 node IP address:" << interfacesgen.GetAddress(1) << "\n";

    // Reading topology
    AnnotatedTopologyReader topologyReader("", 25);
    topologyReader.SetFileName("src/ndnSIM/examples/topologies/topology.txt");
    topologyReader.Read();

    // Install NDN stack on all ndn nodes
    ndn::StackHelper ndnHelper;

    NodeContainer nodes;

    for (uint32_t i = 0; i < node_count; ++i)
      nodes.Add(Names::Find<Node>("Node" + std::to_string(i)));

    ndnHelper.setPolicy("nfd::cs::popularity_priority_queue");
    ndnHelper.Install(nodes);

    // Set BestRoute strategy
    std::string broadcast_prefix = "/broadcast";
    ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/best-route");
    ndn::StrategyChoiceHelper::Install(nodes, broadcast_prefix, "/localhost/nfd/strategy/multicast");
    

    // Installing global routing interface on all ndn nodes
    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
    ndnGlobalRoutingHelper.Install(nodes);
    ndnGlobalRoutingHelper.AddOrigins(broadcast_prefix, nodes);

        boost::asio::io_service io_service2;
    // boost::asio::ip::tcp::acceptor acceptor(io_service2);

    // boost::asio::socket_base::reuse_address option(true);
    // acceptor.set_option(option);

    
    UDPClient::instance().connect(tempNodes0.Get(1), "192.168.1.6", 3000, io_service2);
    std::thread thread1([&io_service2]() { io_service2.run(); });


    // Install NDN applications
    std::string prefix = "";

    int producer_count = 2;

    ndn::AppHelper consumerHelper("DWebConsumer");
    // Consumer will request /prefix/0, /prefix/1, ...
    // consumerHelper.SetPrefix("/prefixtemp");
    consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
    consumerHelper.SetAttribute("MaxSeq", IntegerValue(0));
    consumerHelper.SetAttribute("ProduceRate", StringValue("0.25"));
    consumerHelper.SetAttribute("TotalProducerCount", UintegerValue(producer_count));
    consumerHelper.SetAttribute("StartingMetadataCap", UintegerValue(10));
    consumerHelper.SetAttribute("NumberOfContents", StringValue("100")); // 10 different contents
    consumerHelper.Install(nodes.Get(3));                        // first node
    consumerHelper.Install(nodes.Get(5)); 
    // consumerHelper.SetPrefix("/prefix");
    // consumerHelper.Install(nodes.Get(0));  

    // Producer
    ndn::AppHelper producerHelper("DWebProducer");
    // Producer will reply to all requests starting with /prefix
    // ndnGlobalRoutingHelper.AddOrigins(prefix, nodes.Get(2));
    // ndnGlobalRoutingHelper.AddOrigins(prefix, nodes.Get(7));

    
    // producerHelper.SetPrefix("/prefixtemp");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.SetAttribute("ProduceRate", StringValue("0.25"));
    producerHelper.SetAttribute("TotalProducerCount", UintegerValue(producer_count));

    NodeContainer producers;

    producers.Add(nodes.Get(2));
    producers.Add(nodes.Get(7));

    for (int i = 0; i < producer_count; ++i){
      producerHelper.SetAttribute("ProducerNumber", UintegerValue(i));
      producerHelper.Install(producers.Get(i));
    }



    ndn::GlobalRoutingHelper::CalculateRoutes();

    Simulator::Stop(Seconds(30)); // We need to modify time of simulation as per our requirements. We can have more simulation time if we cants to test Ethereum or Docker Container.
    ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1));
    ndn::AppDelayTracer::InstallAll("app-delays-trace_3.txt");
    Simulator::Run();
    Simulator::Destroy();

    return 0;
  }

} // namespace ns3

int main(int argc, char *argv[])
{
  return ns3::main(argc, argv);
}