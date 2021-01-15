#ifndef NFD_DAEMON_FW_GAIN_STORE_HPP
#define NFD_DAEMON_FW_GAIN_STORE_HPP

#include <utility>
#include <map>
#include <iostream>
#include <string.h>
#include <chrono>
#include <vector>
#include "ndn-cxx/name.hpp"

namespace nfd {
//class Name;

class PopularityCounter {
public:
    int getPopularity(ndn::Name n);
    void recordRequest(ndn::Name n);
    void print(std::vector<std::chrono::nanoseconds>* vec);

private:
    std::map<std::string, std::vector<std::chrono::nanoseconds>*> popularityMap;
};


class MaxGainPathMap{

public:
    static MaxGainPathMap& instance() {
        static MaxGainPathMap singleton;
        return singleton;
    }

    void update(uint32_t nonce, int local_popularity);

    uint32_t getPopularity(uint32_t nonce);

    void printAll(){
        std::map<uint32_t, int>::iterator it;
        for (it = popularityMap.begin(); it != popularityMap.end(); it++)
        {
            std::cout << it->first << ": " << it->second << std::endl;
        }
    }

    ~MaxGainPathMap(){ std::cout << "Final map size " << popularityMap.size() << std::endl; }

private:
    std::map<uint32_t, int> popularityMap;
};

}
#endif
