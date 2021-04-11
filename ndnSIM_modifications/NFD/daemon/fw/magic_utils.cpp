#include "magic_utils.hpp"
#include "common/logger.hpp"
#include <ndn-cxx/lp/tags.hpp>
#include "table/pit.hpp"
#include <string>
#include <boost/algorithm/string.hpp>
#include "ndn-cxx/meta-info.hpp"
#include "ndn-cxx/encoding/block-helpers.hpp"



namespace nfd {
namespace magic {

NFD_LOG_INIT(Magic);


uint32_t PopularityCounter::getPopularity(ndn::Name n){
    std::map<std::string, std::vector<std::chrono::nanoseconds>*>::iterator it = requestHistoryMap.find(n.toUri(ndn::name::UriFormat::DEFAULT));

    if (it != requestHistoryMap.end()){
        auto temp = it->second;

        if (temp->size() > 1){
            double elapsed_time = std::chrono::duration<double, std::milli>(temp->back() - temp->front()).count() / 1000;

            elapsed_time = (elapsed_time < req_history_time) ? req_history_time : elapsed_time;

            uint32_t res = 0;

            if (elapsed_time)
                res = temp->size();// / elapsed_time;

            return res;
        }
    }

    return 0;
}

void PopularityCounter::recordRequest(const ndn::Interest& interest){
    // prior to recording the new request, update the packet with the
    // current local popularity
    uint32_t current_popularity = getPopularity(interest.getName());
    updateInterestPopularityField(interest, current_popularity);


    std::string name_str = interest.getName().toUri(ndn::name::UriFormat::DEFAULT);


    std::chrono::nanoseconds currentTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

    std::map<std::string, std::vector<std::chrono::nanoseconds>*>::iterator it = requestHistoryMap.find(name_str);

    if (it == requestHistoryMap.end()){
        requestHistoryMap[name_str] = new std::vector<std::chrono::nanoseconds>();
    }

    auto req_hist = requestHistoryMap[name_str];

    req_hist->push_back(currentTime);

    auto new_end = std::remove_if(req_hist->begin(), req_hist->end(), 
        [&currentTime, this](std::chrono::nanoseconds i) {
            return std::chrono::duration<double, std::milli>(currentTime - i).count() > (req_history_time * 1000);
        }
    );


    if (new_end != req_hist->end())
        req_hist->erase(new_end, req_hist->end());
}

void PopularityCounter::updateInterestPopularityField(const ndn::Interest& interest, uint32_t popularity){
    MAGICParams params(interest);
    params.insertHop(id, interest);

    NFD_LOG_DEBUG(id << " " << params.getLogUUID() << " outbound max " \
                  << params.getPopularity() \
                  << " local " << popularity << " req " << interest.getName());

    params.updatePopularity(popularity);
    params.addToInterest(interest);
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
    MAGICParams params(data);

    uint32_t local_popularity = getPopularity(data.getName());

    NFD_LOG_DEBUG(id << " " << params.getLogUUID()  << " return max " << \
        params.getPopularity() << " local " << local_popularity << " req " << data.getName());

    uint32_t min_popularity_required = (popularity_heap) ? popularity_heap->peekPopularity() : 1;

    if (popularity_heap && !popularity_heap->isFull()){
        // while the cache is not full, cache regardless of popularity
        NFD_LOG_DEBUG("Recommending caching " << data.getName() << " due to non-full cache");
        return true;
    }

    if (params.getPopularity() > min_popularity_required && params.getPopularity() <= local_popularity){
        NFD_LOG_DEBUG("Recommending caching " << data.getName() << " due to local popularity " << \
                      params.getPopularity() << " >= min required " << min_popularity_required << " and local popularity "\
                      << local_popularity);
        return true;
    }
        

    return false;
}

void  PopularityCounter::setPopularityHeap(std::shared_ptr<MinHeap> heap){
    popularity_heap = heap;
}


MAGICParams::MAGICParams(const ndn::Interest& interest)
: m_rand(ns3::CreateObject<ns3::UniformRandomVariable>())
{
    log_uuid = m_rand->GetValue(0, std::numeric_limits<uint32_t>::max());

    if (interest.hasApplicationParameters()){
        auto raw_data = interest.getApplicationParameters();
        init(raw_data, true);
    }
}

MAGICParams::MAGICParams(const ndn::Data& data)
: m_rand(ns3::CreateObject<ns3::UniformRandomVariable>())
{   
    log_uuid = m_rand->GetValue(0, std::numeric_limits<uint32_t>::max());

    if (!data.getMetaInfo().getAppMetaInfo().empty()){
        auto raw_data = data.getMetaInfo().getAppMetaInfo().front();
        init(raw_data, true);
    }
    else
        std::cout << data.getName() << " metainfo empty" << std::endl;

}

void MAGICParams::init(const ndn::Block& parameters, bool interest_packet){
    readStringFromBlock(parameters);

    if (interest_packet && buffer != ""){
        boost::split(hops, buffer, boost::is_any_of(" "));
        std::string end = hops.back();
        m_popularity = static_cast<uint32_t>(std::stoul(end));
        
        hops.pop_back();
        end = hops.back();
        hops.pop_back();
        log_uuid = static_cast<uint32_t>(std::stoul(end));
    }
}

void MAGICParams::insertHop(std::string id, const ndn::Interest& interest){
    if (hops.size() > 0){
        updateBuffer();
        // std::cout << log_uuid << ": " << " previous hops " << buffer << std::endl;
    }
    else{
        // std::cout << log_uuid << ": " << " starting hop " << id << std::endl;
    }
    hops.push_back(id);
}

std::string MAGICParams::getParams(){
    updateBuffer();
    return buffer;
}

void MAGICParams::updateBuffer(){
    buffer = "~";
    for (auto i : hops)
        buffer += i + " ";

    buffer += std::to_string(log_uuid) + " ";
    buffer += std::to_string(m_popularity);
}

void MAGICParams::updatePopularity(uint32_t popularity){
    if (popularity > m_popularity)
        m_popularity = popularity;
}

void MAGICParams::readStringFromBlock(const ndn::Block& parameters){
    buffer = ndn::encoding::readString(parameters);
    std::string::size_type i = buffer.find("~");

    if (i != std::string::npos)
        buffer = buffer.substr(i + 1);

}

void MAGICParams::addToInterest(const ndn::Interest& interest)
{
    ndn::Interest* non_const_interest = (ndn::Interest*)&interest;
    updateBuffer();
    Block metainf_block = ndn::encoding::makeStringBlock(150, buffer);
    non_const_interest->setApplicationParameters(metainf_block);
}

void MAGICParams::addToData(std::shared_ptr<ndn::Data> data)
{
    std::shared_ptr<ndn::MetaInfo> metainf = make_shared<ndn::MetaInfo>();
    std::string pop_field = " " + std::to_string(m_popularity);
    
    updateBuffer();
    Block metainf_block = ndn::encoding::makeStringBlock(150, buffer);

    std::string temp = ndn::encoding::readString(metainf_block);
    metainf->addAppMetaInfo(metainf_block);
    
    data->setMetaInfo(*metainf);
}

void MAGICParams::addToData(ndn::Data& data)
{
    std::shared_ptr<ndn::MetaInfo> metainf = make_shared<ndn::MetaInfo>();
    std::string pop_field = " " + std::to_string(m_popularity);
    
    updateBuffer();
    Block metainf_block = ndn::encoding::makeStringBlock(150, buffer);

    std::string temp = ndn::encoding::readString(metainf_block);
    metainf->addAppMetaInfo(metainf_block);
    
    data.setMetaInfo(*metainf);
}

}
}