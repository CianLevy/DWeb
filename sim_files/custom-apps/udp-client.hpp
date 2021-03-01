#ifndef UDP_CLIENT_HPP
#define UDP_CLIENT_HPP


#include <string>

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include <map>
#include "ns3/random-variable-stream.h"

#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <mutex>

using boost::asio::ip::udp;



namespace ns3{

class UDPClient {
private:
    std::map<uint32_t, Callback<void, std::vector<std::string>>> registeredCallbacks;
    std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> callbackResponses;
    Ptr<UniformRandomVariable> m_rand;

    std::shared_ptr<udp::socket> socket;

    std::shared_ptr<udp::endpoint> dockerEndpoint;
    std::shared_ptr<udp::endpoint> recvEndpoint;
    std::array<char, 1024> recvBuffer;

    std::mutex m;

    UDPClient();
    ~UDPClient();

public:
    static UDPClient& instance() {
        static UDPClient singleton;
        return singleton;
    };

    void handleReceive(const boost::system::error_code& error,
                       std::size_t bytes_transferred);

    void sendData(std::string data);

    void sendCallback(std::shared_ptr<std::string> message,
                      const boost::system::error_code& ec,
                      std::size_t bytes_transferred);

    void connect(std::string server_addr, unsigned short port, boost::asio::io_service& io_service2);

    void registerReceiveCallback(Callback<void, std::vector<std::string>> on_receive, uint32_t callback_id);

    uint32_t getCallbackID();

    void scheduleCallback(uint32_t callback_id);

};
}

#endif 