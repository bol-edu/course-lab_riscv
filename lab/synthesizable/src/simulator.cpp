#include "elfFile.h"
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include "core.h"
#include "perf.h"
#include "portability.h"

using namespace std;

#define MEM_SIZE 8192

int main(int argc, char *argv[]) {
	CORE_INT(8) ins_arr[MEM_SIZE] = {0};
	CORE_INT(8) data_arr[MEM_SIZE] = {0};
    CORE_INT(32) perf_arr[32] = {0};
    map<CORE_UINT(32), bool> ins_used;
	map<CORE_UINT(32), bool> data_used;
    CORE_UINT(32) addr;
    CORE_INT(8) value;
    CORE_UINT(32) pc, pc_begin, pc_end;

    string dir = "";
    if (argc < 2 || argc > 3) {
        cout << "usage: ./vivado.sim <elf file> [output directory].\n";
        exit(1);
    }
    if (argc == 3) {
        dir = argv[2];
    }

    char* binaryFile = argv[1];
    ElfFile elfFile(binaryFile);

    for (unsigned int sectionCounter = 0; sectionCounter < elfFile.sectionTable->size(); sectionCounter++) {
        ElfSection *oneSection = elfFile.sectionTable->at(sectionCounter);
        if(oneSection->address != 0 && oneSection->getName().compare(".text")){
            //If the address is not null we place its content into memory
            unsigned char* sectionContent = oneSection->getSectionCode();
            for (unsigned int byteNumber = 0; byteNumber < oneSection->size; byteNumber++) {
                addr = (oneSection->address + byteNumber) % MEM_SIZE;
                value = sectionContent[byteNumber];
                if (data_used[addr]) {
                    cout << "Error! Data memory size exceeded" << endl;
	                exit(-1);
                }
                data_arr[addr] = value;
                data_used[addr] = true;
			}
        }

        if (!oneSection->getName().compare(".text")){
        	unsigned char* sectionContent = oneSection->getSectionCode();
            for (unsigned int byteNumber = 0; byteNumber < oneSection->size; byteNumber++){
                addr = (oneSection->address + byteNumber) % MEM_SIZE;
                value = sectionContent[byteNumber];
                if (ins_used[addr]) {
                    cout << "Error! Instruction memory size exceeded" << endl;
	                exit(-1);
                }
                ins_arr[addr] = value;
                ins_used[addr] = true;
			}
    	}
    }

    for (int oneSymbol = 0; oneSymbol < elfFile.symbols->size(); oneSymbol++) {
		ElfSymbol *symbol = elfFile.symbols->at(oneSymbol);
		const char* name = (const char*) &(elfFile.sectionTable->at(elfFile.indexOfSymbolNameSection)->getSectionCode()[symbol->name]);
	    if (strcmp(name, "_start") == 0) {
            pc_begin = symbol->offset;
			cout << "pc_begin: " << pc_begin << endl;
		}
        if (strcmp(name, "_exit") == 0) {
            pc_end = symbol->offset + 0xc;
			cout << "pc_end: " << pc_end << endl;
		}
	}

    fstream fout;

    CORE_INT(32)* ins_out = (CORE_INT(32)*)ins_arr;
    fout.open(dir + "ins.dat", ios::out);
	for (int i = 0; i < MEM_SIZE / 4; i++) {
		fout << ins_out[i] << "\n";
	}
	fout.close();

    CORE_INT(32)* data_out = (CORE_INT(32)*)data_arr;
	fout.open(dir + "data.dat", ios::out);
	for (int i = 0; i < MEM_SIZE / 4; i++) {
		fout << data_out[i] << "\n";
	}
	fout.close();

	fout.open(dir + "pc.dat", ios::out);
	fout << pc_begin << "\n";
    fout << pc_end << "\n";
	fout.close();

	cout << "Continue C simulation? [y/n]:";
	char c;
	cin >> c;
	if (c != 'y') return 0;

    pc = pc_begin;
    CORE_UINT(32) pc_check;
    doStep(&pc, &pc_check, 0xffffffff, (CORE_INT(32)*)ins_arr, (CORE_INT(32)*)data_arr, perf_arr);

    cout << "exit at pc " << pc_check << ".\n";
	assert(pc_check == pc_end || pc_check - 8 == pc_end);

    fout.open(dir + "data_out_sim.dat", ios::out);
	for (int i = 0; i < MEM_SIZE / 4; i++) {
		fout << (CORE_INT(32))data_out[i] << "\n";
	}
	fout.close();

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
