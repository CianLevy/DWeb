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

#include "cs-policy-popularity-priority.hpp"
#include "cs.hpp"
#include "common/global.hpp"

namespace nfd {
namespace cs {
namespace popularity_priority_queue {

const std::string PopularityPriorityPolicy::POLICY_NAME = "popularity_priority_queue";
NFD_REGISTER_CS_POLICY(PopularityPriorityPolicy);

PopularityPriorityPolicy::PopularityPriorityPolicy()
  : Policy(POLICY_NAME)
{
  Cs* c = getCs();
  m_popCounter = c->m_popCounter;
}


void
PopularityPriorityPolicy::doAfterInsert(EntryRef i)
{
  this->insertToQueue(i, true);
  this->evictEntries();
}

void
PopularityPriorityPolicy::doAfterRefresh(EntryRef i)
{
  this->insertToQueue(i, false);
}

void
PopularityPriorityPolicy::doBeforeErase(EntryRef i)
{
  shared_ptr<heapEntry> entry = m_entryInfoMap[i];
  m_heap.remove(entry);
  // m_queue.get<1>().erase(i);
}

void
PopularityPriorityPolicy::doBeforeUse(EntryRef i)
{
  this->insertToQueue(i, false);
}

void
PopularityPriorityPolicy::evictEntries()
{
  BOOST_ASSERT(this->getCs() != nullptr);
  while (this->getCs()->size() > this->getLimit()) {
    // BOOST_ASSERT(!m_queue.empty());
    shared_ptr<heapEntry> entry = m_heap.pop();

    this->emitSignal(beforeEvict, *entry->entry);
  }
}

void
PopularityPriorityPolicy::insertToQueue(EntryRef i, bool isNewEntry)
{
  if (isNewEntry){
    shared_ptr<heapEntry> new_entry = make_shared<heapEntry>();
    new_entry->entry = &i;

    Name n = i->getData().getName();
    new_entry->popularity = m_popCounter->getPopularity(n);
    m_heap.insert(new_entry);

    m_entryInfoMap[i] = new_entry;
  }
  else{
    shared_ptr<heapEntry> entry = m_entryInfoMap[i];
    Name n = i->getData().getName();
    uint32_t popularity = m_popCounter->getPopularity(n);

    m_heap.update(entry, popularity);
  }

}

} // namespace popularity_priority_queue
} // namespace cs
} // namespace nfd
