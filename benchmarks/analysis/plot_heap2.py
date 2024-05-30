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
    result = []
    current_group = {}
    for line in input_text.split('\n')[:-1]:
        if "END" in line.strip():
            last_free = int(line.strip().split(" ")[1])
            result.append((last_free, current_group))
            current_group = {}
        else:
            values = line.strip().split()
            current_group[int(values[1])] = int(values[0])
    if current_group:
        result.append(current_group)
    return result

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

        # 16
        # 32
        # 48-256
        # 256-2048
        # 2048-
        bins = [0] * 6

        for size, count in holes.items():
            total_size = size * count
            if size <= 16:
                bins[0] += total_size
            elif size <= 32:
                bins[1] += total_size
            elif size <= 128:
                bins[2] += total_size
            elif size <= 1024:
                bins[3] += total_size
            elif size <= 8192:
                bins[4] += total_size
            else:
                bins[5] += total_size

        if hole_total != 0:
            bins = [b for b in bins]
            # bins = [round(b / hole_total, 4) for b in bins]

        transformed_data.append((round(used_ratio, 3), round(heap_ratio, 3), round(hole_ratio, 3), bins))

    return transformed_data

def main():
    PROGRAM = "nano"
    binary_buddy = transform_data(parse_input_file(f"./collections/binary_{PROGRAM}.txt"))
    bt_buddy = transform_data(parse_input_file(f"./collections/bt_{PROGRAM}.txt"))
    ibuddy = transform_data(parse_input_file(f"./collections/ibuddy_{PROGRAM}.txt"))

    print(*binary_buddy, sep='\n')
    print()
    print(*bt_buddy, sep='\n')
    print()
    print(*ibuddy, sep='\n')

    x = np.array([n for n in range(len(bt_buddy))])
    # fig, ((ax0, ax1), (ax2, ax3)) = plt.subplots(nrows=2, ncols=2)

    # Plotting using seaborn histplot with multiple="stack"
    # plt.plot(x, [t[0] for t in binary_buddy], label="Binary buddy")
    # plt.plot(x, [t[0] for t in bt_buddy], label="Binary tree buddy")
    # plt.plot(x, [t[0] for t in ibuddy], label="iBuddy")
    # plt.xlabel('# operations (malloc/free)')
    # plt.ylabel('Heap usage')
    # plt.title("Heap usage comparison over time")
    # plt.xlim(x[0], x[-1])
    # plt.ylim(0, 2)

    print()
    y1 = [np.array([p[3][i] for p in binary_buddy]) for i in range(5)]
    y2 = [np.array([p[3][i] for p in bt_buddy]) for i in range(5)]
    y3 = [np.array([p[3][i] for p in ibuddy]) for i in range(5)]
    # y = []

    n = len(y1)  # Assuming all arrays have the same length
    y = []
    # print("n: ", n)
    print("y1: ", y1)

    # print(y)

    # for yp in y:
        # print(yp)
    # print()
    # print([[p[3][i] for p in ibuddy] for i in range(5)])

    width = 0.25
    multiplier = 0

    plt.bar(x,y1[0], width, color="r", label="<=16")
    plt.bar(x,y1[1], width, bottom=y1[0], color="tab:blue", label="<=32")
    plt.bar(x,y1[2], width, bottom=y1[0]+y1[1], color="b", label="<=128")
    plt.bar(x,y1[3], width, bottom=y1[0]+y1[1]+y1[2], color="y", label="<=512")
    plt.bar(x,y1[4], width, bottom=y1[0]+y1[1]+y1[2]+y1[3], color="m", label="<=8192")
    plt.bar(x,y1[4], width, bottom=y1[0]+y1[1]+y1[2]+y1[3]+y1[4], color="c", label=">8192")
    plt.legend()
    plt.title("Binary buddy")
    plt.xticks(x+width)

    plt.bar(x+0.25,y2[0], width, color="r", label="<=16")
    plt.bar(x+0.25,y2[1], width, bottom=y2[0], color="tab:blue", label="<=32")
    plt.bar(x+0.25,y2[2], width, bottom=y2[0]+y2[1], color="b", label="<=128")
    plt.bar(x+0.25,y2[3], width, bottom=y2[0]+y2[1]+y2[2], color="y", label="<=512")
    plt.bar(x+0.25,y2[4], width, bottom=y2[0]+y2[1]+y2[2]+y2[3], color="m", label="<=8192")
    plt.bar(x+0.25,y2[4], width, bottom=y2[0]+y2[1]+y2[2]+y2[3]+y2[4], color="c", label=">8192")
    # ax1.legend()
    # ax1.set_title("Binary tree buddy")

    # plt.bar(x+0.5,y3[0], width, color="r", label="<=16")
    # plt.bar(x+0.5,y3[1], width, bottom=y3[0], color="tab:blue", label="<=32")
    # plt.bar(x+0.5,y3[2], width, bottom=y3[0]+y3[1], color="b", label="<=128")
    # plt.bar(x+0.5,y3[3], width, bottom=y3[0]+y3[1]+y3[2], color="y", label="<=512")
    # plt.bar(x+0.5,y3[4], width, bottom=y3[0]+y3[1]+y3[2]+y3[3], color="m", label="<=8192")
    # plt.bar(x+0.5,y3[4], width, bottom=y3[0]+y3[1]+y3[2]+y3[3]+y3[4], color="c", label=">8192")
    # ax2.legend()
    # ax2.set_title("iBuddy")


    # ax3.plot(x, [t[0] for t in binary_buddy], label="Binary buddy")
    # ax3.plot(x, [t[0] for t in bt_buddy], label="Binary tree buddy")
    # ax3.plot(x, [t[0] for t in ibuddy], label="iBuddy")
    # ax3.legend()
    # ax3.set_title("Heap cutoff")

    plt.legend()
    plt.show()

if __name__ == "__main__":
    main()
