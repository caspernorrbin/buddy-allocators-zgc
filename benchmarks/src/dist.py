import numpy as np
import matplotlib.pyplot as plt
np.random.seed(1)

def myround(x, base=8):
    return base * round(x/base)

num_sizes = 500000
# num_sizes = 1000
sizes = np.random.poisson(6, num_sizes)

sizes = np.array([2**size for size in sizes if size >= 4])
sizes = np.array([int(size + 0.5 * size * np.random.uniform(-1, 1)) for size in sizes])
sizes = np.array([myround(size) for size in sizes])
np.clip(sizes, 2**4, 2**18, out=sizes)

paired_sizes = [(i+1, sizes[i]) for i in range(len(sizes))]

# print(sizes)
# for (id, size) in paired_sizes:
#     print(f"a {id} {size}")

fig, ((ax0, ax1), (ax2, ax3)) = plt.subplots(nrows=2, ncols=2)
ax0.hist(sizes,bins=4000)
ax0.set_xlim(0, 300)
ax1.hist(sizes,bins=1000)
ax1.set_xlim(0, 2000)
ax2.hist(sizes,bins=1000)
ax2.set_xlim(0, 10000)
ax3.hist(sizes,bins=1000)
ax3.set_xlim(0, 250000)
plt.hist(sizes,bins=1000)
plt.xlim(0,1000)
plt.show()

