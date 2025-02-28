/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/pointer.h"
#include "edca-txop-n.h"
#include "mac-low.h"
#include "dcf-manager.h"
#include "mac-tx-middle.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac.h"
#include "random-stream.h"
#include "wifi-mac-queue.h"
#include "msdu-aggregator.h"
#include "mgt-headers.h"
#include "qos-blocked-destinations.h"
#include "ns3/simulator.h" // for debug
#include "ns3/trace-source-accessor.h"
#include "ns3/time-series-adaptor.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EdcaTxopN");

class EdcaTxopN::Dcf : public DcfState
{
public:
  Dcf (EdcaTxopN * txop)
    : m_txop (txop)
  {
  }

private:
  virtual void DoNotifyAccessGranted (void)
  {
    m_txop->NotifyAccessGranted ();
  }
  virtual void DoNotifyInternalCollision (void)
  {
    m_txop->NotifyInternalCollision ();
  }
  virtual void DoNotifyCollision (void)
  {
    m_txop->NotifyCollision ();
  }
  virtual void DoNotifyChannelSwitching (void)
  {
    m_txop->NotifyChannelSwitching ();
  }
  virtual void DoNotifySleep (void)
  {
    m_txop->NotifySleep ();
  }
  virtual void DoNotifyOff (void)
  {
    m_txop->NotifyOff ();
  }
  virtual void DoNotifyWakeUp (void)
  {
    m_txop->NotifyWakeUp ();
  }
  virtual void DoNotifyOn (void)
  {
    m_txop->NotifyOn ();
  }

  EdcaTxopN *m_txop;
};


class EdcaTxopN::TransmissionListener : public MacLowTransmissionListener
{
public:
  TransmissionListener (EdcaTxopN * txop)
    : MacLowTransmissionListener (),
      m_txop (txop)
  {
  }

  virtual ~TransmissionListener ()
  {
  }

  virtual void GotCts (double snr, WifiMode txMode)
  {
    m_txop->GotCts (snr, txMode);
  }
  virtual void MissedCts (void)
  {
    m_txop->MissedCts ();
  }
  virtual void GotAck (double snr, WifiMode txMode)
  {
    m_txop->GotAck (snr, txMode);
  }
  virtual void MissedAck (void)
  {
    m_txop->MissedAck ();
  }
  virtual void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address source, WifiMode txMode)
  {
    m_txop->GotBlockAck (blockAck, source,txMode);
  }
  virtual void MissedBlockAck (void)
  {
    m_txop->MissedBlockAck ();
  }
  virtual void StartNext (void)
  {
    m_txop->StartNext ();
  }
  virtual void Cancel (void)
  {
    m_txop->Cancel ();
  }
  virtual void EndTxNoAck (void)
  {
    m_txop->EndTxNoAck ();
  }
  virtual Ptr<WifiMacQueue> GetQueue (void)
  {
    return m_txop->GetEdcaQueue ();
  }

private:
  EdcaTxopN *m_txop;
};


class EdcaTxopN::AggregationCapableTransmissionListener : public MacLowAggregationCapableTransmissionListener
{
public:
  AggregationCapableTransmissionListener (EdcaTxopN * txop)
    : MacLowAggregationCapableTransmissionListener (),
      m_txop (txop)
  {
  }
  virtual ~AggregationCapableTransmissionListener ()
  {
  }

  virtual void BlockAckInactivityTimeout (Mac48Address address, uint8_t tid)
  {
    m_txop->SendDelbaFrame (address, tid, false);
  }
  virtual Ptr<WifiMacQueue> GetQueue (void)
  {
    return m_txop->GetEdcaQueue ();
  }
  virtual  void CompleteTransfer (Mac48Address recipient, uint8_t tid)
  {
    m_txop->CompleteAmpduTransfer (recipient, tid);
  }
  virtual void SetAmpdu (bool ampdu)
  {
    return m_txop->SetAmpduExist (ampdu);
  }
  virtual void CompleteMpduTx (Ptr<const Packet> packet, WifiMacHeader hdr, Time tstamp)
  {
    m_txop->CompleteMpduTx (packet, hdr, tstamp);
  }
  virtual uint16_t GetNextSequenceNumberfor (WifiMacHeader *hdr)
  {
    return m_txop->GetNextSequenceNumberfor (hdr);
  }
  virtual uint16_t PeekNextSequenceNumberfor (WifiMacHeader *hdr)
  {
    return m_txop->PeekNextSequenceNumberfor (hdr);
  }
  virtual Ptr<const Packet> PeekNextPacketInBaQueue (WifiMacHeader &header, Mac48Address recipient, uint8_t tid, Time *timestamp)
  {
    return m_txop->PeekNextRetransmitPacket (header, recipient, tid, timestamp);
  }
  virtual void RemoveFromBaQueue (uint8_t tid, Mac48Address recipient, uint16_t seqnumber)
  {
    m_txop->RemoveRetransmitPacket (tid, recipient, seqnumber);
  }
  virtual bool GetBlockAckAgreementExists (Mac48Address address, uint8_t tid)
  {
    return m_txop->GetBaAgreementExists (address,tid);
  }
  virtual uint32_t GetNOutstandingPackets (Mac48Address address, uint8_t tid)
  {
    return m_txop->GetNOutstandingPacketsInBa (address, tid);
  }
  virtual uint32_t GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const
  {
    return m_txop->GetNRetryNeededPackets (recipient, tid);
  }
  virtual Ptr<MsduAggregator> GetMsduAggregator (void) const
  {
    return m_txop->GetMsduAggregator ();
  }
  virtual Mac48Address GetSrcAddressForAggregation (const WifiMacHeader &hdr)
  {
    return m_txop->MapSrcAddressForAggregation (hdr);
  }
  virtual Mac48Address GetDestAddressForAggregation (const WifiMacHeader &hdr)
  {
    return m_txop->MapDestAddressForAggregation (hdr);
  }

private:
  EdcaTxopN *m_txop;
};

NS_OBJECT_ENSURE_REGISTERED (EdcaTxopN);

TypeId
EdcaTxopN::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EdcaTxopN")
    .SetParent<ns3::Dcf> ()
    .SetGroupName ("Wifi")
    .AddConstructor<EdcaTxopN> ()
    .AddAttribute ("BlockAckThreshold",
                   "If number of packets in this queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&EdcaTxopN::SetBlockAckThreshold,
                                         &EdcaTxopN::GetBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block"
                   "ack frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&EdcaTxopN::SetBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Queue",
                   "The WifiMacQueue object",
                   PointerValue (),
                   MakePointerAccessor (&EdcaTxopN::GetEdcaQueue),
                   MakePointerChecker<WifiMacQueue> ())
    .AddTraceSource ("AccessQuest_record",
                   "The header of successfully transmitted packet"
                   "The header of successfully transmitted packet",
                   MakeTraceSourceAccessor (&EdcaTxopN::m_AccessQuest_record),
                   "ns3::TimeSeriesAdaptor::OutputTracedCallback")
    .AddAttribute ("NrOfTransmissionsDuringRAW", "Number of transmissions done during RAW period",
					UintegerValue(),
				    MakeUintegerAccessor(&EdcaTxopN::nrOfTransmissionsDuringRaw),
					MakeUintegerChecker<uint16_t> ())
	.AddTraceSource("Collision", "Fired when a collision occurred",
					MakeTraceSourceAccessor(&EdcaTxopN::m_collisionTrace), "ns3::EdcaTxopN::CollisionCallback")

	.AddTraceSource("TransmissionWillCrossRAWBoundary", "Fired when a transmission is held off because it won't fit inside the RAW slot",
					MakeTraceSourceAccessor(&EdcaTxopN::m_transmissionWillCrossRAWBoundary), "ns3::EdcaTxopN::TransmissionWillCrossRAWBoundaryCallback")
  ;
  return tid;
}

EdcaTxopN::EdcaTxopN ()
  : m_manager (0),
    m_currentPacket (0),
    m_aggregator (0),
    m_typeOfStation (STA),
    m_blockAckType (COMPRESSED_BLOCK_ACK),
    m_ampduExist (false)
{
  NS_LOG_FUNCTION (this);
  AccessAllowedIfRaw (true);
  m_crossSlotBoundaryAllowed = true;
  m_transmissionListener = new EdcaTxopN::TransmissionListener (this);
  m_blockAckListener = new EdcaTxopN::AggregationCapableTransmissionListener (this);
  m_dcf = new EdcaTxopN::Dcf (this);
  m_queue = CreateObject<WifiMacQueue> ();
  m_rng = new RealRandomStream ();
  m_qosBlockedDestinations = new QosBlockedDestinations ();
  m_baManager = new BlockAckManager ();
  m_baManager->SetQueue (m_queue);
  m_baManager->SetBlockAckType (m_blockAckType);
  m_baManager->SetBlockDestinationCallback (MakeCallback (&QosBlockedDestinations::Block, m_qosBlockedDestinations));
  m_baManager->SetUnblockDestinationCallback (MakeCallback (&QosBlockedDestinations::Unblock, m_qosBlockedDestinations));
  m_baManager->SetMaxPacketDelay (m_queue->GetMaxDelay ());
  m_baManager->SetTxOkCallback (MakeCallback (&EdcaTxopN::BaTxOk, this));
  m_baManager->SetTxFailedCallback (MakeCallback (&EdcaTxopN::BaTxFailed, this));
  m_rawDuration = Time::Max();
}

EdcaTxopN::~EdcaTxopN ()
{
  NS_LOG_FUNCTION (this);
}

void
EdcaTxopN::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  delete m_dcf;
  delete m_rng;
  delete m_qosBlockedDestinations;
  delete m_baManager;
  delete m_blockAckListener;
  m_transmissionListener = 0;
  m_dcf = 0;
  m_rng = 0;
  m_qosBlockedDestinations = 0;
  m_baManager = 0;
  m_blockAckListener = 0;
  m_txMiddle = 0;
  m_aggregator = 0;
}

bool
EdcaTxopN::GetBaAgreementExists (Mac48Address address, uint8_t tid)
{
  return m_baManager->ExistsAgreement (address, tid);
}

uint32_t
EdcaTxopN::GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid)
{
  return m_baManager->GetNBufferedPackets (address, tid);
}

uint32_t
EdcaTxopN::GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const
{
  return m_baManager->GetNRetryNeededPackets (recipient, tid);
}

void
EdcaTxopN::CompleteAmpduTransfer (Mac48Address recipient, uint8_t tid)
{
  m_baManager->CompleteAmpduExchange (recipient, tid);
}

void
EdcaTxopN::SetManager (DcfManager *manager)
{
  NS_LOG_FUNCTION (this << manager);
  m_manager = manager;
  m_manager->Add (m_dcf);
}

void
EdcaTxopN::SetTxOkCallback (TxOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkCallback = callback;
}

void
EdcaTxopN::SetTxFailedCallback (TxFailed callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txFailedCallback = callback;
}

void
EdcaTxopN::SetsleepCallback (sleepCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_sleepCallback = callback;
}

void
EdcaTxopN::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
  m_baManager->SetWifiRemoteStationManager (m_stationManager);
}

void
EdcaTxopN::SetTypeOfStation (enum TypeOfStation type)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (type));
  m_typeOfStation = type;
}

enum TypeOfStation
EdcaTxopN::GetTypeOfStation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_typeOfStation;
}

Ptr<WifiMacQueue >
EdcaTxopN::GetEdcaQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
EdcaTxopN::SetMinCw (uint32_t minCw)
{
  NS_LOG_FUNCTION (this << minCw);
  m_dcf->SetCwMin (minCw);
}

void
EdcaTxopN::SetMaxCw (uint32_t maxCw)
{
  NS_LOG_FUNCTION (this << maxCw);
  m_dcf->SetCwMax (maxCw);
}

void
EdcaTxopN::SetAifsn (uint32_t aifsn)
{
  NS_LOG_FUNCTION (this << aifsn);
  m_dcf->SetAifsn (aifsn);
}

uint32_t
EdcaTxopN::GetMinCw (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetCwMin ();
}

uint32_t
EdcaTxopN::GetMaxCw (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetCwMax ();
}

uint32_t
EdcaTxopN::GetAifsn (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetAifsn ();
}

void
EdcaTxopN::SetTxMiddle (MacTxMiddle *txMiddle)
{
  NS_LOG_FUNCTION (this << txMiddle);
  m_txMiddle = txMiddle;
}

Ptr<MacLow>
EdcaTxopN::Low (void)
{
  NS_LOG_FUNCTION (this);
  return m_low;
}

void
EdcaTxopN::SetLow (Ptr<MacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}

bool
EdcaTxopN::NeedsAccess (void) const
{
  NS_LOG_FUNCTION (this);
  return !m_queue->IsEmpty () || m_currentPacket != 0 || m_baManager->HasPackets ();
}

uint16_t EdcaTxopN::GetNextSequenceNumberfor (WifiMacHeader *hdr)
{
  return m_txMiddle->GetNextSequenceNumberfor (hdr);
}

uint16_t EdcaTxopN::PeekNextSequenceNumberfor (WifiMacHeader *hdr)
{
  return m_txMiddle->PeekNextSequenceNumberfor (hdr);
}

Ptr<const Packet>
EdcaTxopN::PeekNextRetransmitPacket (WifiMacHeader &header,Mac48Address recipient, uint8_t tid, Time *timestamp)
{
  return m_baManager->PeekNextPacket (header,recipient,tid, timestamp);
}

void
EdcaTxopN::RemoveRetransmitPacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber)
{
  m_baManager->RemovePacket (tid, recipient, seqnumber);
}

void
EdcaTxopN::SetaccessList (std::map<Mac48Address, bool> list)
{
    m_accessList = list;
}

void
EdcaTxopN::SetsleepList (std::map<Mac48Address, bool> list)
{
    m_sleepList = list;
}

void
EdcaTxopN::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  //Time remainingRawTime = m_rawDuration - (Simulator::Now() - m_rawStartedAt);
  if (!AccessIfRaw)
    {
        return;
    }
   //NS_LOG_UNCOND ("EdcaTxopN::NotifyAccessGranted, " << Simulator::Now ()<< ", " << m_low->GetAddress ());
    int newdata=99;
    m_AccessQuest_record (Simulator::Now ().GetMicroSeconds (), newdata);
  if (m_currentPacket == 0)
    {
      if (m_queue->IsEmpty () && !m_baManager->HasPackets ())
        {
          NS_LOG_DEBUG ("queue is empty");
          return;
        }
      if (m_baManager->HasBar (m_currentBar))
        {
          SendBlockAckRequest (m_currentBar);
          return;
        }
      /* check if packets need retransmission are stored in BlockAckManager */
      m_currentPacket = m_baManager->GetNextPacket (m_currentHdr);
      if (m_currentPacket == 0)
        {
          if (m_queue->PeekFirstAvailable (&m_currentHdr, m_currentPacketTimestamp, m_qosBlockedDestinations) == 0)
            {
              NS_LOG_DEBUG ("no available packets in the queue");
              return;
            }
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ()
              && m_blockAckThreshold > 0
              && !m_baManager->ExistsAgreement (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ())
              && SetupBlockAckIfNeeded ())
            {
              return;
            }

          //temporary, should be removed when ps-poll is suported
          //Ptr<const Packet> PacketTest = m_queue->PeekFirstAvailable (&m_currentHdr, m_currentPacketTimestamp, m_qosBlockedDestinations);
          
          //while (1)
           // {
              if (!(!m_sleepList.find(m_currentHdr.GetAddr1())->second || m_sleepList.size ()== 0)) // "m_sleepList.size ()== 0" for non-ap stations
              // no sleep 
                {
            	  return;
                }

             //}
          // TODO implement restrictions regarding non-Cross slot boundary
          m_currentPacket = m_queue->DequeueFirstAvailable (&m_currentHdr, m_currentPacketTimestamp, m_qosBlockedDestinations);
          NS_ASSERT (m_currentPacket != 0);

          uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
          m_currentHdr.SetSequenceNumber (sequence);
          m_currentHdr.SetFragmentNumber (0);
          m_currentHdr.SetNoMoreFragments ();
          m_currentHdr.SetNoRetry ();
          m_fragmentNumber = 0;
          NS_LOG_DEBUG ("dequeued size=" << m_currentPacket->GetSize () <<
                        ", to=" << m_currentHdr.GetAddr1 () <<
                        ", seq=" << m_currentHdr.GetSequenceControl ());
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ())
            {
              VerifyBlockAck ();
            }
        }
    }
  MacLowTransmissionParameters params;
  params.DisableOverrideDurationId ();
  if (m_currentHdr.GetAddr1 ().IsGroup () || m_currentHdr.IsPsPoll ())
    {
      params.DisableRts ();
      params.DisableAck ();
      params.DisableNextData ();

      /*Time txDuration = m_low->CalculateTransmissionTime(m_currentPacket, &m_currentHdr, params);
      NS_LOG_UNCOND ("m_crossSlotBoundaryAllowed " << m_crossSlotBoundaryAllowed);
     	if (!m_crossSlotBoundaryAllowed && txDuration > remainingRawTime) { // don't transmit if it can't be done inside RAW window, the ACK won't be received anyway
     		NS_LOG_DEBUG("TX will take longer (" << txDuration << ") than the remaining RAW time (" << remainingRawTime << "), not transmitting");
     		m_transmissionWillCrossRAWBoundary(txDuration,remainingRawTime);
     		return;
     	}
     	else {
     		NS_LOG_DEBUG("TX can be done (" << txDuration << ") before the RAW expires (" << remainingRawTime << " remaining)");
     	}
*/
      m_low->StartTransmission (m_currentPacket,
                                &m_currentHdr,
                                params,
                                m_transmissionListener);
      nrOfTransmissionsDuringRaw++;
      NS_LOG_DEBUG ("tx broadcast");
    }
  else if (m_currentHdr.GetType () == WIFI_MAC_CTL_BACKREQ)
    {
      SendBlockAckRequest (m_currentBar);
    }
  else
    {
      if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
        {
          params.DisableAck ();
        }
      else
        {
          params.EnableAck ();
        }
      if (NeedFragmentation () && ((m_currentHdr.IsQosData ()
                                    && !m_currentHdr.IsQosAmsdu ())
                                   ||
                                   (m_currentHdr.IsData ()
                                    && !m_currentHdr.IsQosData () && m_currentHdr.IsQosAmsdu ()))
          && (m_blockAckThreshold == 0
              || m_blockAckType == BASIC_BLOCK_ACK))
        {
          //With COMPRESSED_BLOCK_ACK fragmentation must be avoided.
          params.DisableRts ();
          WifiMacHeader hdr;
          Ptr<Packet> fragment = GetFragmentPacket (&hdr);
          if (IsLastFragment ())
            {
              NS_LOG_DEBUG ("fragmenting last fragment size=" << fragment->GetSize ());
              params.DisableNextData ();
            }
          else
            {
              NS_LOG_DEBUG ("fragmenting size=" << fragment->GetSize ());
              params.EnableNextData (GetNextFragmentSize ());
            }
          /*Time txDuration = m_low->CalculateTransmissionTime(fragment,
                			&hdr, params);
			if (!m_crossSlotBoundaryAllowed && txDuration > remainingRawTime) { // don't transmit if it can't be done inside RAW window, the ACK won't be received anyway
				NS_LOG_DEBUG("TX will take longer than the remaining RAW time, not transmitting");
				m_transmissionWillCrossRAWBoundary(txDuration,remainingRawTime);
				return;
			}
			else
                        {*/
	      		//NS_LOG_DEBUG("TX can be done (" << txDuration << ") before the RAW expires (" << remainingRawTime << " remaining)");
                        //don't go to sleep after you send the packet, since it has to receive the ack
                        if(!m_low->GetPhy()->IsStateSleep())
                           {
                              if (!m_sleepCallback.IsNull())
                              {
                                  m_sleepCallback (false);
                                  // NS_LOG_UNCOND ("**--not go to sleep, start transmit " << m_low->GetAddress() << "size "<< m_currentPacket->GetSize()  );
                              }
                              //temporary double check in order to not start the transmission if not my slot or shared slot
                            if(!m_low->GetPhy()->IsStateSleep())
                           {  
                              m_low->StartTransmission (fragment, &m_currentHdr,
                                                   params, m_transmissionListener);
                              nrOfTransmissionsDuringRaw++;
                            }
                          }
                      //}
                        //m_low->StartTransmission (fragment, &hdr, params, m_transmissionListener);     
      }
      
      else
        {
          WifiMacHeader peekedHdr;
          Time tstamp;
          if (m_currentHdr.IsQosData ()
              && m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                               WifiMacHeader::ADDR1, m_currentHdr.GetAddr1 (), &tstamp)
              && !m_currentHdr.GetAddr1 ().IsBroadcast ()
              && m_aggregator != 0 && !m_currentHdr.IsRetry ())
            {
              /* here is performed aggregation */
              Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
              m_aggregator->Aggregate (m_currentPacket, currentAggregatedPacket,
                                       MapSrcAddressForAggregation (peekedHdr),
                                       MapDestAddressForAggregation (peekedHdr));
              bool aggregated = false;
              bool isAmsdu = false;
              Ptr<const Packet> peekedPacket = m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                                             WifiMacHeader::ADDR1,
                                                                             m_currentHdr.GetAddr1 (), &tstamp);
              while (peekedPacket != 0)
                {
                  aggregated = m_aggregator->Aggregate (peekedPacket, currentAggregatedPacket,
                                                        MapSrcAddressForAggregation (peekedHdr),
                                                        MapDestAddressForAggregation (peekedHdr));
                  if (aggregated)
                    {
                      isAmsdu = true;
                      m_queue->Remove (peekedPacket);
                    }
                  else
                    {
                      break;
                    }
                  peekedPacket = m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                               WifiMacHeader::ADDR1, m_currentHdr.GetAddr1 (), &tstamp);
                }
              if (isAmsdu)
                {
                  m_currentHdr.SetQosAmsdu ();
                  m_currentHdr.SetAddr3 (m_low->GetBssid ());
                  m_currentPacket = currentAggregatedPacket;
                  currentAggregatedPacket = 0;
                  NS_LOG_DEBUG ("tx unicast A-MSDU");
                }
            }
          if (NeedRts ())
            {
              params.EnableRts ();
              NS_LOG_DEBUG ("tx unicast rts");
            }
          else
            {
              params.DisableRts ();
              NS_LOG_DEBUG ("tx unicast");
            }
          params.DisableNextData ();
          
          /*Time txDuration = m_low->CalculateTransmissionTime(m_currentPacket,
                         			&m_currentHdr, params);
	      if (!m_crossSlotBoundaryAllowed && txDuration > remainingRawTime) { // don't transmit if it can't be done inside RAW window, the ACK won't be received anyway
			NS_LOG_DEBUG("TX will take longer (" << txDuration << ") than the remaining RAW time (" << remainingRawTime << "), not transmitting");
			m_transmissionWillCrossRAWBoundary(txDuration,remainingRawTime);
			return;
		  }
	      else
              {
	      	NS_LOG_DEBUG("TX can be done (" << txDuration << ") before the RAW expires (" << remainingRawTime << " remaining)");
*/
                //m_low->StartTransmission (m_currentPacket, &m_currentHdr, params, m_transmissionListener);

                //don't go to sleep after you send the packet, since it has to receive the ack
                if(!m_low->GetPhy()->IsStateSleep())
                   {
                      if (!m_sleepCallback.IsNull())
                      {
                          m_sleepCallback (false);
                          // NS_LOG_UNCOND ("**--not go to sleep, start transmit " << m_low->GetAddress() << "size "<< m_currentPacket->GetSize()  );
                      }
                    //temporary double check in order to not start the transmission if not my slot or shared slot
                        if(!m_low->GetPhy()->IsStateSleep())
                        {
                        m_low->StartTransmission (m_currentPacket, &m_currentHdr, params, m_transmissionListener);
                        nrOfTransmissionsDuringRaw++;
                        }
                  }   
              //}
          
          if (!GetAmpduExist ())
            {
              CompleteTx ();
            }
        }
    }
}

void EdcaTxopN::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  NotifyCollision ();
}

void
EdcaTxopN::NotifyCollision (void)
{
  NS_LOG_FUNCTION (this);
  auto cw = m_rng->GetNext (0, m_dcf->GetCw ());
  m_collisionTrace(cw);

  m_dcf->StartBackoffNow (cw);
  RestartAccessIfNeeded ();
}

void
EdcaTxopN::GotCts (double snr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  NS_LOG_DEBUG ("got cts");
}

void
EdcaTxopN::MissedCts (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed cts");
  if (!NeedRtsRetransmission ())
    {
      NS_LOG_DEBUG ("Cts Fail");
      m_stationManager->ReportFinalRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      if (GetAmpduExist ())
        {
          m_low->FlushAggregateQueue ();

          NS_LOG_DEBUG ("Transmit Block Ack Request");
          CtrlBAckRequestHeader reqHdr;
          reqHdr.SetType (COMPRESSED_BLOCK_ACK);
          uint8_t tid = m_currentHdr.GetQosTid ();
          reqHdr.SetStartingSequence (m_txMiddle->PeekNextSequenceNumberfor (&m_currentHdr));
          reqHdr.SetTidInfo (tid);
          reqHdr.SetHtImmediateAck (true);
          Ptr<Packet> bar = Create<Packet> ();
          bar->AddHeader (reqHdr);
          Bar request (bar, m_currentHdr.GetAddr1 (), tid, reqHdr.MustSendHtImmediateAck ());
          m_currentBar = request;
          WifiMacHeader hdr;
          hdr.SetType (WIFI_MAC_CTL_BACKREQ);
          hdr.SetAddr1 (request.recipient);
          hdr.SetAddr2 (m_low->GetAddress ());
          hdr.SetAddr3 (m_low->GetBssid ());
          hdr.SetDsNotTo ();
          hdr.SetDsNotFrom ();
          hdr.SetNoRetry ();
          hdr.SetNoMoreFragments ();
          m_currentPacket = request.bar;
          m_currentHdr = hdr;
        }
      else
        {
          m_currentPacket = 0;
        }
      //to reset the dcf.
      m_dcf->ResetCw ();
    }
  else
    {
      m_dcf->UpdateFailedCw ();
    }
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
EdcaTxopN::NotifyChannelSwitching (void)
{
  NS_LOG_FUNCTION (this);
  m_queue->Flush ();
  m_currentPacket = 0;
}

void
EdcaTxopN::NotifySleep (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket != 0)
    {
      m_queue->PushFront (m_currentPacket, m_currentHdr);
      m_currentPacket = 0;
    }
}

void EdcaTxopN::NotifyOff (void)
{
  NS_LOG_FUNCTION (this);
  m_queue->Flush ();
  m_currentPacket = 0;
}

void
EdcaTxopN::NotifyWakeUp (void)
{
  NS_LOG_FUNCTION (this);
  RestartAccessIfNeeded ();
}

void EdcaTxopN::NotifyOn (void)
{
   NS_LOG_FUNCTION (this);
   StartAccessIfNeeded ();
}

void
EdcaTxopN::WakeUp (void)
{
    m_low->GetPhy()->ResumeFromSleep();
}

void
EdcaTxopN::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  WifiMacTrailer fcs;
  uint32_t fullPacketSize = hdr.GetSerializedSize () + packet->GetSize () + fcs.GetSerializedSize ();
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr,
                                     packet, fullPacketSize);
  m_queue->Enqueue (packet, hdr);
  StartAccessIfNeeded ();
}

void
EdcaTxopN::GotAck (double snr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  if (!NeedFragmentation ()
      || IsLastFragment ()
      || m_currentHdr.IsQosAmsdu ())
    {
      NS_LOG_DEBUG ("got ack. tx done.");
      int newdata=77;
      m_AccessQuest_record (Simulator::Now ().GetMicroSeconds (), newdata);
      if (!m_txOkCallback.IsNull ())
        {
          m_txOkCallback (m_currentHdr);
        }

      if (m_currentHdr.IsAction ())
        {
          WifiActionHeader actionHdr;
          Ptr<Packet> p = m_currentPacket->Copy ();
          p->RemoveHeader (actionHdr);
          if (actionHdr.GetCategory () == WifiActionHeader::BLOCK_ACK
              && actionHdr.GetAction ().blockAck == WifiActionHeader::BLOCK_ACK_DELBA)
            {
              MgtDelBaHeader delBa;
              p->PeekHeader (delBa);
              if (delBa.IsByOriginator ())
                {
                  m_baManager->TearDownBlockAck (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                }
              else
                {
                  m_low->DestroyBlockAckAgreement (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                }
            }
        }
      m_currentPacket = 0;

      m_dcf->ResetCw ();
      m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
      RestartAccessIfNeeded ();
    }
  else
    {
      NS_LOG_DEBUG ("got ack. tx not done, size=" << m_currentPacket->GetSize ());
    }
  //after receiving ack
    if (!m_sleepCallback.IsNull())
    {
        //NS_LOG_UNCOND ("the queue is empty");
        //callback SleepIfQueueIsEmpty
        //NS_LOG_UNCOND ("go to sleep true" << m_low->GetAddress());
        m_sleepCallback (true);        
    }
}

void
EdcaTxopN::MissedAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed ack");
  if (!NeedDataRetransmission ())
    {
      NS_LOG_DEBUG ("Ack Fail");
      //after receiving ack
    if (!m_sleepCallback.IsNull())
    {
        //NS_LOG_UNCOND ("the queue is empty");
        //callback SleepIfQueueIsEmpty
        //NS_LOG_UNCOND ("go to sleep true" << m_low->GetAddress());
        m_sleepCallback (true);        
    }
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      if (!GetAmpduExist ())
        {
          //to reset the dcf.
          m_currentPacket = 0;
        }
      else
        {
          NS_LOG_DEBUG ("Transmit Block Ack Request");
          CtrlBAckRequestHeader reqHdr;
          reqHdr.SetType (COMPRESSED_BLOCK_ACK);
          uint8_t tid = m_currentHdr.GetQosTid ();
          reqHdr.SetStartingSequence (m_txMiddle->PeekNextSequenceNumberfor (&m_currentHdr));
          reqHdr.SetTidInfo (tid);
          reqHdr.SetHtImmediateAck (true);
          Ptr<Packet> bar = Create<Packet> ();
          bar->AddHeader (reqHdr);
          Bar request (bar, m_currentHdr.GetAddr1 (), tid, reqHdr.MustSendHtImmediateAck ());
          m_currentBar = request;
          WifiMacHeader hdr;
          hdr.SetType (WIFI_MAC_CTL_BACKREQ);
          hdr.SetAddr1 (request.recipient);
          hdr.SetAddr2 (m_low->GetAddress ());
          hdr.SetAddr3 (m_low->GetBssid ());
          hdr.SetDsNotTo ();
          hdr.SetDsNotFrom ();
          hdr.SetNoRetry ();
          hdr.SetNoMoreFragments ();
          m_currentPacket = request.bar;
          m_currentHdr = hdr;
        }
      m_dcf->ResetCw ();
    }
  else
    {
      NS_LOG_DEBUG ("Retransmit");
      if (!m_sleepCallback.IsNull())
        {
          //If retransmit, shouldn't go to sleep if my slot
          //but should go to sleep if not my slot
          //NS_LOG_UNCOND ("go to sleep false retransmit " << m_low->GetAddress());
          
          m_sleepCallback (false);
        }
      m_currentHdr.SetRetry ();
      m_dcf->UpdateFailedCw ();
    }
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
EdcaTxopN::MissedBlockAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed block ack");
  if (NeedBarRetransmission ())
    {
      if (!GetAmpduExist ())
        {
          //should i report this to station addressed by ADDR1?
          NS_LOG_DEBUG ("Retransmit block ack request");
          m_currentHdr.SetRetry ();
        }
      else
        {
          //standard says when loosing a BlockAck originator may send a BAR page 139
          NS_LOG_DEBUG ("Transmit Block Ack Request");
          CtrlBAckRequestHeader reqHdr;
          reqHdr.SetType (COMPRESSED_BLOCK_ACK);
          uint8_t tid = 0;
          if (m_currentHdr.IsQosData ())
            {
              tid = m_currentHdr.GetQosTid ();
              reqHdr.SetStartingSequence (m_currentHdr.GetSequenceNumber ());
            }
          else if (m_currentHdr.IsBlockAckReq ())
            {
              CtrlBAckRequestHeader baReqHdr;
              m_currentPacket->PeekHeader (baReqHdr);
              tid = baReqHdr.GetTidInfo ();
              reqHdr.SetStartingSequence (baReqHdr.GetStartingSequence ());
            }
          else if (m_currentHdr.IsBlockAck ())
            {
              CtrlBAckResponseHeader baRespHdr;
              m_currentPacket->PeekHeader (baRespHdr);
              tid = baRespHdr.GetTidInfo ();
              reqHdr.SetStartingSequence (m_currentHdr.GetSequenceNumber ());
            }
          reqHdr.SetTidInfo (tid);
          reqHdr.SetHtImmediateAck (true);
          Ptr<Packet> bar = Create<Packet> ();
          bar->AddHeader (reqHdr);
          Bar request (bar, m_currentHdr.GetAddr1 (), tid, reqHdr.MustSendHtImmediateAck ());
          m_currentBar = request;
          WifiMacHeader hdr;
          hdr.SetType (WIFI_MAC_CTL_BACKREQ);
          hdr.SetAddr1 (request.recipient);
          hdr.SetAddr2 (m_low->GetAddress ());
          hdr.SetAddr3 (m_low->GetBssid ());
          hdr.SetDsNotTo ();
          hdr.SetDsNotFrom ();
          hdr.SetNoRetry ();
          hdr.SetNoMoreFragments ();

          m_currentPacket = request.bar;
          m_currentHdr = hdr;
        }
      m_dcf->UpdateFailedCw ();
    }
  else
    {
      NS_LOG_DEBUG ("Block Ack Request Fail");
      //to reset the dcf.
      m_currentPacket = 0;
      m_dcf->ResetCw ();
    }
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

Ptr<MsduAggregator>
EdcaTxopN::GetMsduAggregator (void) const
{
  return m_aggregator;
}

void
EdcaTxopN::AccessAllowedIfRaw (bool allowed)
{
  AccessIfRaw = allowed;
    /*if (Simulator::Now ().GetMicroSeconds () > 50000000)
    {
         NS_LOG_UNCOND ("EdcaTxopN::NotifyAccessGranted, " << Simulator::Now () << ", " << allowed << ", " << m_low->GetAddress ());
    }*/
    
}

void
EdcaTxopN::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty () || m_baManager->HasPackets ())
      && !m_dcf->IsAccessRequested ()
      && AccessIfRaw)
    {
      m_manager->RequestAccess (m_dcf);
        int newdata=10;
        m_AccessQuest_record (Simulator::Now ().GetMicroSeconds (), newdata);
    }
}

void
EdcaTxopN::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && (!m_queue->IsEmpty () || m_baManager->HasPackets ())
      && !m_dcf->IsAccessRequested ()
      && AccessIfRaw)    // always TRUE outside RAW
    {
        int newdata=20;
        m_AccessQuest_record (Simulator::Now ().GetMicroSeconds (), newdata);
        m_manager->RequestAccess (m_dcf);
    }
}

void
EdcaTxopN::StartAccessIfNeededRaw (void)
{
    NS_LOG_FUNCTION (this);
    if ((!m_queue->IsEmpty () || m_baManager->HasPackets () || m_currentPacket != 0)
        && !m_dcf->IsAccessRequested ()
        && AccessIfRaw)    // always TRUE outside RAW
    {
        m_manager->RequestAccess (m_dcf);
    }
}

void
EdcaTxopN::RawStart (Time duration, bool crossSlotBoundaryAllowed)
{
  NS_LOG_FUNCTION (this);
  this->m_rawDuration = duration;
  this->m_crossSlotBoundaryAllowed = crossSlotBoundaryAllowed;
  nrOfTransmissionsDuringRaw = 0;
  //m_rawStartedAt = Simulator::Now();

  //NS_LOG_DEBUG("RAW START, duration is " << duration);

  int newdata=66;
  m_AccessQuest_record (Simulator::Now ().GetMicroSeconds (), newdata);

  m_dcf->RawStart ();
  m_stationManager->RawStart ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
	if ((!m_queue->IsEmpty () || m_baManager->HasPackets ()) && AccessIfRaw)
	  {
		int newdata=30;
		m_AccessQuest_record (Simulator::Now ().GetMicroSeconds (), newdata);
	  }
  StartAccessIfNeededRaw (); //access could start even no packet
  //RestartAccessIfNeeded();

}

void
EdcaTxopN::OutsideRawStart ()
{
  NS_LOG_FUNCTION (this);

  AccessAllowedIfRaw (true); // TODO this is different accross versions
  m_dcf-> OutsideRawStart ();
  m_stationManager->OutsideRawStart ();
  m_dcf->StartBackoffNow (m_dcf->GetBackoffSlots());
  StartAccessIfNeededRaw ();

}

bool
EdcaTxopN::NeedRts (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedRts (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                    m_currentPacket);
}

bool
EdcaTxopN::NeedRtsRetransmission (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedRtsRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                  m_currentPacket);
}

bool
EdcaTxopN::NeedDataRetransmission (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedDataRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket);
}

bool
EdcaTxopN::NeedBarRetransmission (void)
{
  uint8_t tid = 0;
  uint16_t seqNumber = 0;
  if (m_currentHdr.IsQosData ())
    {
      tid = m_currentHdr.GetQosTid ();
      seqNumber = m_currentHdr.GetSequenceNumber ();
    }
  else if (m_currentHdr.IsBlockAckReq ())
    {
      CtrlBAckRequestHeader baReqHdr;
      m_currentPacket->PeekHeader (baReqHdr);
      tid = baReqHdr.GetTidInfo ();
      seqNumber = baReqHdr.GetStartingSequence ();
    }
  else if (m_currentHdr.IsBlockAck ())
    {
      CtrlBAckResponseHeader baRespHdr;
      m_currentPacket->PeekHeader (baRespHdr);
      tid = baRespHdr.GetTidInfo ();
      seqNumber = m_currentHdr.GetSequenceNumber ();
    }
  return m_baManager->NeedBarRetransmission (tid, seqNumber, m_currentHdr.GetAddr1 ());
}

void
EdcaTxopN::NextFragment (void)
{
  NS_LOG_FUNCTION (this);
  m_fragmentNumber++;
}

void
EdcaTxopN::StartNext (void)
{
  NS_LOG_FUNCTION (this);
  if(!AccessIfRaw)
	return;
  NS_LOG_DEBUG ("start next packet fragment");
  /* this callback is used only for fragments. */
  NextFragment ();
  WifiMacHeader hdr;
  Ptr<Packet> fragment = GetFragmentPacket (&hdr);
  MacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.DisableOverrideDurationId ();
  if (IsLastFragment ())
    {
      params.DisableNextData ();
    }
  else
    {
      params.EnableNextData (GetNextFragmentSize ());
    }
  Low ()->StartTransmission (fragment, &hdr, params, m_transmissionListener);
  nrOfTransmissionsDuringRaw++;
}

void
EdcaTxopN::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("transmission cancelled");
}

void
EdcaTxopN::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  StartAccessIfNeeded ();
}

bool
EdcaTxopN::NeedFragmentation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket);
}

uint32_t
EdcaTxopN::GetFragmentSize (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber);
}

uint32_t
EdcaTxopN::GetNextFragmentSize (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber + 1);
}

uint32_t
EdcaTxopN::GetFragmentOffset (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentOffset (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket, m_fragmentNumber);
}


bool
EdcaTxopN::IsLastFragment (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->IsLastFragment (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                           m_currentPacket, m_fragmentNumber);
}

Ptr<Packet>
EdcaTxopN::GetFragmentPacket (WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  *hdr = m_currentHdr;
  hdr->SetFragmentNumber (m_fragmentNumber);
  uint32_t startOffset = GetFragmentOffset ();
  Ptr<Packet> fragment;
  if (IsLastFragment ())
    {
      hdr->SetNoMoreFragments ();
    }
  else
    {
      hdr->SetMoreFragments ();
    }
  fragment = m_currentPacket->CreateFragment (startOffset,
                                              GetFragmentSize ());
  return fragment;
}

void
EdcaTxopN::SetAccessCategory (enum AcIndex ac)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (ac));
  m_ac = ac;
}

Mac48Address
EdcaTxopN::MapSrcAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  Mac48Address retval;
  if (m_typeOfStation == STA || m_typeOfStation == ADHOC_STA)
    {
      retval = hdr.GetAddr2 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

Mac48Address
EdcaTxopN::MapDestAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  Mac48Address retval;
  if (m_typeOfStation == AP || m_typeOfStation == ADHOC_STA)
    {
      retval = hdr.GetAddr1 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

void
EdcaTxopN::SetMsduAggregator (Ptr<MsduAggregator> aggr)
{
  NS_LOG_FUNCTION (this << aggr);
  m_aggregator = aggr;
}

void
EdcaTxopN::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  WifiMacTrailer fcs;
  uint32_t fullPacketSize = hdr.GetSerializedSize () + packet->GetSize () + fcs.GetSerializedSize ();
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr,
                                     packet, fullPacketSize);
  m_queue->PushFront (packet, hdr);
  StartAccessIfNeeded ();
}

void
EdcaTxopN::GotAddBaResponse (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << respHdr << recipient);
  NS_LOG_DEBUG ("received ADDBA response from " << recipient);
  uint8_t tid = respHdr->GetTid ();
  if (m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::PENDING))
    {
      if (respHdr->GetStatusCode ().IsSuccess ())
        {
          NS_LOG_DEBUG ("block ack agreement established with " << recipient);
          m_baManager->UpdateAgreement (respHdr, recipient);
        }
      else
        {
          NS_LOG_DEBUG ("discard ADDBA response" << recipient);
          m_baManager->NotifyAgreementUnsuccessful (recipient, tid);
        }
    }
  RestartAccessIfNeeded ();
}

void
EdcaTxopN::GotDelBaFrame (const MgtDelBaHeader *delBaHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << delBaHdr << recipient);
  NS_LOG_DEBUG ("received DELBA frame from=" << recipient);
  m_baManager->TearDownBlockAck (recipient, delBaHdr->GetTid ());
}

void
EdcaTxopN::GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << blockAck << recipient);
  NS_LOG_DEBUG ("got block ack from=" << recipient);
  m_baManager->NotifyGotBlockAck (blockAck, recipient, txMode);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentHdr);
    }
  m_currentPacket = 0;
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
EdcaTxopN::VerifyBlockAck (void)
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();
  uint16_t sequence = m_currentHdr.GetSequenceNumber ();
  if (m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::INACTIVE))
    {
      m_baManager->SwitchToBlockAckIfNeeded (recipient, tid, sequence);
    }
  if (m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
    {
      m_currentHdr.SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
    }
}

bool EdcaTxopN::GetAmpduExist (void)
{
  return m_ampduExist;
}

void EdcaTxopN::SetAmpduExist (bool ampdu)
{
  m_ampduExist = ampdu;
}

void
EdcaTxopN::CompleteTx (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
    {
      if (!m_currentHdr.IsRetry ())
        {
          m_baManager->StorePacket (m_currentPacket, m_currentHdr, m_currentPacketTimestamp);
        }
      m_baManager->NotifyMpduTransmission (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid (),
                                           m_txMiddle->GetNextSeqNumberByTidAndAddress (m_currentHdr.GetQosTid (),
                                                                                        m_currentHdr.GetAddr1 ()), WifiMacHeader::BLOCK_ACK);
    }
}

void
EdcaTxopN::CompleteMpduTx (Ptr<const Packet> packet, WifiMacHeader hdr, Time tstamp)
{
  m_baManager->StorePacket (packet, hdr, tstamp);
  m_baManager->NotifyMpduTransmission (hdr.GetAddr1 (), hdr.GetQosTid (),
                                       m_txMiddle->GetNextSeqNumberByTidAndAddress (hdr.GetQosTid (),
                                                                                    hdr.GetAddr1 ()), WifiMacHeader::NORMAL_ACK);
}

bool
EdcaTxopN::SetupBlockAckIfNeeded ()
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();

  uint32_t packets = m_queue->GetNPacketsByTidAndAddress (tid, WifiMacHeader::ADDR1, recipient);

  if (packets >= m_blockAckThreshold)
    {
      /* Block ack setup */
      uint16_t startingSequence = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
      SendAddBaRequest (recipient, tid, startingSequence, m_blockAckInactivityTimeout, true);
      return true;
    }
  return false;
}

void
EdcaTxopN::SendBlockAckRequest (const struct Bar &bar)
{
  NS_LOG_FUNCTION (this << &bar);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKREQ);
  hdr.SetAddr1 (bar.recipient);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetBssid ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  m_currentPacket = bar.bar;
  m_currentHdr = hdr;

  MacLowTransmissionParameters params;
  params.DisableRts ();
  params.DisableNextData ();
  params.DisableOverrideDurationId ();
  if (bar.immediate)
    {
      if (m_blockAckType == BASIC_BLOCK_ACK)
        {
          params.EnableBasicBlockAck ();
        }
      else if (m_blockAckType == COMPRESSED_BLOCK_ACK)
        {
          params.EnableCompressedBlockAck ();
        }
      else if (m_blockAckType == MULTI_TID_BLOCK_ACK)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported");
        }
    }
  else
    {
      //Delayed block ack
      params.EnableAck ();
    }
  Time txDuration = Low()->CalculateTransmissionTime(m_currentPacket, &m_currentHdr, params);
  /*Time remainingRawTime =  m_rawDuration -  (Simulator::Now() - m_rawStartedAt);
  // Non-cross-slot boundary should be used with bidirectional
  // Don't transmit if it can't be done inside RAW window, the ACK won't be received anyway
  if(!m_crossSlotBoundaryAllowed && txDuration > remainingRawTime) {
      NS_LOG_DEBUG("TX will take longer (" << txDuration << ") than the remaining RAW time (" << remainingRawTime << "), not transmitting");
      m_transmissionWillCrossRAWBoundary(txDuration,remainingRawTime);
	  return;
  }*/
  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params, m_transmissionListener);
  nrOfTransmissionsDuringRaw++;
}

void
EdcaTxopN::CompleteConfig (void)
{
  NS_LOG_FUNCTION (this);
  m_baManager->SetTxMiddle (m_txMiddle);
  m_low->RegisterBlockAckListenerForAc (m_ac, m_blockAckListener);
  m_baManager->SetBlockAckInactivityCallback (MakeCallback (&EdcaTxopN::SendDelbaFrame, this));
}

void
EdcaTxopN::SetBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (threshold));
  m_blockAckThreshold = threshold;
  m_baManager->SetBlockAckThreshold (threshold);
}

void
EdcaTxopN::SetBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_blockAckInactivityTimeout = timeout;
}

uint8_t
EdcaTxopN::GetBlockAckThreshold (void) const
{
  NS_LOG_FUNCTION (this);
  return m_blockAckThreshold;
}

void
EdcaTxopN::SendAddBaRequest (Mac48Address dest, uint8_t tid, uint16_t startSeq,
                             uint16_t timeout, bool immediateBAck)
{
  NS_LOG_FUNCTION (this << dest << static_cast<uint32_t> (tid) << startSeq << timeout << immediateBAck);
  NS_LOG_DEBUG ("sent ADDBA request to " << dest);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (dest);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  /*Setting ADDBARequest header*/
  MgtAddBaRequestHeader reqHdr;
  reqHdr.SetAmsduSupport (true);
  if (immediateBAck)
    {
      reqHdr.SetImmediateBlockAck ();
    }
  else
    {
      reqHdr.SetDelayedBlockAck ();
    }
  reqHdr.SetTid (tid);
  /* For now we don't use buffer size field in the ADDBA request frame. The recipient
   * will choose how many packets it can receive under block ack.
   */
  reqHdr.SetBufferSize (0);
  reqHdr.SetTimeout (timeout);
  reqHdr.SetStartingSequence (startSeq);

  m_baManager->CreateAgreement (&reqHdr, dest);

  packet->AddHeader (reqHdr);
  packet->AddHeader (actionHdr);

  m_currentPacket = packet;
  m_currentHdr = hdr;

  uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
  m_currentHdr.SetSequenceNumber (sequence);
  m_currentHdr.SetFragmentNumber (0);
  m_currentHdr.SetNoMoreFragments ();
  m_currentHdr.SetNoRetry ();

  MacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.DisableNextData ();
  params.DisableOverrideDurationId ();

	/*Time txDuration = m_low->CalculateTransmissionTime(m_currentPacket,
			&m_currentHdr, params);
	Time remainingRawTime = m_rawDuration - (Simulator::Now() - m_rawStartedAt);
	if (!m_crossSlotBoundaryAllowed && txDuration > remainingRawTime) { // don't transmit if it can't be done inside RAW window, the ACK won't be received anyway
		NS_LOG_DEBUG("TX will take longer (" << txDuration << ") than the remaining RAW time (" << remainingRawTime << "), not transmitting");
		m_transmissionWillCrossRAWBoundary(txDuration,remainingRawTime);
		return;
	}
	else
		NS_LOG_DEBUG("TX can be done (" << txDuration << ") before the RAW expires (" << remainingRawTime << " remaining)");
		*/
  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params,
                            m_transmissionListener);
  nrOfTransmissionsDuringRaw++;
}

void
EdcaTxopN::SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator)
{
  NS_LOG_FUNCTION (this << addr << static_cast<uint32_t> (tid) << byOriginator);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (addr);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  MgtDelBaHeader delbaHdr;
  delbaHdr.SetTid (tid);
  if (byOriginator)
    {
      delbaHdr.SetByOriginator ();
    }
  else
    {
      delbaHdr.SetByRecipient ();
    }

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_DELBA;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (delbaHdr);
  packet->AddHeader (actionHdr);

  PushFront (packet, hdr);
}

int64_t
EdcaTxopN::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rng->AssignStreams (stream);
  return 1;
}

void
EdcaTxopN::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_dcf->ResetCw ();
  m_dcf->StartBackoffNow (m_rng->GetNext (0, m_dcf->GetCw ()));
  ns3::Dcf::DoInitialize ();
}

void
EdcaTxopN::BaTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentHdr);
    }
}

void
EdcaTxopN::BaTxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txFailedCallback.IsNull ())
    {
      m_txFailedCallback (m_currentHdr);
    }
}

} //namespace ns3
