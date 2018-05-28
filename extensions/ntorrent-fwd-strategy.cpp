#include "ntorrent-fwd-strategy.hpp"

#include <boost/random/uniform_int_distribution.hpp>

#include <ndn-cxx/util/random.hpp>

#include "core/logger.hpp"

#define MAX_SCORE 100

NFD_LOG_INIT("NTorrentStrategy");

namespace nfd {
namespace fw {

NTorrentStrategy::NTorrentStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder)
{
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

NTorrentStrategy::~NTorrentStrategy()
{
}

static bool
canForwardToNextHop(const Face& inFace, shared_ptr<pit::Entry> pitEntry, const fib::NextHop& nexthop)
{
  return !wouldViolateScope(inFace, pitEntry->getInterest(), nexthop.getFace()) &&
    canForwardToLegacy(*pitEntry, nexthop.getFace());
}

static bool
hasFaceForForwarding(const Face& inFace, const fib::NextHopList& nexthops, const shared_ptr<pit::Entry>& pitEntry)
{
  return std::find_if(nexthops.begin(), nexthops.end(), bind(&canForwardToNextHop, cref(inFace), pitEntry, _1))
         != nexthops.end();
}

void
NTorrentStrategy::afterReceiveInterest (const Face& inFace, const Interest& interest,
                                                 const shared_ptr<pit::Entry>& pitEntry)
{
  NFD_LOG_TRACE("afterReceiveInterest");

  if (hasPendingOutRecords(*pitEntry)) {
    // not a new Interest, don't forward
    return;
  }

  //After you get a match, you might have multiple next hops. You pick the best one naively. Keep some score for next hop of the FIB entry.
  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();

  // Ensure there is at least 1 Face is available for forwarding
  if (!hasFaceForForwarding(inFace, nexthops, pitEntry)) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

    boost::random::uniform_int_distribution<> dist(1, MAX_SCORE);
    
    Name interestName = interest.getName();
    std::unordered_map<Name, face_score>::iterator it = interest_hop_score_map.find(interestName);
    if(it==interest_hop_score_map.end())
    {
        face_score f;
        interest_hop_score_map.insert(std::make_pair(interestName, f));
    }
    
    //Add randomScore to the data structure
    fib::NextHopList::const_iterator selected;
    for (selected = nexthops.begin(); selected != nexthops.end(); ++selected) {
        size_t randomScore = dist(m_randomGenerator);
        int face_id = selected->getFace().getId();
        
        auto it = interest_hop_score_map.find(interestName);
         
        face_score f = it->second;
        auto f_it = f.find(face_id);
        if(f_it == f.end())
        {
            //"Force" best score, always use same hop
            //if(face_id==256) randomScore=MAX_SCORE+1;
            f.insert(std::pair<int,int>(face_id, randomScore));
        }
        else
        {
            f_it->second += randomScore;
        }
        it->second = f;
    }
    
    //pick best hop
    it = interest_hop_score_map.find(interestName);
    face_score f = it->second;
    int best_hop=-1; int best_score=0;
    
    for(std::pair<int, int> element : it->second)
    {
        if(element.second > best_score)
        {
            Face *face = getFace(element.first);
            if(canForwardToNextHop(inFace, pitEntry, fib::NextHop(*face))){
                best_score = element.second;
                best_hop = element.first;
            }
        }
    }
    if(best_hop!=-1)
    {
        //std::cout << "Using best hop: " << best_hop << std::endl;
        this->sendInterest(pitEntry, *getFace(best_hop), interest);
    }
    //else send NACK
        
    /*Print the interest_hop_score_map
     * for (std::pair<Name, face_score> element : interest_hop_score_map)
    {
        std::cout << element.first << std::endl;
        for(std::pair<int, int> e1 : element.second)
            std::cout << e1.first << ", " << e1.second << std::endl;
    }*/

}

void
NTorrentStrategy::beforeSatisfyInterest (const shared_ptr< pit::Entry > &pitEntry, const Face &inFace, const Data &data)
{
  NFD_LOG_TRACE("beforeSatisfyInterest");
}
  
void
NTorrentStrategy::beforeExpirePendingInterest (const shared_ptr< pit::Entry > &pitEntry)
{
  NFD_LOG_TRACE("beforeExpirePendingInterest");
}

void  
NTorrentStrategy::afterReceiveNack (const Face &inFace, const lp::Nack &nack, 
          const shared_ptr< pit::Entry > &pitEntry)
{
  NFD_LOG_TRACE("afterReceiveNack");
}

void 
NTorrentStrategy::onDroppedInterest (const Face &outFace, const Interest &interest)
{
  NFD_LOG_TRACE("onDroppedInterest");
}
  
void  
NTorrentStrategy::afterReceiveData (const shared_ptr<pit::Entry>& pitEntry,
        const Face& inFace, const Data& data)
{
  NFD_LOG_TRACE("afterReceiveData");
}


const Name&
NTorrentStrategy::getStrategyName()
{
  static Name strategyName("ndn:/localhost/nfd/strategy/ntorrent-strategy/%FD%01");
  return strategyName;
}

} // namespace fw
} // namespace nfd
