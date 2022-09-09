# Sim Parameters
seed=10
simulationTime=$((300*1))
DataMode="'MCS1_10'" #"'MCS2_1'"
rho="'50'"
TrafficPath="'./OptimalRawGroup/traffic/data-32-0.82.txt'"
S1g1MfieldEnabled='false'
TrafficInterval=$((1024*60))

# Capacitor parameters
eh=0.001
capacitance=10
capEnabled='false'
payloadSize=$((1024 * 1))
beaconinterval=$((1024*1000))
trafficType="udp"

# Raw Config
NumSta=1
NRawGroups=1
NumSlot=1
RawBeaconInterval=102400
./scratch/RAWGenerate.sh $NumSta $NRawGroups $NumSlot $RawBeaconInterval 0 1
RAWConfigFile="./OptimalRawGroup/RawConfig-$NumSta-$NRawGroups-$NumSlot-$RawBeaconInterval-0-1.txt"

# Parameters
params1='cap-test --seed='$seed' --simulationTime='$simulationTime' --TrafficType='$trafficType
params2=' --DataMode='$DataMode' --rho='$rho' --TrafficPath='$TrafficPath 
params3=' --S1g1MfieldEnabled='$S1g1MfieldEnabled' --TrafficInterval='$TrafficInterval
params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
params5=' --payloadSize='$payloadSize' --BeaconInterval='$beaconinterval' --RAWConfigFile='$RAWConfigFile
params=$params1$params2$params3$params4$params5

# Run experiment
./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test

expFolder=./Experiments/1_Sta
cp ./remainingVoltage.txt ./$expFolder/remainingVoltage.txt

# # Plot capacitor voltage
cd $expFolder
python3 plot.py