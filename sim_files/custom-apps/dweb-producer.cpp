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
 **/

#include "dweb-producer.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/names.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <string>
#include "ns3/nstime.h"
#include "common/logger.hpp"
#include "NFD/daemon/fw/magic_utils.hpp"

NS_LOG_COMPONENT_DEFINE("DWebProducer");

namespace ns3
{
namespace ndn
{

NS_OBJECT_ENSURE_REGISTERED(DWebProducer);

TypeId
DWebProducer::GetTypeId(void)
{

  static TypeId tid =
      TypeId("DWebProducer")
          .SetParent<App>()
          .AddConstructor<DWebProducer>()
          .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                        MakeNameAccessor(&DWebProducer::m_prefix), MakeNameChecker())
          .AddAttribute(
              "Postfix",
              "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
              StringValue("/"), MakeNameAccessor(&DWebProducer::m_postfix), MakeNameChecker())
          .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                        MakeUintegerAccessor(&DWebProducer::m_virtualPayloadSize),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                        TimeValue(Seconds(0)), MakeTimeAccessor(&DWebProducer::m_freshness),
                        MakeTimeChecker())
          .AddAttribute("Signature",
                        "Fake signature, 0 valid signature (default), other values application-specific",
                        UintegerValue(0), MakeUintegerAccessor(&DWebProducer::m_signature),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("KeyLocator",
                        "Name to be used for key locator.  If root, then key locator is not used",
                        NameValue(), MakeNameAccessor(&DWebProducer::m_keyLocator), MakeNameChecker())
          .AddAttribute("ProduceRate", "Objects published per second", StringValue("0.25"),
                        MakeDoubleAccessor(&DWebProducer::setProduceRate,
                                            &DWebProducer::getProduceRate),
                        MakeDoubleChecker<double>())
          .AddAttribute("TotalProducerCount", "Total number of producers", UintegerValue(0),
                        MakeUintegerAccessor(&DWebProducer::total_producers_count),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("ProducerNumber",
                        "The producer's associated id within the count from 0 to total number of producers",
                        UintegerValue(0),
                        MakeUintegerAccessor(&DWebProducer::producer_num),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("InitialObjects",
                        "The producer's associated id within the count from 0 to total number of producers",
                        UintegerValue(10),
                        MakeUintegerAccessor(&DWebProducer::starting_objects),
                        MakeUintegerChecker<uint32_t>())
          .AddAttribute("EnableObjectBroadcast",
                        "Enable DWeb's broadcast based caching behaviour",
                        StringValue("disabled"),
                        MakeStringAccessor(&DWebProducer::objectBroadcast), MakeStringChecker());

  return tid;
}

DWebProducer::DWebProducer()
    : m_rand(CreateObject<UniformRandomVariable>())
{
  NS_LOG_FUNCTION_NOARGS();
}

void
DWebProducer::publishInitialObjects()
{
  for (uint32_t i = 0; i < starting_objects; ++i)
  {
    std::string metadata = getNextMetadataValue();
    publishAndAdvertise(metadata);
  }
  if (produce_delay > 0)
    publishNext();
}

// inherited from Application base class.
void
DWebProducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  m_name = Names::FindName(m_node);

  if (m_name.empty())
    m_name = boost::lexical_cast<std::string>(m_node->GetId());

  publishInitialObjects();
  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
DWebProducer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
DWebProducer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());
  auto data = make_shared<Data>();

  nfd::magic::PacketWrapper params(*interest);
  params.addToData(data);

  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  int metadata = lookupOID(dataName[dataName.size() - 2].toUri(ndn::name::UriFormat::DEFAULT));

  shared_ptr<::ndn::Buffer> buffer = createData(std::to_string(metadata));
  data->setContent(buffer);
  sendData(data);
}

void
DWebProducer::sendData(shared_ptr<Data> data)
{
  Signature signature;
  SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0)
  {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
}

void
DWebProducer::publishDataOnBlockchain(std::string metadata, shared_ptr<::ndn::Buffer> data, uint32_t callback_id)
{
  char *temp = (char *)&(*data)[0];
  std::string test(temp);
  UDPClient::instance().sendData("set/" + std::to_string(callback_id) + "/" + std::to_string(GetNode()->GetId()) + "/" + metadata + "/" + test + "\n");

  NFD_LOG_DEBUG(m_name << ": Attempting to publish metadata: " << metadata);
}

shared_ptr<::ndn::Buffer>
DWebProducer::createData(std::string metadata)
{
  shared_ptr<::ndn::Buffer> buffer = make_shared<::ndn::Buffer>(m_virtualPayloadSize);

  for (size_t i = 0; i < m_virtualPayloadSize; ++i)
  {
    for (size_t j = 0; j < metadata.size(); ++j)
    {
      if (i >= m_virtualPayloadSize)
        break;

      buffer->at(i) = metadata[j];

      if (j < metadata.size() - 1)
        i++;
    }
  }
  return buffer;
}

void
DWebProducer::publishAndAdvertise(std::string metadata)
{
  uint32_t callback_id = UDPClient::instance().getCallbackID();
  shared_ptr<::ndn::Buffer> data = createData(metadata);
  publishDataOnBlockchain(metadata, data, callback_id);

  UDPClient::instance().registerReceiveCallback(MakeCallback(&DWebProducer::successfulBlockchainPublishCallback, this), callback_id);
}

void
DWebProducer::successfulBlockchainPublishCallback(std::vector<std::string> split_response)
{
  std::string oid = split_response.at(2);

  if (oid != "None")
  {
    shared_ptr<Name> prefix = make_shared<Name>(oid);
    FibHelper::AddRoute(GetNode(), *prefix, m_face, 0);

    shared_ptr<Interest> new_interest = make_shared<Interest>();
    new_interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    std::string name3 = "/broadcast/" + oid;
    shared_ptr<Name> n = make_shared<Name>(name3);
    new_interest->setName(*n);

    m_transmittedInterests(new_interest, this, m_face);
    m_appLink->onReceiveInterest(*new_interest);

    // broadcast object itself
    if (objectBroadcast == "enabled")
    {
      NFD_LOG_DEBUG("Starting object broadcast: " << oid);

      Name dataName("/broadcast/object/" + oid);
      auto data = make_shared<Data>();

      data->setName(dataName);
      data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

      shared_ptr<::ndn::Buffer> buffer = createData(split_response.at(3));
      data->setContent(buffer);
      sendData(data);
    }

    NFD_LOG_DEBUG(m_name << ": Published new object. Metadata: " << split_response.at(3) << " oid: " << *prefix);

    recordPublishedOID(split_response.at(3), oid);
  }
  else
    NFD_LOG_DEBUG(m_name << ": Publication failed for metadata: " << split_response.at(3));
}

void
DWebProducer::recordPublishedOID(std::string metadata, std::string oid)
{
  int metadata_conv = std::stoi(metadata);

  int curr_metadata = lookupOID(oid);

  if (curr_metadata == -1)
  {
    OIDtoMetadata[oid] = metadata_conv;
  }

  else
    NFD_LOG_DEBUG("Attempted to re-record metadata value for oid: " << oid);
}

int
DWebProducer::lookupOID(std::string oid)
{
  std::map<std::string, int>::iterator it = OIDtoMetadata.find(oid);
  if (it != OIDtoMetadata.end())
    return it->second;
  else
    return -1;
}

std::string
DWebProducer::getNextMetadataValue()
{
  uint32_t next = producer_num + produced_count * total_producers_count;
  produced_count++;
  return std::to_string(next);
}

void
DWebProducer::publishNext()
{
  std::string metadata = getNextMetadataValue();
  NFD_LOG_DEBUG("Publishing object with metadata: " + metadata);
  publishAndAdvertise(metadata);
  ScheduleNextProduce();
}

void
DWebProducer::ScheduleNextProduce()
{
  if (produce_delay > 0 && !m_publishEvent.IsRunning())
    m_publishEvent = Simulator::Schedule(Seconds(produce_delay), &DWebProducer::publishNext, this);
}

void
DWebProducer::setProduceRate(double rate)
{
  produce_rate = rate;
  produce_delay = (produce_rate > 0) ? 1 / rate : -1;
}

double
DWebProducer::getProduceRate() const
{
  return produce_rate;
}

} // namespace ndn
} // namespace ns3
