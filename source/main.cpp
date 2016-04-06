/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 94):
 * <joshuahuelsman@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Josh Huelsman
 * ----------------------------------------------------------------------------
 */
#include "platform.h"
#include <cstdio>
#include <stdlib.h>
#include <cstring>
#include "mips.h"
#include "disasm.h"
#include "joypad.h"
#include "vi.h"
#include "z64.h"

void
ResetCpu(MIPS_R3000 *Cpu)
{
    Cpu->pc = RESET_VECTOR;
}

static u32 InterruptMask;

int main(int argc, char **argv)
{
    InitPlatform(argc, argv);

    MIPS_R3000 Cpu;

    FILE *f = fopen("n64_ipl.bin", "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    u8 *BiosBuffer = (u8 *)linearAlloc(0x800);
    fread(BiosBuffer, fsize, 1, f);
    fclose(f);

    u8 *CartBuffer = (u8 *)linearAlloc(0x01000000);
    z64 Z64Cart;
    Z64Open(&Z64Cart, Z64_FLAG_MIDDLE_ENDIAN, "cart.v64");
    Z64Read(&Z64Cart, CartBuffer, Z64GetCartSize(&Z64Cart));
    Z64Close(&Z64Cart);

    MapMemoryRegion(&Cpu, (mmm) {linearAlloc(0x400000), 0x00000000, 0x400000});        // RDRAM
    MapMemoryRegion(&Cpu, (mmm) {BiosBuffer, 0x1FC00000, 0x0800});     // PIF ROM/RAM
    MapMemoryRegion(&Cpu, (mmm) {linearAlloc(sizeof(VideoInterface)), 0x04400000, sizeof(VideoInterface)}); // VI
    MapMemoryRegion(&Cpu, (mmm) {CartBuffer, 0x10000000, 0x01000000});

    ResetCpu(&Cpu);

    bool Step = false;
    int CyclesToRun = 10000;
    bool AutoStep = true;
    u32 IRQ0Steps = 0;
    bool Down = false;

    if (PlatformHasDebugger())
    {
        CyclesToRun = 1;
        AutoStep = false;
        PlatformAttachDebugger(&Cpu);
    }

    while (MainLoopPlatform())
    {
#ifdef _3DS
        u32 KeysDown = hidKeysDown();

        if (KeysDown & KEY_START)
            break;
#endif


        if (((GetDigitalSwitchesPlatform() & PLATFORM_START) == 0) && Down)
        {
            Step = true;
            Down = false;
        }

        if (GetDigitalSwitchesPlatform() & PLATFORM_START)
        {
            Down = true;
        }

        if (Step || AutoStep)
        {
            Step = false;
            StepCpu(&Cpu, CyclesToRun);
            IRQ0Steps += CyclesToRun;
            if (IRQ0Steps >= 50000)
            {
                C0GenerateException(&Cpu, C0_CAUSE_INT, Cpu.pc - 4);
                IRQ0Steps = 0;
                InterruptMask |= 1;
            }
        }

        SwapBuffersPlatform();
    }
    ExitPlatform();

    return 0;
}



