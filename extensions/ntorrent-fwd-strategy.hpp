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

protected:
  boost::random::mt19937 m_randomGenerator;
  
  typedef std::unordered_map<int, int> face_score;
  std::unordered_map<Name, face_score> interest_hop_score_map;
  
  typedef std::unordered_map<int, std::pair<float,float>> i_f_f; //Face mapped with two floats
  i_f_f face_satisfaction_rate; //Face mapped to interests satisfied/interests recieved
  i_f_f face_average_delay; //Face mapped to total delay/number of transfers
  
  std::unordered_map<Name,std::vector<int>> nackedname_nexthop;
};

} // namespace fw
} // namespace nfd

#endif // NTORRENT_STRATEGY_HPP
