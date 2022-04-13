AP=/opt/Xilinx/2019.2/include
INC=$(AP) ./include
LINC=./include
INC_PARAMS=$(foreach d, $(INC), -I$d)
# VARS_VIV=-D__DEBUG__=1 -D__VIVADO__=1
SIM_VARS_VIV=-D__VIVADO__=1 -D__SIMULATOR__=1
TEST_VARS_VIV=-D__VIVADO__=1 -D__TESTBENCH__=1
# FILES=core.cpp elfFile.cpp reformeddm_sim.cpp syscall.cpp
SIM_FILES=core.cpp elfFile.cpp simulator.cpp
TEST_FILES=core.cpp testbench.cpp
S=./src
S_SIM_FILES=$(foreach f, $(SIM_FILES), $(S)/$f)
S_TEST_FILES=$(foreach f, $(TEST_FILES), $(S)/$f)
HEADER=core.h elf.h elfFile.h perf.h portability.h registers.h riscvISA.h DataMemory.h branchPredictor.h
I_HEADER=$(foreach f, $(HEADER), $(INC)/$f)
.DEFAULT_GOAL := default

default: $(S_TEST_FILES) $(S_SIM_FILES) $(I_HEADER) 
	g++ -o testbench.sim $(INC_PARAMS) $(S_TEST_FILES) $(TEST_VARS_VIV)
	g++ -o simulator.sim $(INC_PARAMS) $(S_SIM_FILES) $(SIM_VARS_VIV)

testbench.sim: $(S_TEST_FILES) $(I_HEADER) 
	g++ -o testbench.sim $(INC_PARAMS) $(S_TEST_FILES) $(TEST_VARS_VIV)

simulator.sim: $(S_SIM_FILES) $(I_HEADER) 
	g++ -o simulator.sim $(INC_PARAMS) $(S_SIM_FILES) $(SIM_VARS_VIV)

elfFile.o: $(S)/elfFile.cpp $(LINC)/elf.h $(LINC)/elfFile.h
	g++ $(INC_PARAMS) $(S)/elfFile.cpp -c 

clean:
	rm -rf *.o simulator.sim testbench.sim
