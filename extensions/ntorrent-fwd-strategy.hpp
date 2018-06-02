#ifndef NTORRENT_STRATEGY_HPP
#define NTORRENT_STRATEGY_HPP
#include "src/torrent-file.hpp"
#include "src/file-manifest.hpp"
#include "src/torrent-manager.hpp"
#include "src/util/shared-constants.hpp"
#include "src/util/simulation-constants.hpp"
#include "src/util/io-util.hpp"

namespace ndn_ntorrent = ndn::ntorrent;

#include <boost/random/mersenne_twister.hpp>
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/algorithm.hpp"
#include <sys/time.h>

namespace nfd {
namespace fw {

class NTorrentStrategy : public Strategy {
public:
  NTorrentStrategy(Forwarder& forwarder, const Name& name = getStrategyName());

  virtual ~NTorrentStrategy() override;

  virtual void
  afterReceiveInterest (const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  //TODO: Add afterReceiveData
    
  virtual void
  beforeSatisfyInterest (const shared_ptr< pit::Entry > &pitEntry, 
                        const Face &inFace, const Data &data) override;

  virtual void
  beforeExpirePendingInterest (const shared_ptr< pit::Entry > &pitEntry) override;

  virtual void  
  afterReceiveNack (const Face &inFace, const lp::Nack &nack, 
          const shared_ptr< pit::Entry > &pitEntry) override;

  virtual void  
  onDroppedInterest (const Face &outFace, const Interest &interest)
  override;

  virtual void
  afterReceiveData (const shared_ptr<pit::Entry>& pitEntry,
          const Face& inFace, const Data& data);
  
  static const Name&
  getStrategyName();

  static long int getTimestamp(){
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int us = tp.tv_sec*1000000 + tp.tv_usec;
    return us;
  }

protected:
  boost::random::mt19937 m_randomGenerator;

  typedef std::unordered_map<Name,long int> name_incoming_time;
  std::unordered_map<int, name_incoming_time> face_name_incoming_time;
  std::unordered_map<int, std::pair<int,int>> face_average_delay;
  
  std::unordered_map<Name,std::vector<int>> nackedname_nexthop;
};

} // namespace fw
} // namespace nfd

#endif // NTORRENT_STRATEGY_HPP
