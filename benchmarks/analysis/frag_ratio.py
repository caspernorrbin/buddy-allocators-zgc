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
    fail_size = 0
    frag_size = 0
    ratios = []
    fails = []
    for line in input_text.split('\n')[:-1]:
        if "FAILED" in line.strip():
            fail_size = int(line.strip().split(" ")[1])
            fails.append(fail_size)
        elif "END" in line.strip():
            free_hole = int(line.strip().split(" ")[1])
            frag_size += free_hole
            print(fail_size, frag_size)
            if frag_size == 0:
                ratios.append(0)
            else:
                ratios.append(frag_size / fail_size)
            fail_size = 0
            frag_size = 0
        else:
            count, size = line.strip().split()
            frag_size += int(count) * int(size)
    return ratios, fails

def total_hole_free(data):
    return sum([size * count for size, count in data.items()])

def transform_data(data):
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

        num_bins = 6
        thresholds = [16, 32, 128, 1024, 8192]
        # 16
        # 32
        # 48-256
        # 256-2048
        # 2048-
        bins = [0] * num_bins

        for size, count in holes.items():
            # print(split_powers(size), count)
            # total_size = size * count
            powers = split_powers(size)
            for p in powers:
                total_size = p * count
                if size <= thresholds[0]:
                    bins[0] += total_size
                elif size <= thresholds[1]:
                    bins[1] += total_size
                elif size <= thresholds[2]:
                    bins[2] += total_size
                elif size <= thresholds[3]:
                    bins[3] += total_size
                elif size <= thresholds[4]:
                    bins[4] += total_size
                else:
                    bins[5] += total_size

        if hole_total != 0:
            bins = [b for b in bins]
            # bins = [round(b / hole_total, 4) for b in bins]

        transformed_data.append((round(used_ratio, 3), round(heap_ratio, 3), round(hole_ratio, 3), bins))

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
    binary_ratios, binary_fails  = parse_input_file(f"./collections/{PROGRAM}_binary.txt")
    bt_ratios, bt_fails = parse_input_file(f"./collections/{PROGRAM}_bt.txt")
    ibuddy_ratios, ibuddy_fails = parse_input_file(f"./collections/{PROGRAM}_ibuddy.txt")

    print(binary_ratios)

    # binary_buddy = transform_data(binary_allocs)
    # bt_buddy = transform_data(bt_allocs)
    # ibuddy = transform_data(ibuddy_allocs)

    # print(*binary_buddy, sep='\n')
    # print()
    # print(*bt_buddy, sep='\n')
    # print()
    # print(*ibuddy, sep='\n')
    print(len(binary_ratios))

    x1 = [n for n in range(len(binary_ratios))]
    x2 = [n for n in range(len(bt_ratios))]
    x3 = [n for n in range(len(ibuddy_ratios))]
    fig, ((ax0, ax1), (ax2, ax3)) = plt.subplots(nrows=2, ncols=2)

    # Plotting using seaborn histplot with multiple="stack"
    # plt.plot(x, [t[0] for t in binary_buddy], label="Binary buddy")
    # plt.plot(x, [t[0] for t in bt_buddy], label="Binary tree buddy")
    # plt.plot(x, [t[0] for t in ibuddy], label="iBuddy")
    # plt.xlabel('# operations (malloc/free)')
    # plt.ylabel('Heap usage')
    # plt.title("Heap usage comparison over time")
    # plt.xlim(x[0], x[-1])
    # plt.ylim(0, 2)

    ax01 = ax0.twinx()
    ax0.plot(x1, binary_ratios, label="Binary buddy")
    # ax0.set_xlim(0,10)
    # ax01.set_xlim(0,10)
    print(x1)
    print(binary_fails)
    ax01.scatter(x1, binary_fails, label="Failed allocs", color="black", s=2)
    # ax0.set_yscale('log')
    ax1.plot(x2, bt_ratios, label="Binary tree buddy")
    ax2.plot(x3, ibuddy_ratios, label="iBuddy")

    # print(y)

    # for yp in y:
        # print(yp)
    # print()
    # print([[p[3][i] for p in ibuddy] for i in range(5)])

    # ax01 = ax0.twinx()
    # ax01.scatter(x1, binary_fails, label="Failed allocs", color="black")
    # ax0.bar(x1,y1[0], 1, color="r", label="<=16")
    # ax0.bar(x1,y1[1], 1, bottom=y1[0], color="tab:blue", label="<=32")
    # ax0.bar(x1,y1[2], 1, bottom=y1[0]+y1[1], color="b", label="<=128")
    # ax0.bar(x1,y1[3], 1, bottom=y1[0]+y1[1]+y1[2], color="y", label="<=1024")
    # ax0.bar(x1,y1[4], 1, bottom=y1[0]+y1[1]+y1[2]+y1[3], color="m", label="<=8192")
    # ax0.bar(x1,y1[4], 1, bottom=y1[0]+y1[1]+y1[2]+y1[3]+y1[4], color="c", label=">8192")

    # plot the failed allocs as a line over the bars'

    # # ax0.ticklabel_format(style='plain')
    # ax0.legend()
    # ax0.set_title("Binary buddy")
    # # ax0.set_ylim(0, 30000)
    # ax0.set_xlim(0, 300)
    # # ax01.set_ylim(0, 30000)
    # # ax01.set_xlim(0, 300)

    # ax1.bar(x2,y2[0], 1, color="r", label="<=16")
    # ax1.bar(x2,y2[1], 1, bottom=y2[0], color="tab:blue", label="<=32")
    # ax1.bar(x2,y2[2], 1, bottom=y2[0]+y2[1], color="b", label="<=128")
    # ax1.bar(x2,y2[3], 1, bottom=y2[0]+y2[1]+y2[2], color="y", label="<=1024")
    # ax1.bar(x2,y2[4], 1, bottom=y2[0]+y2[1]+y2[2]+y2[3], color="m", label="<=8192")
    # ax1.bar(x2,y2[4], 1, bottom=y2[0]+y2[1]+y2[2]+y2[3]+y2[4], color="c", label=">8192")
    # # ax1.ticklabel_format(style='plain')
    # ax1.legend()
    # ax1.set_title("Binary tree buddy")
    # # ax1.set_ylim(0, 30000)
    # ax1.set_xlim(0, 300)

    # ax2.bar(x3,y3[0], 1, color="r", label="<=16")
    # ax2.bar(x3,y3[1], 1, bottom=y3[0], color="tab:blue", label="<=32")
    # ax2.bar(x3,y3[2], 1, bottom=y3[0]+y3[1], color="b", label="<=128")
    # ax2.bar(x3,y3[3], 1, bottom=y3[0]+y3[1]+y3[2], color="y", label="<=1024")
    # ax2.bar(x3,y3[4], 1, bottom=y3[0]+y3[1]+y3[2]+y3[3], color="m", label="<=8192")
    # ax2.bar(x3,y3[4], 1, bottom=y3[0]+y3[1]+y3[2]+y3[3]+y3[4], color="c", label=">8192")
    # ax2.ticklabel_format(style='plain')
    # ax2.legend()
    # ax2.set_title("iBuddy")
    # ax2.set_xlim(0, 300)


    # ax3.plot(x, [t[0] for t in binary_buddy], label="Binary buddy")
    # ax3.plot(x, [t[0] for t in bt_buddy], label="Binary tree buddy")
    # ax3.plot(x, [t[0] for t in ibuddy], label="iBuddy")
    # ax3.legend()
    # ax3.set_title("Heap cutoff")

    # plt.legend()
    plt.show()

if __name__ == "__main__":
    main()
