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


int PopularityCounter::getPopularity(ndn::Name n){
    std::map<std::string, std::vector<std::chrono::nanoseconds>*>::iterator it = popularityMap.find(n.toUri(ndn::name::UriFormat::DEFAULT));

    if (it != popularityMap.end()){
        auto temp = it->second;
        

        if (temp->size() > 1){
            // int test = std::chrono::duration<int>(temp->back()).count();
            double elapsed_time = std::chrono::duration<double, std::milli>(temp->back() - temp->front()).count() / 1000;
            // auto test = ((temp->back() - temp->front()) / std::chrono::seconds{1});;
            int res = temp->size() / elapsed_time;
            return res;
        }
    }

    return 0;
}

void PopularityCounter::recordRequest(ndn::Name n){
    std::string name_str = n.toUri(ndn::name::UriFormat::DEFAULT);
    std::chrono::nanoseconds currentTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

    std::map<std::string, std::vector<std::chrono::nanoseconds>*>::iterator it = popularityMap.find(name_str);
    if (it == popularityMap.end()){
        popularityMap[name_str] = new std::vector<std::chrono::nanoseconds>();
    }

    auto req_hist = popularityMap[name_str];
    // std::cout << "Before: " << req_hist->size() << std::endl;
    // print(popularityMap[n.toUri(ndn::name::UriFormat::DEFAULT)]);
    req_hist->push_back(currentTime);

    std::remove_if(req_hist->begin(), req_hist->end(), [&currentTime](std::chrono::nanoseconds i){ return currentTime - i > std::chrono::seconds{5}; });
    // std::cout << "After: " << req_hist->size() << std::endl;
    // print(popularityMap[n.toUri(ndn::name::UriFormat::DEFAULT)]);
}

void PopularityCounter::print(std::vector<std::chrono::nanoseconds>* vec){
    if (!vec)
        return;

    for (auto i : *vec){
      std::cout << i.count() << " ";
    }
    std::cout << std::endl;
}

}