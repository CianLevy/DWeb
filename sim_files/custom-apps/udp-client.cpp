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

using namespace boost;

UDPClient::UDPClient()
  : m_rand(CreateObject<UniformRandomVariable>())
{

}

UDPClient::~UDPClient(){
  socket->cancel();
}

void UDPClient::connect(std::string server_addr, unsigned short port, boost::asio::io_service& io_service){
  unsigned short recvPort = 3001;

  socket = std::make_shared<udp::socket>(io_service, udp::endpoint(udp::v4(), recvPort));
  socket->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  boost::system::error_code ec;

  asio::ip::address ip_address = asio::ip::address::from_string(server_addr, ec);

  recvEndpoint = std::make_shared<udp::endpoint>(ip_address, recvPort);

  dockerEndpoint = std::make_shared<udp::endpoint>(ip_address, port);
  
  socket->async_receive_from(
    boost::asio::buffer(recvBuffer), *recvEndpoint,
    boost::bind(&UDPClient::handleReceive, this,
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));
}

void UDPClient::handleReceive(const boost::system::error_code& error, std::size_t bytes_transferred) {
  std::string receiveBuffer = std::string((char*)recvBuffer.data(), bytes_transferred);

  NFD_LOG_DEBUG("Data Received: " << receiveBuffer);

  std::shared_ptr<std::vector<std::string>> results = std::make_shared<std::vector<std::string>>();
  boost::split(*results, receiveBuffer, boost::is_any_of("/"));
  std::string::size_type i = receiveBuffer.find('/');

  uint32_t reqID = 0;

  if (i != std::string::npos)
    reqID = static_cast<uint32_t>(std::stoul(receiveBuffer.substr(0, i)));

  m.lock();
  callbackResponses[reqID] = results;
  m.unlock();

  socket->async_receive_from(
    boost::asio::buffer(recvBuffer), *recvEndpoint,
    boost::bind(&UDPClient::handleReceive, this,
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));
}


void UDPClient::sendData(std::string data) {
  auto message = std::make_shared<std::string>(data);

  socket->async_send_to(
    boost::asio::buffer(*message), *dockerEndpoint, 
                  boost::bind(&UDPClient::sendCallback, this, message,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void UDPClient::sendCallback(std::shared_ptr<std::string> message,
                const boost::system::error_code& ec,
                std::size_t bytes_transferred) {
  NFD_LOG_DEBUG("Sent " << bytes_transferred << " bytes");
}

void UDPClient::registerReceiveCallback(Callback<void, std::vector<std::string>> on_receive, uint32_t callback_id){
  std::map<uint32_t, Callback<void, std::vector<std::string>>>::iterator it = registeredCallbacks.find(callback_id);

  if (it == registeredCallbacks.end()){
    registeredCallbacks[callback_id] = on_receive;
    scheduleCallback(callback_id);
  }
  else
    NFD_LOG_DEBUG("Repeat request for callback id: " << callback_id);
}


void UDPClient::scheduleCallback(uint32_t callback_id){
  std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::iterator it = callbackResponses.find(callback_id);

  if (it == callbackResponses.end())
    Simulator::Schedule(Seconds(0.05), &UDPClient::scheduleCallback, this, callback_id);
  else{
    std::map<uint32_t, Callback<void, std::vector<std::string>>>::iterator it2 = registeredCallbacks.find(callback_id);

    if (it2 != registeredCallbacks.end()){
      Callback<void, std::vector<std::string>> callback = (it2->second);
      registeredCallbacks.erase(it2);
      // (*callback)(receiveBuffer);
      callback(*it->second);
    }
    else
      NFD_LOG_DEBUG("Received response for unknown callback: " << callback_id);
   
    m.lock();
    callbackResponses.erase(it);
    m.unlock();
  }
}

uint32_t UDPClient::getCallbackID(){
  return m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()); 
}

}