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

uint32_t nLocality = 10;  //Number of gateway routers
uint32_t n_requests = 20; // Number of interests shown by each consumer in ndn
std::string repoPath = "/home/cian/Documents/GitHub/DWeb";
std::ifstream file(repoPath + "/topologies/topology10.csv");                          // Topology file for both ns-3 and ndn network
std::ifstream file1(repoPath + "/topologies/topology10.csv");                         // Topology file for ndn network static route. We can use above pointer also but I kept both separately.
std::ifstream myfile(repoPath + "/requests.txt"); // This is basically names of ndn prefix. ndnSIM require these prefix to identify requests from different consumers. We can aim for one prefix for one published object and can use OID as a prefix also. Need to study how ndn consumer/producer works to integrate it better with blockchain.

namespace ns3
{

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

    // The next few blocks of instructions will create pool of IP addresses and assign them in arrays (vectors). Later we will assign these IP addresses to ns-3 network. I have created these pools of IPs to support around 10000 nodes in one simulation, but it wil need huge computation power so we need to simulate our experiment for not more than 200-300 nodes.

    std::vector<std::string> networkipsgn, networkipscont, networkipsgnsecond, networkipscontsecond;
    std::string ip1t, ip1t1, ip1, ip2t, ip2t1, ip2, ip4t, ip4t1, ip4, ip5t, ip5t1, ip5;

    uint32_t ipir, addrb, addrincr;

    // Create the pool of IPaddresses for gateway network P2P
    for (addrb = 0; addrb < 255; addrb++)
    {
      addrincr = -8;

      ipir = 1;
      while (ipir <= 32)
      {
        addrincr = addrincr + 8;
        ip1t = "10.1." + std::to_string(addrb); // For gn-gn network
        ip1t1 = ip1t + ".";
        ip1 = ip1t1 + std::to_string(addrincr);
        networkipsgn.push_back(ip1); //networkipsgn: This vector saves the IP addresses for P2P connection which connects gateway node to another gateway node

        ip2t = "10.10." + std::to_string(addrb);
        ip2t1 = ip2t + ".";
        ip2 = ip2t1 + std::to_string(addrincr);
        networkipsgnsecond.push_back(ip2); //networkipsgnsecond: Additional IP address pool for gateway node to gateway node

        ip4t = "192.168." + std::to_string(addrb + 1);
        ip4t1 = ip4t + ".";
        ip4 = ip4t1 + std::to_string(addrincr);
        networkipscont.push_back(ip4); // networkipscont: for ghost nodes to docker container

        ip5t = "192.168." + std::to_string(addrb + 101);
        ip5t1 = ip5t + ".";
        ip5 = ip5t1 + std::to_string(addrincr);
        networkipscontsecond.push_back(ip5); //networkipscontsecond: for ghost nodes to gateway nodes

        ipir = ipir + 1;
      }
    }

    // Adding more addresses for gn-gn and WiFi STAs
    for (addrb = 0; addrb < 255; addrb++)
    {
      addrincr = -8;
      ipir = 1;
      while (ipir <= 32)
      {
        addrincr = addrincr + 8;
        ip1t = "10.2." + std::to_string(addrb); // For gn-gn network
        ip1t1 = ip1t + ".";
        ip1 = ip1t1 + std::to_string(addrincr);
        networkipsgn.push_back(ip1);

        ip2t = "10.11." + std::to_string(addrb); // For gn-gn network
        ip2t1 = ip2t + ".";
        ip2 = ip2t1 + std::to_string(addrincr);
        networkipsgnsecond.push_back(ip2);

        // ip4t = "192.17." + std::to_string(addrb);
        // ip4t1 = ip4t +".";
        // ip4 = ip4t1 + std::to_string(addrincr);
        // networkipscont.push_back(ip4);

        ipir = ipir + 1;
      }
    }

    // Adding Even more addresses for gateway to gateway connection. Remember we created pool of IPs for more than 10000 nodes.
    for (addrb = 0; addrb < 255; addrb++)
    {
      addrincr = -8;
      ipir = 1;
      while (ipir <= 32)
      {
        addrincr = addrincr + 8;
        ip1t = "10.3." + std::to_string(addrb); // For gn-gn network
        ip1t1 = ip1t + ".";
        ip1 = ip1t1 + std::to_string(addrincr);
        networkipsgn.push_back(ip1);

        ip2t = "10.12." + std::to_string(addrb); // For gn-gn network
        ip2t1 = ip2t + ".";
        ip2 = ip2t1 + std::to_string(addrincr);
        networkipsgnsecond.push_back(ip2);

        //   ip4t = "172.18." + std::to_string(addrb);
        //   ip4t1 = ip4t +".";
        //   ip4 = ip4t1 + std::to_string(addrincr);
        //   networkipscont.push_back(ip4);

        ipir = ipir + 1;
      }
    }

    // Here we are creating (nLocality+2) number of nodes (named as ghost_nodes) for creation of ns-3 network. 2 extra nodes are created for accomodating bootnode and miner node of Ethereum network.

    InternetStackHelper internetTempN;
    NodeContainer nodes_ghost;
    nodes_ghost.Create(nLocality + 2);
    internetTempN.Install(nodes_ghost); // Interet stack is installed on ns-3 network (ghost_nodes) which will make it IP network.
    Ipv4AddressHelper address;

    // These Temp nodes are created many places. Actually we can not connect ns-3 node with Docker COntainer directly as it has p2p network. So we created these two intermediary for connecting Docker Container with ns-3 nodes. (* ns-3 node) ----- Temp node 0----Temp node 1------Docker Container
    NodeContainer tempNodes0;
    tempNodes0.Create(2);

    CsmaHelper csma0;
    csma0.SetChannelAttribute("DataRate", StringValue("200Mbps"));
    csma0.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    NetDeviceContainer csmadevices0 = csma0.Install(tempNodes0); // Since Docker Container can not be connected with P2P nodes, we installed CSMA on intermediary nodes.

    internetTempN.Install(tempNodes0); // Internet stack is installed with intermediary nodes

    address.SetBase("192.168.0.0", "255.255.255.248");                   // IP network is set to install IP address to interfaces of nodes
    Ipv4InterfaceContainer interfacesgen = address.Assign(csmadevices0); // Actual address from network 192.168.0.0/28 is assigned to CSMA interface

    // Now, tapBridge is defined. The tapBride is used to connect ns-3 node with Docker Container. To read more about how a Docker Container is connected to ns-3 node, you can follow my blog at:

    // https://sites.google.com/thapar.edu/ramansinghtechpages/home

    //In this blog I have explained procedures needs to follow for Docker COntainer to ns-3 interaction

    TapBridgeHelper tapBridge(interfacesgen.GetAddress(0));
    tapBridge.SetAttribute("Mode", StringValue("UseBridge"));
    tapBridge.SetAttribute("DeviceName", StringValue("tap_genblk"));
    tapBridge.Install(tempNodes0.Get(0), csmadevices0.Get(0));
    std::cout << "The ipinterface is:" << interfacesgen.GetAddress(0) << "\n";

    // A new P2P connection parameter is created here which will connect one end of intermediary node with ns-3 node since other end is connected with Docker Container now using tapBridge.
    PointToPointHelper p2p_new;
    p2p_new.SetDeviceAttribute("DataRate", StringValue("200Mbps"));
    p2p_new.SetChannelAttribute("Delay", StringValue("5ms"));

    NodeContainer nodes0 = NodeContainer(tempNodes0.Get(1), nodes_ghost.Get(nLocality)); // Here one intermediary end is connected with nLocaity(th) ns-3 node for example 10th node of ns-3 network. For example 10th node is for bootnode, 11th for miner and 0-9 for ns-3 network.
    NetDeviceContainer devices0 = p2p_new.Install(nodes0);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("192.168.100.0", "255.255.255.248"); // Here IP address is assigned to this interface (intermedary node to 10th ns-3 node)
    Ipv4InterfaceContainer interfaces0 = ipv4.Assign(devices0);

    // tap bridge arrangement for miner block. The same thing is repeated for miner block also. Now nLocality+1(th) (for ex. 11th) node is attached with Docker COntainer using intermediary nodes.  Here one intermediary (temp nodes) is attached with Docker Container and other end is attached with 11th ns-3 node.

    NodeContainer tempNodes0_miner;
    tempNodes0_miner.Create(2);

    CsmaHelper csma0_miner;
    csma0_miner.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma0_miner.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    NetDeviceContainer csmadevices0_miner = csma0_miner.Install(tempNodes0_miner);

    internetTempN.Install(tempNodes0_miner);

    address.SetBase("192.168.0.8", "255.255.255.248");
    Ipv4InterfaceContainer interfacesgen_miner = address.Assign(csmadevices0_miner); // IP address is assigned
    TapBridgeHelper tapBridge_miner(interfacesgen_miner.GetAddress(0));
    tapBridge_miner.SetAttribute("Mode", StringValue("UseBridge"));
    tapBridge_miner.SetAttribute("DeviceName", StringValue("tap_miner"));
    tapBridge_miner.Install(tempNodes0_miner.Get(0), csmadevices0_miner.Get(0));
    std::cout << "The ipinterface is:" << interfacesgen_miner.GetAddress(0) << "\n";
    PointToPointHelper p2p_new_miner;
    p2p_new_miner.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p_new_miner.SetChannelAttribute("Delay", StringValue("2ms"));

    NodeContainer nodes0_miner = NodeContainer(tempNodes0_miner.Get(1), nodes_ghost.Get(nLocality + 1));
    NetDeviceContainer devices0_miner = p2p_new_miner.Install(nodes0_miner);

    Ipv4AddressHelper ipv4_miner;
    ipv4.SetBase("192.168.100.8", "255.255.255.248");
    Ipv4InterfaceContainer interfaces0_miner = ipv4.Assign(devices0_miner);

    // tap bridge arrangement for all other gateway nodes. Here each remaining ns-3 node (for example node 0 to node 9) is connected with 2 intermediary node and other end of these intermedary nodes are attached to Docker Container using tapBridge named as tap_multi1-----tap_multi10

    std::string tap_multia, baseip_cont, csmadevicesa, baseip_contp2p;
    std::string tempNodesa, interfacesmulti1;

    CsmaHelper csmaa;
    csmaa.SetChannelAttribute("DataRate", StringValue("200Mbps"));
    csmaa.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    uint32_t cont_count1;

    for (cont_count1 = 0; cont_count1 < (nLocality); cont_count1++)
    {

      tempNodesa = "tempNodes" + std::to_string((cont_count1));
      csmadevicesa = "csmadevices" + std::to_string((cont_count1));
      tap_multia = "tap_multi" + std::to_string((cont_count1 + 1));
      interfacesmulti1 = "interfacesmulti" + std::to_string((cont_count1));
      NodeContainer tempNodesa;
      tempNodesa.Create(2);
      internetTempN.Install(tempNodesa);

      NetDeviceContainer csmadevicesa = csmaa.Install(tempNodesa);

      baseip_cont = networkipscont[cont_count1];
      address.SetBase(ns3::Ipv4Address(baseip_cont.c_str()), "255.255.255.248"); // Here IP addresses are assigned to ns-3 network and these IP addresses are taken from IP pools we created earlier.
      Ipv4InterfaceContainer interfacesmulti1 = address.Assign(csmadevicesa);

      std::cout << "The multi ip is:" << interfacesmulti1.GetAddress(0) << "\n";
      TapBridgeHelper tapBridge(interfacesmulti1.GetAddress(0));
      tapBridge.SetAttribute("Mode", StringValue("UseBridge"));
      tapBridge.SetAttribute("DeviceName", StringValue(tap_multia));
      tapBridge.Install(tempNodesa.Get(0), csmadevicesa.Get(0));

      NodeContainer nodesp2p = NodeContainer(tempNodesa.Get(1), nodes_ghost.Get(cont_count1));
      NetDeviceContainer devicesp2p = p2p_new.Install(nodesp2p);
      baseip_contp2p = networkipscontsecond[(cont_count1)];
      ipv4.SetBase(ns3::Ipv4Address(baseip_contp2p.c_str()), "255.255.255.248");
      Ipv4InterfaceContainer interfacesp2p = ipv4.Assign(devicesp2p);
    }

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

      //   Connect ns-3 nodes to create topology and assign IP address from the pool of IP addresses created earlier
      NodeContainer gname_t_ghost = NodeContainer(nodes_ghost.Get(loc1), nodes_ghost.Get(loc2));
      NetDeviceContainer gnamedev_t_ghost = p2p.Install(gname_t_ghost);
      baseip_t = networkipsgn[cont_count];
      address.SetBase(ns3::Ipv4Address(baseip_t.c_str()), "255.255.255.248");
      Ipv4InterfaceContainer gnameint_t_ghost = address.Assign(gnamedev_t_ghost);
      cont_count = cont_count + 1;
    }

    // Add the p2p links of bootnode and miner node with main network

    NodeContainer gname_bootnode = NodeContainer(nodes_ghost.Get(0), nodes_ghost.Get(nLocality));
    NetDeviceContainer gnamedev_bootnode = p2p.Install(gname_bootnode);
    baseip_t = networkipsgn[cont_count];
    address.SetBase(ns3::Ipv4Address(baseip_t.c_str()), "255.255.255.248");
    Ipv4InterfaceContainer gnameint_bootnode = address.Assign(gnamedev_bootnode);

    cont_count = cont_count + 1;

    NodeContainer gname_miner = NodeContainer(nodes_ghost.Get(1), nodes_ghost.Get(nLocality + 1));
    NetDeviceContainer gnamedev_miner = p2p.Install(gname_miner);
    baseip_t = networkipsgn[cont_count];
    address.SetBase(ns3::Ipv4Address(baseip_t.c_str()), "255.255.255.248");
    Ipv4InterfaceContainer gnameint_miner = address.Assign(gnamedev_miner);
    cont_count = cont_count + 1;

    Ipv4GlobalRoutingHelper::PopulateRoutingTables(); // This will create routing table for ns-3 network and help in routing packets for ns-3 network

    // Install NDN stack on nodes

    ndn::StackHelper ndnHelper;
    ndnHelper.SetDefaultRoutes(true);
    ndnHelper.setPolicy("nfd::cs::lru"); // Here we can define the cache strategy we wants to use like FIFO, LFU or LRU. Custom cache strategy can also be developed
    ndnHelper.setCsSize(5);              // This is size of cache in terms of number of objects it can store
    ndnHelper.Install(nodes);            // ndn strategy is installed on these nodes

    // Install NDN Routes Manually: Here static routing will be defined to route ndn packet. This can not be automatic as we have use dynamic routinf in ns-3 and both can not be dynamic at the same time. The ndn route is taken from the topology file itself.

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

      ndn::FibHelper::AddRoute(nodes.Get(loc11), ndn::Name("/ndnSIM"), nodes.Get(loc21), 1);
    }

    ndn::GlobalRoutingHelper ndnGlobalRoutingHelper; // This will install global routing to nsn nodess
    ndnGlobalRoutingHelper.Install(nodes);

    // Getting containers for the consumer/producer. We need to study these behaviours in order to modify these as per DWeb requirements. As of now it reads words for "requests.txt" file and create these words as prefix for interests generation. We can replace this file with Ethereum Blockchain and then create/satify interest accordingly.

    std::vector<int> producers_all;

    for (unsigned int i = 0; i < nLocality; i++)
      producers_all.push_back(i);
    std::random_shuffle(producers_all.begin(), producers_all.end());

    std::string prefix, prefix1, prefix2;
    std::string line;
    std::vector<std::string> list_names;

    if (myfile.is_open())
    {
      while (getline(myfile, line))
      {
        // std::cout << line << '\n';
        list_names.push_back(line);
      }
      myfile.close();
    }

    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
    consumerHelper.SetAttribute("Frequency", StringValue("2")); // 1 interests a second
    // consumerHelper.SetAttribute("NumberOfContents", StringValue(nconts)); // 10 different contents
    consumerHelper.SetAttribute("NumberOfContents", StringValue("2")); // 10 different contents
                                                                       // consumerHelper.SetAttribute("Frequency", StringValue("1"));  // number of interests in a second
    consumerHelper.SetAttribute("Randomize", StringValue("uniform"));

    std::vector<int> rand_cons;
    for (unsigned int i = 0; i < nLocality; i++)
      rand_cons.push_back(i);
    //std::random_shuffle(rand_cons.begin(), rand_cons.end());

    //uint32_t xd=nLocality-1;

    for (uint32_t i2 = 0; i2 < n_requests; i2++)
    {
      std::random_shuffle(rand_cons.begin(), rand_cons.end());
      for (uint32_t i = 0; i < nLocality; i++)
      {

        prefix = list_names[rand_cons[i]];
        consumerHelper.SetPrefix(prefix);
        consumerHelper.Install(nodes.Get(i));
      }
    }

    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    /****************************************************************************/
    // Register /dst1 to /dst9 prefix with global routing controller and
    // install producer that will satisfy Interests in /dst1 to /dst9 namespace
    std::string prefix_prod;
    for (uint32_t ij = 0; ij < nLocality; ij++)
    {
      prefix_prod = list_names[ij];
      ndnGlobalRoutingHelper.AddOrigins(prefix_prod, nodes.Get(producers_all[ij]));
      producerHelper.SetPrefix(prefix_prod);
      producerHelper.Install(nodes.Get(producers_all[ij]));
    }

    Simulator::Stop(Seconds(50.)); // We need to modify time of simulation as per our requirements. We can have more simulation time if we cants to test Ethereum or Docker Container.
    ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1));
    ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
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
