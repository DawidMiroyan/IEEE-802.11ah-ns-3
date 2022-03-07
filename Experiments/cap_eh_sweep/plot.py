import matplotlib.pyplot as plt
import os

expPath = '/home/dawid/Documents/RP1/IEEE-802.11ah-ns-3/Experiments/cap_eh_sweep'
dataPath = expPath + "/data/"
files = os.listdir(dataPath)

enableOption = set()
capSizeOptions = set()
ehOptions = set()

data = dict()

# Read the files and store their results in dict
for file in files:

    options = file.rstrip(".txt").split(sep="_")
    eopt = options[0]
    csopt = int(options[1])
    ehopt = float(options[2])

    enableOption.add(eopt)
    capSizeOptions.add(csopt)
    ehOptions.add(ehopt)

    key = (eopt, csopt, ehopt)

    x = []
    y = []

    with open(dataPath+file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            xs, ys = line.split()

            x.append(int(xs))
            y.append(float(ys))

    data[key] = {'x': x, 'y': y}


def plotToggleCap():
    for eopt in enableOption:
        legend = []
        for key in sorted(data):
            if key[0]==eopt:
                if key[1] in [3, 6, 10]:
                    if key[2] in [0.001, 0.005]:
                        plt.plot([x/1000 for x in data[key]['x']], data[key]['y'])

                        legend.append( "size: " + str(key[1]) + ", eh: " + str(key[2]))

        plt.axhline(y=1.8, label='V_{th low}', color='red', linestyle='dotted')
        plt.axhline(y=3, label='V_{th high}', color='green', linestyle='dotted')
        plt.ylim(bottom=1.5, top=3.5)


        plt.legend(legend)
        plt.xlabel('Time in seconds')
        plt.ylabel('Capacitor voltage')

        plt.savefig(expPath+"/remainingVoltage_" + eopt + ".png")
        plt.clf()


