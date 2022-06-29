# Sim Parameters
seed=10
simulationTime=$((120*1))
payloadSize=256
DataMode="'MCS1_10'" #"'MCS2_1'"
rho="'50'"
TrafficPath="'./OptimalRawGroup/traffic/data-32-0.82.txt'"
S1g1MfieldEnabled='false'
RAWConfigFile="'./OptimalRawGroup/RawConfig-cap.txt'" 
TrafficInterval=$((1024*10))

params1='cap-test --seed='$seed' --simulationTime='$simulationTime' --payloadSize='$payloadSize
params2=' --DataMode='$DataMode' --rho='$rho' --TrafficPath='$TrafficPath 
params3=' --S1g1MfieldEnabled='$S1g1MfieldEnabled' --RAWConfigFile='$RAWConfigFile' --TrafficInterval='$TrafficInterval


# Capacitor parameters
eh=0.001
capacitance=25
capEnabled='false'
payloadSize=$((100 * 1))

params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
params5=' --payloadSize='$payloadSize
params=$params1$params2$params3$params4$params5

# Run experiment
./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
python3 plot.py