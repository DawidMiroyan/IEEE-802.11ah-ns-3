/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "wifi-phy-state-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhyStateHelper");

NS_OBJECT_ENSURE_REGISTERED (WifiPhyStateHelper);

TypeId
WifiPhyStateHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyStateHelper")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiPhyStateHelper> ()
    .AddTraceSource ("State",
                     "The state of the PHY layer",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_stateLogger),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
    .AddTraceSource ("RxOk",
                     "A packet has been received successfully.",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_rxOkTrace),
                     "ns3::WifiPhyStateHelper::RxOkTracedCallback")
    .AddTraceSource ("RxError",
                     "A packet has been received unsuccessfully.",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_rxErrorTrace),
                     "ns3::WifiPhyStateHelper::RxErrorTracedCallback")
    .AddTraceSource ("Tx", "Packet transmission is starting.",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_txTrace),
                     "ns3::WifiPhyStateHelper::TxTracedCallback")
    .AddTraceSource ("RxStart",
                     "The time Rx start"
                     "The duration of Rx",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_RxStart),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
    .AddTraceSource ("RxEndOk",
                     "The time Rx start"
                     "The duration of Rx",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_RxEndOk),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
    .AddTraceSource ("RxEndError",
                     "The time Rx start"
                     "The duration of Rx",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_RxEndError),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
    /*.AddTraceSource ("RxingTrace",
                     "The time Rx be true"
                     "The duration of Rx",
                     MakeTraceSourceAccessor (&DcfManager::m_RxingTrace),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")*/
    .AddTraceSource ("TxStart",
                     "The time Tx start"
                     "The duration of Tx",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_TxStart),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
    .AddTraceSource ("CcaBusyStart",
                     "The time CcaBusyS start"
                     "The duration CcaBusyS",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_CcaBusyStart),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
  ;
  return tid;
}

WifiPhyStateHelper::WifiPhyStateHelper ()
  : m_rxing (false),
    m_sleeping (false),
    m_isOff (false),
    m_endTx (Seconds (0)),
    m_endRx (Seconds (0)),
    m_endCcaBusy (Seconds (0)),
    m_endSwitching (Seconds (0)),
    m_endSleep(Seconds (0)),
    m_endOff(Seconds (0)),
    m_startTx (Seconds (0)),
    m_startRx (Seconds (0)),
    m_startCcaBusy (Seconds (0)),
    m_startSwitching (Seconds (0)),
    m_startSleep (Seconds (0)),
    m_startOff (Seconds (0)),
    m_previousStateChangeTime (Seconds (0))
{
  NS_LOG_FUNCTION (this);
}

void
WifiPhyStateHelper::SetReceiveOkCallback (WifiPhy::RxOkCallback callback)
{
  m_rxOkCallback = callback;
}

void
WifiPhyStateHelper::SetReceiveErrorCallback (WifiPhy::RxErrorCallback callback)
{
  m_rxErrorCallback = callback;
}

void
WifiPhyStateHelper::RegisterListener (WifiPhyListener *listener)
{
  m_listeners.push_back (listener);
}

void
WifiPhyStateHelper::UnregisterListener (WifiPhyListener *listener)
{
  ListenersI i = find (m_listeners.begin (), m_listeners.end (), listener);
  if (i != m_listeners.end ())
    {
      m_listeners.erase (i);
    }
}

bool
WifiPhyStateHelper::IsStateIdle (void)
{
  return (GetState () == WifiPhy::IDLE);
}

bool
WifiPhyStateHelper::IsStateBusy (void)
{
  return (GetState () != WifiPhy::IDLE);
}

bool
WifiPhyStateHelper::IsStateCcaBusy (void)
{
  return (GetState () == WifiPhy::CCA_BUSY);
}

bool
WifiPhyStateHelper::IsStateRx (void)
{
  return (GetState () == WifiPhy::RX);
}

bool
WifiPhyStateHelper::IsStateTx (void)
{
  return (GetState () == WifiPhy::TX);
}

bool
WifiPhyStateHelper::IsStateSwitching (void)
{
  return (GetState () == WifiPhy::SWITCHING);
}

bool
WifiPhyStateHelper::IsStateSleep (void)
{
  return (GetState () == WifiPhy::SLEEP);
}

bool
WifiPhyStateHelper::IsStateOff (void)
{
  return (GetState () == WifiPhy::OFF);
}

Time
WifiPhyStateHelper::GetStateDuration (void)
{
  return Simulator::Now () - m_previousStateChangeTime;
}

Time
WifiPhyStateHelper::GetDelayUntilIdle (void)
{
  Time retval;

  switch (GetState ())
    {
    case WifiPhy::RX:
      retval = m_endRx - Simulator::Now ();
      break;
    case WifiPhy::TX:
      retval = m_endTx - Simulator::Now ();
      break;
    case WifiPhy::CCA_BUSY:
      retval = m_endCcaBusy - Simulator::Now ();
      break;
    case WifiPhy::SWITCHING:
      retval = m_endSwitching - Simulator::Now ();
      break;
    case WifiPhy::IDLE:
      retval = Seconds (0);
      break;
    case WifiPhy::SLEEP:
    case WifiPhy::OFF:
      NS_FATAL_ERROR ("Cannot determine when the device will wake up.");
      retval = Seconds (0);
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      retval = Seconds (0);
      break;
    }
  retval = Max (retval, Seconds (0));
  return retval;
}

Time
WifiPhyStateHelper::GetLastRxStartTime (void) const
{
  return m_startRx;
}

enum WifiPhy::State
WifiPhyStateHelper::GetState (void)
{
  if (m_isOff)
    {
      return WifiPhy::OFF;
    }
  else if (m_sleeping)
    {
      return WifiPhy::SLEEP;
    }
  else if (m_endTx > Simulator::Now ())
    {
      return WifiPhy::TX;
    }
  else if (m_rxing)
    {
      return WifiPhy::RX;
    }
  else if (m_endSwitching > Simulator::Now ())
    {
      return WifiPhy::SWITCHING;
    }
  else if (m_endCcaBusy > Simulator::Now ())
    {
      return WifiPhy::CCA_BUSY;
    }
  else
    {
      return WifiPhy::IDLE;
    }
}

void
WifiPhyStateHelper::NotifyTxStart (Time duration, double txPowerDbm)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyTxStart (duration, txPowerDbm);
    }
    m_TxStart (Simulator::Now (), duration, WifiPhy::TX);
}

void
WifiPhyStateHelper::NotifyRxStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxStart (duration);
    }
   m_RxStart (Simulator::Now (), duration, WifiPhy::TX);
}

void
WifiPhyStateHelper::NotifyRxEndOk (void)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndOk ();
    }
  m_RxEndOk (Simulator::Now (), Simulator::Now (), WifiPhy::RX);
}

void
WifiPhyStateHelper::NotifyRxEndError (void)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndError ();
    }
  m_RxEndError (Simulator::Now (), Simulator::Now (), WifiPhy::RX);
}

void
WifiPhyStateHelper::NotifyMaybeCcaBusyStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyMaybeCcaBusyStart (duration);
    }
   m_CcaBusyStart (Simulator::Now (), duration, WifiPhy::CCA_BUSY);
}

void
WifiPhyStateHelper::NotifySwitchingStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifySwitchingStart (duration);
    }
}

void
WifiPhyStateHelper::NotifySleep (void)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifySleep ();
    }
}

void
WifiPhyStateHelper::NotifyOff (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyOff ();
    }
}

void
WifiPhyStateHelper::NotifyWakeup (void)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyWakeup ();
    }
}

void
WifiPhyStateHelper::NotifyOn (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyOn ();
    }
}

void
WifiPhyStateHelper::LogPreviousIdleAndCcaBusyStates (void)
{
  Time now = Simulator::Now ();
  Time idleStart = Max (m_endCcaBusy, m_endRx);
  idleStart = Max (idleStart, m_endTx);
  idleStart = Max (idleStart, m_endSwitching);
  //added sleep, also in next if, last condition
  idleStart = Max(idleStart, m_endSleep);
  NS_ASSERT (idleStart <= now);
  if (m_endCcaBusy > m_endRx
      && m_endCcaBusy > m_endSwitching
      && m_endCcaBusy > m_endTx
      && m_endCcaBusy > m_endSleep
      && m_endCcaBusy > m_endOff)
    {
      Time ccaBusyStart = Max (m_endTx, m_endRx);
      ccaBusyStart = Max (ccaBusyStart, m_startCcaBusy);
      ccaBusyStart = Max (ccaBusyStart, m_endSwitching);
      ccaBusyStart = Max (ccaBusyStart, m_endSleep);
      ccaBusyStart = Max (ccaBusyStart, m_endOff);
      m_stateLogger (ccaBusyStart, idleStart - ccaBusyStart, WifiPhy::CCA_BUSY);
    }
  m_stateLogger (idleStart, now - idleStart, WifiPhy::IDLE);
}

void
WifiPhyStateHelper::SwitchToTx (Time txDuration, Ptr<const Packet> packet, double txPowerDbm,
                                WifiTxVector txVector, WifiPreamble preamble)
{
  m_txTrace (packet, txVector.GetMode (), preamble, txVector.GetTxPowerLevel ());
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhy::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_rxing = false;
      m_stateLogger (m_startRx, now - m_startRx, WifiPhy::RX);
      m_endRx = now;
      break;
    case WifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhy::CCA_BUSY);
      } break;
    case WifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhy::SWITCHING:
    case WifiPhy::SLEEP:
    case WifiPhy::OFF:
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_stateLogger (now, txDuration, WifiPhy::TX);
  m_previousStateChangeTime = now;
  m_endTx = now + txDuration;
  m_startTx = now;
  NotifyTxStart (txDuration, txPowerDbm);
}

void
WifiPhyStateHelper::SwitchToRx (Time rxDuration)
{
  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
  NS_ASSERT (!m_rxing);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhy::CCA_BUSY);
      } break;
    case WifiPhy::SWITCHING:
    case WifiPhy::RX:
    case WifiPhy::TX:
    case WifiPhy::SLEEP:
    case WifiPhy::OFF:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_previousStateChangeTime = now;
  m_rxing = true;
  m_startRx = now;
  m_endRx = now + rxDuration;
  NotifyRxStart (rxDuration);
  NS_ASSERT (IsStateRx ());
}

void
WifiPhyStateHelper::SwitchToChannelSwitching (Time switchingDuration)
{
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhy::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_rxing = false;
      m_stateLogger (m_startRx, now - m_startRx, WifiPhy::RX);
      m_endRx = now;
      break;
    case WifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhy::CCA_BUSY);
      } break;
    case WifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhy::TX:
    case WifiPhy::SWITCHING:
    case WifiPhy::SLEEP:
    case WifiPhy::OFF:
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }

  if (now < m_endCcaBusy)
    {
      m_endCcaBusy = now;
    }

  m_stateLogger (now, switchingDuration, WifiPhy::SWITCHING);
  m_previousStateChangeTime = now;
  m_startSwitching = now;
  m_endSwitching = now + switchingDuration;
  NotifySwitchingStart (switchingDuration);
  NS_ASSERT (IsStateSwitching ());
}

void
WifiPhyStateHelper::SwitchFromRxEndOk (Ptr<Packet> packet, double snr, WifiTxVector txVector, enum WifiPreamble preamble)
{
  m_rxOkTrace (packet, snr, txVector.GetMode (), preamble);
  NotifyRxEndOk ();
  DoSwitchFromRx ();
  if (!m_rxOkCallback.IsNull ())
    {
      m_rxOkCallback (packet, snr, txVector, preamble);
    }

}

void
WifiPhyStateHelper::SwitchFromRxEndError (Ptr<const Packet> packet, double snr)
{
  m_rxErrorTrace (packet, snr);
  NotifyRxEndError ();
  DoSwitchFromRx ();
  if (!m_rxErrorCallback.IsNull ())
    {
      m_rxErrorCallback (packet, snr);
    }
}

void
WifiPhyStateHelper::DoSwitchFromRx (void)
{
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_rxing);

  Time now = Simulator::Now ();
  m_stateLogger (m_startRx, now - m_startRx, WifiPhy::RX);
  m_previousStateChangeTime = now;
  m_rxing = false;

  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
}

void
WifiPhyStateHelper::SwitchMaybeToCcaBusy (Time duration)
{
  NotifyMaybeCcaBusyStart (duration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhy::SWITCHING:
      break;
    case WifiPhy::SLEEP:
      break;
    case WifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhy::CCA_BUSY:
      break;
    case WifiPhy::RX:
      break;
    case WifiPhy::TX:
      break;
    case WifiPhy::OFF:
      break;
    }
  if (GetState () != WifiPhy::CCA_BUSY)
    {
      m_startCcaBusy = now;
    }
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
}

void
WifiPhyStateHelper::SwitchToSleep (void)
{
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhy::CCA_BUSY);
      } break;
    case WifiPhy::RX:
    case WifiPhy::SWITCHING:
    case WifiPhy::TX:
    case WifiPhy::SLEEP:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    case WifiPhy::OFF:
      break;
    }
  m_previousStateChangeTime = now;
  m_sleeping = true;
  m_startSleep = now;
  NotifySleep ();
  // NS_ASSERT (IsStateSleep ());
}

void
WifiPhyStateHelper::SwitchFromSleep (Time duration)
{
    NS_ASSERT(IsStateSleep());
    Time now = Simulator::Now();
    m_stateLogger(m_startSleep, now - m_startSleep, WifiPhy::SLEEP);
    m_previousStateChangeTime = now;
    m_sleeping = false;
    m_endSleep = now;
    NotifyWakeup();
    //update m_endCcaBusy after the sleep period
    //m_endCcaBusy = std::max (m_endCcaBusy, now + duration); why?
    m_endCcaBusy = std::max(m_endCcaBusy, now - m_startSleep);
    if (m_endCcaBusy > now)
    {
        NotifyMaybeCcaBusyStart(m_endCcaBusy - now);
    }
}

void
WifiPhyStateHelper::SwitchToOff (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhy::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_stateLogger (m_startRx, now - m_startRx, WifiPhy::RX);
      m_endRx = now;
      break;
    case WifiPhy::TX:
      /* The packet which is being transmitted as well
       * as its endTx event are cancelled by the caller.
       */
      m_stateLogger (m_startTx, now - m_startTx, WifiPhy::TX);
      m_endTx = now;
      break;
    case WifiPhy::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhy::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhy::CCA_BUSY);
      } break;
    case WifiPhy::SLEEP:
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_previousStateChangeTime = now;
  m_isOff = true;
  m_startOff = now;
  NotifyOff ();
  NS_ASSERT (IsStateOff ());
}

void
WifiPhyStateHelper::SwitchFromOff (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  NS_ASSERT (IsStateOff ());
  Time now = Simulator::Now ();
  m_stateLogger(m_startOff, now - m_startOff, WifiPhy::OFF);
  m_previousStateChangeTime = now;
  m_isOff = false;
  m_endOff = now;
  NotifyOn ();
  //update m_endCcaBusy after the off period
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
  if (m_endCcaBusy > now)
    {
      NotifyMaybeCcaBusyStart (m_endCcaBusy - now);
    }
}

} //namespace ns3
