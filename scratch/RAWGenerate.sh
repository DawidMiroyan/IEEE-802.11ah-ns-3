#!/bin/bash

if [ $# -ne 6 ]
then
echo "parameters missing"
exit 1
fi

NumSta=$1
NRawGroups=$2
NumSlot=$3
beaconinterval=$4
pageSliceCount=$5  # Ignore these
pageSliceLen=$6    # Ignore these
                   # Change in RAW-generate so they use default values or smth








RAWConfigPath="./OptimalRawGroup/RawConfig-$NumSta-$NRawGroups-$NumSlot-$beaconinterval-$pageSliceCount-$pageSliceLen.txt"
#-$pagePeriod-$pageSliceLength-$pageSliceCount

./waf --run "RAW-generate --NRawSta=$NumSta --NGroup=$NRawGroups --NumSlot=$NumSlot --RAWConfigPath=$RAWConfigPath --beaconinterval=$beaconinterval --pageSliceCount=$pageSliceCount --pageSliceLen=$pageSliceLen"

#./waf --run RAW-generate --command-template="gdb --args %s <args> --NRawSta=$NumSta --NGroup=$NRawGroups --NumSlot=$NumSlot --RAWConfigPath=$RAWConfigPath --beaconinterval=$beaconinterval --pageSliceCount=$pageSliceCount --pageSliceLen=$pageSliceLen"




