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

#include "dweb-consumer.hpp"

#include <math.h>
#include <iostream>
#include "udp-client.hpp"
#include <vector>
#include "common/logger.hpp"
#include <sstream> 

NS_LOG_COMPONENT_DEFINE("DerivedConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(DerivedConsumer);


DerivedConsumer::DerivedConsumer(){
    requestDistribution = CreateObject<NormalRandomVariable>();
}


TypeId
DerivedConsumer::GetTypeId(void)
{
    static TypeId tid = TypeId("DerivedConsumer")
        .SetParent<ndn::ConsumerZipfMandelbrot>()
        .AddConstructor<DerivedConsumer>()
        .AddAttribute("TotalObjects", "Number of the objects in total", StringValue("0"),
                    MakeUintegerAccessor(&DerivedConsumer::objectCount),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("ProduceRate", "Objects published per second", StringValue("0.25"),
                    MakeDoubleAccessor(&DerivedConsumer::produce_rate),
                    MakeDoubleChecker<double>())
        .AddAttribute("TotalProducerCount", "Total number of producers", UintegerValue(0),
                    MakeUintegerAccessor(&DerivedConsumer::total_producers_count),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("StartingMetadataCap", "Total number of producers", UintegerValue(10),
                    MakeUintegerAccessor(&DerivedConsumer::max_metadata),
                    MakeUintegerChecker<uint32_t>());
    return tid;
}

uint32_t 
DerivedConsumer::GetRandomObjectID(){
    uint32_t random_req = requestDistribution->GetInteger(max_metadata / 2, 10, max_metadata);

    return random_req;
}

void
DerivedConsumer::updateMetadataRange(){
    Time current = Simulator::Now();

    // has a produce period + the request offset margin elapsed?
    if (current - last_send > Seconds(1 / produce_rate + request_offset)){
        auto elapsed_count = (current - last_send - Seconds(request_offset)) / Seconds(1 / produce_rate);
        uint32_t previous = max_metadata;
        std::stringstream ss;
        ss << elapsed_count;

        uint32_t converted_count;
        ss >> converted_count;


        max_metadata += converted_count * total_producers_count;

        NFD_LOG_DEBUG("Expanding metadata range from " << previous << " to " << max_metadata);
        last_send = Simulator::Now(); // should rename this to last update
    }


}

void
DerivedConsumer::SendPacket()
{
    updateMetadataRange();

    uint32_t new_request_id = GetRandomObjectID();
    uint32_t callback_id = m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()); 
    std::string req = "get/" + std::to_string(callback_id) + "/" + std::to_string(new_request_id);
    NFD_LOG_DEBUG("Sending new request for metadata: " << new_request_id);
    UDPClient::instance().sendData(req);

    UDPClient::instance().registerReceiveCallback(MakeCallback(&DerivedConsumer::FinalizeSend, this), callback_id);
    NFD_LOG_DEBUG("Callback registered: " << callback_id);
    
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

    NFD_LOG_DEBUG("Callback response oid: " << oid);

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
        NFD_LOG_DEBUG("Received duplicate for oid: " << oid);



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
        NFD_LOG_DEBUG("Data integrity check failed for oid: " << split_response.at(3));
        return;
    }
    else
        NFD_LOG_DEBUG("Successfully verified data for oid " << split_response.at(3)); 


    if (it != receivedData.end()){
        shared_ptr<const Data> data = it->second;

        receivedData.erase(it);
        ConsumerZipfMandelbrot::OnData(data);
    }

    else
        NFD_LOG_DEBUG("Received data callback for unknown oid: " << split_response.at(3));


}

void
DerivedConsumer::StartApplication(){
    base_prefix = m_interestName;
    ConsumerZipfMandelbrot::StartApplication();
}

} /* namespace ndn */
} /* namespace ns3 */
