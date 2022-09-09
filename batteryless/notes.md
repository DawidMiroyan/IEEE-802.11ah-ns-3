# Running experiments used in the paper "Energy Source management using Wi-Fi HaLow"

The different experiments can be found in the "Experiments" fodler located in the root of the repository.
Therein you can find 6 different experiments, corresponding to the following figures in the paper:


Figure | Experiment
---|---
Fig. 3  | 1_Sta
Fig. 6  | 3_Voltage_For_Diff_Eh_Cap_enabled
Fig. 7  | 4_CapSize_For_BeaconInterval
Fig. 8  | 5_Prob_Transmission_1STA
Fig. 9  | 6_Prob_Transmission_128STA

These experiments expect to be executed from the root directory:

`./Experiments/1_Sta/run-cap-test.sh`

These will run said experiment and generate the plots in the corresping experiment folder. The configuration of each experiment is then also present in each `run-cap-test.sh` file, with options such as:
- Simulation time (seconds)
- TrafficInterval (miliseconds)
- Payload size (byte)
- Number of stations
- Energy harvesting (mW)
- Capacitor size (mF)
- Datamode (e.g. MCS1_10, MCS1_1, MCS1_9)

The data for Experiment 4 was gathered manualy, as such it does not include any automated plotting.

The experiments rely on the network and application setup defined in `scratch/cap-test`. Any additional changes can be made there.