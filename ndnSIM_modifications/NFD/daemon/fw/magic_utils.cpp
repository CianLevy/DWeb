#include "magic_utils.hpp"
#include "common/logger.hpp"
#include <ndn-cxx/lp/tags.hpp>
#include "table/pit.hpp"
#include <string>

namespace nfd {
namespace magic {

NFD_LOG_INIT(Magic);

void MaxPopularityPathMap::update(uint32_t nonce, uint32_t local_popularity, std::string curr_id, ndn::Name n){
    NFD_LOG_DEBUG(curr_id << " " << nonce << " outbound max " \
                  << MaxPopularityPathMap::instance().getPopularity(nonce, curr_id) \
                  << " local " << local_popularity << " req " << n);
    
    std::map<uint32_t, std::pair<uint32_t, std::string>>::iterator it = requestHistoryMap.find(nonce);

    if (it != requestHistoryMap.end()){
        if (local_popularity > it->second.first)
            it->second.first = local_popularity;
    } else{
        // initial popularity should be 0?
        requestHistoryMap[nonce] = make_pair(0, curr_id);
    }       
}

uint32_t MaxPopularityPathMap::getPopularity(uint32_t nonce, std::string curr_id){
    std::map<uint32_t, std::pair<uint32_t, std::string>>::iterator it = requestHistoryMap.find(nonce);

    if (it != requestHistoryMap.end()){
        uint32_t res = it->second.first;

        if (it->second.second == curr_id)
            requestHistoryMap.erase(it);
        
        return res;
    } else{
        return 0;
    }
}


uint32_t PopularityCounter::getPopularity(ndn::Name n){
    std::map<std::string, std::vector<std::chrono::nanoseconds>*>::iterator it = requestHistoryMap.find(n.toUri(ndn::name::UriFormat::DEFAULT));

    if (it != requestHistoryMap.end()){
        auto temp = it->second;

        if (temp->size() > 1){
            double elapsed_time = std::chrono::duration<double, std::milli>(temp->back() - temp->front()).count() / 1000;

            elapsed_time = (elapsed_time < 5) ? 5 : elapsed_time;

            uint32_t res = 0;

            if (elapsed_time)
                res = temp->size() / elapsed_time;

            return res;
        }
    }

    return 0;
}

void PopularityCounter::recordRequest(ndn::Name n){
    std::string name_str = n.toUri(ndn::name::UriFormat::DEFAULT);
    std::chrono::nanoseconds currentTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

    std::map<std::string, std::vector<std::chrono::nanoseconds>*>::iterator it = requestHistoryMap.find(name_str);

    if (it == requestHistoryMap.end()){
        requestHistoryMap[name_str] = new std::vector<std::chrono::nanoseconds>();
    }

    auto req_hist = requestHistoryMap[name_str];

    req_hist->push_back(currentTime);

    auto new_end = std::remove_if(req_hist->begin(), req_hist->end(), 
        [&currentTime](std::chrono::nanoseconds i) {
            return std::chrono::duration<double, std::milli>(currentTime - i).count() > 5000;
        }
    );


    if (new_end != req_hist->end())
        req_hist->erase(new_end, req_hist->end());
}

void PopularityCounter::print(std::vector<std::chrono::nanoseconds>* vec){
    if (!vec)
        return;

    for (auto i : *vec){
      std::cout << i.count() << " ";
    }
    std::cout << std::endl;
}

bool PopularityCounter::isMaxPopularity(const ndn::Data& data){
  typedef ndn::SimpleTag<shared_ptr<nfd::pit::DataMatchResult>, 999> pitMatchesTag;
  auto tag = data.getTag<pitMatchesTag>();
  data.removeTag<pitMatchesTag>();

  auto pitMatches = tag->get();

  bool res = false;

  for (const auto& pitEntry : *pitMatches){
    uint32_t interest_nonce = pitEntry->getInterest().getNonce();
    int local_popularity = getPopularity(pitEntry->getInterest().getName());
    int max_path_popularity = MaxPopularityPathMap::instance().getPopularity(interest_nonce, id);
    
    NFD_LOG_DEBUG(id << " " << interest_nonce << " return max " << \
                  max_path_popularity << " local " << local_popularity << " req " << data.getName());

    // if (local_popularity >= max_path_popularity && local_popularity > 0){
    //     return false; // TO DO FIX
    // }
  }

  return false;
}


MAGICParams::MAGICParams(const ndn::Block& parameters){
    // content = std::string((char*)value);
    bool encountered_start = false;

    for (auto val : parameters){
        char current = (char)val;

        if (current == 'N')
            encountered_start = true;

        if (encountered_start)
            content += current;
    }
        
}

const uint8_t* MAGICParams::encode(){
    if (content != "")
        return reinterpret_cast<const uint8_t*>(&content[0]);
    else
        return nullptr;
}

void MAGICParams::insertHop(std::string id){
    content += " " + id;
}

}
}