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
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <fstream>

uint32_t nLocality = 10;                                                              //Number of gateway routers
uint32_t n_requests = 20;                                                             // Number of interests shown by each consumer in ndn
std::string repoPath = "/home/cian/Documents/GitHub/DWeb";
std::ifstream file(repoPath + "/topologies/topology10.csv");  // Topology file for both ns-3 and ndn network
std::ifstream file1(repoPath + "/topologies/topology10.csv"); // Topology file for ndn network static route. We can use above pointer also but I kept both separately.
std::ifstream myfile(repoPath + "/txts/requests.txt");  // This is basically names of ndn prefix. ndnSIM require these prefix to identify requests from different consumers. We can aim for one prefix for one published object and can use OID as a prefix also. Need to study how ndn consumer/producer works to integrate it better with blockchain.

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("ndn dweb");
  /*static void SinkRx (Ptr<const Packet> p, const Address &ad)
{
  std::cout << *p << std::endl;
}

static void PingRtt (std::string context, Time rtt)
{
  std::cout << context << " " << rtt << std::endl;
} */

  int main(int argc, char *argv[])
  {

    // These two instructions will make sure that ns-3 simulation work in real time and accept data from Docker Containers.
    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // These instructions set the default value of P2P network parameters like bandwidth, latency and Queue size.
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("200Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
    Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));

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
    std::cout << "The IP addreess of ns-3 node is:" << interfacesgen.GetAddress(1) << "\n";

    // FROM HERE NDN network is created + parallel ns-3 nodes are connected to similar topology
    NodeContainer nodes;
    nodes.Create(nLocality); // nLocality number of nodes are created for ndnSIM

    //  some names like node0,node1 etc is assigned to each node. This is not a mandatory step though.
    std::string nodename;
    for (uint32_t kk = 0; kk < nLocality; kk++)
    {
      nodename = "node" + std::to_string(kk);
      Names::Add(nodename, nodes.Get(kk));
    }

    // Read topology and create links and Connecting nodes using two links
    PointToPointHelper p2p;

    std::string str1;
    uint32_t loc1, loc2;
    uint32_t cont_count = 0;
    std::string gname_t, gname_t_ghost;
    std::string gnamedev_t, gnamedev_t_ghost;
    std::string gnameint_t, gnameint_t_ghost;
    std::string baseip_t;

    // Topology file is opened to read how the nodes are connected to each other to make topology
    while (std::getline(file, str1))
    {
      std::vector<int> vect;

      std::stringstream ss(str1);

      for (int i; ss >> i;)
      {
        vect.push_back(i);
        if (ss.peek() == ',')
          ss.ignore();
      }
      loc1 = vect[0];
      loc2 = vect[1];
      //std::cout<<loc1<<"\t"<<loc2<<"\n";
      if (loc1 == nLocality || loc2 == nLocality)
      {
        continue;
      }
      gname_t = "gname" + std::to_string((cont_count));
      gname_t_ghost = "gname_ghost" + std::to_string((cont_count));
      gnamedev_t = "gnamedev" + std::to_string((cont_count));
      gnamedev_t_ghost = "gnamedev_ghost" + std::to_string((cont_count));
      gnameint_t = "gnameint" + std::to_string((cont_count));
      gnameint_t_ghost = "gnameint_ghost" + std::to_string((cont_count));

      //   Connect ndn nodes to create topology

      NodeContainer gname_t = NodeContainer(nodes.Get(loc1), nodes.Get(loc2));
      NetDeviceContainer gnamedev_t = p2p.Install(gname_t);
    }

    // Install NDN stack on nodes

    ndn::StackHelper ndnHelper;
    ndnHelper.SetDefaultRoutes(true);
    ndnHelper.setPolicy("nfd::cs::lru"); // Here we can define the cache strategy we wants to use like FIFO, LFU or LRU. Custom cache strategy can also be developed
    ndnHelper.setCsSize(5);              // This is size of cache in terms of number of objects it can store
    ndnHelper.Install(nodes);            // ndn strategy is installed on these nodes

    // Install NDN Routes Manually: Here static routing will be defined to route ndn packet.
    // This can not be automatic as we have use dynamic routing in ns-3 and both can not be dynamic at the same time.
    // The ndn route is taken from the topology file itself.

    std::string str11;
    uint32_t loc11, loc21;
    while (std::getline(file1, str11))
    {
      std::vector<int> vect1;

      std::stringstream ss1(str11);

      for (int i1; ss1 >> i1;)
      {
        vect1.push_back(i1);
        if (ss1.peek() == ',')
          ss1.ignore();
      }
      loc11 = vect1[0];
      loc21 = vect1[1];
      //std::cout<<loc1<<"\t"<<loc2<<"\n";
      if (loc11 == nLocality || loc21 == nLocality)
      {
        continue;
      }
      std::cout << cont_count << " x " << loc1 << " y " << loc2 << std::endl;
      ndn::FibHelper::AddRoute(nodes.Get(loc11), ndn::Name("/ndnSIM"), nodes.Get(loc21), 1);
    }

    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper; // This will install global routing to nsn nodess
    ndnGlobalRoutingHelper.Install(nodes);

    // Add producer/consumer behaviour

    // Installing applications

    // Consumer
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    // Consumer will request /prefix/0, /prefix/1, ...
    consumerHelper.SetPrefix("/prefix");
    consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
    consumerHelper.Install(nodes.Get(0));                        // first node

    // Producer
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    // Producer will reply to all requests starting with /prefix
    ndnGlobalRoutingHelper.AddOrigins("/prefix", nodes.Get(5));
    producerHelper.SetPrefix("/prefix");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(nodes.Get(5)); // last node
    

    Simulator::Stop(Seconds(6)); // We need to modify time of simulation as per our requirements. We can have more simulation time if we cants to test Ethereum or Docker Container.
    ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1));
    ndn::AppDelayTracer::InstallAll("app-delays-trace_3.txt");
    //ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(1.0));
    //L2RateTracer::InstallAll("drop-trace.txt", Seconds(0.5));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
  }

} // namespace ns3

int main(int argc, char *argv[])
{
  return ns3::main(argc, argv);
}
