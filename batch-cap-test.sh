#!/bin/bash

# Sim Parameters
seed=10
simulationTime=60
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

# Clear log file content
> ./Experiments/log.txt


function capacitance_eh_sweep {
    expFolder=./Experiments/cap_eh_sweep
    dataFolder=$expFolder/data
    mkdir -p $expFolder
    mkdir -p $dataFolder


    for capEnabled in 'true' 'false'
    do
        for capacitance in 1 3 6 10 20 50
        do  
            for eh in 0 0.0005 0.001 0.002 0.005 0.01 0.02 0.05 0.1
            do
                # Set experiment parameters
                params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
                params=$params1$params2$params3$params4
                # Run experiment
                # ./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
                
                # Move results to data folder
                # cp ./remainingVoltage.txt $dataFolder/${capEnabled}_${capacitance}_${eh}.txt

                


    #             # Write experiment results to log
    #             # echo "capEnabled: ${capEnabled}, capacitance: ${capacitance}, eh: $eh"  >> ./Experiments/log.txt
    #             # grep "STA 0 sent" ./output-cap-test >> ./Experiments/log.txt 
    #             # #tail -n 19 ./output-cap-test >> ./Experiments/log.txt
    #             # echo >> ./Experiments/log.txt
            done
        done 
    done

    # # Plot capacitor voltage
    cd $expFolder
    python3 plot.py
}

function payload_sweep {
    expFolder=./Experiments/payload_sweep
    dataFolder=$expFolder/data
    mkdir -p $expFolder
    mkdir -p $dataFolder

    capEnabled='false'
    for payloadSize in 100 200 500 1000
    do
        for capacitance in 3 10
        do  
            for eh in 0.001 0.002 0.005
            do
                # Set experiment parameters
                params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
                params5=' --payloadSize='$payloadSize
                params=$params1$params2$params3$params4$params5
                # Run experiment
                # ./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
                
                # Move results to data folder
                # cp ./remainingVoltage.txt $dataFolder/${payloadSize}_${capacitance}_${eh}.txt
            done
        done 
    done

    # # Plot capacitor voltage
    cd $expFolder
    python3 plot.py
}

function mcs_sweep {
    expFolder=./Experiments/payload_sweep
    dataFolder=$expFolder/data
    mkdir -p $expFolder
    mkdir -p $dataFolder

    capEnabled='false'
    for mcs in #100 200 500 1000
    do
        for capacitance in 3 10
        do  
            for eh in 0.001 0.002 0.005
            do
                # Set experiment parameters
                params4=' --eh='$eh' --capacitance='$capacitance' --capEnabled='$capEnabled
                params5=' --DataMode='$mcs
                params=$params1$params2$params3$params4$params5
                # Run experiment
                # ./waf --run "$params" | awk '! /Connecting to visualizer/' > output-cap-test
                
                # Move results to data folder
                # cp ./remainingVoltage.txt $dataFolder/${payloadSize}_${capacitance}_${eh}.txt
            done
        done 
    done

    # # Plot capacitor voltage
    cd $expFolder
    python3 plot.py
}

# capacitance_eh_sweep
#payload_sweep