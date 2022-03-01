#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include "core.h"
#include "perf.h"
#include "portability.h"
#include "testbench.h"

using namespace std;

#define MEM_SIZE 8192

int main(int argc, char *argv[]) {
	CORE_INT(8) ins_arr[MEM_SIZE] = {0};
	CORE_INT(8) data_arr[MEM_SIZE / 4][4] = {0};
    CORE_INT(32) perf_arr[32] = {0};
    CORE_UINT(32) pc, pc_begin, pc_end;
    
    string dir = "";

#ifdef __TESTBENCH__
    if (argc > 2) {
        cout << "usage: ./testbench.sim [directory].\n";
        exit(1);
    }
    if (argc == 2) {
        dir = argv[1];
    }
#endif

    ifstream fin;
    // char* buffer = new char[12];
    string buffer = "";
    CORE_INT(32) num;

    CORE_INT(32)* ins_in = (CORE_INT(32)*)ins_arr;
    fin.open(dir + "ins.dat", ios::in);
	for (int i = 0; i < MEM_SIZE / 4; i++) {
        getline(fin, buffer);
#ifdef __TESTBENCH__
        num = stoi(buffer);
#else
        num = ins[i];
#endif
		ins_in[i] = num;
	}
	fin.close();

    CORE_INT(32)* data_in = (CORE_INT(32)*)data_arr;
	fin.open(dir + "data.dat", ios::in);
	for (int i = 0; i < MEM_SIZE / 4; i++) {
        getline(fin, buffer);
#ifdef __TESTBENCH__
        num = stoi(buffer);
#else
        num = data[i];
#endif
		data_in[i] = num;
	}
	fin.close();

    fin.open(dir + "pc.dat", ios::in);
    getline(fin, buffer);
#ifdef __TESTBENCH__
    num = stoi(buffer);
#else
    num = pc1;
#endif
    pc_begin = num;
    getline(fin, buffer);
#ifdef __TESTBENCH__
    num = stoi(buffer);
#else
    num = pc2;
#endif
    pc_end = num;
    fin.close();

    pc = pc_begin;
    CORE_UINT(32) pc_check;
    doStep(&pc, &pc_check, 0xffffffff, (CORE_INT(32)*)ins_arr, (CORE_INT(32)*)data_arr, perf_arr);
    
    cout << "exit at pc " << pc_check << ".\n";
    assert(pc_check == pc_end || pc_check - 8 == pc_end);

    fin.open(dir + "data_out_sim.dat", ios::in);
	for (int i = 0; i < MEM_SIZE / 4; i++) {
        getline(fin, buffer);
#ifdef __TESTBENCH__
        num = stoi(buffer);
#else
        num = data_o[i];
#endif
        if (data_in[i] != num) {
            cout << "data error at address " << i << endl;
            cout << "want: " << num << ", got: " << data_in[i] << endl;
            exit(1);
        }
	}
	fin.close();

    cout << endl;
	cout << "clock cycle: " << perf_arr[CLOCK_CYCLE] << endl;
	cout << "instruction count: " << perf_arr[INSTRUCTION_COUNT] << endl;
	cout << "branch non-taken & predict non-taken: " << perf_arr[BRANCH_NT_PDNT] << endl;
	cout << "branch non-taken & predict taken: " << perf_arr[BRANCH_NT_PDT] << endl;
	cout << "branch taken & predict non-taken: " << perf_arr[BRANCH_T_PDNT] << endl;
	cout << "branch taken & predict taken: " << perf_arr[BRANCH_T_PDT] << endl;
	cout << "pipeline stall: " << perf_arr[STALL_COUNT] << endl;
    cout << "pipeline flush: " << perf_arr[FLUSH_COUNT] << endl;

    return 0;
}
