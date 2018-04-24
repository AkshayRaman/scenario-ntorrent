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

#include "ntorrent-producer-app.hpp"

NS_LOG_COMPONENT_DEFINE("NTorrentProducerApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(NTorrentProducerApp);

TypeId
NTorrentProducerApp::GetTypeId(void)
{
    static TypeId tid = TypeId("NTorrentProducerApp")
      .SetParent<Application>()
      .AddConstructor<NTorrentProducerApp>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&NTorrentProducerApp::m_prefix), MakeNameChecker())
      .AddAttribute("namesPerSegment", "Number of names (files) per segment", IntegerValue(2),
                    MakeIntegerAccessor(&NTorrentProducerApp::m_namesPerSegment), MakeIntegerChecker<int32_t>())
      .AddAttribute("namesPerManifest", "Number of names per manifest", IntegerValue(64),
                    MakeIntegerAccessor(&NTorrentProducerApp::m_namesPerManifest), MakeIntegerChecker<int32_t>())
      .AddAttribute("dataPacketSize", "Size of each data packet", IntegerValue(64),
                    MakeIntegerAccessor(&NTorrentProducerApp::m_dataPacketSize), MakeIntegerChecker<int32_t>())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", IntegerValue(1024),
              MakeIntegerAccessor(&NTorrentProducerApp::m_virtualPayloadSize),
              MakeIntegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
              TimeValue(Seconds(0)), MakeTimeAccessor(&NTorrentProducerApp::m_freshness),
              MakeTimeChecker())
      .AddAttribute("Signature",
              "Fake signature, 0 valid signature (default), other values application-specific",
              IntegerValue(0), MakeIntegerAccessor(&NTorrentProducerApp::m_signature),
              MakeIntegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
              "Name to be used for key locator.  If root, then key locator is not used",
              NameValue(), MakeNameAccessor(&NTorrentProducerApp::m_keyLocator), MakeNameChecker());

    return tid;
}

NTorrentProducerApp::NTorrentProducerApp()
{
}

NTorrentProducerApp::~NTorrentProducerApp()
{
}

void
NTorrentProducerApp::StartApplication()
{
    App::StartApplication();
    ndn::FibHelper::AddRoute(GetNode(), "/", m_face, 0);
    generateTorrentFile();
}

void
NTorrentProducerApp::StopApplication()
{
    App::StopApplication();
}

void
NTorrentProducerApp::OnInterest(shared_ptr<const Interest> interest)
{
    ndn::App::OnInterest(interest);
    const auto& interestName = interest->getName();

    ndn_ntorrent::IoUtil::NAME_TYPE interestType = ndn_ntorrent::IoUtil::findType(interestName);

    std::shared_ptr<Data> data = nullptr;

    auto cmp = [&interestName](const Data& t){return t.getFullName() == interestName;};

    if(interestType != ndn_ntorrent::IoUtil::UNKNOWN)
    {
        //ndn::FibHelper::AddRoute(GetNode(), interestName, m_face, 0);
        GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.AddOrigin(interestName.toUri(), GetNode());
        
        //TODO: This can probably be optimized
        GlobalRoutingHelper::CalculateRoutes();
        //GlobalRoutingHelper::CalculateAllPossibleRoutes();
    }
    
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
                NS_LOG_INFO("Don't have this torrent...");
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
                NS_LOG_INFO("Don't have this manifest...");
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
                NS_LOG_INFO("Don't have this data...");
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
NTorrentProducerApp::generateTorrentFile()
{
    NS_LOG_DEBUG("Creating torrent file!");
    const auto& content = ndn_ntorrent::TorrentFile::generate(ndn_ntorrent::DUMMY_FILE_PATH,
            m_namesPerSegment, m_namesPerManifest, m_dataPacketSize, true);

    m_torrentSegments = content.first;

    for (const auto& ms : content.second) {
        manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
        dataPackets.insert(dataPackets.end(), ms.second.begin(), ms.second.end());
    }

    for(const auto& t : m_torrentSegments)
        NS_LOG_DEBUG("Torrent segment name: " << t.getFullName());
    for(uint32_t i=0;i<manifests.size();i++)
        NS_LOG_DEBUG("Manifest name: " << manifests.at(i).getFullName());
    for(uint32_t i=0;i<dataPackets.size();i++)
        NS_LOG_DEBUG("Data: " << dataPackets.at(i).getFullName());

    NS_LOG_DEBUG("Producer stats: ");
    NS_LOG_DEBUG("Torrent segments: " << m_torrentSegments.size());
    NS_LOG_DEBUG("Manifests: " << manifests.size());
    NS_LOG_DEBUG("Data Packets: " << dataPackets.size());
}
void
NTorrentProducerApp::OnNack(shared_ptr<const lp::Nack> nack)
{
  App::OnNack(nack);

  NS_LOG_INFO("NACK received for: " << nack->getInterest().getName()
              << ", reason: " << nack->getReason());
}

} // namespace ndn
} // namespace ns3
