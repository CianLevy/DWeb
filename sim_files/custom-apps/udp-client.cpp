#include "udp-client.hpp"
#include <vector>
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include <string>
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/node.h"
#include "ns3/internet-stack-helper.h"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include "common/logger.hpp"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("UDPClient");

UDPClient::UDPClient()
  : m_rand(CreateObject<UniformRandomVariable>())
{
}

void UDPClient::connect(Ptr<Node> node, std::string server_addr, uint16_t port){
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    socket = Socket::CreateSocket(node, tid);

    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);

    socket->Bind();

    Ipv4Address serverIP(server_addr.c_str());
    socket->Connect(InetSocketAddress(serverIP, port));
    NFD_LOG_DEBUG("connected");
    socket->SetRecvCallback(MakeCallback(&UDPClient::receivePacket, this));
}

void UDPClient::sendData(std::string data){
    std::vector<uint8_t> data_copy(data.begin(), data.end());

    std::stringstream peerAddressStringStream;
    // peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);

    uint8_t *m_data = &data_copy[0];
    Ptr<Packet> packet = Create<Packet>(m_data, data_copy.size());
    int res = socket->Send(packet);
    if (res >= 0)
      NFD_LOG_DEBUG("Sent " << res);
    else
      NFD_LOG_DEBUG("Failed");
      
}

void UDPClient::receivePacket(Ptr<Socket> sock){
    Ptr<Packet> packet = sock->Recv(1472,0);

    uint8_t *buffer = new uint8_t[packet->GetSize () + 1];

    packet->CopyData(buffer, packet->GetSize());
    buffer[packet->GetSize()] = 0;
    std::string receiveBuffer = std::string((char*)buffer);

    NFD_LOG_DEBUG("Data Received: " << receiveBuffer);

    std::vector<std::string> results;
    boost::split(results, receiveBuffer, boost::is_any_of("/"));
    std::string::size_type i = receiveBuffer.find('/');

    uint32_t reqID = 0;

    if (i != std::string::npos)
      reqID = static_cast<uint32_t>(std::stoul(receiveBuffer.substr(0, i)));

    std::map<uint32_t, Callback<void, std::vector<std::string>>>::iterator it = registeredCallbacks.find(reqID);

    if (it != registeredCallbacks.end()){
      Callback<void, std::vector<std::string>> callback = (it->second);
      registeredCallbacks.erase(it);
      // (*callback)(receiveBuffer);
      callback(results);
    }
    else
      NFD_LOG_DEBUG("Received response for unknown callback: " << reqID);
}


void UDPClient::registerReceiveCallback(Callback<void, std::vector<std::string>> on_receive, uint32_t callback_id){
  std::map<uint32_t, Callback<void, std::vector<std::string>>>::iterator it = registeredCallbacks.find(callback_id);

  if (it == registeredCallbacks.end())
    registeredCallbacks[callback_id] = on_receive;
  else
    NFD_LOG_DEBUG("Repeat request for callback id: " << callback_id);
}



uint32_t UDPClient::getCallbackID(){
  return m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()); 
}

std::string UDPClient::extractOID(std::string response){
  std::string::size_type i = response.find("oid/");

  std::string oid = "None";

  if (i != std::string::npos)
    oid = response.substr(i + 4);

  return oid;
}

}