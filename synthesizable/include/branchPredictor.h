#ifndef _BRANCH_PREDICTOR_H_
#define _BRANCH_PREDICTOR_H_

#include "portability.h"

template <bool BRANCH>
class NaivePredictor {
public:
    CORE_UINT(32) record[2][2] = {0};
    NaivePredictor() {}
    CORE_UINT(1) _predict(CORE_UINT(32) pc) {
        return BRANCH;
    }
    void _update(CORE_UINT(32) pc, CORE_UINT(1) predict, CORE_UINT(1) result) { record[predict][result]++; }
};

template <int LOG2_ENTRY>
class TwoBitsPredictor {
    CORE_UINT(2) status_table[1 << LOG2_ENTRY];
    CORE_UINT(2) MAX = (1 << 2) - 1;
    CORE_UINT(2) MIN = 0;
public:
    CORE_UINT(32) record[2][2] = {0};
    TwoBitsPredictor() {
        for (int i = 0; i < (1 << LOG2_ENTRY); i++) {
            status_table[i] = MAX;
        }
    }
    CORE_UINT(1) _predict(CORE_UINT(32) pc) {
        return ((status_table[pc & ((1 << LOG2_ENTRY) - 1)] & (1 << (2 - 1))) != 0);
    }
    void _update(CORE_UINT(32) pc, CORE_UINT(1) predict, CORE_UINT(1) result) {
#pragma HLS ARRAY_PARTITION variable = status_table complete
        CORE_UINT(1) entry = pc & ((1 << LOG2_ENTRY) - 1);
        if (result && status_table[entry] != MAX) status_table[entry]++;
        if (!result && status_table[entry] != MIN) status_table[entry]--;
        record[predict][result]++;
    }
};

using BranchPredictor = TwoBitsPredictor<3>;
// using BranchPredictor = NaivePredictor<1>;

#endif /* _BRANCH_PREDICTOR_H_ */
