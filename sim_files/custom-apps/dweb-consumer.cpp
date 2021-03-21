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

NS_LOG_COMPONENT_DEFINE("DWebConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(DWebConsumer);



TypeId
DWebConsumer::GetTypeId(void)
{
    static TypeId tid = TypeId("DWebConsumer")
        .SetParent<ndn::ConsumerCbr>()
        .AddConstructor<DWebConsumer>()
        .AddAttribute("TotalObjects", "Number of the objects in total", StringValue("0"),
                    MakeUintegerAccessor(&DWebConsumer::objectCount),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("ProduceRate", "Objects published per second", StringValue("0.25"),
                    MakeDoubleAccessor(&DWebConsumer::produce_rate),
                    MakeDoubleChecker<double>())
        .AddAttribute("TotalProducerCount", "Total number of producers", UintegerValue(0),
                    MakeUintegerAccessor(&DWebConsumer::total_producers_count),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("StartingMetadataCap", "", UintegerValue(10),
                    MakeUintegerAccessor(&DWebConsumer::max_metadata),
                    MakeUintegerChecker<uint32_t>())
        .AddAttribute("q", "parameter of improve rank", StringValue("0.7"),
                    MakeDoubleAccessor(&DWebConsumer::SetQ,
                                       &DWebConsumer::GetQ),
                    MakeDoubleChecker<double>())
        .AddAttribute("s", "parameter of power", StringValue("0.7"),
                    MakeDoubleAccessor(&DWebConsumer::SetS,
                                       &DWebConsumer::GetS),
                    MakeDoubleChecker<double>());
    return tid;
}


DWebConsumer::DWebConsumer()
  : max_metadata(10) // needed here to make sure when SetQ/SetS are called, there is a valid value of N
  , m_q(0.7)
  , m_s(0.7)
  , m_seqRng(CreateObject<UniformRandomVariable>())
{
    requestDistribution = CreateObject<NormalRandomVariable>();
}

void
DWebConsumer::UpdateMetadataDistribution(uint32_t numOfContents)
{
  max_metadata = numOfContents;

  NS_LOG_DEBUG(m_q << " and " << m_s << " and " << max_metadata);

  m_Pcum = std::vector<double>(max_metadata + 1);

  m_Pcum[0] = 0.0;
  for (uint32_t i = 1; i <= max_metadata; i++) {
    m_Pcum[i] = m_Pcum[i - 1] + 1.0 / std::pow(i + m_q, m_s);
  }

  for (uint32_t i = 1; i <= max_metadata; i++) {
    m_Pcum[i] = m_Pcum[i] / m_Pcum[max_metadata];
    NS_LOG_LOGIC("Cumulative probability [" << i << "]=" << m_Pcum[i]);
  }
}


void
DWebConsumer::SetQ(double q)
{
  m_q = q;
  UpdateMetadataDistribution(max_metadata);
}

double
DWebConsumer::GetQ() const
{
  return m_q;
}

void
DWebConsumer::SetS(double s)
{
  m_s = s;
  UpdateMetadataDistribution(max_metadata);
}

double
DWebConsumer::GetS() const
{
  return m_s;
}

uint32_t 
DWebConsumer::GetRandomObjectID(){
    // uint32_t random_req = requestDistribution->GetInteger(max_metadata / 5, 3, max_metadata);
    // random_req %= max_metadata;
    // if (random_req > max_metadata)
    //     std::cout << "failed";
    // return random_req;

    uint32_t content_index = 1;
    double p_sum = 0;

    double p_random = m_seqRng->GetValue();
    while (p_random == 0) {
        p_random = m_seqRng->GetValue();
    }

    NS_LOG_LOGIC("p_random=" << p_random);
    for (uint32_t i = 1; i <= max_metadata; i++) {
        p_sum = m_Pcum[i]; // m_Pcum[i] = m_Pcum[i-1] + p[i], p[0] = 0;   e.g.: p_cum[1] = p[1],
                        // p_cum[2] = p[1] + p[2]
        if (p_random <= p_sum) {
        content_index = i;
        break;
        } // if
    }   // for

    return content_index; 
}

void
DWebConsumer::updateMetadataRange(){
    Time current = Simulator::Now();

    // has a produce period + the request offset margin elapsed?
    if (produce_rate > 0 && current - last_send > Seconds(1 / produce_rate + request_offset)){
        auto elapsed_count = (current - last_send - Seconds(request_offset)) / Seconds(1 / produce_rate);
        uint32_t previous = max_metadata;
        std::stringstream ss;
        ss << elapsed_count;

        uint32_t converted_count;
        ss >> converted_count;

        uint32_t new_max_metadata = max_metadata + converted_count * total_producers_count;
        UpdateMetadataDistribution(new_max_metadata);

        NFD_LOG_DEBUG("Expanding metadata range from " << previous << " to " << max_metadata);
        last_send = Simulator::Now(); // should rename this to last update
    }
}

void
DWebConsumer::SendPacket()
{
    std::cout << "DWeb consumer send" << std::endl;
    updateMetadataRange();

    uint32_t new_request_id = GetRandomObjectID();
    uint32_t callback_id = m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()); 
    std::string req = "get/" + std::to_string(callback_id) + "/" + std::to_string(new_request_id);
    NFD_LOG_DEBUG("Sending new request for metadata: " << new_request_id);
    UDPClient::instance().sendData(req);

    UDPClient::instance().registerReceiveCallback(MakeCallback(&DWebConsumer::FinalizeSend, this), callback_id);
    NFD_LOG_DEBUG("Callback registered: " << callback_id);
    ScheduleNextPacket();
}

void
DWebConsumer::FinalizeSend(std::vector<std::string> split_response)
{
    std::string oid = split_response.at(2);

    if (oid != "None"){
        NFD_LOG_DEBUG("Received oid for metadata " << split_response.at(3) << " " << oid);
        Name temp(oid);
        m_interestName = temp;

        // to do probably add a check here
        OIDtoMetadata[oid] = split_response.at(3);
        m_seq = 0;

        ConsumerCbr::SendPacket();
    }
    // else
    //     // manually schedule next packet
    //     ScheduleNextPacket();

    NFD_LOG_DEBUG("Callback response oid: " << oid);

}

void
DWebConsumer::OnData(shared_ptr<const Data> data)
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

    
    UDPClient::instance().registerReceiveCallback(MakeCallback(&DWebConsumer::onDataCallback, this), callback_id);

    // std::cout << "Callback registered: " << callback_id << std::endl;  
}

void
DWebConsumer::onDataCallback(std::vector<std::string> split_response)
{
    // to do: this should be request IDs?
    std::map<std::string, shared_ptr<const Data>>::iterator it = receivedData.find(split_response.at(3));

    if (split_response.at(2) == "False"){
        NFD_LOG_DEBUG("Data integrity check result for oid: " << split_response.at(3) << " res: " << split_response.at(2) << " metadata " << split_response.at(4));
        return;
    }
    else
        NFD_LOG_DEBUG("Data integrity check result for oid: "  << split_response.at(3) << " res: " << split_response.at(2) << " metadata " << split_response.at(4)); 


    if (it != receivedData.end()){
        shared_ptr<const Data> data = it->second;

        receivedData.erase(it);
        ConsumerCbr::OnData(data);
    }
    else
        NFD_LOG_DEBUG("Received data callback for unknown oid: " << split_response.at(3));
}


void
DWebConsumer::StartApplication(){
    base_prefix = m_interestName;
    ConsumerCbr::StartApplication();
}

void
DWebConsumer::ScheduleNextPacket()
{

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &DWebConsumer::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &DWebConsumer::SendPacket, this);
}


} /* namespace ndn */
} /* namespace ns3 */
