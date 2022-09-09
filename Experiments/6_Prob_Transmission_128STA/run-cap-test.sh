# Sim Parameters
seed=10
simulationTime=$((60*40))

rho="'50'"
TrafficPath="'./OptimalRawGroup/traffic/data-32-0.82.txt'"
S1g1MfieldEnabled='false'
TrafficInterval=$((1024*10)) #in ms
capEnabled='true'
payloadSize=$((256))
beaconinterval=$((1024*2000)) #in ns
trafficType="udp"

# Raw Config
NumSta=128
NRawGroups=4
NumSlot=1
RawBeaconInterval=512000
./scratch/RAWGenerate.sh $NumSta $NRawGroups $NumSlot $RawBeaconInterval 0 1
RAWConfigFile="./OptimalRawGroup/RawConfig-$NumSta-$NRawGroups-$NumSlot-$RawBeaconInterval-0-1.txt"

# Parameters
params1='cap-test --seed='$seed' --simulationTime='$simulationTime' --TrafficType='$trafficType
params2=' --rho='$rho' --TrafficPath='$TrafficPath 
params3=' --S1g1MfieldEnabled='$S1g1MfieldEnabled
params5=' --payloadSize='$payloadSize' --BeaconInterval='$beaconinterval' --RAWConfigFile='$RAWConfigFile

expFolder=./Experiments/6_Prob_Transmission_128STA
dataFolder=$expFolder/data
mkdir -p $expFolder
mkdir -p $dataFolder

# Delete data
rm $dataFolder/*
COUNTER=0
for eh in 0.01
do
    for DataMode in "'MCS1_10'" "'MCS1_1'" "'MCS1_9'"
    do
        for TrafficInterval in 5120 10240
        do
            echo "header "$eh" "$DataMode" "$TrafficInterval >> $dataFolder/data.txt
            for capacitance in {1..20}
            do

                params4=' --eh='$eh' --DataMode='$DataMode' --capacitance='$capacitance' --TrafficInterval='$TrafficInterval
                params=$params1$params2$params3$params4$params5

                echo Experiment $COUNTER
                let COUNTER++
                ./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
                
                # Store totalPacketsSent and totalPacketsDelivered in file
                cat output-cap-test | grep -oP '(totalPacketsSent) [0-9]*' >> $dataFolder/data.txt
                cat output-cap-test | grep -oP '(totalPacketsDelivered) [0-9]*' >> $dataFolder/data.txt
                cat output-cap-test | grep -oP '(latency) [0-9]*[.][0-9]*' >> $dataFolder/data.txt
                cat output-cap-test | grep -oP '(throughput) [0-9]*[.][0-9]*' >> $dataFolder/data.txt
            done
        done
    done
done


# # Plot capacitor voltage
cd $expFolder
python3 plot.py

