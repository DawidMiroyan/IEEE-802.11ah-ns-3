import matplotlib.pyplot as plt
import os

expPath = './'
dataPath = expPath + "/data/"
files = os.listdir(dataPath)

capSizeOptions = set()
ehOptions = set()

data = dict()

# Read the files and store their results in dict
for file in files:
    print
    options = file.rstrip(".txt").split(sep="_")
    ehopt = float(options[0])
    csopt = int(options[1])

    ehOptions.add(ehopt)
    capSizeOptions.add(csopt)

    key = (ehopt, csopt)

    x = []
    y = []

    with open(dataPath+file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            xs, ys = line.split()

            x.append(int(xs))
            y.append(float(ys))

    data[key] = {'x': x, 'y': y}


for eh in ehOptions:
    legend = []
    for key in sorted(data):
        if key[0]==eh:
            plt.plot([x/1000 for x in data[key]['x']], data[key]['y'])
            legend.append( "C= " + str(key[1]) + "mF")

    plt.axhline(y=1.8, color='red', linestyle='dotted')
    plt.annotate("V_{th_low}", xy=(-4, 1.83))
    plt.axhline(y=3, color='green', linestyle='dotted')
    plt.annotate("V_{th_high}", xy=(-4, 3.04))
    plt.ylim(bottom=1.5, top=3.5)

    plt.legend(legend)
    plt.xlabel('Time (s)')
    plt.ylabel('Voltage (V)')

    plt.savefig(expPath+"/VariedCapsize_" + "eh" + str(eh) + ".png")
    plt.clf()

