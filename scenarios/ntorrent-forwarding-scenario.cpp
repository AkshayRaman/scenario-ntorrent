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
 * Authors: Akshay Raman <akshay.raman@cs.ucla.edu>
 */

#include "simulation-common.hpp"

namespace ns3 {
namespace ndn {

int
main(int argc, char *argv[])
{

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("32kbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));

  //defaults for command line arguments
  uint32_t namesPerSegment = 2;
  uint32_t namesPerManifest = 2;
  uint32_t dataPacketSize = 64;
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("namesPerSegment", "Number of names per segment", namesPerSegment);
  cmd.AddValue("namesPerManifest", "Number of names per manifest", namesPerManifest);
  cmd.AddValue("dataPacketSize", "Data Packet size", dataPacketSize);
  cmd.Parse(argc, argv);

  std::cout << "Running with parameters: " << std::endl;
  std::cout << "namesPerSegment: " << namesPerSegment << std::endl;
  std::cout << "namesPerManifest: " << namesPerManifest << std::endl;
  std::cout << "dataPacketSize: " << dataPacketSize << std::endl;
  
  // Creating nodes
  NodeContainer nodes;
  nodes.Create(8);
  
  // Connecting nodes using two links
  PointToPointHelper p2p;
  createLink(p2p, nodes.Get(0), nodes.Get(1));
  createLink(p2p, nodes.Get(3), nodes.Get(2));
  createLink(p2p, nodes.Get(1), nodes.Get(3));
  createLink(p2p, nodes.Get(3), nodes.Get(4));
  createLink(p2p, nodes.Get(4), nodes.Get(5));
  createLink(p2p, nodes.Get(2), nodes.Get(6));
  createLink(p2p, nodes.Get(2), nodes.Get(7));
  
  // Install NDN stack on all nodes
  StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  //StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  StrategyChoiceHelper::Install<nfd_fw::NTorrentStrategy>(nodes, "/");

  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications
  ndn::AppHelper p1("NTorrentProducerApp");
  createAndInstall(p1, namesPerSegment, namesPerManifest, dataPacketSize, "producer", nodes.Get(0), 1.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(0), 60, 100);
  
  AnimationInterface::SetConstantPosition (nodes.Get(1), 180, 100);
  
  ndn::AppHelper c1("NTorrentConsumerApp");
  createAndInstall(c1, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(3), 3.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(3), 120, 100);
  
  ndn::AppHelper c2("NTorrentConsumerApp");
  createAndInstall(c2, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(2), 6.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(2), 180, 75);
  
  ndn::AppHelper c3("NTorrentConsumerApp");
  createAndInstall(c3, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(4), 7.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(4), 180, 125);
  
  ndn::AppHelper c4("NTorrentConsumerApp");
  createAndInstall(c4, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(5), 9.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(5), 150, 150);
  
  ndn::AppHelper c5("NTorrentConsumerApp");
  createAndInstall(c5, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(6), 15.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(6), 200, 60);
  
  ndn::AppHelper c6("NTorrentConsumerApp");
  createAndInstall(c6, namesPerSegment, namesPerManifest, dataPacketSize, "consumer", nodes.Get(7), 11.0f);
  AnimationInterface::SetConstantPosition (nodes.Get(7), 200, 90);
  
  ndnGlobalRoutingHelper.AddOrigins("/NTORRENT", nodes.Get(0));
  GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(60.0));
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
