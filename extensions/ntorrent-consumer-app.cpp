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

#include "ntorrent-consumer-app.hpp"

NS_LOG_COMPONENT_DEFINE("NTorrentConsumerApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentConsumerApp);

TypeId
NTorrentConsumerApp::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentConsumerApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentConsumerApp>()
      .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_seq), MakeIntegerChecker<int32_t>())

      .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                    MakeNameAccessor(&NTorrentConsumerApp::m_interestName), MakeNameChecker())
      .AddAttribute("namesPerSegment", "Number of names (files) per segment", IntegerValue(2),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_namesPerSegment), MakeIntegerChecker<int32_t>())
      .AddAttribute("namesPerManifest", "Number of names per manifest", IntegerValue(64),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_namesPerManifest), MakeIntegerChecker<int32_t>())
      .AddAttribute("dataPacketSize", "Size of each data packet", IntegerValue(64),
                    MakeIntegerAccessor(&NTorrentConsumerApp::m_dataPacketSize), MakeIntegerChecker<int32_t>())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("1s"),
                    MakeTimeAccessor(&NTorrentConsumerApp::m_interestLifeTime), MakeTimeChecker());
    return tid;
}

NTorrentConsumerApp::NTorrentConsumerApp()
{
}

NTorrentConsumerApp::~NTorrentConsumerApp()
{
}

void
NTorrentConsumerApp::StartApplication()
{
    App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), "/", m_face, 0);
    copyTorrentFile();

    //Send interest for initial torrent segment
    SendInterest(m_initialSegment.getFullName().toUri());
}

void
NTorrentConsumerApp::StopApplication()
{
    App::StopApplication();
}

void
NTorrentConsumerApp::SendInterest()
{
  //This function is just for testing for testing, not really going to be used..
  std::string interestName = std::string(ndn_ntorrent::SharedConstants::commonPrefix) + "/NTORRENT/" + to_string(m_seq++);
  auto interest = std::make_shared<Interest>(interestName);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));
  NS_LOG_DEBUG("SEND INTEREST::: " << *interest);
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

void
NTorrentConsumerApp::SendInterest(const string& interestName)
{
  auto interest = std::make_shared<Interest>(interestName);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
  interest->setNonce(rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setInterestLifetime(ndn::time::seconds(1));
  NS_LOG_DEBUG("SEND INTEREST::: " << *interest);
  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
}

//TODO: Implement this method better...
void
NTorrentConsumerApp::OnInterest(shared_ptr<const Interest> interest)
{
    ndn::App::OnInterest(interest);
    const auto& interestName = interest->getName();

    ndn_ntorrent::IoUtil::NAME_TYPE interestType = ndn_ntorrent::IoUtil::findType(interestName);

    std::shared_ptr<Data> data = nullptr;

    auto cmp = [&interestName](const Data& t){return t.getFullName() == interestName;};

    switch(interestType)
    {
        case ndn_ntorrent::IoUtil::TORRENT_FILE:
        {
            NS_LOG_DEBUG("RECIEVED INTEREST (torrent-file):::" << interestName);
            auto torrent_it =  std::find_if(m_torrentSegments.begin(), m_torrentSegments.end(), cmp);

            if (m_torrentSegments.end() != torrent_it) {
                data = std::make_shared<Data>(*torrent_it);
            }
            else{
                NS_LOG_ERROR("Don't have this torrent...");
            }
            break;
        }
        case ndn_ntorrent::IoUtil::FILE_MANIFEST:
        {
            NS_LOG_DEBUG("RECIEVED INTEREST (file-manifest):::" << interestName);
            auto manifest_it = std::find_if(manifests.begin(), manifests.end(), cmp);
            if (manifests.end() != manifest_it) {
                data = std::make_shared<Data>(*manifest_it) ;
            }
            else{
                NS_LOG_ERROR("Don't have this manifest...");
            }
            break;
        }
        case ndn_ntorrent::IoUtil::DATA_PACKET:
        {
            NS_LOG_DEBUG("RECIEVED INTEREST (data-packet):::" << interestName);
            auto data_it = std::find_if(dataPackets.begin(), dataPackets.end(), cmp);
            if (dataPackets.end() != data_it) {
                data = std::make_shared<Data>(*data_it) ;
            }
            else{
                NS_LOG_ERROR("Don't have this data...");
            }
            break;
        }
        case ndn_ntorrent::IoUtil::UNKNOWN:
        {
            //This should never happen
            break;
        }
    }

    if(nullptr != data && interestType != ndn_ntorrent::IoUtil::UNKNOWN)
    {
        /*Signature signature;
        SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

        if (m_keyLocator.size() > 0) {
            signatureInfo.setKeyLocator(m_keyLocator);
        }

        signature.setInfo(signatureInfo);
        signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

        data->setSignature(signature);
        NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());*/

        data->wireEncode();
        m_transmittedDatas(data, this, m_face);
        m_appLink->onReceiveData(*data);
    }
}

void
NTorrentConsumerApp::copyTorrentFile()
{
    //This is "technically" the same as what the producer does
    //In a real world application, both parties will have a torrent file
    //Since that isn't possible here, just generate the same torrent file on producer and consumer

    NS_LOG_DEBUG("Copying torrent file!");
    const auto& content = ndn_ntorrent::TorrentFile::generate(ndn_ntorrent::DUMMY_FILE_PATH,
            m_namesPerSegment, m_namesPerManifest, m_dataPacketSize, true);

    //Copy only initial segment. Nothing else will be used.
    //This will be used to make future requests
    m_initialSegment = content.first.at(0);
}

void
NTorrentConsumerApp::OnData(shared_ptr<const Data> data)
{
    NS_LOG_DEBUG("RECEIVED: " << data->getFullName());
    NS_LOG_DEBUG("face: " << m_face);
    ndn_ntorrent::IoUtil::NAME_TYPE interestType = ndn_ntorrent::IoUtil::findType(data->getFullName());

    //shared_ptr<nfd::Forwarder> m_forwarder = GetNode()->GetObject<L3Protocol>()->getForwarder();
    //nfd::Fib& fib = m_forwarder.get()->getFib();
    if(interestType != ndn_ntorrent::IoUtil::UNKNOWN)
    {
        ndn::FibHelper::AddRoute(GetNode(), data->getFullName(), m_face, 0);
        GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.AddOrigin(data->getFullName().toUri(), GetNode());
        
        //TODO: This can probably be optimized
        GlobalRoutingHelper::CalculateRoutes();
        //GlobalRoutingHelper::CalculateAllPossibleRoutes();
    }
    
    /*Verify FIB entries
    uint32_t fib_size = fib.size();
    uint32_t c=0;
    for(nfd::Fib::const_iterator it = fib.begin(); it != fib.end(); it++)
    {
        NS_LOG_DEBUG("Fib entry: [" << ++c << "/" << fib_size << "] " << it->getPrefix());
    }*/

    switch(interestType)
    {
        case ndn_ntorrent::IoUtil::TORRENT_FILE:
        {
            //TODO: Announce prefix - RibManager
            ndn_ntorrent::TorrentFile file(data->wireEncode());
            m_torrentSegments.push_back(file);

            std::vector<Name> manifestCatalog = file.getCatalog();
            shared_ptr<Name> nextSegmentPtr = file.getTorrentFilePtr();
            if(nextSegmentPtr!=nullptr)
            {
                SendInterest(nextSegmentPtr.get()->toUri());
            }
            else
            {
                NS_LOG_DEBUG("W00t! Torrent file is done!");
            }

            for(uint8_t i=0; i<manifestCatalog.size(); i++)
            {
                SendInterest(manifestCatalog.at(i).toUri());
            }
            break;
        }
        case ndn_ntorrent::IoUtil::FILE_MANIFEST:
        {
            //TODO: Announce prefix - RibManager
            ndn_ntorrent::FileManifest fm(data->wireEncode());
            manifests.push_back(fm);

            std::vector<Name> subManifestCatalog = fm.catalog();
            shared_ptr<Name> nextSegmentPtr = fm.submanifest_ptr();
            if(nextSegmentPtr!=nullptr)
            {
                SendInterest(nextSegmentPtr.get()->toUri());
            }
            else
            {
                NS_LOG_DEBUG("W00t! File manifest is done!");
            }

            for(uint8_t i=0; i<subManifestCatalog.size(); i++)
            {
                SendInterest(subManifestCatalog.at(i).toUri());
            }
            break;
        }
        case ndn_ntorrent::IoUtil::DATA_PACKET:
        {
            //TODO: Announce prefix - RibManager
            Data d(data->wireEncode());
            dataPackets.push_back(d);
            Block content = d.getContent();
            std::string output(content.value_begin(), content.value_end());
            NS_LOG_DEBUG("DATA RECEIVED:");
            NS_LOG_DEBUG("=== BEGIN ===");
            std::cout << output << std::endl;
            NS_LOG_DEBUG("=== END ===");
            break;
        }
        case ndn_ntorrent::IoUtil::UNKNOWN:
        {
            //This should never happen
            break;
        }
    }
}
void
NTorrentConsumerApp::OnNack(shared_ptr<const lp::Nack> nack)
{
  App::OnNack(nack);

  NS_LOG_INFO("NACK received for: " << nack->getInterest().getName()
              << ", reason: " << nack->getReason());
}

} // namespace ndn
} // namespace ns3
