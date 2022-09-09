# Sim Parameters
seed=10
simulationTime=$((60*1))

rho="'50'"
TrafficPath="'./OptimalRawGroup/traffic/data-32-0.82.txt'"
S1g1MfieldEnabled='false'
TrafficInterval=$((1024*1))
capEnabled='true'
payloadSize=$((256))
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
params2=' --rho='$rho' --TrafficPath='$TrafficPath 
params3=' --S1g1MfieldEnabled='$S1g1MfieldEnabled' --TrafficInterval='$TrafficInterval
params5=' --payloadSize='$payloadSize' --RAWConfigFile='$RAWConfigFile' --capEnabled='$capEnabled


# MCS1_10 MCS1_1 MCS1_9
DataMode="'MCS1_9'"
# 0 0.001 0.01 0.1
eh=0.01
# 102400 1024000 2048000 4096000 10240000
BeaconInterval=8192000
capacitance=47
params4=' --DataMode='$DataMode' --eh='$eh' --BeaconInterval='$BeaconInterval' --capacitance='$capacitance
params=$params1$params2$params3$params4$params5

./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
