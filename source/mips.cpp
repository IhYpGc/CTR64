/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 94):
 * <joshuahuelsman@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Josh Huelsman
 * ----------------------------------------------------------------------------
 */
#include <3ds.h>
#include <cstdio>
#include "mips.h"



inline u32
ReadMemWord(MIPS_R3000 *Cpu, u32 Address)
{
    u32 Base = Address & 0x00FFFFFF;
    return *((u32 *)((u8 *)Cpu->Memory + Base));
}

inline u8
ReadMemByte(MIPS_R3000 *Cpu, u32 Address)
{
    u32 Base = Address & 0x00FFFFFF;
    return *((u8 *)Cpu->Memory + Base);
}

inline u16
ReadMemHalfWord(MIPS_R3000 *Cpu, u32 Address)
{
    u32 Base = Address & 0x00FFFFFF;
    return *((u16 *)((u8 *)Cpu->Memory + Base));
}

inline void
WriteMemByte(MIPS_R3000 *Cpu, u32 Address, u8 value)
{
    u32 Base = Address & 0x00FFFFFF;
    for (u32 i = 0; i < Cpu->NumMMR; ++i)
    {
        mmr *MMR = &Cpu->MemMappedRegisters[i];
        if (Base == MMR->Address)
        {
            MMR->RegisterFunc(MMR->Object, value);
            return;
        }
    }
    *((u8 *)Cpu->Memory + Base) = value;
}

inline void
WriteMemWord(MIPS_R3000 *Cpu, u32 Address, u32 value)
{
    u32 Base = Address & 0x00FFFFFF;
    for (u32 i = 0; i < Cpu->NumMMR; ++i)
    {
        mmr *MMR = &Cpu->MemMappedRegisters[i];
        if (Base == MMR->Address)
        {
            MMR->RegisterFunc(MMR->Object, value);
            return;
        }
    }
    *((u32 *)((u8 *)Cpu->Memory + Base)) = value;
}

inline void
WriteMemHalfWord(MIPS_R3000 *Cpu, u32 Address, u16 value)
{
    u32 Base = Address & 0x00FFFFFF;
    for (u32 i = 0; i < Cpu->NumMMR; ++i)
    {
        mmr *MMR = &Cpu->MemMappedRegisters[i];
        if (Base == MMR->Address)
        {
            MMR->RegisterFunc(MMR->Object, value);
            return;
        }
    }
    *((u16 *)((u8 *)Cpu->Memory + Base)) = value;
}

static void
C0ExecuteOperation(Coprocessor *Cp, u32 FunctionCode)
{

}

MIPS_R3000::
MIPS_R3000()
{
    CP0.ExecuteOperation = C0ExecuteOperation;
    
    for (int i = 3; i >= 0; --i)
    {
        Stages[i] = -(i + 1);
    }
}

//Exceptions
static void
C0GenerateException(MIPS_R3000 *Cpu, u8 Cause, u32 EPC)
{
    Cpu->CP0.cause = (Cause << 10) & C0_CAUSE_MASK;
    Cpu->CP0.epc = EPC;
    Cpu->CP0.sr |= C0_STATUS_KUc;
    Cpu->pc = GNRAL_VECTOR;
}

static void
ReservedInstructionException(MIPS_R3000 *Cpu, opcode *Op)
{
    // TODO exceptions & coprocessor0
//    DumpState(Cpu, Op);
}


typedef void (*jt_func)(MIPS_R3000 *, opcode *);

static jt_func PrimaryJumpTable[0x40];
static jt_func SecondaryJumpTable[0x40];

//Arithmetic
static void
AddU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->RightValue;
}

static void
AddIU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
}

static void
SubU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue - OpCode->RightValue;
}

static void
Add(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->RightValue;
    // TODO overflow trap
}

static void
AddI(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    // TODO overflow trap
}

static void
Sub(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue - OpCode->RightValue;
    // TODO overflow trap
}

//Store
static void
SW(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_WORD;
    OpCode->MemAccessType = MEM_ACCESS_WRITE;
}

static void
SH(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_HALF;
    OpCode->MemAccessType = MEM_ACCESS_WRITE;
}

static void
SB(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_BYTE;
    OpCode->MemAccessType = MEM_ACCESS_WRITE;
}

//Load
static void
LUI(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->Immediate << 16;
}

static void
LW(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_WORD;
    OpCode->MemAccessType = MEM_ACCESS_READ;
}

static void
LBU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_BYTE;
    OpCode->MemAccessType = MEM_ACCESS_READ;
}

static void
LHU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_HALF;
    OpCode->MemAccessType = MEM_ACCESS_READ;
}

static void
LB(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_BYTE;
    OpCode->MemAccessType = MEM_ACCESS_READ;
    OpCode->FunctionSelect = 1;
}

static void
LH(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue + OpCode->Immediate;
    OpCode->MemAccessMode = MEM_ACCESS_HALF;
    OpCode->MemAccessType = MEM_ACCESS_READ;
    OpCode->FunctionSelect = 1;
}


// Jump/Call
static void
J(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = (Cpu->pc & 0xF0000000) + (OpCode->Immediate * 4);
    OpCode->DestinationRegister = REG_INDEX_PC;
    OpCode->MemAccessType = MEM_ACCESS_BRANCH;
}

static void
JAL(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->RADestinationRegister = REG_INDEX_RA;
    OpCode->Result = (Cpu->pc & 0xF0000000) + (OpCode->Immediate * 4);
    OpCode->DestinationRegister = REG_INDEX_PC;
    OpCode->MemAccessType = MEM_ACCESS_BRANCH;
}

static void
JR(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue;
    OpCode->DestinationRegister = REG_INDEX_PC;
    OpCode->MemAccessType = MEM_ACCESS_BRANCH;
}

static void
JALR(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue;
    OpCode->DestinationRegister = REG_INDEX_PC;
    OpCode->MemAccessType = MEM_ACCESS_BRANCH;
}

static void
BranchZero(MIPS_R3000 *Cpu, opcode *OpCode)
{
    //bltz, bgez, bltzal, bgezal
    u8 type = OpCode->FunctionSelect;
    OpCode->Result = OpCode->CurrentAddress + 4 + OpCode->Immediate * 4;
    s32 Check = OpCode->LeftValue;

    if (type & 0b00001)
    {
        //bgez
        if (Check >= 0)
        {
            if (type & 0b10000)
            {
                OpCode->RADestinationRegister = REG_INDEX_RA;
            }
            OpCode->DestinationRegister = REG_INDEX_PC;
            OpCode->MemAccessType = MEM_ACCESS_BRANCH;
        }
    }
    else
    {
        //bltz
        if (Check < 0)
        {
            if (type & 0b10000)
            {
                OpCode->RADestinationRegister = REG_INDEX_RA;
            }
            OpCode->DestinationRegister = REG_INDEX_PC;
            OpCode->MemAccessType = MEM_ACCESS_BRANCH;
        }
    }
}

static void
BEQ(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->LeftValue == OpCode->RightValue)
    {
        OpCode->Result = OpCode->CurrentAddress + 4 + OpCode->Immediate * 4;
        OpCode->DestinationRegister = REG_INDEX_PC;
        OpCode->MemAccessType = MEM_ACCESS_BRANCH;
    }
}

static void
BNE(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->LeftValue != OpCode->RightValue)
    {
        OpCode->Result = OpCode->CurrentAddress + 4 + OpCode->Immediate * 4;
        OpCode->DestinationRegister = REG_INDEX_PC;
        OpCode->MemAccessType = MEM_ACCESS_BRANCH;
    }
}

static void
BLEZ(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->LeftValue <= 0)
    {
        OpCode->Result = OpCode->CurrentAddress + 4 + OpCode->Immediate * 4;
        OpCode->DestinationRegister = REG_INDEX_PC;
        OpCode->MemAccessType = MEM_ACCESS_BRANCH;
    }
}

static void
BGTZ(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->LeftValue > 0)
    {
        OpCode->Result = OpCode->CurrentAddress + 4 + OpCode->Immediate * 4;
        OpCode->DestinationRegister = REG_INDEX_PC;
        OpCode->MemAccessType = MEM_ACCESS_BRANCH;
    }
}

//Logical
static void
AndI(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue & OpCode->Immediate;
}

static void
OrI(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue | (OpCode->Immediate & 0xFFFF);
}

static void
And(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue & OpCode->RightValue;
}

static void
Or(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue | OpCode->RightValue;
}

static void
XOr(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue ^ OpCode->RightValue;
}
static void
NOr(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = 0xFFFFFFFF ^ (OpCode->LeftValue | OpCode->RightValue);
}

static void
XOrI(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->LeftValue ^ (OpCode->Immediate & 0xFFFF);
}

//shifts
static void
SLLV(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->RightValue << (OpCode->LeftValue & 0x1F);
}

static void
SRLV(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->RightValue >> (OpCode->LeftValue & 0x1F);
}

static void
SRAV(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = ((s32)OpCode->RightValue) >> (OpCode->LeftValue & 0x1F);
}

static void
SLL(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->RightValue << OpCode->Immediate;
}

static void
SRL(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = OpCode->RightValue >> OpCode->Immediate;
}

static void
SRA(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = ((s32)OpCode->RightValue) >> OpCode->Immediate;
}

// comparison
static void
SLT(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = ((s32)OpCode->LeftValue < (s32)OpCode->RightValue ? 1 : 0);
}

static void
SLTU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = (OpCode->LeftValue < OpCode->RightValue ? 1 : 0);
}

static void
SLTI(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = ((s32)OpCode->LeftValue < (s32)OpCode->Immediate ? 1 : 0);
}

static void
SLTIU(MIPS_R3000 *Cpu, opcode *OpCode)
{
    OpCode->Result = (OpCode->LeftValue < OpCode->Immediate ? 1 : 0);
}

// coprocessor ops
static void
COP0(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->FunctionSelect < 0b10000)
    {
        if (OpCode->FunctionSelect < 0b01000)
        {
            OpCode->Result = OpCode->LeftValue;
        }
        else
        {
            if (OpCode->RightValue)
            {
                if (Cpu->CP0.sr & C0_STATUS_CU0)
                {
                    OpCode->MemAccessType = MEM_ACCESS_BRANCH;
                    OpCode->Result = OpCode->CurrentAddress + OpCode->Immediate;
                }
            }
            else
            {
                if ((Cpu->CP0.sr & C0_STATUS_CU0) == 0)
                {
                    OpCode->MemAccessType = MEM_ACCESS_BRANCH;
                    OpCode->Result = OpCode->CurrentAddress + OpCode->Immediate;
                }
            }
        }
    }
    else
    {
        Cpu->CP0.ExecuteOperation(&Cpu->CP0, OpCode->Immediate);
    }
}

static void
COP1(MIPS_R3000 *Cpu, opcode *OpCode)
{
    // PSX missing cop1
    // TODO exceptions
}

static void
COP2(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (Cpu->CP1)
    {
        if (OpCode->FunctionSelect < 0b10000)
        {
            if (OpCode->FunctionSelect > 0b00100) // < 0b01000
            {
                OpCode->Result = OpCode->RightValue;
            }
            else
            {
                if (OpCode->RightValue)
                {
                    if (Cpu->CP0.sr & C0_STATUS_CU1)
                    {
                        OpCode->MemAccessType = MEM_ACCESS_BRANCH;
                        OpCode->Result = OpCode->CurrentAddress + OpCode->Immediate;
                    }
                }
                else
                {
                    if ((Cpu->CP0.sr & C0_STATUS_CU1) == 0)
                    {
                        OpCode->MemAccessType = MEM_ACCESS_BRANCH;
                        OpCode->Result = OpCode->CurrentAddress + OpCode->Immediate;
                    }
                }
            }
        }
        else
        {
            Cpu->CP1->ExecuteOperation(Cpu->CP1, OpCode->Immediate);
        }
    }
}

static void
COP3(MIPS_R3000 *Cpu, opcode *OpCode)
{
    // PSX missing cop3
    // TODO exceptions
}


void
DecodeOpcode(MIPS_R3000 *Cpu, opcode *OpCode, const u32 Data)
{
    const u8 rs = (Data & REG_RS_MASK) >> 21;
    const u8 rt = (Data & REG_RT_MASK) >> 16;
    const u8 rd = (Data & REG_RD_MASK) >> 11;
    const u8 Select0 = (Data & PRIMARY_OP_MASK) >> 26;
    const u8 Select1 = (Data & SECONDARY_OP_MASK);

    OpCode->CurrentAddress = Cpu->pc - 4;
    OpCode->Select0 = Select0;
    OpCode->Select1 = Select1;

    OpCode->LeftValue = Cpu->registers[rs];
    OpCode->RightValue = Cpu->registers[rt];

    if (Select0 == 0)
    {
        OpCode->DestinationRegister = rd;
        //shift-imm
        if ((Select1 & 0b111100) == 0)
        {
            OpCode->Immediate = (Data & IMM5_MASK) >> 6;

        }
        //jalr
        else if (Select1 == 0b001001)
        {
            OpCode->RADestinationRegister = rd;
        }
        //sys/brk
        else if ((Select1 & 0b111110) == 0b001100)
        {
            OpCode->Immediate = (Data & COMMENT20_MASK) >> 6;
        }
        //mthi/mtlo: these can set destination register in function
        return;
    }

    OpCode->Immediate = SignExtend16((Data & IMM16_MASK) >> 0);
    OpCode->DestinationRegister = rt;
    //bltz, bgez, bltzal bgezal
    if (Select0 == 0b000001)
    {
        OpCode->FunctionSelect = rt;
        //destination registers set within function
    }
    //j/jal
    else if ((Select0 & 0b111110) == 0b000010)
    {
        OpCode->Immediate = (Data & IMM26_MASK) >> 0;
        //RADestinationRegister set within function
    }
    // coprocessor main instruction decoding
    else if ((Select0 & 0b010000) == 0b010000)
    {
        OpCode->FunctionSelect = rs;
        // mfc, cfc
        if (rs < 0b00100)
        {
            OpCode->LeftValue = Cpu->CP0.registers[rd + (rs & 0b00010 ? 32 : 0)];
        }
        // mtc, ctc
        else if (rs < 0b01000)
        {
            OpCode->WriteBackMode = OpCode->Select0 & 0b111;
            OpCode->DestinationRegister = rd + (rs & 0b00010 ? 32 : 0);
        }
        // BCnF, BCnT
        else if (rs == 0b01000)
        {
            OpCode->RightValue = rt; // used as secondary function select
        }
        else if (rs & 0b10000)
        {
            OpCode->Immediate = (Data & IMM25_MASK) >> 0;
        }
    }
    //lwc
    else if ((Select0 & 0b111000) == 0b110000)
    {
        OpCode->WriteBackMode = Select0 & 0b111;
        OpCode->MemAccessType = MEM_ACCESS_READ;
    }
    //swc
    else if ((Select0 & 0b111000) == 0b111000)
    {
        OpCode->RightValue = Cpu->CP0.registers[rt];
        OpCode->MemAccessType = MEM_ACCESS_WRITE;
    }
}

inline u32
InstructionFetch(MIPS_R3000 *Cpu)
{
    u32 Result = ReadMemWord(Cpu, Cpu->pc);
    Cpu->pc += 4;
    return Result;
}

inline void
ExecuteWriteRegisters(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->WriteBackMode == WRITE_BACK_CPU && (OpCode->MemAccessType == MEM_ACCESS_NONE || OpCode->MemAccessType == MEM_ACCESS_BRANCH))
    {
        if (OpCode->DestinationRegister) // Never overwrite Zero
        {
            Cpu->registers[OpCode->DestinationRegister] = OpCode->Result;
        }
        if (OpCode->RADestinationRegister)
        {
            Cpu->registers[OpCode->RADestinationRegister] = OpCode->CurrentAddress + 8;
        }

        OpCode->DestinationRegister = 0;
        OpCode->RADestinationRegister = 0;
    }
}

inline void
ExecuteOpCode(MIPS_R3000 *Cpu, opcode *OpCode)
{
    u8 Select0 = OpCode->Select0;
    jt_func Func;
    if (Select0)
    {
        Func = PrimaryJumpTable[Select0];
    }
    else
    {
        Func = SecondaryJumpTable[OpCode->Select1];
    }

    Func(Cpu, OpCode);
    ExecuteWriteRegisters(Cpu, OpCode);
}

inline void
WriteBack(MIPS_R3000 *Cpu, opcode *OpCode)
{

    if (OpCode->WriteBackMode == WRITE_BACK_CPU && OpCode->MemAccessType == MEM_ACCESS_NONE)
    {
        if (OpCode->DestinationRegister) // Never overwrite Zero
        {
            Cpu->registers[OpCode->DestinationRegister] = OpCode->Result;
        }
        if (OpCode->RADestinationRegister)
        {
            Cpu->registers[OpCode->RADestinationRegister] = OpCode->CurrentAddress + 8;
        }
    }
    else if (OpCode->WriteBackMode == WRITE_BACK_C0)
    {
        Cpu->CP0.registers[OpCode->DestinationRegister] = OpCode->Result;
    }
    else if (OpCode->WriteBackMode == WRITE_BACK_C2)
    {
        // TODO GTE
    }
}

void
MemoryAccess(MIPS_R3000 *Cpu, opcode *OpCode)
{
    if (OpCode->MemAccessType == MEM_ACCESS_WRITE)
    {
        if (OpCode->MemAccessMode == MEM_ACCESS_BYTE)
        {
            WriteMemByte(Cpu, OpCode->Result, OpCode->RightValue);
        }

        if (OpCode->MemAccessMode == MEM_ACCESS_HALF)
        {
            WriteMemHalfWord(Cpu, OpCode->Result, OpCode->RightValue);
        }

        if (OpCode->MemAccessMode == MEM_ACCESS_WORD)
        {
            WriteMemWord(Cpu, OpCode->Result, OpCode->RightValue);
        }
        OpCode->DestinationRegister = 0;
    }
    if (OpCode->MemAccessType == MEM_ACCESS_READ)
    {
        if (OpCode->MemAccessMode == MEM_ACCESS_BYTE)
        {
            u32 Value = ReadMemByte(Cpu, OpCode->Result);
            if (OpCode->FunctionSelect)
            {
                Value = SignExtend8(Value);
            }
            OpCode->Result = Value;
        }

        if (OpCode->MemAccessMode == MEM_ACCESS_HALF)
        {
            u32 Value = ReadMemHalfWord(Cpu, OpCode->Result);
            if (OpCode->FunctionSelect)
            {
                Value = SignExtend16(Value);
            }
            OpCode->Result = Value;
        }

        if (OpCode->MemAccessMode == MEM_ACCESS_WORD)
        {
            OpCode->Result = ReadMemWord(Cpu, OpCode->Result);
        }
    }

    WriteBack(Cpu, OpCode);
}

void
MapRegister(MIPS_R3000 *Cpu, mmr MMR)
{
    Cpu->MemMappedRegisters[Cpu->NumMMR] = MMR;
    Cpu->MemMappedRegisters[Cpu->NumMMR].Address &= 0x00FFFFFF;
    ++Cpu->NumMMR;
}

#define STAGE_IF 0
#define STAGE_DC 1
#define STAGE_EO 2
#define STAGE_MA 3
#define STAGE_WB 4

void
StepCpu(MIPS_R3000 *Cpu, u32 Steps)
{
    for (u32 t = 0; t < Steps; ++t)
    {
        MemoryAccess(Cpu, &Cpu->OpCodes[0]);
        ExecuteOpCode(Cpu, &Cpu->OpCodes[1]);
        opcode *OpCode = &Cpu->OpCodes[2];
        *OpCode = {};
        DecodeOpcode(Cpu, OpCode, Cpu->MachineCode);
        Cpu->MachineCode = InstructionFetch(Cpu);

        MemoryAccess(Cpu, &Cpu->OpCodes[1]);
        ExecuteOpCode(Cpu, &Cpu->OpCodes[2]);
        OpCode = &Cpu->OpCodes[3];
        *OpCode = {};
        DecodeOpcode(Cpu, OpCode, Cpu->MachineCode);
        Cpu->MachineCode = InstructionFetch(Cpu);

        MemoryAccess(Cpu, &Cpu->OpCodes[2]);
        ExecuteOpCode(Cpu, &Cpu->OpCodes[3]);
        OpCode = &Cpu->OpCodes[0];
        *OpCode = {};
        DecodeOpcode(Cpu, OpCode, Cpu->MachineCode);
        Cpu->MachineCode = InstructionFetch(Cpu);

        MemoryAccess(Cpu, &Cpu->OpCodes[3]);
        ExecuteOpCode(Cpu, &Cpu->OpCodes[0]);
        OpCode = &Cpu->OpCodes[1];
        *OpCode = {};
        DecodeOpcode(Cpu, OpCode, Cpu->MachineCode);
        Cpu->MachineCode = InstructionFetch(Cpu);
    }
}

static void __attribute__((constructor))
InitJumpTables()
{
    for (int i = 0; i < 0x3F; ++i)
    {
        PrimaryJumpTable[i] = ReservedInstructionException;
        SecondaryJumpTable[i] = ReservedInstructionException;
    }

//    PrimaryJumpTable[0x00] = ExecuteSecondary;
    PrimaryJumpTable[0x01] = BranchZero;
    PrimaryJumpTable[0x02] = J;
    PrimaryJumpTable[0x03] = JAL;
    PrimaryJumpTable[0x04] = BEQ;
    PrimaryJumpTable[0x05] = BNE;
    PrimaryJumpTable[0x06] = BLEZ;
    PrimaryJumpTable[0x07] = BGTZ;
    PrimaryJumpTable[0x08] = AddI;
    PrimaryJumpTable[0x09] = AddIU;
    PrimaryJumpTable[0x0A] = SLTI;
    PrimaryJumpTable[0x0B] = SLTIU;
    PrimaryJumpTable[0x0C] = AndI;
    PrimaryJumpTable[0x0D] = OrI;
    PrimaryJumpTable[0x0E] = XOrI;
    PrimaryJumpTable[0x0F] = LUI;
    PrimaryJumpTable[0x10] = COP0;
    PrimaryJumpTable[0x11] = COP1;
    PrimaryJumpTable[0x12] = COP2;
    PrimaryJumpTable[0x13] = COP3;
    PrimaryJumpTable[0x20] = LB;
    PrimaryJumpTable[0x21] = LH;
    //    PrimaryJumpTable[0x22] = LWL;
    PrimaryJumpTable[0x23] = LW;
    PrimaryJumpTable[0x24] = LBU;
    PrimaryJumpTable[0x25] = LHU;
    //    PrimaryJumpTable[0x26] = LWR;
    PrimaryJumpTable[0x28] = SB;
    PrimaryJumpTable[0x29] = SH;
    //    PrimaryJumpTable[0x2A] = SWL;
    PrimaryJumpTable[0x2B] = SW;
    //    PrimaryJumpTable[0x2E] = SWR;
    //    PrimaryJumpTable[0x30] = LWC0;
    //    PrimaryJumpTable[0x31] = LWC1;
    //    PrimaryJumpTable[0x32] = LWC2;
    //    PrimaryJumpTable[0x33] = LWC3;
    //    PrimaryJumpTable[0x38] = SWC0;
    //    PrimaryJumpTable[0x39] = SWC1;
    //    PrimaryJumpTable[0x3A] = SWC2;
    //    PrimaryJumpTable[0x3B] = SWC3;

    SecondaryJumpTable[0x00] = SLL;
    SecondaryJumpTable[0x02] = SRL;
    SecondaryJumpTable[0x03] = SRA;
    SecondaryJumpTable[0x04] = SLLV;
    SecondaryJumpTable[0x06] = SRLV;
    SecondaryJumpTable[0x07] = SRAV;
    SecondaryJumpTable[0x08] = JR;
    SecondaryJumpTable[0x09] = JALR;
    //    SecondaryJumpTable[0x0C] = SysCall;
    //    SecondaryJumpTable[0x0D] = Break;
    //    SecondaryJumpTable[0x10] = MFHI;
    //    SecondaryJumpTable[0x11] = MTHI;
    //    SecondaryJumpTable[0x12] = MFLO;
    //    SecondaryJumpTable[0x13] = MTLO;
    //    SecondaryJumpTable[0x18] = Mutl;
    //    SecondaryJumpTable[0x19] = MultU;
    //    SecondaryJumpTable[0x1A] = Div;
    //    SecondaryJumpTable[0x1B] = DivU;
    SecondaryJumpTable[0x20] = Add;
    SecondaryJumpTable[0x21] = AddU;
    SecondaryJumpTable[0x22] = Sub;
    SecondaryJumpTable[0x23] = SubU;
    SecondaryJumpTable[0x24] = And;
    SecondaryJumpTable[0x25] = Or;
    SecondaryJumpTable[0x26] = XOr;
    SecondaryJumpTable[0x27] = NOr;
    SecondaryJumpTable[0x2A] = SLT;
    SecondaryJumpTable[0x2B] = SLTU;
}
