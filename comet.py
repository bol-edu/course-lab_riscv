
# coding: utf-8


from __future__ import print_function
from pynq import MMIO

import sys
import numpy as np
from time import time

sys.path.append('/home/xilinx')
from pynq import Overlay
from pynq import allocate
ol = Overlay("/home/xilinx/comet.bit")
top = ol.doStep_0


MEM_SIZE = 8192
file_path = "correct/matmul4_4/"


ins_memory = allocate(shape=(MEM_SIZE >> 2,), dtype=np.int32)
mem_file = open(file_path + "ins.dat", "r+")
for i in range(MEM_SIZE >> 2):
    value = mem_file.readline()
    ins_memory[i] = int(value)
top.write(0x30, ins_memory.device_address)
top.write(0x34, 0x0)
    
data_memory = allocate(shape=(MEM_SIZE >> 2,), dtype=np.int32)
mem_file = open(file_path + "data.dat", "r+")
for i in range(MEM_SIZE >> 2):
    value = mem_file.readline()
    data_memory[i] = int(value)
top.write(0x3c, data_memory.device_address)
top.write(0x40, 0x0)

pc_file = open(file_path + "pc.dat", "r+")
value = pc_file.readline()
print("pc_begin:", value)
top.write(0x10, int(value))
value = pc_file.readline()
pc_end = int(value)
print("pc_end:", pc_end)
print("done")


perf_arr = allocate(shape=(32,), dtype=np.int32)
top.write(0x48, perf_arr.device_address)


top.write(0x28, 0xffffffff)
top.write(0x00, 0x01)
    
timeKernelStart = time()
while (top.read(0x00) & 0x04) == 0x0:
    continue
timeKernelEnd = time()
print("execution time: " + str(timeKernelEnd - timeKernelStart) + " s")


if top.read(0x18) != pc_end and top.read(0x18) - 8 != pc_end:
    print("warning: incomplete execution!")


mem_file = open(file_path + "data_out.dat", "w")
for i in range(MEM_SIZE >> 2):
    value = data_memory[i]
    mem_file.write(str(value) + "\n")
mem_file = open(file_path + "data_out.dat", "w")
print("done")


f1 = open(file_path + "data_out.dat")
f2 = open(file_path + "data_out_sim.dat")

i = 0
correct = True
for f1_line, f2_line in zip(f1, f2):
    if f1_line != f2_line:
        print("incorrect result", i)
        correct = False
        break
    i += 1
    
if correct:
    print("correct result")


perf = {
    "CLOCK_CYCLE": 0,
    "INSTRUCTION_COUNT": 1,

    "BRANCH_NT_PDNT": 9,
    "BRANCH_NT_PDT": 10,
    "BRANCH_T_PDNT": 11,
    "BRANCH_T_PDT": 12,
    "STALL_COUNT": 13,
    "FLUSH_COUNT": 14
}

for counter in perf:
    print('{:<20} {:>10}'.format(counter, perf_arr[perf[counter]]))

