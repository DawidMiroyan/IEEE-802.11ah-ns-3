import matplotlib.pyplot as plt
import os

expPath = '/home/dawid/Documents/RP1/IEEE-802.11ah-ns-3/Experiments/cap_eh_sweep'
dataPath = expPath + "/data/"
files = os.listdir(dataPath)

capEnableOptions = set()
capSizeOptions = set()
ehOptions = set()

data = dict()

# Read the files and store their results in dict
for file in files:

    options = file.rstrip(".txt").split(sep="_")
    ceopt = int(options[0])
    csopt = int(options[1])
    ehopt = float(options[2])

    capEnableOptions.add(ceopt)
    capSizeOptions.add(csopt)
    ehOptions.add(ehopt)

    key = (ceopt, csopt, ehopt)

    x = []
    y = []

    with open(dataPath+file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            xs, ys = line.split()

            x.append(int(xs))
            y.append(float(ys))

    data[key] = {'x': x, 'y': y}


def plotVaryEH():
    for ceopt in capEnableOptions:
        for capSize in capSizeOptions:
            legend = []
            for key in sorted(data):
                if key[0]==ceopt and key[1]==capSize:
                    plt.plot([x/1000 for x in data[key]['x']], data[key]['y'])

                    legend.append( "size: " + str(key[1]) + ", eh: " + str(key[2]))

            plt.axhline(y=1.8, label='V_{th low}', color='red', linestyle='dotted')
            plt.axhline(y=3, label='V_{th high}', color='green', linestyle='dotted')
            plt.ylim(bottom=1.5, top=3.5)


            plt.legend(legend)
            plt.xlabel('Time in seconds')
            plt.ylabel('Capacitor voltage')

            plt.savefig(expPath+"/VariedEH_" + "payloadSize" + str(popt) + "_" + "capSize" + str(capSize) + ".png")
            plt.clf()

def plotVaryPayloadSize():
    for eh in ehOptions:
        for capSize in capSizeOptions:
            legend = []
            for key in sorted(data):
                if key[2]==eh and key[1]==capSize:
                    plt.plot([x/1000 for x in data[key]['x']], data[key]['y'])

                    legend.append( "Payload size: " + str(key[0]))

            plt.axhline(y=1.8, label='V_{th low}', color='red', linestyle='dotted')
            plt.axhline(y=3, label='V_{th high}', color='green', linestyle='dotted')
            plt.ylim(bottom=1.5, top=3.5)


            plt.legend(legend)
            plt.xlabel('Time in seconds')
            plt.ylabel('Capacitor voltage')

            plt.savefig(expPath+"/VariedPayload_" + "capSize" + str(capSize) + "_" + "eh" + str(eh) + ".png")
            plt.clf()

def plotVaryCapSize():
    pass

plotVaryEH()
plotVaryPayloadSize()
plotVaryCapSize()