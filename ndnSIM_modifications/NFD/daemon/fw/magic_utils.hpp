#ifndef NFD_DAEMON_FW_GAIN_STORE_HPP
#define NFD_DAEMON_FW_GAIN_STORE_HPP

#include <utility>
#include <map>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/encoding/block.hpp>

namespace nfd {

namespace magic {

class Magic;

class PopularityCounter {
public:
    void setId(std::string curr_id) { id = curr_id; };
    uint32_t getPopularity(ndn::Name n);
    void recordRequest(ndn::Name n);
    void print(std::vector<std::chrono::nanoseconds>* vec);
    bool isMaxPopularity(const ndn::Data& data);
private:
    std::map<std::string, std::vector<std::chrono::nanoseconds>*> requestHistoryMap;
    std::string id = "null";
};


class MaxPopularityPathMap{

public:
    static MaxPopularityPathMap& instance() {
        static MaxPopularityPathMap singleton;
        return singleton;
    }

    void update(uint32_t nonce, uint32_t local_popularity, std::string curr_id, ndn::Name n);

    uint32_t getPopularity(uint32_t nonce, std::string curr_id);

    ~MaxPopularityPathMap(){ std::cout << "Final map size " << requestHistoryMap.size() << std::endl; }

private:
    std::map<uint32_t, std::pair<uint32_t, std::string>> requestHistoryMap;
};


class MAGICParams{
public:
    MAGICParams(){};
    MAGICParams(const ndn::Block& parameters);

    const uint8_t* encode();
    void insertHop(std::string id);
    std::string getParams() { return content; };

private:
    std::string content;
};

}
}
#endif
