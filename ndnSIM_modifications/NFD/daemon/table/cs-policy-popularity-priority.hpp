/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_TABLE_CS_POLICY_POPULARITY_PRIORITY_HPP
#define NFD_DAEMON_TABLE_CS_POLICY_POPULARITY_PRIORITY_HPP

#include "cs-policy.hpp"
#include "priority_queue.hpp"
#include "fw/magic_utils.hpp"

#include <list>

namespace nfd {
namespace cs {
namespace popularity_priority_queue {


/** \brief Priority FIFO replacement policy
 *
 *  This policy maintains a set of cleanup queues to decide the eviction order of CS entries.
 *  The cleanup queues are three doubly linked lists that store EntryRefs.
 *  The three queues keep track of unsolicited, stale, and fresh Data packet, respectively.
 *  EntryRef is placed into, removed from, and moved between suitable queues
 *  whenever an Entry is added, removed, or has other attribute changes.
 *  Each Entry should be in exactly one queue at any moment.
 *  Within each queue, the EntryRefs are kept in first-in-first-out order.
 *  Eviction procedure exhausts the first queue before moving onto the next queue,
 *  in the order of unsolicited, stale, and fresh queue.
 */
class PopularityPriorityPolicy : public Policy
{
public:
  PopularityPriorityPolicy();


public:
  static const std::string POLICY_NAME;

private:
  void
  doAfterInsert(EntryRef i) override;

  void
  doAfterRefresh(EntryRef i) override;

  void
  doBeforeErase(EntryRef i) override;

  void
  doBeforeUse(EntryRef i) override;

  void
  evictEntries() override;

private:
  /** \brief moves an entry to the end of queue
   */
  void
  insertToQueue(EntryRef i, bool isNewEntry);

private:
  std::map<std::string, std::shared_ptr<heapEntry>> m_entryInfoMap;
  shared_ptr<MinHeap> m_heap;
  shared_ptr<magic::PopularityCounter> m_popCounter;
  bool first_use = true;
};

} // namespace popularity_priority_queue

using popularity_priority_queue::PopularityPriorityPolicy;

} // namespace cs
} // namespace nfd

#endif // NFD_DAEMON_TABLE_CS_POLICY_POPULARITY_PRIORITY_HPP
