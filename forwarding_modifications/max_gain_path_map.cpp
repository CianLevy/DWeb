#include "max_gain_path_map.hpp"

namespace nfd {
    
void MaxGainPathMap::update(uint32_t nonce, int local_popularity){
    std::map<uint32_t, int>::iterator it = popularityMap.find(nonce);

    if (it != popularityMap.end()){
        if (local_popularity > it->second)
            it->second = local_popularity;
    } else{
        popularityMap[nonce] = 0;
    }       
}

uint32_t MaxGainPathMap::getPopularity(uint32_t nonce){
    std::map<uint32_t, int>::iterator it = popularityMap.find(nonce);

    if (it != popularityMap.end()){
        return it->second;
    } else{
        return -1;
    }
}
}