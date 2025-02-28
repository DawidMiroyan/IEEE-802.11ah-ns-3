
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
 * Authors:  Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#ifndef CAPACITOR_ENERGY_SOURCE_H
#define CAPACITOR_ENERGY_SOURCE_H

#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/energy-source.h"
#include "ns3/wifi-phy.h"
#include <bits/stdint-intn.h>
#include <vector>

namespace ns3 {

/**
 * \ingroup energy
 * BasicEnergySource decreases/increases remaining energy stored in itself
 * according to a capacitor behavior.
 *
 */
class CapacitorEnergySource : public EnergySource
{
public:
  static TypeId GetTypeId (void);

  CapacitorEnergySource ();
  virtual ~CapacitorEnergySource ();

  // Pure virtual methods from Header that need to be implemented by this class
  virtual TypeId GetInstanceTypeId (void) const;

  /**
   * \return Initial energy stored in energy source, in Joules.
   *
   * Implements GetInitialEnergy.
   */
  virtual double GetInitialEnergy (void) const;

  /**
   * \returns Supply voltage at the energy source.
   *
   * Implements GetSupplyVoltage.
   */
  virtual double GetSupplyVoltage (void) const;

  /**
   * \return Remaining energy in energy source, in Joules
   *
   * Implements GetRemainingEnergy.
   */
  virtual double GetRemainingEnergy (void);

  /**
   * \returns Energy fraction.
   *
   * Implements GetEnergyFraction.
   */
  virtual double GetEnergyFraction (void);

  /**
   * Implements UpdateEnergySource.
   */
  virtual void UpdateEnergySource (void);

  /**
   * \param initialEnergyJ Initial energy, in Joules
   *
   * Sets initial energy stored in the energy source. Note that initial energy
   * is assumed to be set before simulation starts and is set only once per
   * simulation.
   */

  double GetInitialVoltage (void) const;

  /**
   * \param supplyVoltageV Supply voltage at the energy source, in Volts.
   *
   * Sets supply voltage of the energy source.
   */
  void SetSupplyVoltage (double supplyVoltageV);

  /**
   * \param interval Energy update interval.
   *
   * This function sets the interval between each energy update.
   */
  void SetUpdateInterval (Time interval);

  /**
   * \returns The interval between each energy update.
   */
  Time GetUpdateInterval (void) const;

  double GetActualVoltage (void);

  /**
   * fraction with respect to the max voltage reacheable
   */
  double GetVoltageFraction (void);

  /**
   Verify if the energy source is depleted
  */
  bool IsDepleted (void);

  /**
   * Compute new voltage given Iload and time duration starting from initial voltage
   */
  double ComputeVoltage (double initialVoltage, double Iload, double harvestedPower,
                         Time duration);
  /**
   * Compute the initial voltage given Iload and time duration when ending in final voltage
   */
  double ComputeInitialVoltage (double finalVoltage, double Iload, double hp,
                                Time duration);
  /**
   * Predict voltage variation for a given state and duration starting from initialVoltage
   */
  double PredictVoltageForWifiState (WifiPhy::State status,
                                        double initialvoltage, Time duration);

  /**
   * Set event to check energy depletion when estimated while staying in this state
   */
  void SetCheckForEnergyDepletion (void);

  /**
   * Compute resistances: Rload, ri, Req
   */
  std::vector<double> GetResistances (double Iload, double hp);

  /**
   * Compute the energy consumption of the load only when starting from voltage
   * V0 and consuming Iload for a given duration.
   */
  double ComputeLoadEnergyConsumption (double Iload, double V0, Time duration);

  int64_t AssignStreams (int64_t stream);

  // void NotifyEnergyConstant (void);

  /**
   * Compute the current produced by the harvesters
   */
  double GetHarvestersPower (void);
  double GetAveragePower (Time time, double samples);

  double GetLowVoltageTh (void);
  double GetHighVoltageTh (void);

  double GetEnergyFromVoltage (double voltage);

  std::vector<Ptr<EnergyHarvester>> GetEnergyHarvesters(void);

  void setPendingEnable (void);
  bool isPendingEnable (void);
  bool IsEnabled (void);
  void Enable (void);
  void Disable (void);

private:
  /// Defined in ns3::Object
  void DoInitialize (void);

  /// Defined in ns3::Object
  void DoDispose (void);

  /**
   * Handles the remaining energy going to zero event. This function notifies
   * all the energy models aggregated to the node about the energy being
   * depleted. Each energy model is then responsible for its own handler.
   */
  void HandleEnergyDrainedEvent (void);

  /**
   * Handles the remaining energy exceeding the high threshold after it went
   * below the low threshold. This function notifies all the energy models
   * aggregated to the node about the energy being recharged. Each energy model
   * is then responsible for its own handler.
   */
  void HandleEnergyRechargedEvent (void);

  void HandleEnergyChangedEvent (void);
  
  //void HandleEnergyConstantEvent (void); TODO

  /**
   * Compute the voltage at this time.
   */
  void UpdateVoltage (void);

  /**
   * Set initial voltage. Employs a random variable given as attribute
   */
  void SetInitialVoltage (void);

  /**
   * Compute the current consumed by the devices in this moment
   */
  double CalculateDevicesCurrent (void);

  /**
   * Write on the output file the actual voltage state.
   * It is called by UpdateSource.
   */
  void TrackVoltage (void);

private:
  Ptr<RandomVariableStream> m_initialVoltageRV; // random variable for the initial voltage, in Volt
  double m_initialVoltageV; // initial voltage, in Volts
  double m_supplyVoltageV; // supply voltage, in Volts
  double m_capacitance; //capacitance, in Farad
  double m_lowVoltageTh; // low voltage threshold, as a fraction of the initial voltage
  double m_highVoltageTh; // high voltage threshold, as a fraction of the initial voltage
  bool m_depleted; // set to true when the remaining energy goes below the low threshold,
      // set to false again when the remaining energy exceeds the high threshold
  bool m_enabled; // do not consume energy when disabled
  bool m_pendingEnable; // WifiRadioEnergyModel will Enable when pending

  TracedValue<double> m_remainingEnergyJ; // remaining energy, in Joules
  TracedValue<double> m_actualVoltageV; // the voltage at the present moment, in V
  EventId m_voltageUpdateEvent; // voltage update event
  EventId m_checkForEnergyDepletion; // Event called when we expect to deplete energy
  Time m_lastUpdateTime; // last update time
  Time m_updateInterval; // voltage update interval

  std::string m_filenameVoltageTracking; // name of the output file w/ voltage values
};

} // namespace ns3

#endif /* CAPACITOR_ENERGY_SOURCE_H */
