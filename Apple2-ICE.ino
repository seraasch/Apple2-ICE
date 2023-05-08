//
//
//  File Name   :  MCL66_A2Plus_ICE.ino
//  Used on     :  
//  Author      :  Ted Fried, MicroCore Labs
//  Creation    :  1/1/2021
//
//   Description:
//   ============
//   
//  MOS 6502 emulator with bus interface.     Apple II+ version with acceleration via keystrokes or UART
//
//
// Accelerartion using Apple II keyboard keystrokes:
//  - Press left-arrow(L), right-arrow(R), left-arrow(L), then the number for the addressing mode desired.
//  - This method only works if the Apple II is scanning for keystrokes.  If not, then the UART method of acceleration must be used.
//      Example:  press LRL3 for acceletaion moode 3
//
// Accelerartion using the Teensy 4.1 USB UART
//  - Open a UART terminal to the Teensy 4.1 board. You can use the Arduino GUI Tools --> Serial Monitor or any other terminal emulator
//  - Simply send a number over the UART to the MCL65 board for the addressing mode desired
//      Example:  press 3 for addressing moode 3
//
// Acceleration addr_mode may also be hard-coded or the accelerated address ranges can be changed in the internal_address_check procedure below
//------------------------------------------------------------------------
//
// Modification History:
// =====================
//
// Revision 1 1/1/2021
// Initial revision
//
// Revision 2 9/22/2021
// Added methods to change acceleration addr_modes
//
// Revision 2a 4/18/2023
// Begin working on adding in-circuit emulator features
//
// Revision 2b 5/5/2023
// Initial testing
//
//------------------------------------------------------------------------
//
// Copyright (c) 2021 Ted Fried
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//------------------------------------------------------------------------

// Address Modes:
//     0 - All exernal memory accesses
//     1 - Reads use cycle accurate internal memory and writes pass through to motherboard
//     2 - Reads accelerated using internal memory and writes pass through to motherboard
//     3 - All read and write accesses use accelerated internal memory 

#define VERSION_NUM "1.0"

#define OFFLINE_DEBUG true

#include <stdint.h>


// Teensy 4.1 pin assignments
//
#define PIN_CLK0 24
#define PIN_RESET 40
#define PIN_READY_n 26
#define PIN_IRQ 25
#define PIN_NMI 41
#define PIN_RDWR_n 12
#define PIN_SYNC 39

#define PIN_ADDR0 27
#define PIN_ADDR1 38
#define PIN_ADDR2 28
#define PIN_ADDR3 37
#define PIN_ADDR4 29
#define PIN_ADDR5 36
#define PIN_ADDR6 30
#define PIN_ADDR7 35
#define PIN_ADDR8 31
#define PIN_ADDR9 34
#define PIN_ADDR10 32
#define PIN_ADDR11 33
#define PIN_ADDR12 1
#define PIN_ADDR13 0
#define PIN_ADDR14 2
#define PIN_ADDR15 23

#define PIN_DATAIN0 14
#define PIN_DATAIN1 15
#define PIN_DATAIN2 16
#define PIN_DATAIN3 17
#define PIN_DATAIN4 18
#define PIN_DATAIN5 19
#define PIN_DATAIN6 20
#define PIN_DATAIN7 21

#define PIN_DATAOUT0 11
#define PIN_DATAOUT1 10
#define PIN_DATAOUT2 9
#define PIN_DATAOUT3 8
#define PIN_DATAOUT4 7
#define PIN_DATAOUT5 6
#define PIN_DATAOUT6 5
#define PIN_DATAOUT7 4
#define PIN_DATAOUT_OE_n 3

// 6502 Flags
//
#define flag_n (register_flags & 0x80) >> 7 // register_flags[7]
#define flag_v (register_flags & 0x40) >> 6 // register_flags[6]
#define flag_b (register_flags & 0x10) >> 4 // register_flags[4]
#define flag_d (register_flags & 0x08) >> 3 // register_flags[3]
#define flag_i (register_flags & 0x04) >> 2 // register_flags[2]
#define flag_z (register_flags & 0x02) >> 1 // register_flags[1]
#define flag_c (register_flags & 0x01) >> 0 // register_flags[0]

// 6502 stack always in Page 1
//
#define register_sp_fixed (0x0100 | register_sp)

enum ADDR_MODE {
    All_External=0,
    Read_Internal_Write_External,
    Read_Fast_Internal_Write_External,
    All_Fast_Internal
};


// CPU register for direct reads of the GPIOs
//
uint8_t register_flags = 0x34;
uint8_t next_instruction;
uint8_t internal_memory_range = 0;
uint8_t nmi_n_old = 1;
uint8_t register_a = 0;
uint8_t register_x = 0;
uint8_t register_y = 0;
uint8_t register_sp = 0xFF;
uint8_t direct_datain = 0;
uint8_t direct_reset = 0;
uint8_t direct_ready_n = 0;
uint8_t direct_irq = 0;
uint8_t direct_nmi = 0;
uint8_t assert_sync = 0;
uint8_t global_temp = 0;
uint8_t last_access_internal_RAM = 0;
uint8_t rx_byte_state = 0;
ADDR_MODE addr_mode = All_External;
uint8_t internal_RAM[65536];

uint16_t register_pc = 0;
uint16_t current_address = 0;
uint16_t effective_address = 0;
int incomingByte;

uint8_t AppleIIP_ROM_D0[0x0800] = {
    0x6f,
    0xd8,
    0x65,
    0xd7,
    0xf8,
    0xdc,
    0x94
};
uint8_t AppleIIP_ROM_D8[0x0800] = {
    0xb8,
    0x90,
    0x2,
    0xe6,
    0xb9,
    0x24,
    0xf2
};

uint8_t AppleIIP_ROM_E0[0x0800] = {
    0x4c,
    0x28,
    0xf1,
    0x4c,
    0x3c,
    0xd4,
    0x0
};
uint8_t AppleIIP_ROM_E8[0x0800] = {
    0xe0,
    0xa5,
    0xf0,
    0x2,
    0xa0,
    0xa5,
    0x38
};

uint8_t AppleIIP_ROM_F0[0x0800] = {
    0x20,
    0x23,
    0xec,
    0xa9,
    0x0,
    0x85,
    0xab
};
uint8_t AppleIIP_ROM_F8[0x0800] = {
    0x4a,
    0x8,
    0x20,
    0x47,
    0xf8,
    0x28,
    0xa9
};

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
//
//  Define interactive commands
//

// Take the first two characters and turn them into a value that can be used in a switch/case statement
#define command_int(_str_) (((word)_str_[0]<<8) + (word)_str_[1])

const word CMD_MD = command_int("md");    // MD - Memory addressing mode
const word CMD_SS = command_int("ss");    // SS - Single Step
const word CMD_GO = command_int("go");    // GO - Execute from current PC
const word CMD_BK = command_int("bk");    // BK - Set a breakpoint
const word CMD_RD = command_int("rd");    // RD - Read from memory
const word CMD_WR = command_int("wr");    // WR - Write to memory
const word CMD_DR = command_int("dr");    // DR - Display registers
const word CMD_SR = command_int("sr");    // SR - Display registers
const word CMD_IN = command_int("in");    // IN - Display info
const word CMD_QM = command_int("?\0");   // ?  - Help
const word CMD_HE = command_int("h\0");   // H  - Help
const word CMD_NOP = 0;

word breakpoint = 0;

enum ENUM_RUN_MODE {WAITING=0, SINGLE_STEP, RUNNING}  run_mode;

bool debug_mode = true;


// -------------------------------------------------
// Check for CLK activity --> determines debug mode
// -------------------------------------------------
bool check_for_CLK_activity() {
    unsigned long start = millis();
    while (((GPIO6_DR >> 12) & 0x1) == 0) {  // Teensy 4.1 Pin-24  GPIO6_DR[12]  CLK
        if (millis() - start > 500) {
            return(false);
        }
    }
    while (((GPIO6_DR >> 12) & 0x1) != 0) {
        if (millis() - start > 500) {
            return(false);
        }
    }

    return(true);
}


// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------

// Setup Teensy 4.1 IO's
//
void setup() {

    pinMode(PIN_CLK0, INPUT);
    pinMode(PIN_RESET, INPUT);
    pinMode(PIN_READY_n, INPUT);
    pinMode(PIN_IRQ, INPUT);
    pinMode(PIN_NMI, INPUT);
    pinMode(PIN_RDWR_n, OUTPUT);
    pinMode(PIN_SYNC, OUTPUT);

    pinMode(PIN_ADDR0, OUTPUT);
    pinMode(PIN_ADDR1, OUTPUT);
    pinMode(PIN_ADDR2, OUTPUT);
    pinMode(PIN_ADDR3, OUTPUT);
    pinMode(PIN_ADDR4, OUTPUT);
    pinMode(PIN_ADDR5, OUTPUT);
    pinMode(PIN_ADDR6, OUTPUT);
    pinMode(PIN_ADDR7, OUTPUT);
    pinMode(PIN_ADDR8, OUTPUT);
    pinMode(PIN_ADDR9, OUTPUT);
    pinMode(PIN_ADDR10, OUTPUT);
    pinMode(PIN_ADDR11, OUTPUT);
    pinMode(PIN_ADDR12, OUTPUT);
    pinMode(PIN_ADDR13, OUTPUT);
    pinMode(PIN_ADDR14, OUTPUT);
    pinMode(PIN_ADDR15, OUTPUT);

    pinMode(PIN_DATAIN0, INPUT);
    pinMode(PIN_DATAIN1, INPUT);
    pinMode(PIN_DATAIN2, INPUT);
    pinMode(PIN_DATAIN3, INPUT);
    pinMode(PIN_DATAIN4, INPUT);
    pinMode(PIN_DATAIN5, INPUT);
    pinMode(PIN_DATAIN6, INPUT);
    pinMode(PIN_DATAIN7, INPUT);

    pinMode(PIN_DATAOUT0, OUTPUT);
    pinMode(PIN_DATAOUT1, OUTPUT);
    pinMode(PIN_DATAOUT2, OUTPUT);
    pinMode(PIN_DATAOUT3, OUTPUT);
    pinMode(PIN_DATAOUT4, OUTPUT);
    pinMode(PIN_DATAOUT5, OUTPUT);
    pinMode(PIN_DATAOUT6, OUTPUT);
    pinMode(PIN_DATAOUT7, OUTPUT);
    pinMode(PIN_DATAOUT_OE_n, OUTPUT);

    // Copy ROM contents into the Teensy's internal RAM
    //
    for (uint32_t u = 0; u <= 0x07FF; u++) {
        internal_RAM[0xD000 + u] = AppleIIP_ROM_D0[u];
    }
    for (uint32_t u = 0; u <= 0x07FF; u++) {
        internal_RAM[0xD800 + u] = AppleIIP_ROM_D8[u];
    }

    for (uint32_t u = 0; u <= 0x07FF; u++) {
        internal_RAM[0xE000 + u] = AppleIIP_ROM_E0[u];
    }
    for (uint32_t u = 0; u <= 0x07FF; u++) {
        internal_RAM[0xE800 + u] = AppleIIP_ROM_E8[u];
    }

    for (uint32_t u = 0; u <= 0x07FF; u++) {
        internal_RAM[0xF000 + u] = AppleIIP_ROM_F0[u];
    }
    for (uint32_t u = 0; u <= 0x07FF; u++) {
        internal_RAM[0xF800 + u] = AppleIIP_ROM_F8[u];
    }

    Serial.begin(115200);
    Serial.setTimeout(5000);

    Serial.println(String("Apple ][+ In-circuit Emulator\n\rVersion ") + VERSION_NUM);

    if ( !check_for_CLK_activity() ) {
        debug_mode = true;
        addr_mode = All_Fast_Internal;
        Serial.println("No CLK activity detected. Starting in DEBUG MODE using INTERNAL MEMORY");
    }
    else {
        debug_mode = false;
        addr_mode = All_External;
        Serial.println("CLK activity detected... Starting in NORMAL MODE using external memory");
    }

    run_mode = WAITING;
}

// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------
//
// Begin 6502 Bus Interface Unit 
//
// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

// ----------------------------------------------------------
// Address range check
//  Return: 0x0 - All exernal memory accesses
//          0x1 - Reads use cycle accurate internal memory and writes pass through to motherboard
//          0x2 - Reads accelerated using internal memory and writes pass through to motherboard
//          0x3 - All read and write accesses use accelerated internal memory 
// ----------------------------------------------------------
inline ADDR_MODE internal_address_check(int32_t local_address) {

    if ((local_address >= 0x0000) && (local_address < 0x0400)) return addr_mode; //  6502 ZeroPage and Stack
    if ((local_address >= 0x0400) && (local_address < 0x0C00)) return Read_Internal_Write_External; //  Apple II Plus Text Page 1 and 2
    if ((local_address >= 0x0C00) && (local_address < 0x2000)) return addr_mode; //  Apple II Plus RAM 
    if ((local_address >= 0x2000) && (local_address < 0x6000)) return addr_mode; //  Apple IIPlus  HIRES Page 1 and 2
    if ((local_address >= 0x6000) && (local_address < 0xC000)) return addr_mode; //  Apple II Plus RAM 
    if ((local_address >= 0xD000) && (local_address <= 0xFFFF)) return All_External; //  Apple II Plus ROMs 
    //    Bank switching does not currently work, so set to 0x0 to use the Language card and have 64KB total memory
    //    and to accelerate ROM but run with only 48KB of memory, set to 'addr_mode'  

    return All_External;
}

// -------------------------------------------------
// Wait for the CLK1 rising edge and sample signals
// -------------------------------------------------
inline void wait_for_CLK_rising_edge() {
    register uint32_t GPIO6_data = 0;
    register uint32_t GPIO6_data_d1 = 0;
    uint32_t d10, d2, d3, d4, d5, d76;

    if (debug_mode)
        return;

    while (((GPIO6_DR >> 12) & 0x1) != 0) {} // Teensy 4.1 Pin-24  GPIO6_DR[12]     CLK

    //while (((GPIO6_DR >> 12) & 0x1)==0) {GPIO6_data=GPIO6_DR;}                  // This method is ok for VIC-20 and Apple-II+ non-DRAM ranges 

    do {
        GPIO6_data_d1 = GPIO6_DR;
    } while (((GPIO6_data_d1 >> 12) & 0x1) == 0); // This method needed to support Apple-II+ DRAM read data setup time
    GPIO6_data = GPIO6_data_d1;

    d10 = (GPIO6_data & 0x000C0000) >> 18; // Teensy 4.1 Pin-14  GPIO6_DR[19:18]  D1:D0
    d2 = (GPIO6_data & 0x00800000) >> 21; // Teensy 4.1 Pin-16  GPIO6_DR[23]     D2
    d3 = (GPIO6_data & 0x00400000) >> 19; // Teensy 4.1 Pin-17  GPIO6_DR[22]     D3
    d4 = (GPIO6_data & 0x00020000) >> 13; // Teensy 4.1 Pin-18  GPIO6_DR[17]     D4
    d5 = (GPIO6_data & 0x00010000) >> 11; // Teensy 4.1 Pin-19  GPIO6_DR[16]     D5
    d76 = (GPIO6_data & 0x0C000000) >> 20; // Teensy 4.1 Pin-20  GPIO6_DR[27:26]  D7:D6

    direct_irq = (GPIO6_data & 0x00002000) >> 13; // Teensy 4.1 Pin-25  GPIO6_DR[13]     IRQ
    direct_ready_n = (GPIO6_data & 0x40000000) >> 30; // Teensy 4.1 Pin-26  GPIO6_DR[30]     READY
    direct_reset = (GPIO6_data & 0x00100000) >> 20; // Teensy 4.1 Pin-40  GPIO6_DR[20]     RESET
    direct_nmi = (GPIO6_data & 0x00200000) >> 21; // Teensy 4.1 Pin-41  GPIO6_DR[21]     NMI

    direct_datain = d76 | d5 | d4 | d3 | d2 | d10;

    return;
}

// -------------------------------------------------
// Wait for the CLK1 falling edge 
// -------------------------------------------------
inline void wait_for_CLK_falling_edge() {

    if (!debug_mode) {
        while (((GPIO6_DR >> 12) & 0x1) == 0) {} // Teensy 4.1 Pin-24  GPIO6_DR[12]  CLK
        while (((GPIO6_DR >> 12) & 0x1) != 0) {}
    }
    return;
}

// -------------------------------------------------
// Drive the 6502 Address pins
// -------------------------------------------------
inline void send_address(uint32_t local_address) {
    register uint32_t writeback_data = 0;

    writeback_data = (0x6DFFFFF3 & GPIO6_DR); // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x8000) << 10; // 6502_Address[15]   TEENSY_PIN23   GPIO6_DR[25]
    writeback_data = writeback_data | (local_address & 0x2000) >> 10; // 6502_Address[13]   TEENSY_PIN0    GPIO6_DR[3]
    writeback_data = writeback_data | (local_address & 0x1000) >> 10; // 6502_Address[12]   TEENSY_PIN1    GPIO6_DR[2]
    writeback_data = writeback_data | (local_address & 0x0002) << 27; // 6502_Address[1]    TEENSY_PIN38   GPIO6_DR[28]
    GPIO6_DR = writeback_data | (local_address & 0x0001) << 31; // 6502_Address[0]    TEENSY_PIN27   GPIO6_DR[31]

    writeback_data = (0xCFF3EFFF & GPIO7_DR); // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x0400) << 2; // 6502_Address[10]   TEENSY_PIN32   GPIO7_DR[12]
    writeback_data = writeback_data | (local_address & 0x0200) << 20; // 6502_Address[9]    TEENSY_PIN34   GPIO7_DR[29]
    writeback_data = writeback_data | (local_address & 0x0080) << 21; // 6502_Address[7]    TEENSY_PIN35   GPIO7_DR[28]
    writeback_data = writeback_data | (local_address & 0x0020) << 13; // 6502_Address[5]    TEENSY_PIN36   GPIO7_DR[18]
    GPIO7_DR = writeback_data | (local_address & 0x0008) << 16; // 6502_Address[3]    TEENSY_PIN37   GPIO7_DR[19]

    writeback_data = (0xFF3BFFFF & GPIO8_DR); // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x0100) << 14; // 6502_Address[8]    TEENSY_PIN31   GPIO8_DR[22]
    writeback_data = writeback_data | (local_address & 0x0040) << 17; // 6502_Address[6]    TEENSY_PIN30   GPIO8_DR[23]
    GPIO8_DR = writeback_data | (local_address & 0x0004) << 16; // 6502_Address[2]    TEENSY_PIN28   GPIO8_DR[18]

    writeback_data = (0x7FFFFF6F & GPIO9_DR); // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x4000) >> 10; // 6502_Address[14]   TEENSY_PIN2    GPIO9_DR[4]
    writeback_data = writeback_data | (local_address & 0x0800) >> 4; // 6502_Address[11]   TEENSY_PIN33   GPIO9_DR[7]
    GPIO9_DR = writeback_data | (local_address & 0x0010) << 27; // 6502_Address[4]    TEENSY_PIN29   GPIO9_DR[31]

    return;
}

// -------------------------------------------------
// Send the address for a read cyle
// -------------------------------------------------
inline void start_read(uint32_t local_address) {

    current_address = local_address;

    if (internal_address_check(current_address) > Read_Internal_Write_External) {  // Either Fast mode
        //last_access_internal_RAM=1;
    } else {
        if (last_access_internal_RAM == 1) wait_for_CLK_rising_edge();
        last_access_internal_RAM = 0;

        digitalWriteFast(PIN_RDWR_n, 0x1);
        digitalWriteFast(PIN_SYNC, assert_sync);
        send_address(local_address);
    }
    return;
}

// -------------------------------------------------
// On the rising CLK edge, read in the data
// -------------------------------------------------
inline uint8_t finish_read_byte() {

    if (internal_address_check(current_address) > Read_Internal_Write_External) { // Either fast mode
        last_access_internal_RAM = 1;
        return internal_RAM[current_address];
    } else {
        if (last_access_internal_RAM == 1) wait_for_CLK_rising_edge();
        last_access_internal_RAM = 0;
        digitalWriteFast(PIN_SYNC, 0x0);

        do {
            wait_for_CLK_rising_edge();
        } while (direct_ready_n == 0x1); // Delay a clock cycle until ready is active 

        if (internal_address_check(current_address) != All_External) {
            return internal_RAM[current_address];
        } else {
            return direct_datain;
        }
    }
}

// -------------------------------------------------
// Full read cycle with address and data read in
// -------------------------------------------------
inline uint8_t read_byte(uint16_t local_address) {

    if (internal_address_check(local_address) > Read_Internal_Write_External) {  // Either Fast mode
        last_access_internal_RAM = 1;
        return internal_RAM[local_address];
    } else {
        if (last_access_internal_RAM == 1) wait_for_CLK_rising_edge();
        last_access_internal_RAM = 0;

        start_read(local_address);
        do {
            wait_for_CLK_rising_edge();
        } while (direct_ready_n == 0x1); // Delay a clock cycle until ready is active 

        // Set Acceleration using Apple II keystrokes
        // For level 0 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   0
        // For level 1 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   1
        // For level 2 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   2
        // For level 3 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   3
        //
        // These sequences can be entered at any time, however they only work when the Apple II software is polling for keystrokes.
        // If the software is not polling for keystrokes, then the UART RX character receiver can be used to set the addressing mode.  
        // 
        if (local_address == 0xC000) {
            if (rx_byte_state == 0 && direct_datain == 0x88) rx_byte_state = 1;
            if (rx_byte_state == 1 && direct_datain == 0x95) rx_byte_state = 2;
            if (rx_byte_state == 2 && direct_datain == 0x88) rx_byte_state = 3;
            if (rx_byte_state == 3) {
                if (direct_datain == 0xB0) {
                    addr_mode = All_External;
                    rx_byte_state = 0;
                }
                if (direct_datain == 0xB1) {
                    addr_mode = Read_Internal_Write_External;
                    rx_byte_state = 0;
                }
                if (direct_datain == 0xB2) {
                    addr_mode = Read_Fast_Internal_Write_External;
                    rx_byte_state = 0;
                }
                if (direct_datain == 0xB3) {
                    addr_mode = All_Fast_Internal;
                    rx_byte_state = 0;
                }
            }

        }

        if (internal_address_check(current_address) != All_External) {
            return internal_RAM[current_address];
        } else {
            return direct_datain;
        }
    }
}

// -------------------------------------------------
// Full write cycle with address and data written
// -------------------------------------------------
inline void write_byte(uint16_t local_address, uint8_t local_write_data) {

    // Always mirror writes to internal RAM, except ROM range at 0xD000 - 0xFFFF
    //
    if (local_address < 0xC000) internal_RAM[local_address] = local_write_data;

    // Internal RAM
    //
    if (internal_address_check(local_address) > 0x2) {
        last_access_internal_RAM = 1;
    } else {
        if (last_access_internal_RAM == 1) wait_for_CLK_rising_edge();
        last_access_internal_RAM = 0;

        digitalWriteFast(PIN_RDWR_n, 0x0);
        digitalWriteFast(PIN_SYNC, 0x0);
        send_address(local_address);

        // Drive the data bus pins from the Teensy to the bus driver which is inactive
        //
        digitalWriteFast(PIN_DATAOUT0, (local_write_data & 0x01));
        digitalWriteFast(PIN_DATAOUT1, (local_write_data & 0x02) >> 1);
        digitalWriteFast(PIN_DATAOUT2, (local_write_data & 0x04) >> 2);
        digitalWriteFast(PIN_DATAOUT3, (local_write_data & 0x08) >> 3);
        digitalWriteFast(PIN_DATAOUT4, (local_write_data & 0x10) >> 4);
        digitalWriteFast(PIN_DATAOUT5, (local_write_data & 0x20) >> 5);
        digitalWriteFast(PIN_DATAOUT6, (local_write_data & 0x40) >> 6);
        digitalWriteFast(PIN_DATAOUT7, (local_write_data & 0x80) >> 7);

        // During the second CLK phase, enable the data bus output drivers
        //
        wait_for_CLK_falling_edge();
        digitalWriteFast(PIN_DATAOUT_OE_n, 0x0);

        wait_for_CLK_rising_edge();
        digitalWriteFast(PIN_DATAOUT_OE_n, 0x1);
    }
    return;
}

// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------
//
// End 6502 Bus Interface Unit
//
// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

void push(uint8_t push_data) {
    write_byte(register_sp_fixed, push_data);
    register_sp = register_sp - 1;
    return;
}

uint8_t pop() {
    uint8_t temp = 0;
    register_sp = register_sp + 1;
    temp = read_byte(register_sp_fixed);
    return temp;
}

void Calc_Flags_NEGATIVE_ZERO(uint8_t local_data) {

    if (0x80 & local_data) register_flags = register_flags | 0x80; // Set the N flag
    else register_flags = register_flags & 0x7F; // Clear the N flag

    if (local_data == 0) register_flags = register_flags | 0x02; // Set the Z flag
    else register_flags = register_flags & 0xFD; // Clear the Z flag 

    return;
}

uint16_t Sign_Extend16(uint16_t reg_data) {
    if ((reg_data & 0x0080) == 0x0080) {
        return (reg_data | 0xFF00);
    } else {
        return (reg_data & 0x00FF);
    }
}

void Begin_Fetch_Next_Opcode() {
    register_pc++;
    assert_sync = 1;
    start_read(register_pc);
    return;
}

// -------------------------------------------------
// Addressing Modes
// -------------------------------------------------
uint8_t Fetch_Immediate() {
    register_pc++;
    return read_byte(register_pc);
}

uint8_t Fetch_ZeroPage() {
    effective_address = Fetch_Immediate();
    return read_byte(effective_address);
}

uint8_t Fetch_ZeroPage_X() {
    uint16_t bal;
    bal = Fetch_Immediate();
    read_byte(register_pc + 1);
    effective_address = (0x00FF & (bal + register_x));
    return read_byte(effective_address);
}

uint8_t Fetch_ZeroPage_Y() {
    uint16_t bal;
    bal = Fetch_Immediate();
    read_byte(register_pc + 1);
    effective_address = (0x00FF & (bal + register_y));
    return read_byte(effective_address);
}

uint16_t Calculate_Absolute() {
    uint16_t adl, adh;

    adl = Fetch_Immediate();
    adh = Fetch_Immediate() << 8;
    effective_address = adl + adh;
    return effective_address;
}

uint8_t Fetch_Absolute() {
    uint16_t adl, adh;

    adl = Fetch_Immediate();
    adh = Fetch_Immediate() << 8;
    effective_address = adl + adh;
    return read_byte(effective_address);
}

uint8_t Fetch_Absolute_X(uint8_t page_cross_check) {
    uint16_t bal, bah;
    uint8_t local_data;

    bal = Fetch_Immediate();
    bah = Fetch_Immediate() << 8;
    effective_address = bah + bal + register_x;
    local_data = read_byte(effective_address);

    if (page_cross_check == 1 && ((0xFF00 & effective_address) != (0xFF00 & bah))) {
        local_data = read_byte(effective_address);
    }
    return local_data;
}

uint8_t Fetch_Absolute_Y(uint8_t page_cross_check) {
    uint16_t bal, bah;
    uint8_t local_data;

    bal = Fetch_Immediate();
    bah = Fetch_Immediate() << 8;
    effective_address = bah + bal + register_y;
    local_data = read_byte(effective_address);

    if (page_cross_check == 1 && ((0xFF00 & effective_address) != (0xFF00 & bah))) {
        local_data = read_byte(effective_address);
    }
    return local_data;
}

uint8_t Fetch_Indexed_Indirect_X() {
    uint16_t bal;
    uint16_t adl, adh;
    uint8_t local_data;

    bal = Fetch_Immediate() + register_x;
    read_byte(bal);
    adl = read_byte(0xFF & bal);
    adh = read_byte(0xFF & (bal + 1)) << 8;
    effective_address = adh + adl;
    local_data = read_byte(effective_address);
    return local_data;
}

uint8_t Fetch_Indexed_Indirect_Y(uint8_t page_cross_check) {
    uint16_t ial, bah, bal;
    uint8_t local_data;

    ial = Fetch_Immediate();
    bal = read_byte(0xFF & ial);
    bah = read_byte(0xFF & (ial + 1)) << 8;

    effective_address = bah + bal + register_y;
    local_data = read_byte(effective_address);

    if (page_cross_check == 1 && ((0xFF00 & effective_address) != (0xFF00 & bah))) {
        local_data = read_byte(effective_address);
    }
    return local_data;
}

void Write_ZeroPage(uint8_t local_data) {
    effective_address = Fetch_Immediate();
    write_byte(effective_address, local_data);
    return;
}

void Write_Absolute(uint8_t local_data) {
    effective_address = Fetch_Immediate();
    effective_address = (Fetch_Immediate() << 8) + effective_address;
    write_byte(effective_address, local_data);
    return;
}

void Write_ZeroPage_X(uint8_t local_data) {
    effective_address = Fetch_Immediate();
    read_byte(effective_address);
    write_byte((0x00FF & (effective_address + register_x)), local_data);
    return;
}

void Write_ZeroPage_Y(uint8_t local_data) {
    effective_address = Fetch_Immediate();
    read_byte(effective_address);
    write_byte((0x00FF & (effective_address + register_y)), local_data);
    return;
}

void Write_Absolute_X(uint8_t local_data) {
    uint16_t bal, bah;

    bal = Fetch_Immediate();
    bah = Fetch_Immediate() << 8;
    effective_address = bal + bah + register_x;
    read_byte(effective_address);
    write_byte(effective_address, local_data);
    return;
}

void Write_Absolute_Y(uint8_t local_data) {
    uint16_t bal, bah;

    bal = Fetch_Immediate();
    bah = Fetch_Immediate() << 8;
    effective_address = bal + bah + register_y;
    read_byte(effective_address);

    if ((0xFF00 & effective_address) != (0xFF00 & bah)) {
        read_byte(effective_address);
    }
    write_byte(effective_address, local_data);
    return;
}

void Write_Indexed_Indirect_X(uint8_t local_data) {
    uint16_t bal;
    uint16_t adl, adh;

    bal = Fetch_Immediate();
    read_byte(bal);
    adl = read_byte(0xFF & (bal + register_x));
    adh = read_byte(0xFF & (bal + register_x + 1)) << 8;
    effective_address = adh + adl;
    write_byte(effective_address, local_data);
    return;
}

void Write_Indexed_Indirect_Y(uint8_t local_data) {
    uint16_t ial;
    uint16_t bal, bah;

    ial = Fetch_Immediate();
    bal = read_byte(ial);
    bah = read_byte(ial + 1) << 8;
    effective_address = bah + bal + register_y;
    read_byte(effective_address);
    write_byte(effective_address, local_data);
    return;
}

void Double_WriteBack(uint8_t local_data) {
    write_byte(effective_address, local_data);
    write_byte(effective_address, local_data);
    return;
}

// -------------------------------------------------
// Reset sequence for the 6502
// -------------------------------------------------
void reset_sequence() {
    uint16_t temp1, temp2;

#ifndef OFFLINE_DEBUG
    while (digitalReadFast(PIN_RESET) != 0) {} // Stay here until RESET deasserts
#endif

    digitalWriteFast(PIN_RDWR_n, 0x1);
    digitalWriteFast(PIN_DATAOUT_OE_n, 0x1);

    temp1 = read_byte(register_pc); // Address ??
    temp1 = read_byte(register_pc + 1); // Address ?? + 1
    temp1 = read_byte(register_sp_fixed); // Address SP
    temp1 = read_byte(register_sp_fixed - 1); // Address SP - 1
    temp1 = read_byte(register_sp_fixed - 2); // Address SP - 2

    temp1 = read_byte(0xFFFC); // Fetch Vector PCL
    temp2 = read_byte(0xFFFD); // Fetch Vector PCH

    register_flags = 0x34; // Set the I and B flags

    register_pc = (temp2 << 8) | temp1;
    assert_sync = 1;
    start_read(register_pc); // Fetch first opcode at vector PCH,PCL

    Serial.println("[RESET Complete]");

    return;
}

// -------------------------------------------------
// NMI Interrupt Processing
// -------------------------------------------------
void nmi_handler() {
    uint16_t temp1, temp2;

    wait_for_CLK_rising_edge(); // Begin processing on next CLK edge

    register_flags = register_flags | 0x20; // Set the flag[5]          
    register_flags = register_flags & 0xEF; // Clear the B flag     

    read_byte(register_pc + 1); // Fetch PC+1 (Discard)
    push(register_pc >> 8); // Push PCH
    push(register_pc); // Push PCL
    push(register_flags); // Push P
    temp1 = read_byte(0xFFFA); // Fetch Vector PCL
    temp2 = read_byte(0xFFFB); // Fetch Vector PCH

    register_flags = register_flags | 0x34; // Set the I flag and restore the B flag

    register_pc = (temp2 << 8) | temp1;
    assert_sync = 1;
    start_read(register_pc); // Fetch first opcode at vector PCH,PCL

    return;
}

// -------------------------------------------------
// BRK & IRQ Interrupt Processing
// -------------------------------------------------
void irq_handler(uint8_t opcode_is_brk) {
    uint16_t temp1, temp2;

    wait_for_CLK_rising_edge(); // Begin processing on next CLK edge

    register_flags = register_flags | 0x20; // Set the flag[5]          
    if (opcode_is_brk == 1) register_flags = register_flags | 0x10; // Set the B flag
    else register_flags = register_flags & 0xEF; // Clear the B flag

    read_byte(register_pc + 1); // Fetch PC+1 (Discard)
    push(register_pc >> 8); // Push PCH
    push(register_pc); // Push PCL
    push(register_flags); // Push P
    temp1 = read_byte(0xFFFE); // Fetch Vector PCL
    temp2 = read_byte(0xFFFF); // Fetch Vector PCH

    register_flags = register_flags | 0x34; // Set the I flag and restore the B flag

    register_pc = (temp2 << 8) | temp1;
    assert_sync = 1;
    start_read(register_pc); // Fetch first opcode at vector PCH,PCL

    return;
}

// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

#include "opcodes.h"

// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

void display_next_instruction(uint16_t pc, uint8_t opcode) {
    char buffer[32];
    sprintf(buffer, "PC:%04X - %02X", pc, opcode);
    Serial.println(buffer);
}

void display_registers() {
    char buf[32];
    sprintf(buf, "Registers:  A=%02X, X=%02X, Y=%02X", register_a, register_x, register_y);
    Serial.println(buf);
}

void display_info() {
    char buf[64];
    sprintf(buf, "Run-mode = %d, Address-mode = %d\n\rBreakpoint = %04X", run_mode, addr_mode, breakpoint);
    Serial.println(buf);
}

String get_command() {
    String s = "";

    // Print prompt
    Serial.print(">> ");
    while (1) {
        if (Serial.available()) {
            char c = Serial.read();
            // Serial.println("got 0x" + String(c, 16));
            switch(c) {
                case '\r':
                    return(s.toLowerCase());
                    break;
                case '\b':
                    s.remove(s.length()-1,1);
                    Serial.print("\b \b");
                    break;
                default:
                    if (isprint(c)) {
                        s.concat(c);
                        Serial.print(c);
                    }
                    break;
            }
        }
    }
}

String get_arg(String args, uint8_t arg_number) {

    //  Remove all multi-space substrings and use "|" between args
    do {
        args.replace("  ", " ");
    } while (args.indexOf("  ") >= 0);
    args.replace(" ", "|");

    // Check for empty 
    if (args.length()==0 or args[0] == '|')
        return("");

    int start_idx = 0;
    int end_idx;
    for (int i=0; i<=arg_number; i++) {
        if (i > 0)
            start_idx = end_idx + 1;
        end_idx = args.indexOf(" ", start_idx);
        if (end_idx == -1)                      // Must be last arg
            end_idx = args.length();
    }

    String rv = args.substring(start_idx, end_idx);
    return(rv);
}

String parse_next_arg(String &_src, String &remainder) {
    String arg = "";

    String src = _src.trim();

    // zero-length means nothing to parse
    if (src.length()) {
        int idx = src.indexOf(' ');

        // Serial.print("idx = "+String(idx) + ", ");

        if (idx>0) {
            //  Space in source, pull first arg
            arg = src.substring(0, idx);
            remainder = src.substring(idx+1);
        }
        else {
            // No space... just one arg
            arg = src;
            remainder = "";
        }
    }

    // Serial.println("Result = "+arg+ ", remainder = "+remainder);

    return arg;
}

ENUM_RUN_MODE process_command(String input) {

    // Serial.println("\nProcessing command: "+input);

	//
	//  All commands are of the form: <2-char command>( <arg> (<arg> ...))
	//

    String remainder = "";
    String cmd  = parse_next_arg(input, remainder);
    String arg1 = parse_next_arg(remainder, remainder);
    String arg2 = parse_next_arg(remainder, remainder);

    word cmd_int = command_int(cmd);

    if (debug_mode && false) {
        Serial.println("Command and args: \'"+cmd+"\', \'"+arg1+"\', \'"+arg2+"\'");
        char buf[32];
        sprintf(buf, "Command-int = %04X", cmd_int);
        Serial.println(buf);
    }

    switch (cmd_int) {
        case CMD_NOP:
            //  User entered a zero-length line at prompt
            break;

        case CMD_QM:
        case CMD_HE:
            Serial.println(String("Available Commands:\n\r")+
                           "    MD <mode>               Set memory addressing mode (0-3 see below)\n\r"+
                           "    DR                      Dump registers\n\r"+
                           "    SS                      Single-step execution\n\r"+
                           "    GO (<address>)          Begin execution (at optional address)\n\r"+
                           "    BK <address>            Set execution breakpoint\n\r"+
                           "    SR <reg> <value>        Set register (PC, A, X, Y) to value\n\r"+
                           "    RD <address> (<count>)  Read from memory address, displays <count> values\n\r"+
                           "    WR <address> <value>    Write value to memory address\n\r"+
                           "\n"+
                           "    Addressing Modes:\n\r"+
                           "       0 - All exernal memory accesses\n\r"+
                           "       1 - Reads use cycle accurate internal memory and writes pass through to motherboard\n\r"+
                           "       2 - Reads accelerated using internal memory and writes pass through to motherboard\n\r"+
                           "       3 - All read and write accesses use accelerated internal memory\n\r");

            break;

        case CMD_MD:
            {
                byte a_mode = strtoul(arg1.c_str(), 0, 10);
                if (a_mode < 4)
                    addr_mode = (ADDR_MODE)a_mode;
                else
                    Serial.println("MD error. Illegal argument: "+arg1);
            }
            break;

        case CMD_DR:
            display_registers();
            break;

        case CMD_SR:
            {
                word value = strtoul(arg2.c_str(), 0, 16);
                // char buf[64]; sprintf(buf, "reg=%s, arg=%s, value=%04X", arg1.c_str(), arg2.c_str(), value); Serial.println(buf);
                if (arg1=="pc") {
                    register_pc = value & 0xFFFF;
                }
                else if (arg1=="a") {
                    register_a = value & 0xFF;
                }
                else if (arg1=="x") {
                    register_x = value & 0xFF;
                }
                else if (arg1=="y") {
                    register_y = value & 0xFF;
                }
                else {
                    Serial.println("ERROR: unknown register identifier");
                }
            }
            display_registers();
            break;

        case CMD_IN:
            display_registers();
            display_info();
            break;

        case CMD_GO:
            run_mode = RUNNING;
            if (arg1.length()) {
                register_pc = strtoul(arg1.c_str(), 0, 16);
            }
            break;

        case CMD_SS:
            run_mode = SINGLE_STEP;
            break;

        case CMD_BK:
            {
                word addr = strtoul(arg1.c_str(), 0, 16);
                breakpoint = addr;
                Serial.println("OK");                
            }
            break;

        //
        //  Command:  RD <addr> (<count>)
        //
        //      Read one or more bytes from <address>. Default count is one.
        //
        case CMD_RD:
            {
                char s[32];
                byte count = 1;
                word addr = strtoul(arg1.c_str(), 0, 16);
                if (arg2.length()) {
                    count = arg2.toInt() & 0xFF;
                }

                sprintf(s,"[%04X] = ", addr);
                Serial.print(s);

                for (byte i=0; i<count; i++) {
                    if ((i != 0) && (i % 8 == 0)) {
                        sprintf(s,"\n\r[%04X] = ", addr);
                        Serial.print(s);
                    }
                    byte data = read_byte(addr++);
                    sprintf(s, "%02X ", data);
                    Serial.print(s);
                }
                Serial.println("");
            }
            break;

        //
        //  Command:  WR <addr> <value> (<value> ...)
        //
        //      Write one or more bytes to <address>.
        //
        case CMD_WR:
            {
                word addr = strtoul(arg1.c_str(), 0, 16);
                byte data = strtoul(arg2.c_str(), 0, 16);
                write_byte(addr, data);
                while (remainder.length()) {
                    String d = parse_next_arg(remainder, remainder);
                    data = strtoul(d.c_str(), 0, 16);
                    write_byte(++addr, data);
                }
                Serial.println("OK");
            }
            break;

        default:
            Serial.println("\n\nERROR: Can't parse command: \"" + input + "\" ("+cmd+", "+arg1+", "+arg2+")");
            break;
    }

    return(run_mode);
}


// -------------------------------------------------
//
// Main loop 
//
// -------------------------------------------------
void loop() {

    Serial.println("[Starting Loop]");

    // Give Teensy 4.1 a moment
    delay(50);
    wait_for_CLK_rising_edge();
    wait_for_CLK_rising_edge();
    wait_for_CLK_rising_edge();

    reset_sequence();

    while (1) {

        if (direct_reset == 1) reset_sequence();

        // Poll for NMI and IRQ
        //
        if (nmi_n_old == 0 && direct_nmi == 1) nmi_handler();
        nmi_n_old = direct_nmi;
        if (direct_irq == 0x1 && (flag_i) == 0x0) irq_handler(0x0);

        next_instruction = finish_read_byte();

        //============================================================================
        //  ICE interface code
        //
        if (breakpoint && (run_mode==RUNNING) && (register_pc==breakpoint)) {
            run_mode = WAITING;
        }

        if (run_mode != RUNNING) {

            do {
                display_next_instruction(register_pc, next_instruction);
                String c = get_command();
                Serial.println(" ");
                run_mode = process_command(c);
                Serial.println(" ");

                //  Update the next_instruction, as PC or memory may have changed
                if (run_mode != RUNNING) {
                    next_instruction = read_byte(register_pc);
                }
            } while (run_mode == WAITING);
        }

        assert_sync = 0;
        switch (next_instruction) {

        case 0x00:
            irq_handler(0x1);
            break; // BRK - Break
        case 0x01:
            opcode_0x01();
            break; // OR - Indexed Indirect X
        case 0x02:
            opcode_0x02();
            break; // JAM
        case 0x03:
            opcode_0x03();
            break; // SLO - Indexed Indirect X
        case 0x04:
            opcode_0x04();
            break; // NOP - ZeroPage
        case 0x05:
            opcode_0x05();
            break; // OR ZeroPage
        case 0x06:
            opcode_0x06();
            break; // ASL A - Arithmetic Shift Left - ZeroPage
        case 0x07:
            opcode_0x07();
            break; // SLO - ZeroPage
        case 0x08:
            opcode_0x08();
            break; // PHP - Push processor status to the stack
        case 0x09:
            opcode_0x09();
            break; // OR - Immediate
        case 0x0A:
            opcode_0x0A();
            break; // ASL A
        case 0x0B:
            opcode_0x0B();
            break; // ANC - Immediate
        case 0x0C:
            opcode_0x0C();
            break; // NOP - Absolute
        case 0x0D:
            opcode_0x0D();
            break; // OR - Absolute
        case 0x0E:
            opcode_0x0E();
            break; // ASL A - Arithmetic Shift Left - Absolute
        case 0x0F:
            opcode_0x0F();
            break; // SLO - Absolute
        case 0x10:
            opcode_0x10();
            break; // BNE - Branch on Zero Clear
        case 0x11:
            opcode_0x11();
            break; // OR Indirect Indexed  Y
        case 0x12:
            opcode_0x12();
            break; // JAM
        case 0x13:
            opcode_0x13();
            break; // Indirect Indexed  Y
        case 0x14:
            opcode_0x14();
            break; // NOP - ZeroPage , X
        case 0x15:
            opcode_0x15();
            break; // OR - ZeroPage,X
        case 0x16:
            opcode_0x16();
            break; // ASL A - Arithmetic Shift Left - ZeroPage , X
        case 0x17:
            opcode_0x17();
            break; // SLO - ZeroPage , X
        case 0x18:
            opcode_0x18();
            break; // CLC
        case 0x19:
            opcode_0x19();
            break; // OR - Absolute,Y
        case 0x1A:
            opcode_0xEA();
            break; // NOP
        case 0x1B:
            opcode_0x1B();
            break; // SLO - Absolute , Y
        case 0x1C:
            opcode_0x1C();
            break; // NOP - Absolute , X
        case 0x1D:
            opcode_0x1D();
            break; // OR - Absolute,X
        case 0x1E:
            opcode_0x1E();
            break; // ASL A - Arithmetic Shift Left - Absolute , X
        case 0x1F:
            opcode_0x1F();
            break; // SLO - Absolute , X
        case 0x20:
            opcode_0x20();
            break; // JSR - Jump to Subroutine
        case 0x21:
            opcode_0x21();
            break; // AND - Indexed Indirect
        case 0x22:
            opcode_0x22();
            break; // JAM
        case 0x23:
            opcode_0x23();
            break; // RLA - Indexed Indirect X
        case 0x24:
            opcode_0x24();
            break; // BIT - ZeroPage
        case 0x25:
            opcode_0x25();
            break; // AND - ZeroPage
        case 0x26:
            opcode_0x26();
            break; // ROL - Rotate Left - ZeroPage
        case 0x27:
            opcode_0x27();
            break; // RLA - ZeroPage
        case 0x28:
            opcode_0x28();
            break; // PLP - Pop processor status from the stack
        case 0x29:
            opcode_0x29();
            break; // AND - Immediate
        case 0x2A:
            opcode_0x2A();
            break; // ROL A
        case 0x2B:
            opcode_0x2B();
            break; // ANC - Immediate
        case 0x2C:
            opcode_0x2C();
            break; // BIT - Absolute
        case 0x2D:
            opcode_0x2D();
            break; // AND - Absolute
        case 0x2E:
            opcode_0x2E();
            break; // ROL - Rotate Left - Absolute
        case 0x2F:
            opcode_0x2F();
            break; // RLA - Absolute
        case 0x30:
            opcode_0x30();
            break; // BMI - Branch on Minus (N Flag Set)
        case 0x31:
            opcode_0x31();
            break; // AND - Indirect Indexed
        case 0x32:
            opcode_0x32();
            break; // JAM
        case 0x33:
            opcode_0x33();
            break; // RLA - Indirect Indexed  Y
        case 0x34:
            opcode_0x34();
            break; // NOP - ZeroPage , X
        case 0x35:
            opcode_0x35();
            break; // AND - ZeroPage,X
        case 0x36:
            opcode_0x36();
            break; // ROL - Rotate Left - ZeroPage , X
        case 0x37:
            opcode_0x37();
            break; // RLA - ZeroPage , X
        case 0x38:
            opcode_0x38();
            break; // SEC
        case 0x39:
            opcode_0x39();
            break; // AND - Absolute,Y
        case 0x3A:
            opcode_0xEA();
            break; // NOP
        case 0x3B:
            opcode_0x3B();
            break; // RLA - Absolute , Y
        case 0x3C:
            opcode_0x3C();
            break; // NOP - Absolute , X
        case 0x3D:
            opcode_0x3D();
            break; // AND - Absolute,X
        case 0x3E:
            opcode_0x3E();
            break; // ROL - Rotate Left - Absolute , X
        case 0x3F:
            opcode_0x3F();
            break; // RLA - Absolute , X
        case 0x40:
            opcode_0x40();
            break; // RTI - Return from Interrupt
        case 0x41:
            opcode_0x41();
            break; // EOR - Indexed Indirect X
        case 0x42:
            opcode_0x42();
            break; // JAM
        case 0x43:
            opcode_0x43();
            break; // SRE - Indexed Indirect X
        case 0x44:
            opcode_0x44();
            break; // NOP - ZeroPage
        case 0x45:
            opcode_0x45();
            break; // EOR - ZeroPage
        case 0x46:
            opcode_0x46();
            break; // LSR - Logical Shift Right - ZeroPage
        case 0x47:
            opcode_0x47();
            break; // SRE - ZeroPage
        case 0x48:
            opcode_0x48();
            break; // PHA - Push Accumulator to the stack
        case 0x49:
            opcode_0x49();
            break; // EOR - Immediate
        case 0x4A:
            opcode_0x4A();
            break; // LSR A
        case 0x4B:
            opcode_0x4B();
            break; // ALR - Immediate
        case 0x4C:
            opcode_0x4C();
            break; // JMP - Jump Absolute
        case 0x4D:
            opcode_0x4D();
            break; // EOR - Absolute
        case 0x4E:
            opcode_0x4E();
            break; // LSR - Logical Shift Right - Absolute
        case 0x4F:
            opcode_0x4F();
            break; // SRE - Absolute
        case 0x50:
            opcode_0x50();
            break; // BVC - Branch on Overflow Clear
        case 0x51:
            opcode_0x51();
            break; // EOR - Indirect Indexed  Y
        case 0x52:
            opcode_0x52();
            break; // JAM
        case 0x53:
            opcode_0x53();
            break; // SRE - Indirect Indexed  Y
        case 0x54:
            opcode_0x54();
            break; // NOP - ZeroPage , X
        case 0x55:
            opcode_0x55();
            break; // EOR - ZeroPage,X
        case 0x56:
            opcode_0x56();
            break; // LSR - Logical Shift Right - ZeroPage , X
        case 0x57:
            opcode_0x57();
            break; // SRE - ZeroPage , X
        case 0x58:
            opcode_0x58();
            break; // CLI
        case 0x59:
            opcode_0x59();
            break; // EOR - Absolute,Y
        case 0x5A:
            opcode_0xEA();
            break; // NOP
        case 0x5B:
            opcode_0x5B();
            break; // RE - Absolute , Y
        case 0x5C:
            opcode_0x5C();
            break; // NOP - Absolute , X
        case 0x5D:
            opcode_0x5D();
            break; // EOR - Absolute,X
        case 0x5E:
            opcode_0x5E();
            break; // LSR - Logical Shift Right - Absolute , X
        case 0x5F:
            opcode_0x5F();
            break; // SRE - Absolute , X
        case 0x60:
            opcode_0x60();
            break; // RTS - Return from Subroutine
        case 0x61:
            opcode_0x61();
            break; // ADC - Indexed Indirect X
        case 0x62:
            opcode_0x62();
            break; // JAM
        case 0x63:
            opcode_0x63();
            break; // RRA - Indexed Indirect X
        case 0x64:
            opcode_0x64();
            break; // NOP - ZeroPage
        case 0x65:
            opcode_0x65();
            break; // ADC - ZeroPage
        case 0x66:
            opcode_0x66();
            break; // ROR - Rotate Right - ZeroPage
        case 0x67:
            opcode_0x67();
            break; // RRA - ZeroPage
        case 0x68:
            opcode_0x68();
            break; // PLA - Pop Accumulator from the stack
        case 0x69:
            opcode_0x69();
            break; // ADC - Immediate
        case 0x6A:
            opcode_0x6A();
            break; // ROR A
        case 0x6B:
            opcode_0x6B();
            break; // ARR - Immediate
        case 0x6C:
            opcode_0x6C();
            break; // JMP - Jump Indirect
        case 0x6D:
            opcode_0x6D();
            break; // ADC - Absolute
        case 0x6E:
            opcode_0x6E();
            break; // ROR - Rotate Right - Absolute
        case 0x6F:
            opcode_0x6F();
            break; // RRA - Absolute
        case 0x70:
            opcode_0x70();
            break; // BVS - Branch on Overflow Set
        case 0x71:
            opcode_0x71();
            break; // ADC - Indirect Indexed  Y
        case 0x72:
            opcode_0x72();
            break; // JAM
        case 0x73:
            opcode_0x73();
            break; // RRA - Indirect Indexed  Y
        case 0x74:
            opcode_0x74();
            break; // NOP - ZeroPage , X
        case 0x75:
            opcode_0x75();
            break; // ADC - ZeroPage , X
        case 0x76:
            opcode_0x76();
            break; // ROR - Rotate Right - ZeroPage , X
        case 0x77:
            opcode_0x77();
            break; // RRA - ZeroPage , X
        case 0x78:
            opcode_0x78();
            break; // SEI
        case 0x79:
            opcode_0x79();
            break; // ADC - Absolute , Y
        case 0x7A:
            opcode_0xEA();
            break; // NOP
        case 0x7B:
            opcode_0x7B();
            break; // RRA - Absolute , Y
        case 0x7C:
            opcode_0x7C();
            break; // NOP - Absolute , X
        case 0x7D:
            opcode_0x7D();
            break; // ADC - Absolute , X
        case 0x7E:
            opcode_0x7E();
            break; // ROR - Rotate Right - Absolute , X
        case 0x7F:
            opcode_0x7F();
            break; // RRA - Absolute , X
        case 0x80:
            opcode_0x80();
            break; // NOP - Immediate
        case 0x81:
            opcode_0x81();
            break; // STA - Indexed Indirect X
        case 0x82:
            opcode_0x82();
            break; // NOP - Immediate
        case 0x83:
            opcode_0x83();
            break; // SAX - Indexed Indirect X
        case 0x84:
            opcode_0x84();
            break; // STY - ZeroPage
        case 0x85:
            opcode_0x85();
            break; // STA - ZeroPage
        case 0x86:
            opcode_0x86();
            break; // STX - ZeroPage
        case 0x87:
            opcode_0x87();
            break; // SAX - ZeroPage
        case 0x88:
            opcode_0x88();
            break; // DEY
        case 0x89:
            opcode_0x89();
            break; // NOP - Immediate
        case 0x8A:
            opcode_0x8A();
            break; // TXA
        case 0x8B:
            opcode_0x8B();
            break; // ANE - Immediate
        case 0x8C:
            opcode_0x8C();
            break; // STY - Absolute
        case 0x8D:
            opcode_0x8D();
            break; // STA - Absolute
        case 0x8E:
            opcode_0x8E();
            break; // STX - Absolute
        case 0x8F:
            opcode_0x8F();
            break; // SAX - Absolute
        case 0x90:
            opcode_0x90();
            break; // BCC - Branch on Carry Clear
        case 0x91:
            opcode_0x91();
            break; // STA - Indirect Indexed  Y
        case 0x92:
            opcode_0x92();
            break; // JAM
        case 0x93:
            opcode_0x93();
            break; // SHA - ZeroPage , Y
        case 0x94:
            opcode_0x94();
            break; // STY - ZeroPage , X
        case 0x95:
            opcode_0x95();
            break; // STA - ZeroPage , X
        case 0x96:
            opcode_0x96();
            break; // STX - ZeroPage , Y
        case 0x97:
            opcode_0x97();
            break; // SAX - ZeroPage , Y
        case 0x98:
            opcode_0x98();
            break; // TYA
        case 0x99:
            opcode_0x99();
            break; // STA - Absolute , Y
        case 0x9A:
            opcode_0x9A();
            break; // TXS
        case 0x9B:
            opcode_0x9B();
            break; // TAS - Absolute , Y 
        case 0x9C:
            opcode_0x9C();
            break; // SHY - Absolute , X
        case 0x9D:
            opcode_0x9D();
            break; // STA - Absolute , X
        case 0x9E:
            opcode_0x9E();
            break; // SHX - Absolute , Y
        case 0x9F:
            opcode_0x9F();
            break; // SHA - Absolute , Y
        case 0xA0:
            opcode_0xA0();
            break; // LDY - Immediate
        case 0xA1:
            opcode_0xA1();
            break; // LDA - Indexed Indirect X
        case 0xA2:
            opcode_0xA2();
            break; // LDX - Immediate
        case 0xA3:
            opcode_0xA3();
            break; // LAX - Indexed Indirect X
        case 0xA4:
            opcode_0xA4();
            break; // LDY - ZeroPage
        case 0xA5:
            opcode_0xA5();
            break; // LDA - ZeroPage
        case 0xA6:
            opcode_0xA6();
            break; // LDX - ZeroPage
        case 0xA7:
            opcode_0xA7();
            break; // LAX - ZeroPage
        case 0xA8:
            opcode_0xA8();
            break; // TAY
        case 0xA9:
            opcode_0xA9();
            break; // LDA - Immediate
        case 0xAA:
            opcode_0xAA();
            break; // TAX
        case 0xAB:
            opcode_0xAB();
            break; // LAX - Immediate
        case 0xAC:
            opcode_0xAC();
            break; // LDY - Absolute
        case 0xAD:
            opcode_0xAD();
            break; // LDA - Absolute
        case 0xAE:
            opcode_0xAE();
            break; // LDX - Absolute
        case 0xAF:
            opcode_0xAF();
            break; // LAX - Absolute
        case 0xB0:
            opcode_0xB0();
            break; // BCS - Branch on Carry Set
        case 0xB1:
            opcode_0xB1();
            break; // LDA - Indirect Indexed  Y
        case 0xB2:
            opcode_0xB2();
            break; // JAM
        case 0xB3:
            opcode_0xB3();
            break; // LAX - Indirect Indexed  Y
        case 0xB4:
            opcode_0xB4();
            break; // LDY - ZeroPage , X
        case 0xB5:
            opcode_0xB5();
            break; // LDA - ZeroPage , X
        case 0xB6:
            opcode_0xB6();
            break; // LDX - ZeroPage , Y
        case 0xB7:
            opcode_0xB7();
            break; // LAX - ZeroPage , Y
        case 0xB8:
            opcode_0xB8();
            break; // CLV
        case 0xB9:
            opcode_0xB9();
            break; // LDA - Absolute , Y
        case 0xBA:
            opcode_0xBA();
            break; // TSX
        case 0xBB:
            opcode_0xBB();
            break; // LAS - Absolute , Y
        case 0xBC:
            opcode_0xBC();
            break; // LDY - Absolute , X
        case 0xBD:
            opcode_0xBD();
            break; // LDA - Absolute , X
        case 0xBE:
            opcode_0xBE();
            break; // LDX - Absolute , Y
        case 0xBF:
            opcode_0xBF();
            break; // LAX - Absolute , Y
        case 0xC0:
            opcode_0xC0();
            break; // CPY - Immediate
        case 0xC1:
            opcode_0xC1();
            break; // CMP - Indexed Indirect X
        case 0xC2:
            opcode_0xC2();
            break; // NOP - Immediate
        case 0xC3:
            opcode_0xC3();
            break; // DCP - Indexed Indirect X
        case 0xC4:
            opcode_0xC4();
            break; // CPY - ZeroPage
        case 0xC5:
            opcode_0xC5();
            break; // CMP - ZeroPage
        case 0xC6:
            opcode_0xC6();
            break; // DEC - ZeroPage
        case 0xC7:
            opcode_0xC7();
            break; // DCP - ZeroPage
        case 0xC8:
            opcode_0xC8();
            break; // INY
        case 0xC9:
            opcode_0xC9();
            break; // CMP - Immediate
        case 0xCA:
            opcode_0xCA();
            break; // DEX
        case 0xCB:
            opcode_0xCB();
            break; // SBX - Immediate
        case 0xCC:
            opcode_0xCC();
            break; // CPY - Absolute
        case 0xCD:
            opcode_0xCD();
            break; // CMP - Absolute
        case 0xCE:
            opcode_0xCE();
            break; // DEC - Absolute
        case 0xCF:
            opcode_0xCF();
            break; // DCP - Absolute
        case 0xD0:
            opcode_0xD0();
            break; // BNE - Branch on Zero Clear
        case 0xD1:
            opcode_0xD1();
            break; // CMP - Indirect Indexed  Y
        case 0xD2:
            opcode_0xD2();
            break; // JAM
        case 0xD3:
            opcode_0xD3();
            break; // DCP - Indirect Indexed  Y
        case 0xD4:
            opcode_0xD4();
            break; // NOP - ZeroPage , X
        case 0xD5:
            opcode_0xD5();
            break; // CMP - ZeroPage , X
        case 0xD6:
            opcode_0xD6();
            break; // DEC - ZeroPage , X
        case 0xD7:
            opcode_0xD7();
            break; // DCP - ZeroPage , X
        case 0xD8:
            opcode_0xD8();
            break; // CLD
        case 0xD9:
            opcode_0xD9();
            break; // CMP - Absolute , Y
        case 0xDA:
            opcode_0xEA();
            break; // NOP
        case 0xDB:
            opcode_0xDB();
            break; // DCP - Absolute , Y
        case 0xDC:
            opcode_0xDC();
            break; // NOP - Absolute , X
        case 0xDD:
            opcode_0xDD();
            break; // CMP - Absolute , X
        case 0xDE:
            opcode_0xDE();
            break; // DEC - Absolute , X
        case 0xDF:
            opcode_0xDF();
            break; // DCP - Absolute , X
        case 0xE0:
            opcode_0xE0();
            break; // CPX - Immediate
        case 0xE1:
            opcode_0xE1();
            break; // SBC - Indexed Indirect X
        case 0xE2:
            opcode_0xE2();
            break; // NOP - Immediate
        case 0xE3:
            opcode_0xE3();
            break; // ISC - Indexed Indirect X
        case 0xE4:
            opcode_0xE4();
            break; // CPX - ZeroPage
        case 0xE5:
            opcode_0xE5();
            break; // SBC - ZeroPage
        case 0xE6:
            opcode_0xE6();
            break; // INC - ZeroPage
        case 0xE7:
            opcode_0xE7();
            break; // ISC - ZeroPage
        case 0xE8:
            opcode_0xE8();
            break; // INX
        case 0xE9:
            opcode_0xE9();
            break; // SBC - Immediate
        case 0xEA:
            opcode_0xEA();
            break; // NOP
        case 0xEB:
            opcode_0xE9();
            break; // SBC - Immediate
        case 0xEC:
            opcode_0xEC();
            break; // CPX - Absolute
        case 0xED:
            opcode_0xED();
            break; // SBC - Absolute
        case 0xEE:
            opcode_0xEE();
            break; // INC - Absolute
        case 0xEF:
            opcode_0xEF();
            break; // ISC - Absolute
        case 0xF0:
            opcode_0xF0();
            break; // BEQ - Branch on Zero Set
        case 0xF1:
            opcode_0xF1();
            break; // SBC - Indirect Indexed  Y
        case 0xF2:
            opcode_0xF2();
            break; // JAM
        case 0xF3:
            opcode_0xF3();
            break; // ISC - Indirect Indexed  Y
        case 0xF4:
            opcode_0xF4();
            break; // NOP - ZeroPage , X
        case 0xF5:
            opcode_0xF5();
            break; // SBC - ZeroPage , X
        case 0xF6:
            opcode_0xF6();
            break; // INC - ZeroPage , X
        case 0xF7:
            opcode_0xF7();
            break; // ISC - ZeroPage , X
        case 0xF8:
            opcode_0xF8();
            break; // SED
        case 0xF9:
            opcode_0xF9();
            break; // SBC - Absolute , Y
        case 0xFA:
            opcode_0xEA();
            break; // NOP
        case 0xFB:
            opcode_0xFB();
            break; // ISC - Absolute , Y
        case 0xFC:
            opcode_0xFC();
            break; // NOP - Absolute , X
        case 0xFD:
            opcode_0xFD();
            break; // SBC - Absolute , X
        case 0xFF:
            opcode_0xFF();
            break; // 
        default:
            Serial.println("ERROR: Illegal instruction");
            run_mode = WAITING;
            break;
        }
    }
}
