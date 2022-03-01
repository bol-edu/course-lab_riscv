/*
 * DataMemory.h
 *
 *  Created on: Sep 15, 2017
 *      Author: emascare
 */

#ifndef SRC_DATAMEMORY_H_
#define SRC_DATAMEMORY_H_

#include "portability.h"

void mem_set(CORE_INT(8) memory[8192][4],
			 CORE_INT(32) address,
			 CORE_INT(32) value,
			 CORE_UINT(2) op)
{
#pragma HLS ARRAY_PARTITION variable=memory complete dim=2

	CORE_UINT(13) wrapped_address = address % 8192;
	CORE_UINT(2) offset = wrapped_address & 0x3;
	CORE_UINT(13) location = wrapped_address >> 2;
	
	if (offset == 0) {
		memory[location][0] = value.SLC(8, 0);
	}

	if (offset == 1) {
		memory[location][1] = value.SLC(8, 0);
	} else if (offset == 0 && (op & 1)) {
		memory[location][1] = value.SLC(8, 8);
	}

	if (op & 2) {
		memory[location][2] = value.SLC(8, 16);
	} else if (offset == 2) {
		memory[location][2] = value.SLC(8, 0);
	} else if (offset == 1 && (op & 1)) {
		memory[location][2] = value.SLC(8, 8);
	}

	if (op & 2) {
		memory[location][3] = value.SLC(8, 24);
	} else if (offset == 3) {
		memory[location][3] = value.SLC(8, 0);
	} else if (offset == 2 && (op & 1)) {
		memory[location][3] = value.SLC(8, 8);
	}

	// memory[location][offset] = value.SLC(8, 0);
	// if (op & 1) {
	// 	memory[location][offset+1] = value.SLC(8, 8);
	// }
	// if (op & 2) {
	// 	memory[location][2] = value.SLC(8, 16);
	// 	memory[location][3] = value.SLC(8, 24);
	// }
}

CORE_INT(32) mem_get(CORE_INT(8) memory[8192][4],
					 CORE_UINT(32) address,
					 CORE_UINT(2) op,
					 CORE_UINT(1) sign)
{
#pragma HLS ARRAY_PARTITION variable=memory complete dim=2
	CORE_UINT(13) wrapped_address = address % 8192;
	CORE_UINT(2) offset = wrapped_address & 0x3;
	CORE_UINT(13) location = wrapped_address >> 2;

	CORE_INT(32) result;
	result = sign ? -1 : 0;

	// CORE_INT(32) value = memory[location][0].SLC(32, 0);
	// CORE_INT(8) byte0 = value.SLC(8, 0);
	// CORE_INT(8) byte1 = value.SLC(8, 8);
	// CORE_INT(8) byte2 = value.SLC(8, 16);
	// CORE_INT(8) byte3 = value.SLC(8, 24);

	CORE_INT(8) byte0 = memory[location][0];
	CORE_INT(8) byte1 = memory[location][1];
	CORE_INT(8) byte2 = memory[location][2];
	CORE_INT(8) byte3 = memory[location][3];

	switch(offset) {
		case 0:
			break;
		case 1:
			byte0 = byte1;
			break;
		case 2:
			byte0 = byte2;
			byte1 = byte3;
			break;
		case 3:
			byte0 = byte3;
			break;
	}

	result.SET_SLC(0, byte0);
	if(op & 1) {
		result.SET_SLC(8, byte1);
	}
	if(op & 2) {
		result.SET_SLC(16, byte2);
		result.SET_SLC(24, byte3);
	}

	return result;
}

#endif /* SRC_DATAMEMORY_H_ */
