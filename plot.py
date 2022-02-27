import matplotlib.pyplot as plt

f = open("remainingVoltage.txt", "r")
lines = f.readlines()

x = []
y = []

for line in lines:
    xs, ys = line.split()

    x.append(int(xs))
    y.append(float(ys))

plt.plot(x,y)
plt.axhline(y=1.8, label='V_{th low}', color='red', linestyle='dotted')
plt.axhline(y=3, label='V_{th high}', color='green', linestyle='dotted')
plt.show()