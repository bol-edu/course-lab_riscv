#ifndef PORTABILITY_H_
#define PORTABILITY_H_

#include <ap_int.h>
#define CORE_UINT(param) ap_uint<param>
#define CORE_INT(param) ap_int<param>
#define SLC(size, low) range(size + low - 1, low)
#define SET_SLC(low, value) range(low + value.length() - 1, low) = value

#endif /* For PORTABILITY_H_ */
