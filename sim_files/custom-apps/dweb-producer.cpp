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
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
DWebProducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

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
  if (count == 0){
    shared_ptr<Name> new_prefix = make_shared<Name>(m_prefix);
    new_prefix->append("test");
    FibHelper::AddRoute(GetNode(), *new_prefix, m_face, 0);
    count++;
  }
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

  shared_ptr<::ndn::Buffer> buffer = make_shared<::ndn::Buffer>(m_virtualPayloadSize);
  std::string name = dataName[-1].toUri(ndn::name::UriFormat::DEFAULT);

  // name.erase(std::remove(name.begin(), name.end(), "%FE%"), name.end());
  std::cout << "Producing " << dataName << std::endl;
  
  std::string start = "%FE%";
  std::string::size_type i = name.find(start);

  if (i != std::string::npos)
    name.erase(i, start.length());


  for (size_t i = 0; i < m_virtualPayloadSize; ++i){
    // for (char c : name){
    for (int j = 0; j < name.size(); ++j){
      if (i >= m_virtualPayloadSize)
        break;
      
      buffer->at(i) = name[j];

      if (j < name.size() - 1)
        i++;
    }
  }

  data->setContent(buffer);
  char* temp = (char*)&(*buffer)[0];
  UDPClient::instance().sendData("set/producer_id/" + name + "/" + temp);

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

} // namespace ndn
} // namespace ns3
