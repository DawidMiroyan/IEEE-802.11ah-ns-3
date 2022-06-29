import matplotlib.pyplot as plt

f = open("remainingVoltage.txt", "r")
lines = f.readlines()

x = []
y = []

for line in lines:
    xs, ys = line.split()

    x.append(int(xs))
    y.append(float(ys))

plt.plot([x2/1000 for x2 in x],y)
plt.axhline(y=1.8, label='V_{th low}', color='red', linestyle='dotted')
plt.axhline(y=3, label='V_{th high}', color='green', linestyle='dotted')
plt.ylim(bottom=1.5, top=3.5)
plt.xlabel('Time in seconds')
plt.ylabel('Capacitor voltage')
plt.show()
# plt.savefig("./remainingVoltage.png")