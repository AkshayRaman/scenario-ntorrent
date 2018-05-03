#ifndef NTORRENT_STRATEGY_HPP
#define NTORRENT_STRATEGY_HPP

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
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       const shared_ptr<pit::Entry>& pitEntry) override;

  static const Name&
  getStrategyName();

protected:
  boost::random::mt19937 m_randomGenerator;
};

} // namespace fw
} // namespace nfd

#endif // NTORRENT_STRATEGY_HPP
