# Sim Parameters
seed=10
simulationTime=100
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
eh=0.005
capacitance=3
capEnabled='false'
payloadSize=500

params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
params5=' --payloadSize='$payloadSize
params=$params1$params2$params3$params4$params5

# Run experiment
./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
python3 plot.py