/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Spyridon (Spyros) Mastorakis <mastorakis@cs.ucla.edu>
 *          Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "simulation-common.hpp"

namespace ns3 {
namespace ndn {

int
main(int argc, char *argv[])
{

  // setting default parameters for PointToPoint links and channels
  //Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("32kbps"));
  //Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));

  //defaults for command line arguments
  uint32_t namesPerSegment = 1;
  uint32_t namesPerManifest = 2;
  uint32_t dataPacketSize = 64;
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("namesPerSegment", "Number of names per segment", namesPerSegment);
  cmd.AddValue("namesPerManifest", "Number of names per manifest", namesPerManifest);
  cmd.AddValue("dataPacketSize", "Data Packet size", dataPacketSize);
  cmd.Parse(argc, argv);

  int nodeCount = 8;
  
  // Creating nodes
  NodeContainer nodes;
  nodes.Create(nodeCount);
  
  // Connecting nodes using two links
  PointToPointHelper p2p;
  
  //https://named-data.net/wp-content/uploads/2017/06/2017-icccn-ntorrent.pdf#figure.caption.7
  
  //Peer 4 - Acts as seeder (producer)
  AnimationInterface::SetConstantPosition (nodes.Get(0), 75, 0);
  //Router 4 
  AnimationInterface::SetConstantPosition (nodes.Get(1), 75, 30);
  //Router 2
  AnimationInterface::SetConstantPosition (nodes.Get(2), 75, 90);
  //Peer 2 - Downloads data third
  AnimationInterface::SetConstantPosition (nodes.Get(3), 75, 120);
  
  //Peer 1 - Downloads data first
  AnimationInterface::SetConstantPosition (nodes.Get(4), 0, 60);
  //Router 1
  AnimationInterface::SetConstantPosition (nodes.Get(5), 30, 60);
  //Router 3
  AnimationInterface::SetConstantPosition (nodes.Get(6), 120, 60);
  //Peer 3 - Downloads data second
  AnimationInterface::SetConstantPosition (nodes.Get(7), 150, 60);
  
  createLink(p2p, nodes.Get(0), nodes.Get(1), "32kbps", "10ms");
  createLink(p2p, nodes.Get(1), nodes.Get(2), "32kbps", "20ms");
  createLink(p2p, nodes.Get(2), nodes.Get(3), "32kbps", "10ms");
  
  createLink(p2p, nodes.Get(4), nodes.Get(5), "32kbps", "10ms");
  createLink(p2p, nodes.Get(5), nodes.Get(6), "32kbps", "10ms");
  createLink(p2p, nodes.Get(6), nodes.Get(7), "32kbps", "15ms");
  
  createLink(p2p, nodes.Get(1), nodes.Get(5), "32kbps", "20ms");
  createLink(p2p, nodes.Get(2), nodes.Get(6), "32kbps", "10ms");
  createLink(p2p, nodes.Get(1), nodes.Get(6), "32kbps", "20ms");
  createLink(p2p, nodes.Get(2), nodes.Get(5), "32kbps", "10ms");
  
  // Install NDN stack on all nodes
  StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  //StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  //StrategyChoiceHelper::Install<nfd_fw::NTorrentStrategy>(nodes, "/");
  StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/client-control");

  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications
  ndn::AppHelper p1("NTorrentProducerApp");
  createAndInstall(p1, namesPerSegment, namesPerManifest, dataPacketSize, "producer", nodes.Get(0), 1.0);
  
  ndn::AppHelper c1("NTorrentConsumerApp");
  createAndInstall(c1, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(4), 5.0);

  ndn::AppHelper c2("NTorrentConsumerApp");
  createAndInstall(c2, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(7), 7.5);

  ndn::AppHelper c3("NTorrentConsumerApp");
  createAndInstall(c3, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(3), 10.0);

  Simulator::Stop(Seconds(120.0));

  std::cout << "Running with parameters: " << std::endl;
  std::cout << "namesPerSegment: " << namesPerSegment << std::endl;
  std::cout << "namesPerManifest: " << namesPerManifest << std::endl;
  std::cout << "dataPacketSize: " << dataPacketSize << std::endl;
  
  ndnGlobalRoutingHelper.AddOrigins("/NTORRENT", nodes.Get(0));
  GlobalRoutingHelper::CalculateRoutes();

  ndn::L3RateTracer::InstallAll("node-degree-4.txt", Seconds(0.5));

  Simulator::Run();
  Simulator::Destroy();
  
  return 0;
}

} // namespace ndn
} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::ndn::main(argc, argv);
}
