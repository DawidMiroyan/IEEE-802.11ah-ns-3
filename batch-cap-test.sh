#!/bin/bash

# Sim Parameters
seed=10
simulationTime=40
payloadSize=256
DataMode="'MCS2_0'"
rho="'50'"
TrafficPath="'./OptimalRawGroup/traffic/data-32-0.82.txt'"
S1g1MfieldEnabled='false'
RAWConfigFile="'./OptimalRawGroup/RawConfig-cap.txt'" 

params1='cap-test --seed='$seed' --simulationTime='$simulationTime' --payloadSize='$payloadSize
params2=' --DataMode='$DataMode' --rho='$rho' --TrafficPath='$TrafficPath 
params3=' --S1g1MfieldEnabled='$S1g1MfieldEnabled' --RAWConfigFile='$RAWConfigFile

# Capacitor parameters
eh=0.01
capacitance=6 # 6 farads
capEnabled='true'

params4=' --capacitance='$capacitance' --capEnabled='$capEnabled

# Clear log file content
> ./Experiments/log.txt

for eh in 0 0.0005 0.001 0.002 0.005 0.01 0.02 0.05 0.1
do
    params=$params1$params2$params3$params4' --eh='$eh
    # Run experiment
    ./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
    # Plot capacitor voltage
    python3 plot.py

    # Move and rename data for storage
    cp ./remainingVoltage.txt ./Experiments/remainingVoltage_eh$eh.txt
    mv ./remainingVoltage.png ./Experiments/remainingVoltage_eh$eh.png

    # Write experiment results to log
    echo "eh: $eh" >> ./Experiments/log.txt
    grep "STA 0 sent" ./output-cap-test >> ./Experiments/log.txt 
    #tail -n 19 ./output-cap-test >> ./Experiments/log.txt
    echo >> ./Experiments/log.txt
done