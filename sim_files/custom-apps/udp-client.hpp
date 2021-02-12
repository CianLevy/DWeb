#ifndef UDP_CLIENT_HPP
#define UDP_CLIENT_HPP


#include <string>

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include <map>

namespace ns3{

class UDPClient {
private:
    ns3::Ptr<Socket> socket;
    std::map<uint32_t, Callback<void, std::string>> registeredCallbacks;

public:
    static UDPClient& instance() {
        static UDPClient singleton;
        return singleton;
    };

    // static TypeId GetTypeId (void);

    // UDPClient();

    void connect(ns3::Ptr<Node> node, std::string server_addr, uint16_t port);

    void sendData(std::string data);

    void receivePacket(ns3::Ptr<Socket> sock);

    void registerReceiveCallback(Callback<void, std::string> on_receive, uint32_t callback_id);

    // std::string receive();

};
}

#endif 