from matplotlib.lines import Line2D
import matplotlib.pyplot as plt
import os

expPath = '/home/dawid/Documents/RP1/IEEE-802.11ah-ns-3/Experiments/4_CapSize_For_BeaconInterval'
dataPath = expPath + "/data/"
# files = os.listdir(dataPath)



data = {
    "MCS1_10": {
        "0": [4,12,19,33,62,76],
        "0.001": [3,12,18,33,60,75],
        "0.01": [3,10,15,26,48,59],
    },
    "MCS1_1": {
        "0": [3,12,19,33,62,76],
        "0.001": [3,12,18,33,60,75],
        "0.01": [3,10,15,26,48,59],
    },
    "MCS1_9": {
        "0": [3,12,19,33,62,76],
        "0.001": [3,12,18,33,6075],
        "0.01": [3,10,15,26,40,59],
    }
}

x = [0.1, 1, 2, 4, 8, 10]

datamodes = ["MCS1_10", "MCS1_1"]
ehs = ["0", "0.001", "0.01"]
colors = ["#1f77b4", "#ff7f0e", "#2ca02c"]
legend = []

for i in range(len(datamodes)):
    datamode = datamodes[i]
    color = colors[i]

    plt.plot(x, data[datamode][ehs[0]], color=color, linestyle="dotted", marker='o')
    plt.plot(x, data[datamode][ehs[1]], color=color, linestyle="dashed", marker="x")
    plt.plot(x, data[datamode][ehs[2]], color=color, linestyle="dashdot", marker="v")

# plt.axhline(y=1.8, color='red', linestyle='dotted')
# plt.annotate("V_{th_low}", xy=(-4, 1.83))
# plt.axhline(y=3, color='green', linestyle='dotted')
# plt.annotate("V_{th_high}", xy=(-4, 3.04))
# plt.ylim(bottom=1.5, top=3.5)

legend_elements = [
    Line2D([0], [0], color=colors[0], label="MCS1_10"),
    Line2D([0], [0], color=colors[1], label="MCS1_1"),
    Line2D([0], [0], color=colors[2], label="MCS1_9"),


    Line2D([0], [0], color="black", label="EH: 0", linestyle="dotted"),
    Line2D([0], [0], color="black", label="EH: 0.001", linestyle="dashed"),
    Line2D([0], [0], color="black", label="EH: 0.01", linestyle="dashdot"),
]

plt.legend(handles=legend_elements, ncol=2)
plt.xlabel('Beacon Interval (s)')
plt.ylabel('Cmin (mF)')

# plt.show()

plt.savefig(expPath+"/CapsizeBeaconInterval" + ".png")
# plt.clf()

