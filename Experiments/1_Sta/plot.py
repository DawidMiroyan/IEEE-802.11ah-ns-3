import matplotlib.pyplot as plt

f = open("./remainingVoltage.txt", "r")
lines = f.readlines()

x = []
y = []

for line in lines:
    xs, ys = line.split()
    # if int(xs) < 4000:
    #     continue
    x.append(int(xs))
    y.append(float(ys))

plt.plot([x2/1000 for x2 in x],y)
plt.axhline(y=1.8, color='red', linestyle='dotted')
plt.annotate("V_{th_low}", xy=(-10, 1.83))
plt.axhline(y=3, color='green', linestyle='dotted')
plt.annotate("V_{th_high}", xy=(-10, 3.04))
plt.ylim(bottom=1.5, top=3.5)

plt.plot(36.774, 3.2, 'x', label="TX")
# plt.vlines(x=36.774, ymin=0, ymax=3.5, linestyles='dotted', color="grey")
plt.plot(58.4724, 3.2, 'v', label="OFF")
plt.vlines(x=58.4724, ymin=0, ymax=3.5, linestyles='dotted', color="grey")
plt.plot(233.9, 3.2, '^', label="TurnOn")
plt.vlines(x=233.9, ymin=0, ymax=3.5, linestyles='dotted', color="grey")

plt.axvspan(0, 58, 0, color="gray", alpha=0.4)
plt.annotate("ON", xy=(22, 1.6))
plt.annotate("OFF", xy=(140, 1.6))
plt.axvspan(233.9, max(x)/1000, 0, color="gray", alpha=0.4)
plt.annotate("ON", xy=(263, 1.6))

# plt.show()
keg = plt.legend(loc='upper right')
plt.xlabel('Time (s)')
plt.ylabel('Voltage (V)')
plt.savefig("./Experiments/1_Sta/remainingVoltage.png")