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
  uint16_t face_id = inFace.getId();
  Name interestName = interest.getName();
  long int curr_timestamp = getTimestamp();
  std::cout << curr_timestamp << ": ARI " << face_id << " " << interestName << std::endl;

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

  auto it = face_name_incoming_time.find(face_id);
  name_incoming_time n;
  //If there's no such face in the map, create an entry, store the interest name and arrival timestamp
  if(it==face_name_incoming_time.end())
  {
      n.insert(std::make_pair(interestName, curr_timestamp));
      face_name_incoming_time.insert(std::make_pair(face_id, n));
  }
  //If there's this face in the map, check if the interest name already exists...
  else
  {
    n = it->second;
    //auto _it = n.find(interestName);
    //If it doesn't exist, create a new interestName-> arrival time mapping
    //If it exists, update the latest arrival time
    n.insert(std::make_pair(interestName, curr_timestamp));
    it->second = n;
    face_name_incoming_time.insert(std::make_pair(face_id, n));
  }
  

  //Pick one face at random if you have no information about delay
  if(face_average_delay.size()==0){
      fib::NextHopList::const_iterator selected;
      do {
        boost::random::uniform_int_distribution<> dist(0, nexthops.size() - 1);
        const size_t randomIndex = dist(m_randomGenerator);
        
        uint64_t currentIndex = 0;

        for (selected = nexthops.begin(); selected != nexthops.end() && currentIndex != randomIndex;
             ++selected, ++currentIndex) {
        }
      } while (!canForwardToNextHop(inFace, pitEntry, *selected));

      this->sendInterest(pitEntry, selected->getFace(), interest);
      //std::cout << face_id << " -> " << selected->getFace().getId() << std::endl;
  }

  else
  {
      //TODO: Implement logic to pick best one here...
  }
}

void
NTorrentStrategy::beforeSatisfyInterest (const shared_ptr< pit::Entry > &pitEntry, const Face &inFace, const Data &data)
{
  NFD_LOG_TRACE("beforeSatisfyInterest");
  uint16_t face_id = inFace.getId();
  Name dataName = data.getFullName();
  long int curr_timestamp = getTimestamp();
  
  ndn_ntorrent::IoUtil::NAME_TYPE dataType = ndn_ntorrent::IoUtil::findType(dataName);
  if(dataType == ndn_ntorrent::IoUtil::UNKNOWN)
      return;
  std::cout << curr_timestamp << ": BSI " << face_id << " " << dataName << std::endl;

  //Need to revisit this logic...
  /*const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nexthops = fibEntry.getNextHops();
  fib::NextHopList::const_iterator selected;
  for (selected = nexthops.begin(); selected != nexthops.end(); ++selected) {
      uint16_t outface_id = selected->getFace().getId();
      auto it = face_name_incoming_time.find(outface_id);
      
      //This should always be true because you can't satisfy an interest because you receive it!
      if(it!=face_name_incoming_time.end())
      {
        name_incoming_time n;
        n = it->second;
        std::cout << face_id << " ::: " << it->first << std::endl;
      }
      else
      {
          std::cout << "WHAT???" << std::endl;
      }
  }*/
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
