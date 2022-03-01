#include "riscvISA.h"
#include "registers.h"
#include "core.h"
#include "portability.h"
#include "perf.h"
#include "branchPredictor.h"

using namespace std;

#define MEM_SIZE 8192

#define print_simulator_output(...)
#define EX_SYS_CALL()            \
    case RISCV_SYSTEM:           \
        extoMem->sys_status = 1; \
        break;
#define DC_SYS_CALL()
#define WB_SYS_CALL()              \
    if (memtoWB.sys_status == 1) { \
        *early_exit = 1;           \
    }

#ifdef __DEBUG__
#define print_debug(...) PrintDebugStatements(__VA_ARGS__)
#define nl() PrintNewLine()
#else
#define print_debug(...)
#define nl()
#endif

#include "DataMemory.h"
#define MEM_SET(memory, address, value, op) mem_set(memory, address, value, op)
#define MEM_GET(memory, address, op, sign) mem_get(memory, address, op, sign)

CORE_INT(32) REG[32]; // Register file

void Ft(CORE_UINT(32) * pc,
        CORE_INT(32) ins_memory[MEM_SIZE / 4],
        struct FtoDC* ftoDC,
        CORE_UINT(1) _extoFt_jump,
        CORE_UINT(32) _extoFt_pc,
        CORE_UINT(1) _dctoFt_stall,
        CORE_INT(32) * insts) {
#pragma HLS inline off
#pragma HLS LATENCY max = 1 min = 1

    CORE_UINT(32) next_pc = *pc;

    if (!_dctoFt_stall) next_pc += 4;
    *pc = _extoFt_jump ? _extoFt_pc : next_pc;
    if (!_dctoFt_stall) {
        (ftoDC->instruction).SET_SLC(0, ins_memory[(*pc & 0x0FFFF) / 4]);
        ftoDC->pc = *pc;
        // debugger->set(debugger_index, ftoDC->pc, 15);
        (*insts)++;
    }
}

void DC(struct FtoDC ftoDC,
        struct DCtoEx* dctoEx,
        CORE_UINT(7) * prev_opCode,
        CORE_UINT(32) * prev_pc,
        CORE_UINT(5) * prev_dest,
        CORE_UINT(32) * prev_instruction,
        CORE_UINT(1) _extoDc_flush,
        CORE_UINT(1) * dctoDc_stall,
        CORE_UINT(1) _dctoDc_stall_out,
        CORE_UINT(1) & dctoFt_stall,
        CORE_UINT(5) _wbtoDc_reg_dest,
        CORE_INT(32) _wbtoDc_reg_value,
        CORE_INT(32) * stalls,
        CORE_INT(32) * flushs) {
// #pragma HLS inline off
// #pragma HLS PIPELINE
#pragma HLS LATENCY max = 1 min = 1

    if (_extoDc_flush) {
        ftoDC.instruction = NOP;
        _dctoDc_stall_out = 0;
        (*flushs) += 2;
    }
    if (_dctoDc_stall_out) {
        ftoDC.instruction = *prev_instruction;
        ftoDC.pc = *prev_pc;
    }

    CORE_UINT(5) rs1 = ftoDC.instruction.SLC(5, 15); // Decoding the instruction, in the DC stage
    CORE_UINT(5) rs2 = ftoDC.instruction.SLC(5, 20);
    CORE_UINT(5) rd = ftoDC.instruction.SLC(5, 7);
    CORE_UINT(7) opcode = ftoDC.instruction.SLC(7, 0);
    CORE_UINT(7) funct7 = ftoDC.instruction.SLC(7, 25);
    CORE_UINT(7) funct3 = ftoDC.instruction.SLC(3, 12);
    CORE_UINT(7) funct7_smaller = 0;
    funct7_smaller.SET_SLC(1, ftoDC.instruction.SLC(6, 26));
    CORE_UINT(6) shamt = ftoDC.instruction.SLC(6, 20);

    CORE_UINT(13) imm13 = 0;
    imm13[12] = ftoDC.instruction[31];
    imm13.SET_SLC(5, ftoDC.instruction.SLC(6, 25));
    imm13.SET_SLC(1, ftoDC.instruction.SLC(4, 8));
    imm13[11] = ftoDC.instruction[7];

    CORE_INT(13) imm13_signed = 0;
    imm13_signed.SET_SLC(0, imm13);
    CORE_UINT(12) imm12_I = ftoDC.instruction.SLC(12, 20);
    CORE_INT(12) imm12_I_signed = ftoDC.instruction.SLC(12, 20);

    CORE_UINT(21) imm21_1 = 0;
    imm21_1.SET_SLC(12, ftoDC.instruction.SLC(8, 12));
    imm21_1[11] = ftoDC.instruction[20];
    imm21_1.SET_SLC(1, ftoDC.instruction.SLC(10, 21));
    imm21_1[20] = ftoDC.instruction[31];

    CORE_INT(21) imm21_1_signed = 0;
    imm21_1_signed.SET_SLC(0, imm21_1);

    CORE_INT(32) imm31_12 = 0;
    imm31_12.SET_SLC(12, ftoDC.instruction.SLC(20, 12));

    CORE_UINT(1) forward_rs1 = 0;
    CORE_UINT(1) forward_rs2 = 0;
    CORE_UINT(1) forward_ex_or_mem_rs1 = 0;
    CORE_UINT(1) forward_ex_or_mem_rs2 = 0;
    CORE_UINT(1) datab_fwd = 0;

    CORE_INT(12) store_imm = 0;
    store_imm.SET_SLC(0, ftoDC.instruction.SLC(5, 7));
    store_imm.SET_SLC(5, ftoDC.instruction.SLC(7, 25));

#pragma HLS ARRAY_PARTITION variable = REG complete

    CORE_INT(32) reg_rs1 = (rs1 != 0 && rs1 == _wbtoDc_reg_dest) ? _wbtoDc_reg_value : REG[rs1];
    CORE_INT(32) reg_rs2 = (rs2 != 0 && rs2 == _wbtoDc_reg_dest) ? _wbtoDc_reg_value : REG[rs2];
    if (_wbtoDc_reg_dest != 0) {
        REG[_wbtoDc_reg_dest] = _wbtoDc_reg_value;
    }

    dctoEx->opCode = opcode;
    dctoEx->funct3 = funct3;
    dctoEx->funct7_smaller = funct7_smaller;
    dctoEx->funct7 = funct7;
    dctoEx->shamt = shamt;
    dctoEx->rs1 = rs1;
    dctoEx->rs2 = 0;
    dctoEx->pc = ftoDC.pc;

    switch (opcode) {
        case RISCV_LUI:
            dctoEx->dest = rd;
            dctoEx->datab = imm31_12;
            break;
        case RISCV_AUIPC:
            dctoEx->dest = rd;
            dctoEx->datab = imm31_12;
            break;
        case RISCV_JAL:
            dctoEx->dest = rd;
            dctoEx->datab = imm21_1_signed;
            break;
        case RISCV_JALR:
            dctoEx->dest = rd;
            dctoEx->datab = imm12_I_signed;
            break;
        case RISCV_BR:
            dctoEx->rs2 = rs2;
            datab_fwd = 1;
            dctoEx->datac = imm13_signed;
            dctoEx->dest = 0;
            break;
        case RISCV_LD:
            dctoEx->dest = rd;
            dctoEx->memValue = imm12_I_signed;
            break;
        case RISCV_ST:
            dctoEx->rs2 = rs2;
            dctoEx->memValue = store_imm;
            dctoEx->dest = 0;
            break;
        case RISCV_OPI:
            dctoEx->dest = rd;
            dctoEx->memValue = imm12_I_signed;
            dctoEx->datab = imm12_I;
            break;
        case RISCV_OP:
            dctoEx->rs2 = rs2;
            datab_fwd = 1;
            dctoEx->dest = rd;
            break;
            DC_SYS_CALL()
    }

    dctoEx->datab_fwd = datab_fwd;

    dctoEx->dataa = reg_rs1;
    if (opcode == RISCV_ST) {
        dctoEx->datac = reg_rs2;
    }
    if (datab_fwd) {
        dctoEx->datab = reg_rs2;
    }

    CORE_UINT(1) _dctoDc_stall_in = 0;
    CORE_UINT(1) _dctoFt_stall = 0;

    if (*prev_opCode == RISCV_LD && (dctoEx->rs1 == *prev_dest || dctoEx->rs2 == *prev_dest)) {
        _dctoDc_stall_in = 1;
        _dctoFt_stall = 1;
        dctoEx->pc = 0;
        dctoEx->dataa = 0;
        dctoEx->datab = 0;
        dctoEx->datac = 0;
        dctoEx->datad = 0;
        dctoEx->datae = 0;
        dctoEx->dest = 0;
        dctoEx->opCode = 0;
        dctoEx->memValue = 0;
        dctoEx->funct3 = 0;
        dctoEx->funct7 = 0;
        dctoEx->funct7_smaller = 0;
        dctoEx->shamt = 0;
        dctoEx->rs1 = 0;
        dctoEx->rs2 = 0;
        dctoEx->datab_fwd = 0;

        opcode = 0;
        (*stalls)++;
    }

    *prev_opCode = opcode;
    *prev_pc = ftoDC.pc;
    *prev_dest = dctoEx->dest;
    *prev_instruction = ftoDC.instruction;

    *dctoDc_stall = _dctoDc_stall_in;
    dctoFt_stall = _dctoFt_stall;
}

void Ex(struct DCtoEx dctoEx,
        struct ExtoMem* extoMem,
        CORE_UINT(1) & extoFt_jump,
        CORE_UINT(32) & extoFt_pc,
        CORE_UINT(1) & extoDc_flush,
        CORE_UINT(1) * extoEx_flush,
        CORE_UINT(1) _extoEx_flush_out,
        CORE_UINT(5) * extoEx_fwd_dest,
        CORE_UINT(5) _extoEx_fwd_dest_out,
        CORE_INT(32) * extoEx_fwd_value,
        CORE_INT(32) _extoEx_fwd_value_out,
        CORE_UINT(1) * extoEx_fwd_load,
        CORE_UINT(1) _extoEx_fwd_load_out,
        CORE_UINT(5) _memtoEx_fwd_dest_out,
        CORE_INT(32) _memtoEx_fwd_value_out,
        CORE_UINT(5) _wbtoEx_fwd_dest_out,
        CORE_INT(32) _wbtoEx_fwd_value_out) {
// #pragma HLS inline off
// #pragma HLS PIPELINE
#pragma HLS LATENCY max = 1 min = 1

    if (_extoEx_flush_out) {
        dctoEx.dataa = 0;
        dctoEx.datab = 0;
        dctoEx.dest = 0;
        dctoEx.opCode = RISCV_OPI;
        dctoEx.memValue = 0;
        dctoEx.funct3 = RISCV_OPI_ADDI;
    }

    CORE_UINT(1) _extoEx_flush_in = 0;

    if (dctoEx.rs1 != 0) {
        if (dctoEx.rs1 == _extoEx_fwd_dest_out && !_extoEx_fwd_load_out) {
            dctoEx.dataa = _extoEx_fwd_value_out;
        } else if (dctoEx.rs1 == _memtoEx_fwd_dest_out) {
            dctoEx.dataa = _memtoEx_fwd_value_out;
        } else if (dctoEx.rs1 == _wbtoEx_fwd_dest_out) {
            dctoEx.dataa = _wbtoEx_fwd_value_out;
        }
    }
    if (dctoEx.rs2 != 0) {
        if (dctoEx.rs2 == _extoEx_fwd_dest_out && !_extoEx_fwd_load_out) {
            if (dctoEx.opCode == RISCV_ST) {
                dctoEx.datac = _extoEx_fwd_value_out;
            }
            if (dctoEx.datab_fwd) {
                dctoEx.datab = _extoEx_fwd_value_out;
            }
        } else if (dctoEx.rs2 == _memtoEx_fwd_dest_out) {
            if (dctoEx.opCode == RISCV_ST) {
                dctoEx.datac = _memtoEx_fwd_value_out;
            }
            if (dctoEx.datab_fwd) {
                dctoEx.datab = _memtoEx_fwd_value_out;
            }
        } else if (dctoEx.rs2 == _wbtoEx_fwd_dest_out) {
            if (dctoEx.opCode == RISCV_ST) {
                dctoEx.datac = _wbtoEx_fwd_value_out;
            }
            if (dctoEx.datab_fwd) {
                dctoEx.datab = _wbtoEx_fwd_value_out;
            }
        }
    }

    CORE_UINT(32) unsignedReg1 = 0;
    unsignedReg1.SET_SLC(0, (dctoEx.dataa).SLC(32, 0));
    CORE_UINT(32) unsignedReg2 = 0;
    unsignedReg2.SET_SLC(0, (dctoEx.datab).SLC(32, 0));
    CORE_INT(33) mul_reg_a = 0;
    CORE_INT(33) mul_reg_b = 0;
    CORE_INT(66) longResult = 0;
    CORE_INT(33) srli_reg = 0;
    CORE_INT(33) srli_result = 0; // Execution of the Instruction in EX stage

    extoMem->opCode = dctoEx.opCode;
    extoMem->dest = dctoEx.dest;
    extoMem->datac = dctoEx.datac;
    extoMem->funct3 = dctoEx.funct3;
    extoMem->sys_status = 0;

    CORE_UINT(1) _extoFt_jump = 0;
    CORE_UINT(32) _extoFt_pc = 0;
    CORE_UINT(1) _extoDc_flush = 0;

    if ((extoMem->opCode != RISCV_BR) && (extoMem->opCode != RISCV_ST)) {
        extoMem->WBena = 1;
    } else {
        extoMem->WBena = 0;
    }

    switch (dctoEx.opCode) {
        case RISCV_LUI:
            extoMem->result = dctoEx.datab;
            break;
        case RISCV_AUIPC:
            extoMem->result = dctoEx.pc + dctoEx.datab;
            break;
        case RISCV_JAL:
            extoMem->result = dctoEx.pc + 4;
            extoMem->memValue = dctoEx.pc + dctoEx.datab;
            _extoFt_jump = 1;
            _extoFt_pc = extoMem->memValue;
            _extoDc_flush = 1;
            _extoEx_flush_in = 1;
            break;
        case RISCV_JALR:
            extoMem->result = dctoEx.pc + 4;
            extoMem->memValue = (dctoEx.dataa + dctoEx.datab) & 0xfffffffe;
            _extoFt_jump = 1;
            _extoFt_pc = extoMem->memValue;
            _extoDc_flush = 1;
            _extoEx_flush_in = 1;
            break;
        case RISCV_BR: // Switch case for branch instructions
            switch (dctoEx.funct3) {
                case RISCV_BR_BEQ:
                    extoMem->result = (dctoEx.dataa == dctoEx.datab);
                    break;
                case RISCV_BR_BNE:
                    extoMem->result = (dctoEx.dataa != dctoEx.datab);
                    break;
                case RISCV_BR_BLT:
                    extoMem->result = (dctoEx.dataa < dctoEx.datab);
                    break;
                case RISCV_BR_BGE:
                    extoMem->result = (dctoEx.dataa >= dctoEx.datab);
                    break;
                case RISCV_BR_BLTU:
                    extoMem->result = (unsignedReg1 < unsignedReg2);
                    break;
                case RISCV_BR_BGEU:
                    extoMem->result = (unsignedReg1 >= unsignedReg2);
                    break;
            }
            extoMem->memValue = dctoEx.pc + dctoEx.datac;
            _extoFt_jump = extoMem->result;
            _extoFt_pc = extoMem->memValue;
            _extoDc_flush = extoMem->result;
            _extoEx_flush_in = extoMem->result;
            break;
        case RISCV_LD:
            extoMem->result = (dctoEx.dataa + dctoEx.memValue);
            break;
        case RISCV_ST:
            extoMem->result = (dctoEx.dataa + dctoEx.memValue);
            extoMem->datac = dctoEx.datac;
            break;
        case RISCV_OPI:
            switch (dctoEx.funct3) {
                case RISCV_OPI_ADDI:
                    extoMem->result = dctoEx.dataa + dctoEx.memValue;
                    break;
                case RISCV_OPI_SLTI:
                    extoMem->result = (dctoEx.dataa < dctoEx.memValue) ? 1 : 0;
                    break;
                case RISCV_OPI_SLTIU:
                    extoMem->result = (unsignedReg1 < dctoEx.datab) ? 1 : 0;
                    break;
                case RISCV_OPI_XORI:
                    extoMem->result = dctoEx.dataa ^ dctoEx.memValue;
                    break;
                case RISCV_OPI_ORI:
                    extoMem->result = dctoEx.dataa | dctoEx.memValue;
                    break;
                case RISCV_OPI_ANDI:
                    extoMem->result = dctoEx.dataa & dctoEx.memValue;
                    break;
                case RISCV_OPI_SLLI:
                    extoMem->result = dctoEx.dataa << dctoEx.shamt;
                    break;
                case RISCV_OPI_SRI:
                    if (dctoEx.funct7_smaller == RISCV_OPI_SRI_SRLI) {
                        srli_reg.SET_SLC(0, dctoEx.dataa);
                        srli_result = srli_reg >> dctoEx.shamt;
                        extoMem->result = srli_result.SLC(32, 0);
                    } else // SRAI
                        extoMem->result = dctoEx.dataa >> dctoEx.shamt;
                    break;
            }
            break;
        case RISCV_OP:
            if (dctoEx.funct7 == 1) {
                mul_reg_a = dctoEx.dataa;
                mul_reg_b = dctoEx.datab;
                mul_reg_a[32] = dctoEx.dataa[31];
                mul_reg_b[32] = dctoEx.datab[31];
                switch (dctoEx.funct3) { // Switch case for multiplication operations (RV32M)
                    case RISCV_OP_M_MULHSU:
                        mul_reg_b[32] = 0;
                        break;
                    case RISCV_OP_M_MULHU:
                        mul_reg_a[32] = 0;
                        mul_reg_b[32] = 0;
                        break;
                }
                longResult = mul_reg_a * mul_reg_b;
                if (dctoEx.funct3 == RISCV_OP_M_MUL) {
                    extoMem->result = longResult.SLC(32, 0);
                } else {
                    extoMem->result = longResult.SLC(32, 32);
                }
            } else {
                switch (dctoEx.funct3) {
                    case RISCV_OP_ADD:
                        if (dctoEx.funct7 == RISCV_OP_ADD_ADD)
                            extoMem->result = dctoEx.dataa + dctoEx.datab;
                        else
                            extoMem->result = dctoEx.dataa - dctoEx.datab;
                        break;
                    case RISCV_OP_SLL:
                        extoMem->result = dctoEx.dataa << (dctoEx.datab & 0x3f);
                        break;
                    case RISCV_OP_SLT:
                        extoMem->result = (dctoEx.dataa < dctoEx.datab) ? 1 : 0;
                        break;
                    case RISCV_OP_SLTU:
                        extoMem->result = (unsignedReg1 < unsignedReg2) ? 1 : 0;
                        break;
                    case RISCV_OP_XOR:
                        extoMem->result = dctoEx.dataa ^ dctoEx.datab;
                        break;
                    case RISCV_OP_OR:
                        extoMem->result = dctoEx.dataa | dctoEx.datab;
                        break;
                    case RISCV_OP_AND:
                        extoMem->result = dctoEx.dataa & dctoEx.datab;
                        break;
                }
            }
            break;
            EX_SYS_CALL()
    }

    // if (_extoEx_flush_out) {
    //     extoMem->WBena = 0;
    // }

    CORE_UINT(5) _extoEx_fwd_dest_in = extoMem->WBena ? extoMem->dest : (CORE_UINT(5))0;
    CORE_INT(32) _extoEx_fwd_value_in = extoMem->WBena ? extoMem->result : (CORE_INT(32))0;
    CORE_UINT(1)
    _extoEx_fwd_load_in =
        extoMem->WBena ? (CORE_UINT(1))(dctoEx.opCode == RISCV_ST) : (CORE_UINT(1))0;

    extoFt_jump = _extoFt_jump;
    extoFt_pc = _extoFt_pc;
    extoDc_flush = _extoDc_flush;
    *extoEx_flush = _extoEx_flush_in;
    *extoEx_fwd_dest = _extoEx_fwd_dest_in;
    *extoEx_fwd_value = _extoEx_fwd_value_in;
    *extoEx_fwd_load = _extoEx_fwd_load_in;
}

void do_Mem(CORE_INT(8) data_memory[MEM_SIZE / 4][4],
            struct ExtoMem extoMem,
            struct MemtoWB* memtoWB,
            CORE_UINT(5) & memtoEx_fwd_dest,
            CORE_INT(32) & memtoEx_fwd_value) {
// #pragma HLS inline off
#pragma HLS LATENCY max = 1 min = 1
#pragma HLS DEPENDENCE variable = data_memory inter false

    memtoWB->sys_status = extoMem.sys_status;
    memtoWB->result = extoMem.result;
    CORE_INT(32) result = 0;
    CORE_UINT(2) st_op = 0;
    CORE_UINT(2) ld_op = 0;
    CORE_UINT(1) sign = 0;

    memtoWB->WBena = extoMem.WBena;
    memtoWB->dest = extoMem.dest; // Memory operaton in do_Mem stage
    switch (extoMem.opCode) {
        case RISCV_BR:
            memtoWB->WBena = 0;
            memtoWB->dest = 0;
            break;
        case RISCV_JAL:
            break;
        case RISCV_JALR:
            break;
        case RISCV_LD:
            switch (extoMem.funct3) {
                case RISCV_LD_LW:
                    ld_op = 3;
                    sign = 1;
                    break;
                case RISCV_LD_LH:
                    ld_op = 1;
                    sign = 1;
                    break;
                case RISCV_LD_LHU:
                    ld_op = 1;
                    sign = 0;
                    break;
                case RISCV_LD_LB:
                    ld_op = 0;
                    sign = 1;
                    break;
                case RISCV_LD_LBU:
                    ld_op = 1;
                    sign = 0;
                    break;
            }
            memtoWB->result = MEM_GET(data_memory, memtoWB->result, ld_op, sign);
            break;
        case RISCV_ST:
            switch (extoMem.funct3) {
                case RISCV_ST_STW:
                    st_op = 3;
                    break;
                case RISCV_ST_STH:
                    st_op = 1;
                    break;
                case RISCV_ST_STB:
                    st_op = 0;
                    break;
            }
            MEM_SET(data_memory, memtoWB->result, extoMem.datac, st_op);
            break;
    }

    CORE_UINT(5) _memtoEx_fwd_dest = memtoWB->WBena ? memtoWB->dest : (CORE_UINT(5))0;
    CORE_INT(32) _memtoEx_fwd_value = memtoWB->WBena ? memtoWB->result : (CORE_INT(32))0;

    memtoEx_fwd_dest = _memtoEx_fwd_dest;
    memtoEx_fwd_value = _memtoEx_fwd_value;
}

void doWB(struct MemtoWB memtoWB,
          CORE_UINT(5) & wbtoDc_reg_dest,
          CORE_INT(32) & wbtoDc_reg_value,
          CORE_UINT(5) & wbtoEx_fwd_dest,
          CORE_INT(32) & wbtoEx_fwd_value,
          CORE_UINT(1) * early_exit) {
#pragma HLS INLINE off
    CORE_UINT(5) _dest = 0;
    CORE_INT(32) _value = 0;
    if (memtoWB.WBena == 1 && memtoWB.dest != 0) {
        _dest = memtoWB.dest % 32;
        _value = memtoWB.result;
    }
    WB_SYS_CALL()
    wbtoDc_reg_dest = _dest;
    wbtoDc_reg_value = _value;
    wbtoEx_fwd_dest = _dest;
    wbtoEx_fwd_value = _value;
}

void doStep(CORE_UINT(32) * pc_in,
            CORE_UINT(32) * pc_out,
            CORE_UINT(32) nbcycle,
            CORE_INT(32) ins_memory_in[2048],
            CORE_INT(32) data_memory_in_out[2048],
            CORE_INT(32) perf_arr[32]) { //, CORE_INT(32) debug_arr[200]) {
#pragma HLS INTERFACE s_axilite port = pc_in
#pragma HLS INTERFACE s_axilite port = pc_out
#pragma HLS INTERFACE s_axilite port = nbcycle
#pragma HLS INTERFACE s_axilite port = return
#pragma HLS INTERFACE m_axi port = ins_memory_in
#pragma HLS INTERFACE m_axi port = data_memory_in_out
#pragma HLS INTERFACE m_axi port = perf_arr depth = 32 offset = slave

    CORE_UINT(32) pc = *pc_in - 4;

    CORE_INT(8) data_memory[MEM_SIZE / 4][4];

    for (int i = 0; i < MEM_SIZE / 4; i++) {
#pragma HLS PIPELINE
    	CORE_INT(32) data = data_memory_in_out[i];
    	data_memory[i][0] = data.SLC(8, 0);
    	data_memory[i][1] = data.SLC(8, 8);
    	data_memory[i][2] = data.SLC(8, 16);
    	data_memory[i][3] = data.SLC(8, 24);
    }

    CORE_INT(32) ins_memory[MEM_SIZE / 4];
    for (int i = 0; i < MEM_SIZE / 4; i++) {
#pragma HLS PIPELINE
        ins_memory[i] = ins_memory_in[i];
    }

    CORE_UINT(1) extoFt_jump = 0, _extoFt_jump;
    CORE_UINT(32) extoFt_pc = 0, _extoFt_pc;
    CORE_UINT(1) extoDc_flush = 0, _extoDc_flush;
    CORE_UINT(1) extoEx_flush = 0;
    CORE_UINT(5) extoEx_fwd_dest = 0;
    CORE_INT(32) extoEx_fwd_value = 0;
    CORE_UINT(1) extoEx_fwd_load = 0;
    CORE_UINT(5) memtoEx_fwd_dest = 0, _memtoEx_fwd_dest;
    CORE_INT(32) memtoEx_fwd_value = 0, _memtoEx_fwd_value;
    CORE_UINT(1) dctoDc_stall = 0;
    CORE_UINT(1) dctoFt_stall = 0, _dctoFt_stall;
    CORE_UINT(5) wbtoDc_reg_dest = 0, _wbtoDc_reg_dest;
    CORE_INT(32) wbtoDc_reg_value = 0, _wbtoDc_reg_value;
    CORE_UINT(5) wbtoEx_fwd_dest = 0, _wbtoEx_fwd_dest;
    CORE_INT(32) wbtoEx_fwd_value = 0, _wbtoEx_fwd_value;
    CORE_INT(32) insts = 0;
    CORE_INT(32) stalls = 0;
    CORE_INT(32) flushs = 0;

    struct MemtoWB memtoWB = {.result = 0, .dest = 0, .WBena = 0, .sys_status = 0};
    struct ExtoMem extoMem = {.result = 0,
                              .datac = 0,
                              .dest = 0,
                              .WBena = 0,
                              .opCode = 0,
                              .memValue = 0,
                              .funct3 = 0,
                              .sys_status = 0};
    struct DCtoEx dctoEx = {.pc = 0,
                            .dataa = 0,
                            .datab = 0,
                            .datac = 0,
                            .datad = 0,
                            .datae = 0,
                            .dest = 0,
                            .opCode = 0,
                            .memValue = 0,
                            .funct3 = 0,
                            .funct7 = 0,
                            .funct7_smaller = 0,
                            .shamt = 0,
                            .rs1 = 0,
                            .rs2 = 0,
                            .datab_fwd = 0};
    struct FtoDC ftoDC = {.pc = 0, .instruction = 0};

    CORE_UINT(32) n_inst = 0;
    CORE_UINT(7) prev_opCode = 0;
    CORE_UINT(32) prev_pc = 0;
    CORE_UINT(5) prev_dest = 0;
    CORE_UINT(32) prev_instruction = 0;

#ifdef __DEBUG__
    CORE_UINT(32) debug_pc = 0;
    int reg_print_halt = 0;
#endif

    CORE_UINT(1) early_exit = 0;

    for (int i = 0; i < 32; i++) {
#pragma HLS PIPELINE
        REG[i] = 0;
    }

    REG[2] = 0xf00000;

doStep_label1:
    while (n_inst < nbcycle && !early_exit) {
#ifdef __DEBUG__
        print_debug(n_inst, ";", std::hex, (int)pc, ";", (int)ins_memory[(pc & 0x0FFFF) / 4], " ");
#endif

        doWB(memtoWB, _wbtoDc_reg_dest, _wbtoDc_reg_value, _wbtoEx_fwd_dest, _wbtoEx_fwd_value,
             &early_exit);

        do_Mem(data_memory, extoMem, &memtoWB, _memtoEx_fwd_dest, _memtoEx_fwd_value);

        Ex(dctoEx, &extoMem, _extoFt_jump, _extoFt_pc, _extoDc_flush, &extoEx_flush, extoEx_flush,
           &extoEx_fwd_dest, extoEx_fwd_dest, &extoEx_fwd_value, extoEx_fwd_value, &extoEx_fwd_load,
           extoEx_fwd_load, memtoEx_fwd_dest, memtoEx_fwd_value, wbtoEx_fwd_dest, wbtoEx_fwd_value);

        DC(ftoDC, &dctoEx, &prev_opCode, &prev_pc, &prev_dest, &prev_instruction, extoDc_flush,
           &dctoDc_stall, dctoDc_stall, _dctoFt_stall, wbtoDc_reg_dest, wbtoDc_reg_value, &stalls,
           &flushs);

        Ft(&pc, ins_memory, &ftoDC, extoFt_jump, extoFt_pc, dctoFt_stall, &insts);

        memtoEx_fwd_dest = _memtoEx_fwd_dest;
        memtoEx_fwd_value = _memtoEx_fwd_value;
        wbtoEx_fwd_dest = _wbtoEx_fwd_dest;
        wbtoEx_fwd_value = _wbtoEx_fwd_value;

        extoDc_flush = _extoDc_flush;
        wbtoDc_reg_dest = _wbtoDc_reg_dest;
        wbtoDc_reg_value = _wbtoDc_reg_value;

        extoFt_jump = _extoFt_jump;
        extoFt_pc = _extoFt_pc;
        dctoFt_stall = _dctoFt_stall;

        n_inst++;

#ifdef __DEBUG__
        for (i = 0; i < 32; i++) {
            print_debug(";", std::hex, (int)REG[i]);
        }
        nl();
#endif
    }

    *pc_out = pc;

    for (int i = 0; i < MEM_SIZE / 4; i++) {
#pragma HLS PIPELINE
    	CORE_INT(32) data;
    	data.SET_SLC(0, data_memory[i][0]);
    	data.SET_SLC(8, data_memory[i][1]);
    	data.SET_SLC(16, data_memory[i][2]);
    	data.SET_SLC(24, data_memory[i][3]);
    	data_memory_in_out[i] = data;
    }

    /*for (i = 0; i < 32; i++) {
            #pragma HLS PIPELINE
            debug_arr[i].SET_SLC(0, REG[i]);
    }*/

    perf_arr[CLOCK_CYCLE] = n_inst;
    perf_arr[INSTRUCTION_COUNT] = insts;
    // perf_arr[BRANCH_NT_PDNT] = bp.record[0][0];
    // perf_arr[BRANCH_NT_PDT] = bp.record[1][0];
    // perf_arr[BRANCH_T_PDNT] = bp.record[0][1];
    // perf_arr[BRANCH_T_PDT] = bp.record[1][1];
    perf_arr[STALL_COUNT] = stalls;
    perf_arr[FLUSH_COUNT] = flushs;

    print_simulator_output("Successfully executed all instructions in ", n_inst, " cycles.\n");
}
