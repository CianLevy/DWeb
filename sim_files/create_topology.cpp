/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Modified by: Raman Singh <rasingh@tcd.ie>,<raman.singh@thapar.edu> Post Doctoral Fellow, School of Computer Science and Statistics, Trinity College Dublin, University of Dublin, Ireland.
 */
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/wifi-net-device.h"
#include "ns3/qos-txop.h"
#include "ns3/wifi-mac.h"
#include "ns3/netanim-module.h" // header file for NetAnim
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/node.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("DWeb");

double GetDistance(Ptr<Node> node1, Ptr<Node> node2)
{
  Ptr<MobilityModel> model1 = node1->GetObject<MobilityModel>();
  Ptr<MobilityModel> model2 = node2->GetObject<MobilityModel>();
  double distance = model1->GetDistanceFrom(model2);
  return distance;
}

std::vector<uint32_t> BFSUtil(uint32_t u, uint32_t adjse[][2], std::vector<bool> &visited, uint32_t V1)
{

  std::vector<uint32_t> connectedadj;

  std::list<uint32_t> q;

  // Mark the current node as visited and enqueue it

  q.push_back(u);

  while (!q.empty())
  {
    u = q.front();
    connectedadj.push_back(u);
    visited[u] = true;
    q.pop_front();

    // Get all adjacent vertices of the dequeued
    // vertex s. If an adjacent has not been visited,
    // then mark it visited and enqueue it
    for (uint32_t i = 0; i < V1; i++)
    {
      if (u == adjse[i][0] && visited[adjse[i][1]] == false)
      {
        q.push_back(adjse[i][1]);
        visited[adjse[i][1]] = true;
      }
      if (u == adjse[i][1] && visited[adjse[i][0]] == false)
      {
        q.push_back(adjse[i][0]);
        visited[adjse[i][0]] = true;
      }
    }
  }

  return connectedadj;
}

int main(int argc, char *argv[])
{

  // Each locality will have 3 houses i.e. 3 WAPs and each house will have two mobile devices i.e. STAs

  //
  // We are interacting with the outside, real, world.  This means we have to
  // interact in real-time and therefore means we have to use the real-time
  // simulator and take the time to calculate checksums.
  //

  GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

  uint32_t nLocality = 50;           //10,50,100,200,500; //Numbers of locality or gateway nodes
  uint32_t density = 10;             // It shows the density of gateway nodes per square KM. i.e. number of localities in a 1 square KM area (or1000 m.
  uint32_t sim_area = density * 100; // total area = density * 100, so basically 1 gateway node per 100 meter.
  uint32_t nHouse = 3;               //Number of house per Locality
  CommandLine cmd;
  cmd.Parse(argc, argv);
  double dist;
  double **dist_mat = new double *[nLocality];
  for (uint32_t i = 0; i < nLocality; ++i)
  {
    dist_mat[i] = new double[nLocality];
  }
  uint32_t dist_idx[nLocality][2]{};
  uint32_t topology_idx_temp[2 * nLocality][2]{};
  std::vector<std::string> networkipsgn, networkipscont, networkipsgnsecond, networkipscontsecond;

  // Gateway Nodes creation
  NodeContainer gatewayNodes;
  gatewayNodes.Create(nLocality);

  //  WAP Nodes creation
  NodeContainer wapNodes;
  wapNodes.Create((nLocality * nHouse));

  MobilityHelper mobility;

  mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                "X", DoubleValue(sim_area * 0.5),
                                "Y", DoubleValue(sim_area * 0.5),
                                "rho", DoubleValue(sim_area * 0.5));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(gatewayNodes);

  // Mobility model for wapnodes, these nodes should be sticked around its gatewayNodes

  MobilityHelper mobilitywap;
  uint32_t w_sel = 0;
  for (uint32_t i_mob = 0; i_mob < (nLocality); i_mob++)
  {

    Ptr<MobilityModel> position = gatewayNodes.Get(i_mob)->GetObject<MobilityModel>();
    Vector pos = position->GetPosition();
    //     std::cout << "x=" << pos.x << ", y=" << pos.y << std::endl;

    mobilitywap.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                     "X", DoubleValue(pos.x),
                                     "Y", DoubleValue(pos.y),
                                     "rho", DoubleValue(30.0));
    mobilitywap.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilitywap.Install(wapNodes.Get(w_sel));
    mobilitywap.Install(wapNodes.Get(w_sel + 1));
    mobilitywap.Install(wapNodes.Get(w_sel + 2));
    w_sel = w_sel + 3;
  }

  // Create the topology for gatewayNodes
  // Calculate nLocality*nLocality matrix of distances
  for (uint32_t nn = 0; nn <= (nLocality - 1); nn++)
  {
    for (uint32_t nk = 0; nk <= (nLocality - 1); nk++)
    {

      dist = GetDistance(gatewayNodes.Get(nn), gatewayNodes.Get(nk));
      dist_mat[nn][nk] = dist;
      //std::cout<<"The distance is:"<<dist<<"\n";
    }
  }

  // Replace the 0 distances with 20000. This will help to select distance from itself
  uint32_t temp;
  for (uint32_t nn1 = 0; nn1 <= (nLocality - 1); nn1++)
  {
    for (uint32_t nk1 = 0; nk1 <= (nLocality - 1); nk1++)
    {

      temp = dist_mat[nn1][nk1];
      if (temp == 0)
        dist_mat[nn1][nk1] = 20000;
    }
  }

  /*
std::ofstream out("test.csv");

for (auto& row : dist_mat) {
  for (auto col : row)
    out << col <<',';
  out << '\n';
}
*/

  // Select nodes with smallest and second smallest distance from nodes
  uint32_t small1, t_idx1, small2, t_idx2;
  for (uint32_t nn2 = 0; nn2 <= (nLocality - 1); nn2++)
  {
    small1 = 21001;
    small2 = 21001;
    for (uint32_t nk2 = 0; nk2 <= (nLocality - 1); nk2++)
    {
      if (dist_mat[nn2][nk2] < small1)
      {
        small1 = dist_mat[nn2][nk2];
        t_idx1 = nk2;
      }
    }
    dist_mat[nn2][t_idx1] = 20000;

    for (uint32_t nk21 = 0; nk21 <= (nLocality - 1); nk21++)
    {
      if (dist_mat[nn2][nk21] < small2)
      {
        small2 = dist_mat[nn2][nk21];
        t_idx2 = nk21;
      }
    }

    dist_idx[nn2][0] = t_idx1;
    dist_idx[nn2][1] = t_idx2;
  }

  /*
std::ofstream out1("test2.csv");

for (auto& row1 : dist_idx) {
  for (auto col1 : row1)
    out1 << col1 <<',';
  out1 << '\n';
}
*/
  /*
// Display the distance matrix and selected nodes matrix

    for(uint32_t nk3=0;nk3<=(nLocality-1);nk3++)  
    { 
    for(uint32_t nk4=0;nk4<=(nLocality-1);nk4++)  
    { 
        std::cout<<dist_mat[nk3][nk4]<<"\t";
    }
        std::cout<<"\n";
    }

    for(uint32_t nk3=0;nk3<=(nLocality-1);nk3++)  
    { 
    for(uint32_t nk4=0;nk4<=1;nk4++)  
    { 
        std::cout<<dist_idx[nk3][nk4]<<"\t";
    }
        std::cout<<"\n";
    }
*/
  // gatewayNodes connections

  std::string ip1t, ip1t1, ip1, ip2t, ip2t1, ip2, ip4t, ip4t1, ip4, ip5t, ip5t1, ip5;

  uint32_t ipir, addrb, addrincr;
  NS_LOG_INFO("Gateway Nodes Create channels.");

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
      networkipsgn.push_back(ip1);

      ip2t = "10.10." + std::to_string(addrb); // For gn-gn network
      ip2t1 = ip2t + ".";
      ip2 = ip2t1 + std::to_string(addrincr);
      networkipsgnsecond.push_back(ip2);

      ip4t = "192.168." + std::to_string(addrb + 1); // for ghost nodes to docker container
      ip4t1 = ip4t + ".";
      ip4 = ip4t1 + std::to_string(addrincr);
      networkipscont.push_back(ip4);

      ip5t = "192.168." + std::to_string(addrb + 101); // for ghost nodes to gateway nodes
      ip5t1 = ip5t + ".";
      ip5 = ip5t1 + std::to_string(addrincr);
      networkipscontsecond.push_back(ip5);

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

  // Adding Even more addresses for gn-gn and WiFi STAs
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

  std::ofstream out1("ipaddressgn.csv");

  for (auto &row1 : networkipsgn)
  {
    for (auto col1 : row1)
      out1 << col1 << ',';
    out1 << '\n';
  }

  std::ofstream out3("ipaddresscont.csv");

  for (auto &row3 : networkipscont)
  {
    for (auto col3 : row3)
      out3 << col3 << ',';
    out3 << '\n';
  }

  //std::ofstream out4("networkipswap.csv");

  //for (auto& row4 : networkipswap) {
  //  for (auto col4 : row4)
  //    out4 << col4 <<',';
  //  out4 << '\n';
  //  }
  // Convert into topology matrix
  uint32_t nn5, nn5t = 0;

  for (nn5 = 0; nn5 <= (nLocality - 1); nn5++)
  {
    topology_idx_temp[nn5t][0] = nn5;
    topology_idx_temp[(nn5t + 1)][0] = nn5;
    topology_idx_temp[nn5t][1] = dist_idx[nn5][0];
    topology_idx_temp[(nn5t + 1)][1] = dist_idx[nn5][1];
    nn5t = nn5t + 2;
  }

  // Find and mark the redundant/duplicate entries to avoid multiple p2p links later
  uint32_t temp2;
  //,temp21,temp3,temp31;
  temp2 = sizeof(topology_idx_temp) / sizeof(topology_idx_temp[0]);
  for (uint32_t nn4 = 0; nn4 <= temp2; nn4++)
  {
    for (uint32_t nk4 = (nn4 + 1); nk4 <= temp2; nk4++)
    {
      if ((topology_idx_temp[nn4][0] == topology_idx_temp[nk4][0]) && (topology_idx_temp[nn4][1] == topology_idx_temp[nk4][1]))
      {
        topology_idx_temp[nk4][0] = 32000; //32000 is special integer here which represent duplicate entry
      }

      if ((topology_idx_temp[nn4][0] == topology_idx_temp[nk4][1]) && (topology_idx_temp[nn4][1] == topology_idx_temp[nk4][0]))
      {
        topology_idx_temp[nk4][0] = 32000; //32000 is special integer here which represent duplicate entry
      }
    }
  }

  /*
// Display the initial Topology matrix 
std::cout<<"Display the Topology matrix :"<<"\n";
int temp31 = sizeof(topology_idx_temp)/sizeof(topology_idx_temp[0]);
    for(int nk3=0;nk3<=(temp31-1);nk3++)  
    { 
    
        std::cout<<topology_idx_temp[nk3][0]<<"\t";
        std::cout<<topology_idx_temp[nk3][1]<<"\n";
    
     
    }

*/

  int temp21 = sizeof(topology_idx_temp) / sizeof(topology_idx_temp[0]);
  std::vector<std::vector<uint32_t>> topology_idx;
  for (int iml = 0; iml < temp21; iml++)
  {
    // Vector to store column elements
    std::vector<uint32_t> v1;

    if (topology_idx_temp[iml][0] == 32000)
    {
      continue;
    }
    else
    {
      v1.push_back(topology_idx_temp[iml][0]);
      v1.push_back(topology_idx_temp[iml][1]);
    }

    // Pushing back above 1D vector
    // to create the 2D vector
    topology_idx.push_back(v1);
  }

  // topology_idx.push_back({4,7});
  // Display the Vector matrix
  //std::cout<<"Display the Final topology matrix :"<<"\n";

  //    for(uint32_t nk311=0;nk311<topology_idx.size();nk311++)
  //    {
  //        std::cout<<topology_idx[nk311][0]<<"\t";
  //        std::cout<<topology_idx[nk311][1]<<"\n";
  //     }

  uint32_t topo_size = topology_idx.size();
  // Find the disconnected graphs/topology and connect them

  uint32_t V_t = unsigned(topology_idx.size());
  uint32_t adj_t[unsigned(topology_idx.size())][2];
  for (uint32_t td = 0; td < unsigned(topology_idx.size()); td++)
  {

    adj_t[td][0] = topology_idx[td][0];
    adj_t[td][1] = topology_idx[td][1];
  }

  //std::cout<<"The ADJ matrix is"<<"\n";
  //for(uint32_t vv=0;vv<adj_t.size();vv++)
  //{
  //std::cout<<adj_t[vv][0]<<"\t";
  //std::cout<<adj_t[vv][1]<<"\n";
  //}

  std::vector<std::vector<uint32_t>> disconnected_graphs;
  std::vector<bool> visited(V_t, false);
  for (uint32_t u = 0; u < nLocality; u++)
  {
    if (visited[u] == false)
    {

      disconnected_graphs.push_back(BFSUtil(u, adj_t, visited, V_t));
    }
  }
  //     std::cout<<"The connected adj is:\n";
  //for(uint32_t dg=0;dg<disconnected_graphs.size();dg++)
  //{
  //for(uint32_t dg1=0;dg1<disconnected_graphs[dg].size();dg1++)
  //{
  //std::cout << disconnected_graphs[dg][dg1] << "\t";
  //}
  //std::cout << "\n";
  //}

  // Find the connection between dosconnected graps

  double ele_c;
  uint32_t small_y_idx;
  uint32_t ele, ele2; //,sel_dgd1;
  for (uint32_t dgd = 0; dgd < unsigned(disconnected_graphs.size()); dgd++)
  {
    uint32_t small_y = 20000;
    ele = unsigned(disconnected_graphs[dgd][0]);
    uint32_t dgd1_t, dgd1_l;
    if (dgd < (unsigned(disconnected_graphs.size()) - 1))
    {
      dgd1_t = (dgd + 1);
      dgd1_l = unsigned(disconnected_graphs.size());
    }
    else
    {
      dgd1_t = 0;
      dgd1_l = unsigned(disconnected_graphs.size()) - 1;
    }
    std::vector<uint32_t> all_dis;
    for (uint32_t dgd1 = dgd1_t; dgd1 < dgd1_l; dgd1++)
    {
      //if(dgd==dgd1)
      //{
      //continue;
      //}

      uint32_t size1 = unsigned(disconnected_graphs[dgd1].size());

      for (uint32_t sz1 = 0; sz1 < size1; sz1++)
      {
        all_dis.push_back(unsigned(disconnected_graphs[dgd1][sz1]));
      }
    }

    //std::cout<<"The matrix all_this:"<<"\n";
    for (uint32_t ex1 = 0; ex1 < all_dis.size(); ex1++)
    {
      //std::cout<<all_dis[ex1]<<"\t";
      ele2 = all_dis[ex1];
      ele_c = GetDistance(gatewayNodes.Get(ele), gatewayNodes.Get(ele2));

      if (ele_c < small_y)
      {
        small_y = ele_c;
        small_y_idx = ele2;
        //std::cout<<"The small is:"<<small_y_idx<<"\n";
      }
    }
    if (ele == small_y_idx)
    {
      continue;
    }

    //ele_c=GetDistance(gatewayNodes.Get(ele),gatewayNodes.Get(small_y_idx));
    std::vector<uint32_t> temp_idx_sel;
    temp_idx_sel.push_back(ele);
    temp_idx_sel.push_back(small_y_idx);
    topology_idx.push_back(temp_idx_sel);
    //td::cout<<"The Final selected indexes are:"<<ele<<"\t"<<small_y_idx<<"\n";
  }

  // if link is too long, do some manual corrections

  // Find which links requires manual corrections
  std::cout << "The size of first phase topology idx is:" << topo_size << "\n";
  /*
std::cout<<"The following links needs manual corrections: Node1-Node2-Dist-Topo idx:"<<"\n";
for(uint32_t topo_i=topo_size;topo_i<topology_idx.size();topo_i++)
{
ele_c=GetDistance(gatewayNodes.Get(topology_idx[topo_i][0]),gatewayNodes.Get(topology_idx[topo_i][1]));
if(ele_c>(sim_area/5))
{
std::cout<<topology_idx[topo_i][0]<<"\t"<<topology_idx[topo_i][1]<<"\t"<<ele_c<<"\t"<<topo_i<<"\n";
}
}
*/
  // Do some manual corrections or deletion of link

  //Corrections of link
  if (nLocality == 30)
  {
    topology_idx[42][0] = 29;
  }
  if (nLocality == 50)
  {
    topology_idx[69][0] = 14;
  }
  if (nLocality == 100)
  {
    topology_idx[137][0] = 86;
    topology_idx[135][1] = 90;
  }
  if (nLocality == 200)
  {
    topology_idx[266][0] = 168;
    topology_idx[267][1] = 34;
    topology_idx.push_back({154, 103});
    topology_idx.push_back({25, 151});
  }
  if (nLocality == 500)
  {
    topology_idx[676][0] = 292;
    topology_idx[677][0] = 109;
  }
  //topology_idx[677][0]=109;
  //topology_idx[69][0]=14;

  //Deletion of some link
  //unsigned rowToDelete = 42;
  //if (topology_idx.size() > rowToDelete)
  //{
  //  topology_idx.erase( topology_idx.begin() + rowToDelete );
  //}
  /*
std::cout<<"The idx of topology is:"<<"\n"; 
for(uint32_t eret=0;eret<topology_idx.size();eret++)
{
std::cout<<topology_idx[eret][0]<<"\t";
std::cout<<topology_idx[eret][1]<<"\t";
std::cout<<GetDistance(gatewayNodes.Get(topology_idx[eret][0]),gatewayNodes.Get(topology_idx[eret][1]))<<"\n"; 
}
*/

  // Create topology idx for gatewaynodes to wap links
  std::vector<std::vector<uint32_t>> topology_idxwap;

  uint32_t wapcount = 0;
  for (uint32_t twap = 0; twap < nLocality; twap++)
  {

    wapcount = nHouse * twap;
    for (uint32_t wt = 0; wt < 3; wt++)
    {
      std::vector<uint32_t> wap_temp;
      wap_temp.push_back(twap);
      wap_temp.push_back(wapcount + wt);
      topology_idxwap.push_back(wap_temp);
    }
  }

  // Display the Final Topology matrix

  int temp311 = topology_idx.size();
  std::cout << "The size of final topology idx is:" << temp311 << "\n";
  std::cout << "Displaying the Topology matrix :"
            << "\n";
  for (int nk31 = 0; nk31 <= (temp311 - 1); nk31++)
  {

    std::cout << topology_idx[nk31][0] << "\t";
    std::cout << topology_idx[nk31][1] << "\n";
  }

  // Save the Final Topology matrix

  std::string file_name, file_name_t;
  file_name_t = "topology" + std::to_string(nLocality);
  file_name = file_name_t + ".csv";
  std::ofstream out5(file_name);

  for (auto &row5 : topology_idx)
  {
    for (auto col5 : row5)
    {
      out5 << col5 << ',';
    }
    out5 << '\n';
  }

  // Empty Nodes Declaration (In order to define boundries of animation only)

  NodeContainer emptyNodes;
  emptyNodes.Create(4);

  // Empty Nodes Mobility Model: To define the boundaries of grid of animation

  MobilityHelper mobilityEN;
  mobilityEN.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(sim_area),
                                  "DeltaY", DoubleValue(sim_area),
                                  "GridWidth", UintegerValue(2),
                                  "LayoutType", StringValue("RowFirst"));
  mobilityEN.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityEN.Install(emptyNodes);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  Simulator::Stop(Seconds(10.));
  AnimationInterface anim("DWeb.xml");
  anim.SetMaxPktsPerTraceFile(100000000);

  // Animation Node SIze
  for (uint32_t ns = 0; ns < nLocality; ns++)
  {
    anim.UpdateNodeSize(gatewayNodes.Get(ns)->GetId(), 10.0, 10.0);
  }
  //for(uint32_t ns1=0;ns1<(nLocality*nHouse);ns1++)
  // {
  // anim.UpdateNodeSize (wapNodes.Get(ns1)->GetId(), 6.0, 6.0);
  // }

  Simulator::Run();
  return 0;
}
