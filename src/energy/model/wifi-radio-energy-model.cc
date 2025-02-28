/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
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
 * Author: Sidharth Nabar <snabar@uw.edu>, He Wu <mdzz@u.washington.edu>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"
#include "wifi-radio-energy-model.h"
#include "wifi-tx-current-model.h"
#include "capacitor-energy-source.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (WifiRadioEnergyModel);

TypeId
WifiRadioEnergyModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiRadioEnergyModel")
    .SetParent<DeviceEnergyModel> ()
    .SetGroupName ("Energy")
    .AddConstructor<WifiRadioEnergyModel> ()
    .AddAttribute ("IdleCurrentA",
                   "The default radio Idle current in Ampere.",
                   DoubleValue (0.273),  // idle mode = 273mA
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetIdleCurrentA,
                                       &WifiRadioEnergyModel::GetIdleCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaBusyCurrentA",
                   "The default radio CCA Busy State current in Ampere.",
                   DoubleValue (0.273),  // default to be the same as idle mode
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetCcaBusyCurrentA,
                                       &WifiRadioEnergyModel::GetCcaBusyCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxCurrentA",
                   "The radio TX current in Ampere.",
                   DoubleValue (0.380),    // transmit at 0dBm = 380mA
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetTxCurrentA,
                                       &WifiRadioEnergyModel::GetTxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxCurrentA",
                   "The radio RX current in Ampere.",
                   DoubleValue (0.313),    // receive mode = 313mA
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetRxCurrentA,
                                       &WifiRadioEnergyModel::GetRxCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SwitchingCurrentA",
                   "The default radio Channel Switch current in Ampere.",
                   DoubleValue (0.273),  // default to be the same as idle mode
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetSwitchingCurrentA,
                                       &WifiRadioEnergyModel::GetSwitchingCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SleepCurrentA",
                   "The radio Sleep current in Ampere.",
                   DoubleValue (0.033),  // sleep mode = 33mA
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetSleepCurrentA,
                                       &WifiRadioEnergyModel::GetSleepCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("OffCurrentA",
                   "The radio Sleep current in Ampere.",
                   DoubleValue (0.0),  // off mode = 0mA
                   MakeDoubleAccessor (&WifiRadioEnergyModel::SetOffCurrentA,
                                       &WifiRadioEnergyModel::GetOffCurrentA),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxCurrentModel", "A pointer to the attached TX current model.",
                   PointerValue (),
                   MakePointerAccessor (&WifiRadioEnergyModel::m_txCurrentModel),
                   MakePointerChecker<WifiTxCurrentModel> ())
    .AddTraceSource ("TotalEnergyConsumption",
                     "Total energy consumption of the radio device.",
                     MakeTraceSourceAccessor (&WifiRadioEnergyModel::m_totalEnergyConsumption),
                     "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

WifiRadioEnergyModel::WifiRadioEnergyModel ()
  : m_source (0),
    m_currentState (WifiPhy::State::IDLE),
    m_lastUpdateTime (Seconds (0.0)),
    m_nPendingChangeState (0),
    m_isSupersededChangeState (false),
    m_v0 (-1)
{
  NS_LOG_FUNCTION (this);
  m_energyDepletionCallback.Nullify ();
  // set callback for WifiPhy listener
  m_listener = new WifiRadioEnergyModelPhyListener;
  m_listener->SetChangeStateCallback (MakeCallback (&DeviceEnergyModel::ChangeState, this));
  // set callback for updating the TX current
  m_listener->SetUpdateTxCurrentCallback (MakeCallback (&WifiRadioEnergyModel::SetTxCurrentFromModel, this));
}

WifiRadioEnergyModel::~WifiRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_txCurrentModel = 0;
  delete m_listener;
}

void
WifiRadioEnergyModel::SetEnergySource (const Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
  m_switchToOffEvent.Cancel ();
  Time durationToOff = GetMaximumTimeInState (m_currentState);
  m_switchToOffEvent = Simulator::Schedule (durationToOff, &WifiRadioEnergyModel::ChangeState, this, WifiPhy::State::OFF);
}

double
WifiRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ()); // check if duration is valid

  // energy to decrease = current * voltage * time
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double energyToDecrease = duration.GetSeconds () * GetStateA (m_currentState) * supplyVoltage;

  // notify energy source
  m_source->UpdateEnergySource ();

  return m_totalEnergyConsumption + energyToDecrease;
}

double
WifiRadioEnergyModel::GetIdleCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_idleCurrentA;
}

void
WifiRadioEnergyModel::SetIdleCurrentA (double idleCurrentA)
{
  NS_LOG_FUNCTION (this << idleCurrentA);
  m_idleCurrentA = idleCurrentA;
}

double
WifiRadioEnergyModel::GetCcaBusyCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ccaBusyCurrentA;
}

void
WifiRadioEnergyModel::SetCcaBusyCurrentA (double CcaBusyCurrentA)
{
  NS_LOG_FUNCTION (this << CcaBusyCurrentA);
  m_ccaBusyCurrentA = CcaBusyCurrentA;
}

double
WifiRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txCurrentA;
}

void
WifiRadioEnergyModel::SetTxCurrentA (double txCurrentA)
{
  NS_LOG_FUNCTION (this << txCurrentA);
  m_txCurrentA = txCurrentA;
}

double
WifiRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxCurrentA;
}

void
WifiRadioEnergyModel::SetRxCurrentA (double rxCurrentA)
{
  NS_LOG_FUNCTION (this << rxCurrentA);
  m_rxCurrentA = rxCurrentA;
}

double
WifiRadioEnergyModel::GetSwitchingCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_switchingCurrentA;
}

void
WifiRadioEnergyModel::SetSwitchingCurrentA (double switchingCurrentA)
{
  NS_LOG_FUNCTION (this << switchingCurrentA);
  m_switchingCurrentA = switchingCurrentA;
}

double
WifiRadioEnergyModel::GetSleepCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sleepCurrentA;
}

void
WifiRadioEnergyModel::SetSleepCurrentA (double sleepCurrentA)
{
  NS_LOG_FUNCTION (this << sleepCurrentA);
  m_sleepCurrentA = sleepCurrentA;
}

double
WifiRadioEnergyModel::GetOffCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_offCurrentA;
}

void
WifiRadioEnergyModel::SetOffCurrentA (double offCurrentA)
{
  NS_LOG_FUNCTION (this << offCurrentA);
  m_offCurrentA = offCurrentA;
}

WifiPhy::State
WifiRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  return m_currentState;
}

void
WifiRadioEnergyModel::SetEnergyDepletionCallback (
  WifiRadioEnergyDepletionCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("WifiRadioEnergyModel:Setting NULL energy depletion callback!");
    }
  m_energyDepletionCallback = callback;
}

void
WifiRadioEnergyModel::SetEnergyRechargedCallback (
  WifiRadioEnergyRechargedCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("WifiRadioEnergyModel:Setting NULL energy recharged callback!");
    }
  m_energyRechargedCallback = callback;
}

void
WifiRadioEnergyModel::SetTxCurrentModel (const Ptr<WifiTxCurrentModel> model)
{
  m_txCurrentModel = model;
}

void
WifiRadioEnergyModel::SetTxCurrentFromModel (double txPowerDbm)
{
  if (m_txCurrentModel)
    {
      m_txCurrentA = m_txCurrentModel->CalcTxCurrent (txPowerDbm);
    }
}

Time
WifiRadioEnergyModel::GetMaximumTimeInState (int state) const
{
  if (state == WifiPhy::State::OFF)
    {
      NS_FATAL_ERROR ("Requested maximum remaining time for OFF state");
    }
  double remainingEnergy = m_source->GetRemainingEnergy ();
  double supplyVoltage = m_source->GetSupplyVoltage ();
  double current = GetStateA (state);
  return Seconds (remainingEnergy / (current * supplyVoltage));
}

double WifiRadioEnergyModel::ComputeEnergyConsumption (WifiPhy::State state, Time duration)
{
  NS_LOG_FUNCTION(this << state << duration);
  NS_LOG_DEBUG("State: " << state << " duration (s): " << duration.GetSeconds());

  double current = GetStateA (state);
  double energyConsumption = 0;

  Ptr<CapacitorEnergySource> capacitor = m_source -> GetObject<CapacitorEnergySource>();
  if (!(capacitor == 0))
    {
      NS_LOG_DEBUG("Iload " << current);
      if (m_v0 < 0) // First state change
        {
          m_v0 = capacitor->GetInitialVoltage();
        }
      energyConsumption = capacitor->ComputeLoadEnergyConsumption (current, m_v0, duration);
      m_v0 = capacitor->GetActualVoltage (); // The voltage in the capacitor is
                                             // already up-to-date
    }
  else
      {
        double supplyVoltage = m_source->GetSupplyVoltage ();
        energyConsumption = duration.GetSeconds () * current * supplyVoltage;

        NS_ASSERT (m_totalEnergyConsumption + energyConsumption <= m_source->GetInitialEnergy ());
      }

  NS_LOG_DEBUG ("energyconsumption=" << energyConsumption);
  return energyConsumption;
}

void
WifiRadioEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << newState);
  NS_LOG_FUNCTION (this << "Changing state... Computing energy consumption for the previous state");

  m_nPendingChangeState++;
  
  m_source->UpdateEnergySource ();

  Ptr<CapacitorEnergySource> cap =  m_source->GetObject<CapacitorEnergySource>();

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.GetNanoSeconds () >= 0);     // check if duration is valid

  double energyToDecrease = 0.0;

  energyToDecrease = ComputeEnergyConsumption (m_currentState, duration);

  // update total energy consumption
  m_totalEnergyConsumption += energyToDecrease;

  NS_LOG_DEBUG("Energy to decrease: " << energyToDecrease << " total energy consumption " << m_totalEnergyConsumption);

  // update last update time stamp
  m_lastUpdateTime = Simulator::Now ();

  // in case the energy source is found to be depleted during the last update, a callback might be
  // invoked that might cause a change in the PHY state (e.g., the PHY is put into SLEEP mode).
  // This in turn causes a new call to this member function, with the consequence that the previous
  // instance is resumed after the termination of the new instance. In particular, the state set
  // by the previous instance is erroneously the final state stored in m_currentState. The check below
  // ensures that previous instances do not change m_currentState.

  if (!m_isSupersededChangeState)
    {

      // update current state & last update time stamp
      SetWifiRadioState ((WifiPhy::State) newState);
      NS_LOG_DEBUG("[DEBUG] Set a new state");

     
      // After association, STA sends ACK and doesn't go to sleep afterwards
      // So instead create a PendingEnable and actualy enable when state goes from IDLE -> RX
      if (m_currentState == WifiPhy::State::RX &&
          cap ->isPendingEnable ())
      {
          cap ->Enable ();
      }

      // Set OFF event for when energy source would be depleted
      if (newState != WifiPhy::State::OFF)
        {
          m_switchToOffEvent.Cancel ();
          Time durationToOff = GetMaximumTimeInState (newState);

          m_switchToOffEvent = Simulator::Schedule (durationToOff, &WifiRadioEnergyModel::ChangeState, this, WifiPhy::State::OFF);
        }
      else
        {
          m_switchToOffEvent.Cancel ();
        }
    }

  m_isSupersededChangeState = (m_nPendingChangeState > 1);
  m_nPendingChangeState--;
}

void
WifiRadioEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("WifiRadioEnergyModel:Energy is depleted!");
  // invoke energy depletion callback, if set.
  if (!m_energyDepletionCallback.IsNull ())
    {
      m_energyDepletionCallback ();
    }
}

void
WifiRadioEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("WifiRadioEnergyModel:Energy is recharged!");
  // invoke energy recharged callback, if set.
  if (!m_energyRechargedCallback.IsNull ())
    {
      m_energyRechargedCallback ();
    }
}

void
WifiRadioEnergyModel::HandleEnergyChanged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("WifiRadioEnergyModel:Energy is changed!");
  if (m_currentState != WifiPhy::State::OFF)
    {
      m_switchToOffEvent.Cancel ();
      Time durationToOff = GetMaximumTimeInState (m_currentState);
 
      m_switchToOffEvent = Simulator::Schedule (durationToOff, &WifiRadioEnergyModel::ChangeState, this, WifiPhy::State::OFF);
    }
}

WifiRadioEnergyModelPhyListener *
WifiRadioEnergyModel::GetPhyListener (void)
{
  NS_LOG_FUNCTION (this);
  return m_listener;
}

/*
 * Private functions start here.
 */

void
WifiRadioEnergyModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_source = NULL;
  m_energyDepletionCallback.Nullify ();
}

double
WifiRadioEnergyModel::GetStateA (int state) const
{
  switch (state)
    {
    case WifiPhy::State::IDLE:
      return m_idleCurrentA;
    case WifiPhy::State::CCA_BUSY:
      return m_ccaBusyCurrentA;
    case WifiPhy::State::TX:
      return m_txCurrentA;
    case WifiPhy::State::RX:
      return m_rxCurrentA;
    case WifiPhy::State::SWITCHING:
      return m_switchingCurrentA;
    case WifiPhy::State::SLEEP:
      return m_sleepCurrentA;
    case WifiPhy::State::OFF:
      return m_offCurrentA;
    }
  NS_FATAL_ERROR ("WifiRadioEnergyModel: undefined radio state " << state);
}

double
WifiRadioEnergyModel::DoGetCurrentA (void) const
{
  return GetStateA (m_currentState);
}


void
WifiRadioEnergyModel::SetWifiRadioState (const WifiPhy::State state)
{
  NS_LOG_FUNCTION (this << state);
  m_currentState = state;

  std::string stateName;
  switch (state)
    {
    case WifiPhy::State::IDLE:
      stateName = "IDLE";
      break;
    case WifiPhy::State::CCA_BUSY:
      stateName = "CCA_BUSY";
      break;
    case WifiPhy::State::TX:
      stateName = "TX";
      break;
    case WifiPhy::State::RX:
      stateName = "RX";
      break;
    case WifiPhy::State::SWITCHING:
      stateName = "SWITCHING";
      break;
    case WifiPhy::State::SLEEP:
      stateName = "SLEEP";
      break;
    case WifiPhy::State::OFF:
      stateName = "OFF";
      break;
    }

  NS_LOG_DEBUG ("WifiRadioEnergyModel:Switching to state: " << stateName <<
                " at time = " << Simulator::Now ());
}

// -------------------------------------------------------------------------- //

WifiRadioEnergyModelPhyListener::WifiRadioEnergyModelPhyListener ()
{
  NS_LOG_FUNCTION (this);
  m_changeStateCallback.Nullify ();
  m_updateTxCurrentCallback.Nullify ();
}

WifiRadioEnergyModelPhyListener::~WifiRadioEnergyModelPhyListener ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiRadioEnergyModelPhyListener::SetChangeStateCallback (DeviceEnergyModel::ChangeStateCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  NS_ASSERT (!callback.IsNull ());
  m_changeStateCallback = callback;
}

void
WifiRadioEnergyModelPhyListener::SetUpdateTxCurrentCallback (UpdateTxCurrentCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  NS_ASSERT (!callback.IsNull ());
  m_updateTxCurrentCallback = callback;
}

void
WifiRadioEnergyModelPhyListener::NotifyRxStart (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::RX);
  m_switchToIdleEvent.Cancel ();
}

void
WifiRadioEnergyModelPhyListener::NotifyRxEndOk (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::IDLE);
}

void
WifiRadioEnergyModelPhyListener::NotifyRxEndError (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::IDLE);
}

void
WifiRadioEnergyModelPhyListener::NotifyTxStart (Time duration, double txPowerDbm)
{
  NS_LOG_FUNCTION (this << duration << txPowerDbm);
  if (m_updateTxCurrentCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Update tx current callback not set!");
    }
  m_updateTxCurrentCallback (txPowerDbm);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::TX);
  // schedule changing state back to IDLE after TX duration
  m_switchToIdleEvent.Cancel ();
  m_switchToIdleEvent = Simulator::Schedule (duration, &WifiRadioEnergyModelPhyListener::SwitchToIdle, this);
}

void
WifiRadioEnergyModelPhyListener::NotifyMaybeCcaBusyStart (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::CCA_BUSY);
  // schedule changing state back to IDLE after CCA_BUSY duration
  m_switchToIdleEvent.Cancel ();
  m_switchToIdleEvent = Simulator::Schedule (duration, &WifiRadioEnergyModelPhyListener::SwitchToIdle, this);
}

void
WifiRadioEnergyModelPhyListener::NotifySwitchingStart (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::SWITCHING);
  // schedule changing state back to IDLE after CCA_BUSY duration
  m_switchToIdleEvent.Cancel ();
  m_switchToIdleEvent = Simulator::Schedule (duration, &WifiRadioEnergyModelPhyListener::SwitchToIdle, this);
}

void
WifiRadioEnergyModelPhyListener::NotifySleep (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::SLEEP);
  m_switchToIdleEvent.Cancel ();
}

void
WifiRadioEnergyModelPhyListener::NotifyWakeup (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::IDLE);
}

void
WifiRadioEnergyModelPhyListener::NotifyOff (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::OFF);
  m_switchToIdleEvent.Cancel ();
}

void
WifiRadioEnergyModelPhyListener::NotifyOn (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::SLEEP);
}

void
WifiRadioEnergyModelPhyListener::SwitchToIdle (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("WifiRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (WifiPhy::State::IDLE);
}

} // namespace ns3