//
//  MOS 6502 in-circuit emulator (ICE)
//  Apple II+ version with acceleration via keystrokes or UART
//
//  Author: Steven Raasch
//  Based on work by:  Ted Fried, MicroCore Labs
//
//  Description:
//  ============
//
//  Accelerartion using Apple II keyboard keystrokes:
//   - Press left-arrow(L), right-arrow(R), left-arrow(L), then the number for the addressing mode desired.
//   - This method only works if the Apple II is scanning for keystrokes.  If not, then the UART method of acceleration must be used.
//        Example:  press LRL3 for acceletaion moode 3
//
//  Accelerartion using the Teensy 4.1 USB UART
//   - Open a UART terminal to the Teensy 4.1 board. You can use the Arduino GUI Tools --> Serial Monitor or any other terminal emulator
//   - Simply send a number over the UART to the MCL65 board for the addressing mode desired
//       Example:  press 3 for addressing moode 3
//
//  Acceleration addr_mode may also be hard-coded or the accelerated address ranges can be changed in internal_address_check()
//
//--------------------------------------------------------------------------------------------------
//
//  This ICE was inspired by, and based on the original MLC65+ Emulator. I am very grateful to Ted
//  for doing the hard work of creating the bus interface and implementing the instruction emulation
//  that I use here.
//
//  See https://github.com/MicroCoreLabs/Projects for this and other of Ted's projects.
//
//--------------------------------------------------------------------------------------------------
//
//
// Copyright (c) 2021 Ted Fried
// Copyright (c) 2025 Steven Raasch
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

#define VERSION_NUM "1.2"

#define OFFLINE_DEBUG true

#include <stdint.h>

#include "Apple2-ICE.h"

enum ENUM_RUN_MODE { WAITING = 0,
                     SINGLE_STEP,
                     RUNNING,
                     RESETTING };

//-------------------------------------------------------------------------
//  Interface to the Bus Interface Unit
//-------------------------------------------------------------------------
inline void start_read(uint32_t local_address);
inline uint8_t read_byte(uint16_t local_address);
inline void write_byte(uint16_t local_address, uint8_t local_write_data);
void initialize_roms();

//-------------------------------------------------------------------------
//   Interface to the opcode_decoder.ino
//-------------------------------------------------------------------------
struct OpDecoder {
    String opcode;
    String operands;
    String flags;
    uint8_t max_cycles;
    uint8_t length;
    uint16_t (*operation)(void);
} opcode_info[256];

String decode_instruction(uint8_t op, uint8_t op1, uint8_t op2);
void initialize_opcode_info();

//-------------------------------------------------
//
// 6502 Flags
//
#define flag_n (register_flags & 0x80) >> 7  // register_flags[7]
#define flag_v (register_flags & 0x40) >> 6  // register_flags[6]
#define flag_b (register_flags & 0x10) >> 4  // register_flags[4]
#define flag_d (register_flags & 0x08) >> 3  // register_flags[3]
#define flag_i (register_flags & 0x04) >> 2  // register_flags[2]
#define flag_z (register_flags & 0x02) >> 1  // register_flags[1]
#define flag_c (register_flags & 0x01) >> 0  // register_flags[0]

//-------------------------------------------------
//
// 6502 stack always in Page 1
//
#define register_sp_fixed (0x0100 | register_sp)

enum ADDR_MODE {
    All_External = 0,
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

uint8_t direct_reset = 0;
uint8_t direct_ready_n = 0;
uint8_t direct_irq = 0;
uint8_t direct_nmi = 0;
uint8_t global_temp = 0;

ADDR_MODE addr_mode = All_External;

uint16_t register_pc = 0;
uint16_t effective_address = 0;

String flag_status(void) {
    String s;

    s = s + (flag_c ? "C" : "-");
    s = s + (flag_z ? "Z" : "-");
    s = s + (flag_i ? "I" : "-");
    s = s + (flag_d ? "D" : "-");
    s = s + (flag_b ? "B" : "-");
    s = s + (flag_v ? "V" : "-");
    s = s + (flag_n ? "N" : "-");

    return (s);
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
//
//  Define interactive commands
//

// Take the first two characters and turn them into a value that can be used in a switch/case statement
#define command_int(_str_) (((word)_str_[0] << 8) + (word)_str_[1])

const word CMD_MD = command_int("md");    // MD - Memory addressing mode
const word CMD_RS = command_int("rs");    // RS - Reset CPU
const word CMD_SS = command_int("ss");    // SS - Single Step
const word CMD_GO = command_int("go");    // GO - Execute from current PC
const word CMD_BK = command_int("bk");    // BK - Set a breakpoint
const word CMD_RD = command_int("rd");    // RD - Read from memory
const word CMD_WR = command_int("wr");    // WR - Write to memory
const word CMD_FI = command_int("fi");    // FI - Fill memory with a value
const word CMD_DR = command_int("dr");    // DR - Display registers
const word CMD_SR = command_int("sr");    // SR - Display individual register (pc, a, x, y)
const word CMD_TR = command_int("tr");    // TR - Enable PC Tracing
const word CMD_FE = command_int("fe");    // FE - Execution fencing
const word CMD_LI = command_int("li");    // LI - List instructions
const word CMD_IN = command_int("in");    // IN - Display info
const word CMD_DE = command_int("de");    // DE - Toggle debug mode
const word CMD_QM = command_int("?\0");   // ?  - Help
const word CMD_HE = command_int("h\0");   // H  - Help
const word CMD_Test = command_int("tt");  // tt -- TEST operation
const word CMD_NOP = 0;

const int max_command_parts = 10;


word breakpoint = 0;
word runto_address = 0;
bool pc_trace = false;
unsigned pc_trace_index;

bool run_fence = false;
uint16_t run_fence_low, run_fence_high;

ENUM_RUN_MODE run_mode;
const char* run_mode_str[] = {"Waiting", "Single-Step", "Running", "Resetting"};

bool debug_mode = true;
String last_user_command = "";

// -------------------------------------------------
// Check for CLK activity --> determines debug mode
// -------------------------------------------------
bool check_for_CLK_activity() {
    unsigned long start = millis();
    while (((GPIO6_DR >> 12) & 0x1) ==
           0) {  // Teensy 4.1 Pin-24  GPIO6_DR[12]  CLK
        if (millis() - start > 500) {
            return (false);
        }
    }
    while (((GPIO6_DR >> 12) & 0x1) != 0) {
        if (millis() - start > 500) {
            return (false);
        }
    }

    return (true);
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------

// Setup Teensy 4.1 IO's
//
void setup() {
    pinMode(PIN_CLK0_INV, INPUT);
    pinMode(PIN_RESET, INPUT);
    pinMode(PIN_READY_n, INPUT);
    pinMode(PIN_IRQ, INPUT);
    pinMode(PIN_NMI, INPUT);
    pinMode(PIN_RDWR_n, OUTPUT);
    pinMode(PIN_SYNC, OUTPUT);  // We don't assert this signal ATM

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

    initialize_roms();

    Serial.begin(115200);
    Serial.setTimeout(5000);

    Serial.println(String("Apple ][+ In-circuit Emulator\n\rVersion ") +
                   VERSION_NUM);

    if (!check_for_CLK_activity()) {
        debug_mode = true;
        addr_mode = All_Fast_Internal;
        Serial.println("No CLK activity detected. Starting in DEBUG MODE using INTERNAL MEMORY");
    }
    else {
        debug_mode = false;
        addr_mode = All_External;
        Serial.println(
            "CLK activity detected... Starting in NORMAL MODE using external memory");
    }

    run_mode = WAITING;
    initialize_opcode_info();

    //============================================
    //  Reset the machine
    //
    // Give Teensy 4.1 a moment
    delay(50);
    sample_at_CLK0_falling_edge();
    sample_at_CLK0_falling_edge();
    sample_at_CLK0_falling_edge();

    reset_sequence();
}

// -------------------------------------------------
// Reset sequence for the 6502
// -------------------------------------------------
void reset_sequence() {
    uint16_t temp1, temp2;

#ifndef OFFLINE_DEBUG
    while (digitalReadFast(PIN_RESET) != 0) {
    }  // Stay here until RESET deasserts
#endif

    digitalWriteFast(PIN_RDWR_n, 0x1);
    digitalWriteFast(PIN_DATAOUT_OE_n, 0x1);

    temp1 = read_byte(register_pc);            // Address ??
    temp1 = read_byte(register_pc + 1);        // Address ?? + 1
    temp1 = read_byte(register_sp_fixed);      // Address SP
    temp1 = read_byte(register_sp_fixed - 1);  // Address SP - 1
    temp1 = read_byte(register_sp_fixed - 2);  // Address SP - 2

    temp1 = read_byte(0xFFFC);  // Fetch Vector PCL
    temp2 = read_byte(0xFFFD);  // Fetch Vector PCH

    register_flags = 0x34;  // Set the I and B flags

    register_pc = (temp2 << 8) | temp1;
    start_read(register_pc);  // Fetch first opcode at vector PCH,PCL

    Serial.println("[RESET Complete]");

    return;
}

// -------------------------------------------------
// NMI Interrupt Processing
// -------------------------------------------------
void nmi_handler() {
    uint16_t temp1, temp2;

    sample_at_CLK0_falling_edge();  // Begin processing on next CLK edge

    register_flags = register_flags | 0x20;  // Set the flag[5]
    register_flags = register_flags & 0xEF;  // Clear the B flag

    read_byte(register_pc + 1);  // Fetch PC+1 (Discard)
    push(register_pc >> 8);      // Push PCH
    push(register_pc);           // Push PCL
    push(register_flags);        // Push P
    temp1 = read_byte(0xFFFA);   // Fetch Vector PCL
    temp2 = read_byte(0xFFFB);   // Fetch Vector PCH

    register_flags =
        register_flags | 0x34;  // Set the I flag and restore the B flag

    register_pc = (temp2 << 8) | temp1;
    start_read(register_pc);  // Fetch first opcode at vector PCH,PCL

    Serial.println("[NMI Complete]");

    return;
}

// -------------------------------------------------
// BRK & IRQ Interrupt Processing
// -------------------------------------------------
void irq_handler(uint8_t opcode_is_brk) {
    uint16_t temp1, temp2;

    sample_at_CLK0_falling_edge();  // Begin processing on next CLK edge

    register_flags = register_flags | 0x20;  // Set the flag[5]
    if (opcode_is_brk == 1)
        register_flags = register_flags | 0x10;  // Set the B flag
    else
        register_flags = register_flags & 0xEF;  // Clear the B flag

    read_byte(register_pc + 1);  // Fetch PC+1 (Discard)
    push(register_pc >> 8);      // Push PCH
    push(register_pc);           // Push PCL
    push(register_flags);        // Push P
    temp1 = read_byte(0xFFFE);   // Fetch Vector PCL
    temp2 = read_byte(0xFFFF);   // Fetch Vector PCH

    register_flags =
        register_flags | 0x34;  // Set the I flag and restore the B flag

    register_pc = (temp2 << 8) | temp1;
    start_read(register_pc);  // Fetch first opcode at vector PCH,PCL

    Serial.println("[IRQ Complete]");

    return;
}

// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

void display_next_instruction(uint16_t pc, uint8_t opcode) {
    uint8_t op1 = read_byte(pc + 1);
    uint8_t op2 = read_byte(pc + 2);

    Serial.println(String(pc, HEX) + ": " +
                   decode_instruction(opcode, op1, op2));
}

void display_registers() {
    char buf[32];
    sprintf(buf, "Registers:  A=%02X, X=%02X, Y=%02X", register_a, register_x,
            register_y);
    Serial.println(buf);
    sprintf(buf, "            PC=%04X, SP=%04X", register_pc,
            register_sp_fixed);
    Serial.println(buf);
    sprintf(buf, "            Flags: %s", flag_status().c_str());
    Serial.println(buf);
}

void display_info() {
    char buf[64];
    sprintf(buf, "Run-mode = %s, Address-mode = %d\n\rBreakpoint = %04X",
            run_mode_str[run_mode], addr_mode, breakpoint);
    Serial.println(buf);
}

String get_user_command() {
    String s = "";

    // Print prompt
    Serial.print(">> ");
    while (1) {
        if (Serial.available()) {
            char c = Serial.read();
            // Serial.println("got 0x" + String(c, 16));
            switch (c) {
                case '\r':
                    return (s.toLowerCase());
                    break;
                case '\b':
                    s.remove(s.length() - 1, 1);
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
    if (args.length() == 0 or args[0] == '|')
        return ("");

    int start_idx = 0;
    int end_idx;
    for (int i = 0; i <= arg_number; i++) {
        if (i > 0)
            start_idx = end_idx + 1;
        end_idx = args.indexOf(" ", start_idx);
        if (end_idx == -1)  // Must be last arg
            end_idx = args.length();
    }

    String rv = args.substring(start_idx, end_idx);
    return (rv);
}

String parse_next_arg(String& _src, String& remainder) {
    String arg = "";

    String src = _src.trim();

    // zero-length means nothing to parse
    if (src.length()) {
        int idx = src.indexOf(' ');

        // Serial.print("idx = "+String(idx) + ", ");

        if (idx > 0) {
            //  Space in source, pull first arg
            arg = src.substring(0, idx);
            remainder = src.substring(idx + 1);
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


String* tokenize(String str, int max_substrings) {
    String substrings[max_substrings];

    for (int i = 0; i < max_substrings; i++)
        substrings[i] = ""

            int substring_count = 0;
    while (str.length() > 0) {

        int index = str.indexOf(' ');
        if (index == -1) {
            // No space found
            substrings[substring_count++] = str;
            break;
        }
        else {
            substrings[substring_count++] = str.substring(0, index);
            str = str.substring(index + 1);
        }
    }
}



void list_instructions(uint16_t addr, uint8_t count) {
    uint16_t next_pc = addr;
    for (uint8_t i = 0; i < count; i++) {
        // print the instruction at next_pc and return the address
        // of the following instruction
        next_pc = print_instruction(next_pc);
    }
}

ENUM_RUN_MODE process_user_command(String input) {
    // Serial.println("\nProcessing command: "+input);

    String *command_parts;
    command_parts = tokenize(input.toLowerCase, max_command_parts);

    String command = command_parts[0];

    String arg[max_command_parts-1];
    for (int i=1; i<max_command_parts; i++)
        arg[i-1] = command_parts[i];

    Serial.println("Tokenized command and args: \'" + command + "\', \'" + arg[0] + "\', \'" +
                   arg[1] + "\', '" + arg[2] + "'");


    //
    //  All commands are of the form: <2-char command>( <arg> (<arg> ...))
    //

    String remainder = "";
    String cmd = parse_next_arg(input, remainder);
    String arg1 = parse_next_arg(remainder, remainder);
    String arg2 = parse_next_arg(remainder, remainder);
    String arg3 = parse_next_arg(remainder, remainder);

    word cmd_int = command_int(cmd);

    //Serial.println("Command and args: \'" + cmd + "\', \'" + arg1 + "\', \'" +
    //               arg2 + "\', '" + arg3 + "'");
    // char buf[32];
    // sprintf(buf, "Command-int = %04X", cmd_int);
    // Serial.println(buf);

    switch (cmd_int) {
        case CMD_NOP:
            //  User entered a zero-length line at prompt
            break;

        case CMD_TR:
            pc_trace = !pc_trace;
            if (pc_trace)
                pc_trace_index = 0;
            break;

        case CMD_LI:
        {
            int nargs = (arg1.length() > 0) + (arg2.length() > 0);
            if (nargs == 0) {
                // No arguments - print 16 instructions
                list_instructions(register_pc, 16);
            }
            if (nargs == 1) {
                // One argument - print 16 instructions starting at given address
                uint16_t start_address = strtol(arg1.c_str(), 0, 16);
                list_instructions(start_address, 16);
            }
            if (nargs == 2) {
                // Two arguments - Print instructions starting at address, count
                uint16_t start_address = strtol(arg1.c_str(), 0, 16);
                uint8_t count = strtol(arg2.c_str(), 0, 8);
                list_instructions(start_address, count);
            }
        } break;

        case CMD_RS:
            run_mode = RESETTING;  // may not need this any more
            reset_sequence();
            run_mode = WAITING;
            break;

        case CMD_Test:
            sample_at_CLK0_falling_edge();
            digitalWriteFast(PIN_SYNC, 0x1);
            sample_at_CLK0_falling_edge();
            digitalWriteFast(PIN_SYNC, 0x0);
            break;

        case CMD_DE:
            debug_mode = !debug_mode;
            break;

        case CMD_FI:
            if (3 == (arg1.length() > 0) + (arg2.length() > 0) +
                         (arg3.length() > 0)) {
                uint16_t addr = strtoul(arg1.c_str(), 0, 16);
                uint16_t count = arg2.toInt();
                uint8_t value = strtoul(arg3.c_str(), 0, 16) & 0xFF;

                String s = "Filling " + String(count) + " bytes from " +
                           String(addr, HEX) + " with value " +
                           String(value, HEX);
                Serial.println(s);

                for (int i = 0; i < count; i++) {
                    write_byte(addr + i, value);
                }
            }
            else {
                Serial.println(
                    "Fill command requires <address> <count> <value>");
            }
            break;

        case CMD_QM:
        case CMD_HE:
            Serial.println(
                String("Available Commands:\n\r") +
                "    IN                                   Information about ICE state\n\r" +
                "    RS                                   Reset the CPU\n\r" +
                "    MD <mode>                            Set memory addressing mode (0-3 see below)\n\r" +
                "    DR                                   Dump registers\n\r" +
                "    SS                                   Single-step execution\n\r" +
                "    GO [<address>]                       Execute from PC (Stop at optional address)\n\r" +
                "    BK <address>                         Set execution breakpoint\n\r" +
                "    SR <reg> <value>                     Set register (PC, A, X, Y) to value\n\r" +
                "    RD <address> [<count>]               Read from memory address, displays <count> values\n\r" +
                "    WR <address> <value> [<value> ...]   Write values starting at memory address\n\r" +
                "    FE <low_addr> <high_addr>            Enable execution fencing (break if PC is outside specified range)\n\r" +
                "    LI [<address> [<count>]]             List <count> Instructions starting at <address>\n\r" +
                "                                              [default 16 insts from current PC]\n\r" +
                "    FI <address> <count> <value>         Fill memory with a value\n\r" +
                "    DE                                   Toggle ICE application debug mode\n\r" +
                "    TR                                   Enable PC tracing\n\r" +
                "\n" +
                "    Addressing Modes:\n\r" +
                "       0 - All exernal memory accesses\n\r" +
                "       1 - Reads use cycle accurate internal memory and writes pass through to motherboard\n\r" +
                "       2 - Reads accelerated using internal memory and writes pass through to motherboard\n\r" +
                "       3 - All read and write accesses use accelerated internal memory\n\r");

            run_mode = WAITING;
            break;

        case CMD_MD:
        {
            byte a_mode = strtoul(arg1.c_str(), 0, 10);
            if (a_mode < 4)
                addr_mode = (ADDR_MODE)a_mode;
            else
                Serial.println("MD error. Illegal argument: " + arg1);
        }
            run_mode = WAITING;
            break;

        case CMD_DR:
            display_registers();
            run_mode = WAITING;
            break;

        case CMD_SR:
        {
            word value = strtoul(arg2.c_str(), 0, 16);
            // char buf[64]; sprintf(buf, "reg=%s, arg=%s, value=%04X", arg1.c_str(), arg2.c_str(), value); Serial.println(buf);
            if (arg1 == "pc") {
                register_pc = value & 0xFFFF;
            }
            else if (arg1 == "a") {
                register_a = value & 0xFF;
            }
            else if (arg1 == "x") {
                register_x = value & 0xFF;
            }
            else if (arg1 == "y") {
                register_y = value & 0xFF;
            }
            else {
                Serial.println("ERROR: unknown register identifier (options: pc, a, x, y)");
            }
        }
            display_registers();
            run_mode = WAITING;
            break;

        case CMD_IN:
            display_registers();
            display_info();
            run_mode = WAITING;
            break;

        case CMD_GO:
            run_mode = RUNNING;
            if (arg1.length()) {
                runto_address = strtoul(arg1.c_str(), 0, 16);
                Serial.println("Breakpoint set to $" + String(breakpoint, HEX));
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
            run_mode = WAITING;
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

            sprintf(s, "[%04X] = ", addr);
            Serial.print(s);

            for (byte i = 0; i < count; i++) {
                if ((i != 0) && (i % 8 == 0)) {
                    sprintf(s, "\n\r[%04X] = ", addr);
                    Serial.print(s);
                }

                byte data = read_byte(addr++);

                sprintf(s, "%02X ", data);
                Serial.print(s);
            }
            Serial.println("");
        }
            run_mode = WAITING;
            break;

        case CMD_FE:
        {
            if (run_fence) {
                run_fence = false;
                Serial.println("Run fence disabled");
            }
            else {
                run_fence = true;
                run_fence_low = strtoul(arg1.c_str(), 0, 16);
                run_fence_high = strtoul(arg2.c_str(), 0, 16);

                char buf[64];
                sprintf(buf, "Run fence enabled for range $%04X to $%04X",
                        run_fence_low, run_fence_high);
                Serial.println(buf);
            }
        } break;

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
            run_mode = WAITING;
            break;

        default:
            Serial.println("\n\nERROR: Can't parse command: \"" + input +
                           "\" (" + cmd + ", " + arg1 + ", " + arg2 + ")");
            run_mode = WAITING;
            break;
    }

    return (run_mode);
}

// -------------------------------------------------
//
// Main loop
//
// -------------------------------------------------
void loop() {
    if (direct_reset == 1)
        reset_sequence();

    // Poll for NMI and IRQ
    //
    if (nmi_n_old == 0 && direct_nmi == 1)
        nmi_handler();
    nmi_n_old = direct_nmi;
    if (direct_irq == 0x1 && (flag_i) == 0x0)
        irq_handler(0x0);

    //        next_instruction = finish_read_byte();
    next_instruction = read_byte(register_pc);

    //============================================================================
    //  ICE interface code
    //
    if (breakpoint && (run_mode == RUNNING) && (register_pc == breakpoint)) {
        run_mode = WAITING;
    }

    if (runto_address && (run_mode == RUNNING) &&
        (register_pc == runto_address)) {
        run_mode = WAITING;
        runto_address = 0;
    }

    if (run_mode != RUNNING) {
        uint16_t temp_pc = register_pc;

        do {
            display_next_instruction(register_pc, next_instruction);

            String c = get_user_command();

            if (c.length() == 0 && last_user_command.length() != 0) {
                Serial.println(last_user_command);
                run_mode = process_user_command(last_user_command);
            }
            else {
                Serial.println(" ");
                run_mode = process_user_command(c);
                last_user_command = c;
            }

            Serial.println(" ");

            //  Update the next_instruction, as PC or memory may have changed
            if ((run_mode != RUNNING) && (register_pc != temp_pc)) {
                next_instruction = read_byte(register_pc);
                temp_pc = register_pc;
            }
        } while (run_mode == WAITING);
    }
    else {
        while (Serial.available() > 0) {
            // read the incoming byte:
            char b = Serial.read();

            switch (b) {
                case 0x1B:
                    run_mode = WAITING;
            }
        }
    }

    //
    //  Skip the following code if we're waiting for the user
    //
    if (run_mode != WAITING) {
        if (run_fence) {
            if (register_pc < run_fence_low || register_pc > run_fence_high) {
                String s =
                    "EXECPTION: Attempt to execute outside of the run-fence "
                    "(PC=" +
                    String(register_pc, HEX) + ")";
                Serial.println(s);
                run_mode = WAITING;
                return;
            }
        }

        // For SS mode, turn on the SYNC signal for EVERY INSTRUCTION
        if (run_mode == SINGLE_STEP)
            digitalWriteFast(PIN_SYNC, 0x1);

        if (pc_trace) {
            String s = String(pc_trace_index) + ": " + String(register_pc, HEX);
            Serial.println(s);

            pc_trace_index++;
        }

        //==============================================================================
        // If we have a real instruction, execute it now and update the PC,
        //    otherwise handle the IRQ
        if (next_instruction != 0)
            register_pc = opcode_info[next_instruction].operation();
        else
            irq_handler(0x01);
    }
}

