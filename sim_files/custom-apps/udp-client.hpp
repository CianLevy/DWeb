#ifndef UDP_CLIENT_HPP
#define UDP_CLIENT_HPP


#include <string>

#include "ns3/application.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include <map>
#include "ns3/random-variable-stream.h"

namespace ns3{

class UDPClient {
private:
    ns3::Ptr<Socket> socket;
    std::map<uint32_t, Callback<void, std::vector<std::string>>> registeredCallbacks;
    Ptr<UniformRandomVariable> m_rand;

    UDPClient();

public:
    static UDPClient& instance() {
        static UDPClient singleton;
        return singleton;
    };


    void connect(ns3::Ptr<Node> node, std::string server_addr, uint16_t port);

    void sendData(std::string data);

    void receivePacket(ns3::Ptr<Socket> sock);

    void registerReceiveCallback(Callback<void, std::vector<std::string>> on_receive, uint32_t callback_id);

    uint32_t getCallbackID();

    std::string extractOID(std::string response);

};
}

#endif 