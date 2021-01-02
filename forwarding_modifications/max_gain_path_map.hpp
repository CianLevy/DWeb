#ifndef NFD_DAEMON_FW_GAIN_STORE_HPP
#define NFD_DAEMON_FW_GAIN_STORE_HPP

#include <utility>
#include <map>
#include <iostream>


namespace nfd {

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

private:
    std::map<uint32_t, int> popularityMap;
};

}
#endif
//test2