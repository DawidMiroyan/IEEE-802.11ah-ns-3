from posixpath import splitext
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

import os
import math

expPath = './'
dataPath = expPath + "/data/"
filePath = dataPath + "/data.txt"

data = {}

# data = {
#     0.001: {
#         "MCS1_10": {
#             5: [],
#             10: [],
#         },
#         "MCS1_1": {},
#         "MCS1_9": {},
#     }
# }

with open(filePath, 'r') as f:
    lines = f.readlines()

    header = True
    for line in lines:
        splitted = line.split(sep=" ")
        if len(splitted) == 4:
            eh = splitted[1]
            datamode = splitted[2].rstrip("'").lstrip("'")
            interval = splitted[3].rstrip("\n")

            if eh not in data:
                data[eh] = {}
            if datamode not in data[eh]:
                data[eh][datamode] = {}
            if interval not in data[eh][datamode]:
                data[eh][datamode][interval] = []

        else:
            option = splitted[0]
            value = splitted[1]

            if (option == "totalPacketsSent"):
                sent = int(value)
            elif (option == "totalPacketsDelivered"):
                data[eh][datamode][interval].append(min(int(value)/sent, 1))

for eh in data:
    legend = []
    x = range(1,21)
    colors = ["#1f77b4", "#ff7f0e", "#2ca02c"]
    linestyles = ["solid", "dotted"]

    for mode, color in zip(data[eh], colors):
        for interval, linestyle in zip(data[eh][mode], linestyles):
            plt.plot(x, data[eh][mode][interval], color=color, linestyle=linestyle)
            
    modes = list(data[eh].keys())
    intervals = list(data[eh][modes[0]].keys())
    legend_elements = [
        Line2D([0], [0], color=colors[0], linestyle="solid", label="DR "+modes[0] + ", Period:" + str(math.floor(int(intervals[0])/1000)) + "s"),
        Line2D([0], [0], color=colors[1], linestyle="solid",label="DR "+modes[1] + ", Period:" + str(math.floor(int(intervals[0])/1000)) + "s"),
        Line2D([0], [0], color=colors[2], linestyle="solid",label="DR "+modes[2] + ", Period:" + str(math.floor(int(intervals[0])/1000)) + "s"),
    
        Line2D([0], [0], color=colors[0], linestyle="dotted",label="DR "+modes[0] + ", Period:" + str(math.floor(int(intervals[1])/1000)) + "s"),
        Line2D([0], [0], color=colors[1], linestyle="dotted",label="DR "+modes[1] + ", Period:" + str(math.floor(int(intervals[1])/1000)) + "s"),
        Line2D([0], [0], color=colors[2], linestyle="dotted",label="DR "+modes[2] + ", Period:" + str(math.floor(int(intervals[1])/1000)) + "s"),
    ]

    plt.ylim(bottom=-0.05, top=1.05)
    plt.xticks(range(0, 21, 5))

    plt.legend(handles=legend_elements, ncol=2)
    plt.xlabel('Capacity (mF)')
    plt.ylabel('Probability')

    # plt.show()
    plt.savefig(expPath+"/SuccessProbability_128STA_" + "eh" + str(eh) + ".png")
    plt.clf()

