#ifndef PORTABILITY_H_
#define PORTABILITY_H_

#include <ap_int.h>
#define CORE_UINT(param) ap_uint<param>
#define CORE_INT(param) ap_int<param>
#define SLC(size, low) range(size + low - 1, low)
#define SET_SLC(low, value) range(low + value.length() - 1, low) = value

#include <hls_stream.h>
#include <ap_shift_reg.h>
#define STREAM(param) hls::stream<param>
#define SREAD() read()
#define SWRITE(value) write(value)

// #define STREAM(param) ap_shift_reg<param, 1>
// #define SREAD() read(0)
// #define SWRITE(value) shift(value, 0)

// #define STREAM(param) param

#endif /* For PORTABILITY_H_ */
