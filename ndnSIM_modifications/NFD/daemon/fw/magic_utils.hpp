#ifndef NFD_DAEMON_FW_GAIN_STORE_HPP
#define NFD_DAEMON_FW_GAIN_STORE_HPP

#include <utility>
#include <map>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <memory>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include "ndn-cxx/interest.hpp"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "table/priority_queue.hpp"

namespace nfd {

namespace magic {

class Magic;

class PopularityCounter {
public:
    void setId(std::string curr_id) { id = curr_id; };
    uint32_t getPopularity(ndn::Name n);
    void recordRequest(const ndn::Interest& interest);
    void print(std::vector<std::chrono::nanoseconds>* vec);
    bool isMaxPopularity(const ndn::Data& data);

    void updateInterestPopularityField(const ndn::Interest& interest, uint32_t popularity);
    void setPopularityHeap(std::shared_ptr<MinHeap> heap);
private:
    std::map<std::string, std::vector<std::chrono::nanoseconds>*> requestHistoryMap;
    std::string id = "null";
    std::shared_ptr<MinHeap> popularity_heap = nullptr;
    int req_history_time = 15;

};


class MAGICParams{
public:
    MAGICParams(const ndn::Interest& interest);
    MAGICParams(const ndn::Data& data);
    void init(const ndn::Block& parameters, bool interest_packet);

    void insertHop(std::string id, const ndn::Interest& interest);   
    std::string getParams();

    void updatePopularity(uint32_t popularity);
    uint32_t getPopularity() { return m_popularity; };

    void addToInterest(const ndn::Interest& interest);
    void addToData(std::shared_ptr<ndn::Data> data);
    void addToData(ndn::Data& data);

    void setCacheSize(uint32_t size) { cache_size = size; }

    uint32_t getLogUUID() { return log_uuid; };

private:
    void updateBuffer();
    void readStringFromBlock(const ndn::Block& parameters);

    std::string buffer = "";
    std::vector<std::string> hops;
    uint32_t m_popularity = 0;
    uint32_t log_uuid;
    ns3::Ptr<ns3::UniformRandomVariable> m_rand;
    uint32_t cache_size;
};

}
}
#endif
