# Sim Parameters
seed=10
simulationTime=$((200*1))
DataMode="'MCS1_0'"
rho="'50'"
TrafficPath="'./OptimalRawGroup/traffic/data-32-0.82.txt'"
S1g1MfieldEnabled='false'
TrafficInterval=$((1024*30))
capEnabled='true'
payloadSize=$((256))
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
params5=' --payloadSize='$payloadSize' --BeaconInterval='$beaconinterval' --RAWConfigFile='$RAWConfigFile

expFolder=./Experiments/3_Voltage_For_Diff_Eh_Cap_Enabled
dataFolder=$expFolder/data
mkdir -p $expFolder
mkdir -p $dataFolder

# Delete data
rm $dataFolder/*
for eh in 0 0.001 0.01
do
    for capacitance in 3 10 25
    do
        # Set experiment parameters
        params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
        params=$params1$params2$params3$params4$params5


        # Run experiment
        ./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
        
        # Move results to data folder
        cp ./remainingVoltage.txt $dataFolder/${eh}_${capacitance}.txt
    done
done
# # Plot capacitor voltage
cd $expFolder
python3 plot.py

