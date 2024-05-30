#!/usr/bin/python3

import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

import sys

HEAP_SIZE = 2097152

def parse_input_file(file_path):
    with open(file_path, 'r') as file:
        input_text = file.read()
    return parse_input(input_text)

def parse_input(input_text):
    fails = []
    result = []
    fragmentation = []
    current_group = {}
    for line in input_text.split('\n')[:-1]:
        if "END" in line.strip():
            last_free = int(line.strip().split(" ")[1])
            result.append((last_free, current_group))
            current_group = {}
        elif "FAILED" in line.strip():
            fails.append(int(line.strip().split(" ")[1]))
        elif "FRAGMENTATION" in line.strip():
            fragmentation.append(float(line.strip().split(" ")[1]))
        else:
            values = line.strip().split()
            current_group[int(values[1])] = int(values[0])
    if current_group:
        result.append(current_group)
    return result, fails, fragmentation

def total_hole_free(data):
    return sum([size * count for size, count in data.items()])

def transform_data(data, thresholds):
    transformed_data = []
    for (used, holes) in data:
        used_bytes = HEAP_SIZE - used
        hole_total = total_hole_free(holes)

        # How much of the heap is "used"
        used_ratio = used_bytes / HEAP_SIZE

        # Size of all holes in relation to the total heap size
        heap_ratio = hole_total / HEAP_SIZE

        # Size of all holes in relation to used memory
        hole_ratio = hole_total / used_bytes

        
        # 16
        # 32
        # 48-256
        # 256-2048
        # 2048-
        num_bins = len(thresholds) + 1
        bins = [0] * num_bins
        size_bins = [0] * num_bins

        for size, count in holes.items():
            # print(split_powers(size), count)
            # total_size = size * count
            powers = split_powers(size)
            # print(powers, count)
            for p in powers:
                for i, t in enumerate(thresholds):
                    if p <= t:
                        bins[i] += count
                        size_bins[i] += p * count
                        break

        if hole_total != 0:
            bins = [b for b in bins]
            size_bins = [b for b in size_bins]
            # bins = [round(b / hole_total, 4) for b in bins]

        transformed_data.append((round(used_ratio, 3), round(heap_ratio, 3), round(hole_ratio, 3), bins, size_bins))

    return transformed_data

def split_powers(x):
    powers = []
    i = 1
    while i <= x:
        if i & x:
            powers.append(i)
        i <<= 1
    return powers

def main():
    PROGRAM = "heap"
    binary_allocs, binary_fails, _ = parse_input_file(f"./collections/{PROGRAM}_binary.txt")
    bt_allocs, bt_fails, frag = parse_input_file(f"./collections/{PROGRAM}_bt22.txt")
    bt_alt_allocs, bt_fails_alt, _ = parse_input_file(f"./collections/{PROGRAM}_bt2.txt")
    ibuddy_allocs, ibuddy_fails, _ = parse_input_file(f"./collections/{PROGRAM}_ibuddy.txt")

    # print(frag)
    print("Min:", min(frag))
    print("Max:", max(frag))
    print("Mean:", np.mean(frag))
    print("Median:", np.median(frag))
    print("Std:", np.std(frag))
    # plt.hist(frag, bins=100)

    # thresholds = [16, 32, 128, 1024, 8192]
    thresholds = [2**n for n in range(4, 19)]
    binary_buddy = transform_data(binary_allocs, thresholds)
    bt_buddy = transform_data(bt_allocs, thresholds)
    bt_buddy_alt = transform_data(bt_alt_allocs, thresholds)
    ibuddy = transform_data(ibuddy_allocs, thresholds)

    # print(*binary_buddy, sep='\n')
    # print()
    # print(*bt_buddy, sep='\n')
    # print()
    # print(*ibuddy, sep='\n')

    x1 = [n for n in range(len(binary_buddy))]
    x2 = [n for n in range(len(bt_buddy))]
    x3 = [n for n in range(len(ibuddy))]
    x4 = [n for n in range(len(bt_buddy_alt))]

    cm = 1/2.54
    fig, (ax0, ax1) = plt.subplots(nrows=1, ncols=2, figsize=(30*cm, 8*cm))
    ax0.margins(x=0.02)
    ax1.margins(x=0.02)

    print()
    y1 = [np.array([p[3][i] for p in binary_buddy]) for i in range(len(thresholds))]
    y12 = [np.array([p[4][i] for p in binary_buddy]) for i in range(len(thresholds))]
    y2 = [np.array([p[3][i] for p in bt_buddy]) for i in range(len(thresholds))]
    y22 = [np.array([p[4][i] for p in bt_buddy]) for i in range(len(thresholds))]
    y3 = [np.array([p[3][i] for p in ibuddy]) for i in range(len(thresholds))]
    y32 = [np.array([p[4][i] for p in ibuddy]) for i in range(len(thresholds))]
    y4 = [np.array([p[3][i] for p in bt_buddy_alt]) for i in range(len(thresholds))]
    y42 = [np.array([p[4][i] for p in bt_buddy_alt]) for i in range(len(thresholds))]

    y1_m = [np.mean(y) for y in y1]
    y12_m = [np.mean(y) for y in y12]
    y2_m = [np.mean(y) for y in y2]
    y22_m = [np.mean(y) for y in y22]
    y3_m = [np.mean(y) for y in y3]
    y32_m = [np.mean(y) for y in y32]
    y4_m = [np.mean(y) for y in y4]
    y42_m = [np.mean(y) for y in y42]
    
    colors = ["#85C0F9", "#0F2080","#A95AA1", "#F5793A"]
    X_axis = np.arange(len(y1))
    # ax0.bar(X_axis, y1_m, 1, color=colors[0], edgecolor='black', zorder = 3)
    # ax1.bar(X_axis, [y / 1000 for y in y12_m], 1, color=colors[0], edgecolor='black', zorder = 3)

    # ax0.bar(X_axis, y4_m, 1, color=colors[1], edgecolor='black', zorder = 3)
    # ax1.bar(X_axis, [y / 1000 for y in y42_m], 1, color=colors[1], edgecolor='black', zorder = 3)

    ax0.bar(X_axis, y3_m, 1, color=colors[2], edgecolor='black', zorder = 3)
    ax1.bar(X_axis, [y / 1000 for y in y32_m], 1, color=colors[2], edgecolor='black', zorder = 3)

    ax0.set_ylim(0, 900)
    ax1.set_ylim(0, 80)
    

    # x_ticks = [f"$2^{{{i}}}$" for i in range(4, 19)]
    x_ticks = [f"{i}" for i in range(4, 19)]
    ax0.grid(True, which='both', axis='y', linestyle='--', linewidth=0.5, zorder=0)
    ax1.grid(True, which='both', axis='y', linestyle='--', linewidth=0.5, zorder=0)
    ax0.set_xticks(X_axis, x_ticks)
    ax1.set_xticks(X_axis, x_ticks)
    ax0.set_yticks(np.arange(0, 901, 100))  # Add y-ticks for every 100
    ax0.set_xlabel("Block Size (log$_2$(B))")
    ax1.set_xlabel("Block Size (log$_2$(B))")
    ax0.set_ylabel("Number of Blocks")
    ax1.set_ylabel("Combined Size of Blocks (KB)")
    ax0.set_title("External Fragmentation by Number of Blocks", pad=10)
    ax1.set_title("External Fragmentation by Combined Size of Blocks", pad=10)
    plt.savefig("test.svg",bbox_inches='tight')
    plt.show()

if __name__ == "__main__":
    main()
