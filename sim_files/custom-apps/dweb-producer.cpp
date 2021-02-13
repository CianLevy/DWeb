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

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <string>
#include "ns3/nstime.h"


NS_LOG_COMPONENT_DEFINE("DWebProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(DWebProducer);

TypeId
DWebProducer::GetTypeId(void)
{

  // TypeId("CustomApp").SetParent<ndn::App>().AddConstructor<CustomApp>();
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
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&DWebProducer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&DWebProducer::m_keyLocator), MakeNameChecker());
  return tid;
}

DWebProducer::DWebProducer()
{
  // m_virtualPayloadSize = 1024;
  NS_LOG_FUNCTION_NOARGS();

}


void
DWebProducer::publishInitialObjects(){
  for (int i = 0; i < 10; ++i){
      std::string metadata(std::to_string(i));
      publishAndAdvertise(metadata);
  }
}

// inherited from Application base class.
void
DWebProducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  Simulator::Schedule (Time(Seconds(1)), &DWebProducer::publishInitialObjects, this);
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
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());
  // dataName.append(m_postfix);
  // dataName.appendVersion();
  
  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  int metadata = lookupOID(dataName[dataName.size() - 2].toUri(ndn::name::UriFormat::DEFAULT));

  // if (metadata == -1)
  // // 
  //   return;

  shared_ptr<::ndn::Buffer> buffer = createData(std::to_string(metadata));
  data->setContent(buffer);


  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
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
DWebProducer::publishDataOnBlockchain(std::string metadata, shared_ptr<::ndn::Buffer> data, uint32_t callback_id){
  char* temp = (char*)&(*data)[0];
  std::string test(temp);
  UDPClient::instance().sendData("set/" + std::to_string(callback_id) + "/" + std::to_string(GetNode()->GetId()) + "/" + metadata + "/" + test + "\n");

  std::cout << "Attempting to publish " << metadata << std::endl;
}

shared_ptr<::ndn::Buffer>
DWebProducer::createData(std::string metadata){

    shared_ptr<::ndn::Buffer> buffer = make_shared<::ndn::Buffer>(m_virtualPayloadSize);

    for (size_t i = 0; i < m_virtualPayloadSize; ++i){
        for (size_t j = 0; j < metadata.size(); ++j){
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
DWebProducer::publishAndAdvertise(std::string metadata){
    uint32_t callback_id = UDPClient::instance().getCallbackID();
    shared_ptr<::ndn::Buffer> data = createData(metadata);
    publishDataOnBlockchain(metadata, data, callback_id);

    UDPClient::instance().registerReceiveCallback(MakeCallback(&DWebProducer::successfulBlockchainPublishCallback, this), callback_id);
}

void
DWebProducer::successfulBlockchainPublishCallback(std::vector<std::string> split_response){
    std::string oid = split_response.at(2);

    if (oid != "None"){
      shared_ptr<Name> prefix = make_shared<Name>(m_prefix);
      prefix->append(oid);
      FibHelper::AddRoute(GetNode(), *prefix, m_face, 0);
      std::cout << "Published new oid " << *prefix << std::endl;

      // this is a hacky assumption about the reseponse structure being
      // "oid/<oid val>/<metadata>"
      // should be fixed
      recordPublishedOID(split_response.at(3), oid);
    }
}

void
DWebProducer::recordPublishedOID(std::string metadata, std::string oid){
  int metadata_conv = std::stoi(metadata);

  int curr_metadata = lookupOID(oid);

  if (curr_metadata == -1){
    OIDtoMetadata[oid] = metadata_conv;
    bool test = lookupOID(oid) == metadata_conv;
    std::cout << "test " << test << std::endl;
  }

  else
    std::cout << "Attempted to re-record metadata value for oid: " << oid << std::endl;
}

int
DWebProducer::lookupOID(std::string oid){
  std::map<std::string, int>::iterator it = OIDtoMetadata.find(oid);
  if (it != OIDtoMetadata.end())
    return it->second;
  else
    return -1;
}

} // namespace ndn
} // namespace ns3
