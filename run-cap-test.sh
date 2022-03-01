./waf --run 'cap-test --seed=10 --simulationTime=30 --payloadSize=256  --DataMode="MCS2_0"
            --rho="50" --TrafficPath="./OptimalRawGroup/traffic/data-32-0.82.txt"
            --S1g1MfieldEnabled=false --RAWConfigFile="./OptimalRawGroup/RawConfig-cap.txt"
            --eh=0.1' | awk '! /Connecting to visualizer/' > output-cap-test
#./waf --run "cap-test --seed=10 --simulationTime=30 --payloadSize=256 --BeaconInterval=100000 --DataMode="MCS2_8" --rho="50" --TrafficPath="./OptimalRawGroup/traffic/data-32-0.82.txt" --S1g1MfieldEnabled=false --RAWConfigFile="./OptimalRawGroup/RawConfig-cap.txt" "
