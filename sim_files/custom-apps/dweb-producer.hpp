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

#ifndef DWEB_PRODUCER_H
#define DWEB_PRODUCER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include "udp-client.hpp"
#include "ns3/ndnSIM/apps/ndn-app.hpp"

namespace ns3 {
namespace ndn {
/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class DWebProducer : public App {
public:
  static TypeId
  GetTypeId(void);

  DWebProducer();

  // inherited from NdnApp
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;

  uint32_t packet_len = 10;
  int count = 0;

  std::map<std::string, int> OIDtoMetadata;

  void
  publishDataOnBlockchain(std::string metadata, shared_ptr<::ndn::Buffer> data, uint32_t callback_id);

  shared_ptr<::ndn::Buffer>
  createData(std::string metadata);

  void
  successfulBlockchainPublishCallback(std::vector<std::string> split_response);

  void
  publishAndAdvertise(std::string metadata);

  void
  recordPublishedOID(std::string metadata, std::string oid);

  int
  lookupOID(std::string oid);

  void
  publishInitialObjects();
};

}
} // namespace ns3

#endif // NDN_PRODUCER_H
