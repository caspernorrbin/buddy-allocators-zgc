import numpy as np
import matplotlib.pyplot as plt

# files = ["binary.txt", "bt.txt", "ibuddy.txt", "lazy.txt"]
# files = ["binary_free.txt", "bt_free.txt", "ibuddy_free.txt", "lazy_free.txt"]
# files = ["binary_server.txt", "bt_server.txt", "ibuddy_server.txt", "lazy_server.txt"]
files = ["binary_free_server.txt", "bt_free_server.txt", "ibuddy_free_server.txt", "lazy_free_server.txt"]

binary_buddy = {2**x: [] for x in range(4,19)}
bt_buddy = {2**x: [] for x in range(4,19)}
ibuddy_buddy = {2**x: [] for x in range(4,19)}
lazy_buddy = {2**x: [] for x in range(4,19)}

for file in files:
    with open("../runs/" + file) as f:
        for line in f:
            time, size = line.split(" ")
            if "e" in time:
                continue
            size = int(size)
            time = int(time)
            if "binary" in file:
                binary_buddy[size].append(time)
            elif "bt" in file:
                bt_buddy[size].append(time)
            elif "ibuddy" in file:
                ibuddy_buddy[size].append(time)
            elif "lazy" in file:
                lazy_buddy[size].append(time)

cm = 1/2.54
plt.figure(figsize=(20*cm,8*cm))
plt.margins(x=0.016)

# print(binary_buddy)
binary_buddy = {str(k): np.mean(v) for k,v in binary_buddy.items()}
bt_buddy = {str(k): np.mean(v) for k,v in bt_buddy.items()}
ibuddy_buddy = {str(k): np.mean(v) for k,v in ibuddy_buddy.items()}
lazy_buddy = {str(k): np.mean(v) for k,v in lazy_buddy.items()}

X_axis = np.arange(len(binary_buddy.keys()))
plt.grid(True, which='both', axis='y', linestyle='--', linewidth=0.5, zorder=0)

# colors = ["#648fff", "#dc267f", "#785ef0", "#fe6100", "#ffb000", "#000000", "#ffffff"]
# colors = ["#0072b2", "#009e73", "#cc79a7", "#d55e00", "#56b4e9", "#f0e442", "#e69f00"]
# colors = ["#0F2080", "#85C0F9", "#A95AA1", "#F5793A"]
colors = ["#85C0F9", "#0F2080","#A95AA1", "#F5793A"]

plt.bar(X_axis - 0.3, binary_buddy.values(), 0.2, color=colors[0], label='Binary Buddy', zorder=3)
plt.bar(X_axis - 0.1, bt_buddy.values(), 0.2, color=colors[1], label='Binary Tree', zorder=3)
plt.bar(X_axis + 0.1, ibuddy_buddy.values(), 0.2, color=colors[2], label='iBuddy', zorder=3)
plt.bar(X_axis + 0.3, lazy_buddy.values(), 0.2, color=colors[3], label='Lazy Layer', zorder=3)

# plt.ylim(0, 600)
plt.yscale('log')
plt.ylim(0, 1000000)

# plt.tick_params(labelright=True)
# ax0 = plt.twinx()
# ax0.set_ylim(plt.ylim())
# ax0.set_yticks(plt.yticks(), plt.yticklabels(),
#                fontsize= 8)

# plt.gca().xaxis.set_minor_locator(plt.NullLocator())
# plt.gca().ticklabel_format(axis='y', style='plain', useOffset=False)

# Custom formatter to add decimals to ticks that round down to 0
# def custom_formatter(x, pos):
#     if x >= 1 or x == 0:
#         return "{:g}".format(x)
#     else:
#         return "{:.1f}".format(x)

# plt.gca().yaxis.set_major_formatter(plt.FuncFormatter(custom_formatter))

# x_ticks = [f"$2^{{{i}}}$" for i in range(4, 19)]
x_ticks = [str(i) for i in range(4, 19)]
plt.xticks(X_axis, x_ticks)
plt.xlabel("Block Size (log$_2$(B))")
plt.ylabel("Time (ns)")
# plt.title("Average Allocation Time for Different Buddy Allocators and Block Sizes (100000 iterations)", pad=10)
plt.title("Average Deallocation Time for Different Buddy Allocators and Block Sizes (100000 iterations)", pad=10)
plt.legend()
plt.savefig("test.svg",bbox_inches='tight')
plt.show()