import matplotlib.pyplot as plt

for i in range(101):
    with open("./output"+str(i)) as f:
        times = []
        for line in f.readlines():
            time = int(line.split(' ')[-1])
            times.append(time)
        plt.plot(times)
plt.xlabel("Iterations of memory access")
plt.ylabel("Number of Clock Cycles")
plt.show()
