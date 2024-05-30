import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from matplotlib.ticker import ScalarFormatter
import numpy as np

x = [16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144]

# files = ["page_binary.txt", "page_bt.txt", "page_ibuddy.txt"]
files = ["page_binary_server.txt", "page_bt_server.txt", "page_ibuddy_server.txt"]

binary_buddy = {2**x: [] for x in range(4,19)}
bt_buddy = {2**x: [] for x in range(4,19)}
ibuddy_buddy = {2**x: [] for x in range(4,19)}

for file in files:
    with open("../runs/" + file) as f:
        for line in f:
            time, size = line.split(" ")
            # print(time, size)
            size = int(size)
            time = float(time)
            if "binary" in file:
                binary_buddy[size].append(time)
            elif "bt" in file:
                bt_buddy[size].append(time)
            elif "ibuddy" in file:
                ibuddy_buddy[size].append(time)

binary_buddy = {str(k): round(np.mean(v), 3) for k,v in binary_buddy.items()}
bt_buddy = {str(k): round(np.mean(v), 3) for k,v in bt_buddy.items()}
ibuddy_buddy = {str(k): round(np.mean(v), 3) for k,v in ibuddy_buddy.items()}

print(binary_buddy)

cm = 1/2.54
plt.figure(figsize=(16*cm,12*cm))
plt.margins(x=0.016)


colors = ["#85C0F9", "#0F2080","#A95AA1", "#F5793A"]
# plot the data
plt.plot(x, binary_buddy.values(), color=colors[0], label='Binary Buddy')
plt.plot(x, bt_buddy.values(), color=colors[1], label='Binary Tree')
plt.plot(x, ibuddy_buddy.values(), color=colors[2], label='iBuddy')

# Make lines thicker
plt.setp(plt.gca().get_lines(), linewidth=2)

# Add points
plt.scatter(x, binary_buddy.values(), color=colors[0])
plt.scatter(x, bt_buddy.values(), color=colors[1])
plt.scatter(x, ibuddy_buddy.values(), color=colors[2])

# Add a grid
plt.grid(True, which='both', axis="both", linestyle='--', linewidth=0.5)

# make log-log
plt.xscale('log')
plt.yscale('log')
plt.ylim(0, 100000)



# add a title
plt.title('Time Taken to Allocate 2 MiB of Memory with Varied Block Sizes', pad=10)

# add a label to the x-axis
plt.xlabel('Block Size (log$_2$(B))')
plt.ylabel('Time (μs)')

# Change tick to be text as powers of 2
# x_ticks = [f"$2^{{{i}}}$" for i in range(4, 19)]
x_ticks = [str(i) for i in range(4, 19)]
# set the x-ticks to be the same as the x-values
plt.xticks(x, x_ticks)

# plt.xticks(x, x)
# plt.minorticks_off()
plt.gca().xaxis.set_minor_locator(plt.NullLocator())
# plt.gca().ticklabel_format(axis='y', style='plain', useOffset=False)

# Custom formatter to add decimals to ticks that round down to 0
def custom_formatter(x, pos):
    if x >= 1 or x == 0:
        return "{:g}".format(x)
    else:
        return "{:.1f}".format(x)

# plt.gca().yaxis.set_major_formatter(plt.FuncFormatter(custom_formatter))
# plt.gca().yaxis.set_minor_formatter(plt.FuncFormatter(custom_formatter))
# plt.yticks(rotation=45)
# plt.setp(plt.gca().yaxis.get_minorticklabels(), rotation=45)
# plt.gca().yaxis.set_major_formatter(ScalarFormatter())
# plt.gca().yaxis.set_minor_formatter(ScalarFormatter())

# plt.yticks([0.1, 1, 10], ['0.1s', '1s', '10s'])

# Set plot size
# plt.gcf().set_size_inches(8, 6)

plt.legend()
plt.savefig("test.svg",bbox_inches='tight')
plt.show()