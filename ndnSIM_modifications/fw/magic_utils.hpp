#ifndef NFD_DAEMON_FW_GAIN_STORE_HPP
#define NFD_DAEMON_FW_GAIN_STORE_HPP

#include <utility>
#include <map>
#include <iostream>
#include <string.h>
#include <chrono>
#include <vector>
#include <ndn-cxx/data.hpp>

namespace nfd {

namespace magic {

class Magic;

class PopularityCounter {
public:
    uint32_t getPopularity(ndn::Name n);
    void recordRequest(ndn::Name n);
    void print(std::vector<std::chrono::nanoseconds>* vec);
    bool isMaxPopularity(const ndn::Data& data);
private:
    std::map<std::string, std::vector<std::chrono::nanoseconds>*> requestHistoryMap;
};


class MaxPopularityPathMap{

public:
    static MaxPopularityPathMap& instance() {
        static MaxPopularityPathMap singleton;
        return singleton;
    }

    void update(uint32_t nonce, uint32_t local_popularity);

    uint32_t getPopularity(uint32_t nonce);

    ~MaxPopularityPathMap(){ std::cout << "Final map size " << requestHistoryMap.size() << std::endl; }

private:
    std::map<uint32_t, uint32_t> requestHistoryMap;
};

}
}
#endif
