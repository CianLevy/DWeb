/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Tsinghua University, P.R.China.
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
 *
 * @author Xiaoke Jiang <shock.jiang@gmail.com>
 **/

#include "derived_consumer.hpp"

#include <math.h>
#include <iostream>
#include "udp-client.hpp"
#include <vector>

NS_LOG_COMPONENT_DEFINE("DerivedConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(DerivedConsumer);


TypeId
DerivedConsumer::GetTypeId(void)
{
    static TypeId tid = TypeId("DerivedConsumer")
        .SetParent<ndn::ConsumerZipfMandelbrot>()
        .AddConstructor<DerivedConsumer>()
        .AddAttribute("TotalObjects", "Number of the objects in total", StringValue("0"),
                    MakeUintegerAccessor(&DerivedConsumer::objectCount),
                    MakeUintegerChecker<uint32_t>());
    return tid;
}

uint32_t 
DerivedConsumer::GetRandomObjectID(){
    return 1;
}

void
DerivedConsumer::SendPacket()
{
    uint32_t new_request_id = GetRandomObjectID();
    uint32_t callback_id = m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()); 
    std::string req = "get/" + std::to_string(callback_id) + "/" + std::to_string(new_request_id);
    UDPClient::instance().sendData(req);

    UDPClient::instance().registerReceiveCallback(MakeCallback(&DerivedConsumer::FinalizeSend, this), callback_id);
    std::cout << "Callback registered: " << callback_id << std::endl;
}

void
DerivedConsumer::FinalizeSend(std::vector<std::string> split_response)
{
    std::string oid = split_response.at(2);

    if (oid != "None"){
        Name temp(base_prefix);
        temp.append(oid);
        m_interestName = temp;

        // to do probably add a check here
        OIDtoMetadata[oid] = split_response.at(3);
        ConsumerZipfMandelbrot::SendPacket();
    }
    else
    // manually schedule next packet
        ConsumerZipfMandelbrot::ScheduleNextPacket();

    std::cout << "Callback response oid: " << oid << std::endl;

}

void
DerivedConsumer::OnData(shared_ptr<const Data> data)
{
    Name dataName(data->getName());
    std::string oid = dataName.at(dataName.size() - 2).toUri(ndn::name::UriFormat::DEFAULT);

    const uint8_t* buffer = data->getContent().value();
    char* temp = (char*)(buffer);
    std::string test(temp);

    
    std::map<std::string, std::string>::iterator it = OIDtoMetadata.find(oid);
    std::string metadata = "Null";

    if (it != OIDtoMetadata.end()){
        metadata = it->second;
        // OIDtoMetadata.erase(it);
    }

    std::map<std::string, shared_ptr<const Data>>::iterator it2 = receivedData.find(oid);
    if (it2 == receivedData.end())
        receivedData[oid] = data;
    else
        std::cout << "Received duplicate for oid: " << oid << std::endl;



    uint32_t callback_id = UDPClient::instance().getCallbackID();
    UDPClient::instance().sendData("verify/" + std::to_string(callback_id)+ "/" + oid + "/" + metadata + "/" + test);

    
    UDPClient::instance().registerReceiveCallback(MakeCallback(&DerivedConsumer::onDataCallback, this), callback_id);

    // std::cout << "Callback registered: " << callback_id << std::endl;  
}

void
DerivedConsumer::onDataCallback(std::vector<std::string> split_response)
{
    // to do: this should be request IDs?
    std::map<std::string, shared_ptr<const Data>>::iterator it = receivedData.find(split_response.at(3));

    if (split_response.at(2) == "False"){
        std::cout << "Data integrity check failed for oid: " << split_response.at(3) << std::endl;
        return;
    }
    else
        std::cout << "Successfully verified data for oid " << split_response.at(3) << std::endl; 


    if (it != receivedData.end()){
        shared_ptr<const Data> data = it->second;

        receivedData.erase(it);
        ConsumerZipfMandelbrot::OnData(data);
    }

    else
        std::cout << "Received data callback for unknown oid: " << split_response.at(3) << std::endl;


}

void
DerivedConsumer::StartApplication(){
    base_prefix = m_interestName;
    ConsumerZipfMandelbrot::StartApplication();
}

} /* namespace ndn */
} /* namespace ns3 */
