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

#ifndef NDN_CONSUMER_DERIVED_H
#define NDN_CONSUMER_DERIVED_H

#include "ns3/ndnSIM/model/ndn-common.hpp"


#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ndnSIM/apps/ndn-consumer-zipf-mandelbrot.hpp"


namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief NDN app requesting contents following Zipf-Mandelbrot Distbituion
 *
 * The class implements an app which requests contents following Zipf-Mandelbrot Distribution
 * Here is the explaination of Zipf-Mandelbrot Distribution:
 *http://en.wikipedia.org/wiki/Zipf%E2%80%93Mandelbrot_law
 */
class DerivedConsumer : public ConsumerZipfMandelbrot {
public:
  DerivedConsumer();

  static TypeId
  GetTypeId(void);

  virtual void
  SendPacket();

  virtual void
  OnData(shared_ptr<const Data> contentObject);

  void
  FinalizeSend(std::vector<std::string> split_response);

  void
  onDataCallback(std::vector<std::string> split_response);

  uint32_t GetRandomObjectID();

  virtual void
  StartApplication();

  void updateMetadataRange();

private:
  uint32_t objectCount = 0;
  std::map<std::string, std::string> OIDtoMetadata;
  std::map<std::string, shared_ptr<const Data>> receivedData;
  Name base_prefix;
  Time last_send;

  uint32_t total_producers_count;
  double produce_rate;
  double request_offset = 0.5;

  uint32_t max_metadata;

  Ptr<NormalRandomVariable> requestDistribution;

};

} /* namespace ndn */
} /* namespace ns3 */
#endif /* NDN_CONSUMER_ZIPF_MANDELBROT_H_ */
