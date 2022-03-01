# comet-lab

## Compiling code into RISC-V ELF executable

Most of us don't have RISC-V machines, so we need to install the RISC-V cross-compiler on our machines. Here we use [RISC-V GNU Compiler Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain) to demonstrate. To install RISC-V GNU Compiler Toolchain on your Linux machine, you can follow this [tutorial](https://mindchasers.com/dev/rv-getting-started).

Make sure that the toolchain is successfully installed:

```
$ riscv32-unknown-elf-gcc --version
riscv32-unknown-elf-gcc (GCC) 11.1.0
Copyright (C) 2021 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

After installing the toolchain, we can compile our C or C++ code into RISC-V ELF executable. For example, we save the following code as `main.c`:

```c
int c = 0;

int main() {
    int a = 2;
    int b = 3;
    c = a + b - 1;
    return 0;
}
```

Note that Comet only supports RV32I (except for the system calls) and multiplication instructions. Make sure your code does not contain code that is not supported, such as division, remainder, or system calls.

We can then compile our code with the toolchain:

```
$ riscv32-unknown-elf-gcc main.c -o main.out
```



## Parsing the ELF file and dumping out the memory

In order to have our code processed on Comet processor, we should dump out the instruction memory and data memory from the previously compiled ELF file. An ELF parser is already included in our repository. To compile it, we should first modify the `makefile` under `synthesizable/` directory. the `AP` variable in the first line should be set to the directory of the [Xilinx HLS arbitrary precision types library](https://github.com/Xilinx/HLS_arbitrary_Precision_Types):

```
AP=/opt/Xilinx/2019.2/include # path to your ap library
```

After specifying the path, we can `cd` to the `synthesizable/` directory in the repository, then `make`:

```
make
```

Two executables, `simulator.sim` and `testbench.sim`, will then be compiled. Note that `simulator.sim` can only be compiled on Linux, since the code includes some Linux header files.

We can then parse `main.out` and dump out the memory with `simulator.sim`:

```
$ ./simulator.sim path/to/main.out
pc_begin: 65676
pc_end: 66908
Continue C simulation? [y/n]:y 
exit at pc 66916.

clock cycle: 365
instruction count: 354
branch non-taken & predict non-taken: 6
branch non-taken & predict taken: 12
branch taken & predict non-taken: 7
branch taken & predict taken: 6
pipeline stall: 11
pipeline flush: 64
```

This generates 4 files, `ins.dat`, `data.dat`, `pc.dat`, `data_out_sim.dat`. The first two files contain initial value of the instruction memory and the data memory, respectively. `pc.dat` contains the start pc and exit pc. `simulator.sim` then performs C simulation with these file, and generates `data_out_sim.dat`, which contains the value of the data memory after finishing the processing.

The output directory of these files can also be specified with an additional argument:

```
$ ./simulator.sim path/to/main.out path/to/your/directory/
```



## Verifying your processor design

The previously compiled `testbench.sim` can be used to verify your new design, if you have modified `core.cpp`:

```
$ ./testbench.sim
exit at pc 66916.

clock cycle: 365
instruction count: 354
branch non-taken & predict non-taken: 6
branch non-taken & predict taken: 12
branch taken & predict non-taken: 7
branch taken & predict taken: 6
pipeline stall: 11
pipeline flush: 64
```

If you have previouly specified the output directory, so that the `.dat` files are not in the current directory, you should also specify the path to your `.dat` files:

```
$ ./testbench.sim path/to/your/directory/
```

This will take `ins.dat`, `data.dat`, `pc.dat` as input, perform C simulation with these files, and then compare the result with `data_out_sim.dat`. If the result does not match with `data_out_sim.dat`, some error message will be displayed.

Note that `simulator.sim` and `testbench.sim` include the same `core.cpp` (and also other files), so the design will always pass the verification if the `.dat` files are generated by `simulator.sim` with the exactly same design. To correctly verify your design, you should first generate the `.dat` files with your original design with `simulator.sim`, then modify your `core.cpp` (and maybe other files), then compile `testbench.sim` with your same design and verify your design with the previously generated `.dat` files. The `synthesizable/benchmark/correct/` directory already contains some correct testcase, you can run the `test.sh` script to verify your design with these benchmarks (and also check the performance).



## Exporting IP and Generating Bit-stream

Create a new Vitis HLS project, add `core.cpp`, `DataMemory.h`, `branchPredictor.h`, `core.h`, `perf.h`, `portability.h`, `registers.h`, `riscvISA.h` to the Source file, add `testbench.cpp`, `testbench.h` to the Test Bench file. Note that all files should be in the same directory.

Set Top Function as `doStep`, and set Period to `50`. We can then synthesize our design and check if the II = 1. If the design is synthesized successfully, we can export our design as IP.

Create a new Vivado project, import the IP we just generated, and create block design:

![](https://i.imgur.com/VEdS9qR.png)

We can then generate bit-stream and load it into the FPGA.
