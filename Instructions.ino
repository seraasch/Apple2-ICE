
// -------------------------------------------------
//
//  6502 Instruction Set
//    - Decoding utilities
//    - Emulation functions
//    - Initialization of decoding information
//
// -------------------------------------------------

#include "Apple2-ICE.h"

extern ENUM_RUN_MODE run_mode;

//---------------------------------------------------------------
//  Create a String representation of the specified instruction
//---------------------------------------------------------------
String decode_instruction(uint8_t op, uint8_t op1, uint8_t op2)
{
    OpDecoder *instr = &opcode_info[op];
    String s;

    if (instr->length == 2)
    {
        String operand = String(op1, HEX);
        if (instr->operands == "#")
        {
        }
        s = "#$" + operand;

        if ((instr->operands == "zpg") || (instr->operands == "rel"))
        {
            s = "$" + operand;
        }
        if (instr->operands == "(ind,X)")
        {
            s = "($" + operand + ",X)";
        }
        if (instr->operands == "(ind),Y")
        {
            s = "($" + operand + "),Y";
        }
        if (instr->operands == "zpg,X")
        {
            s = "$" + operand + ",X";
        }
        if (s.length() == 0)
        {
            s = "<unknown_format: " + instr->operands + ">";
        }
    }

    if (instr->length == 3)
    {
        String operand1 = String(op1, HEX);
        String operand2 = String(op2, HEX);

        if (instr->operands == "abs")
        {
            s = "$" + operand2 + operand1;
        }
        if (instr->operands == "abs,X")
        {
            s = "$" + operand2 + operand1 + ",X";
        }
        if (instr->operands == "abs,Y")
        {
            s = "$" + operand2 + operand1 + ",Y";
        }
        if (instr->operands == "(ind)")
        {
            s = "($" + operand2 + operand1 + ")";
        }
        if (s.length() == 0)
        {
            s = "<unknown_format: " + instr->operands + ">";
        }
    }

    return (instr->opcode + " " + s);
}

//---------------------------------------------------------------------
//  Print a human-readable version of the instruction at the specified
//  address
//---------------------------------------------------------------------
uint16_t print_instruction(uint16_t address) {
    uint8_t opcode = read_byte(address);
    uint8_t instr_length = opcode_info[opcode].length;

    uint8_t operands[2] = {0, 0};
    for (uint8_t i = 0; i < instr_length - 1; i++)
        operands[i] = read_byte(address + 1 + i);

    String s = decode_instruction(opcode, operands[0], operands[1]);
    Serial.println(String(address, HEX) + ": " + s);

    return (address + instr_length);
}


//===================================================================
//===================================================================
//
//  Utility functions used by the instructions during execution
//
//===================================================================
//===================================================================

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
    if (0x80 & local_data)
        register_flags = register_flags | 0x80;  // Set the N flag
    else
        register_flags = register_flags & 0x7F;  // Clear the N flag

    if (local_data == 0)
        register_flags = register_flags | 0x02;  // Set the Z flag
    else
        register_flags = register_flags & 0xFD;  // Clear the Z flag

    return;
}

uint16_t Sign_Extend16(uint16_t reg_data) {
    if ((reg_data & 0x0080) == 0x0080) {
        return (reg_data | 0xFF00);
    }
    else {
        return (reg_data & 0x00FF);
    }
}

void Begin_Fetch_Next_Opcode() {
    //    register_pc++;
    //    start_read(register_pc, true);
    //    return;
}

// -------------------------------------------------
// Addressing Modes
// -------------------------------------------------
uint8_t Fetch_Immediate(uint8_t offset) {
    return read_byte(register_pc + offset);
}

uint8_t Fetch_ZeroPage() {
    effective_address = Fetch_Immediate(1);
    return read_byte(effective_address);
}

uint8_t Fetch_ZeroPage_X() {
    uint16_t bal;
    bal = Fetch_Immediate(1);
    read_byte(register_pc + 1);
    effective_address = (0x00FF & (bal + register_x));
    return read_byte(effective_address);
}

uint8_t Fetch_ZeroPage_Y() {
    uint16_t bal;
    bal = Fetch_Immediate(1);
    read_byte(register_pc + 1);
    effective_address = (0x00FF & (bal + register_y));
    return read_byte(effective_address);
}

uint16_t Calculate_Absolute() {
    uint16_t adl, adh;

    adl = Fetch_Immediate(1);
    adh = Fetch_Immediate(2) << 8;
    effective_address = adl + adh;
    return effective_address;
}

uint8_t Fetch_Absolute() {
    uint16_t adl, adh;

    adl = Fetch_Immediate(1);
    adh = Fetch_Immediate(2) << 8;
    effective_address = adl + adh;
    return read_byte(effective_address);
}

uint8_t Fetch_Absolute_X(uint8_t page_cross_check) {
    uint16_t bal, bah;
    uint8_t local_data;

    bal = Fetch_Immediate(1);
    bah = Fetch_Immediate(2) << 8;
    effective_address = bah + bal + register_x;
    local_data = read_byte(effective_address);

    if (page_cross_check == 1 &&
        ((0xFF00 & effective_address) != (0xFF00 & bah))) {
        local_data = read_byte(effective_address);
    }
    return local_data;
}

uint8_t Fetch_Absolute_Y(uint8_t page_cross_check) {
    uint16_t bal, bah;
    uint8_t local_data;

    bal = Fetch_Immediate(1);
    bah = Fetch_Immediate(2) << 8;
    effective_address = bah + bal + register_y;
    local_data = read_byte(effective_address);

    if (page_cross_check == 1 &&
        ((0xFF00 & effective_address) != (0xFF00 & bah))) {
        local_data = read_byte(effective_address);
    }
    return local_data;
}

uint8_t Fetch_Indexed_Indirect_X() {
    uint16_t bal;
    uint16_t adl, adh;
    uint8_t local_data;

    bal = Fetch_Immediate(1) + register_x;
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

    ial = Fetch_Immediate(1);
    bal = read_byte(0xFF & ial);
    bah = read_byte(0xFF & (ial + 1)) << 8;

    effective_address = bah + bal + register_y;
    local_data = read_byte(effective_address);

    if (page_cross_check == 1 &&
        ((0xFF00 & effective_address) != (0xFF00 & bah))) {
        local_data = read_byte(effective_address);
    }
    return local_data;
}

void Write_ZeroPage(uint8_t local_data) {
    effective_address = Fetch_Immediate(1);
    write_byte(effective_address, local_data);
    return;
}

void Write_Absolute(uint8_t local_data) {
    effective_address = Fetch_Immediate(1);
    effective_address = (Fetch_Immediate(2) << 8) + effective_address;
    write_byte(effective_address, local_data);
    return;
}

void Write_ZeroPage_X(uint8_t local_data) {
    effective_address = Fetch_Immediate(1);
    read_byte(effective_address);
    write_byte((0x00FF & (effective_address + register_x)), local_data);
    return;
}

void Write_ZeroPage_Y(uint8_t local_data) {
    effective_address = Fetch_Immediate(1);
    read_byte(effective_address);
    write_byte((0x00FF & (effective_address + register_y)), local_data);
    return;
}

void Write_Absolute_X(uint8_t local_data) {
    uint16_t bal, bah;

    bal = Fetch_Immediate(1);
    bah = Fetch_Immediate(2) << 8;
    effective_address = bal + bah + register_x;
    read_byte(effective_address);
    write_byte(effective_address, local_data);
    return;
}

void Write_Absolute_Y(uint8_t local_data) {
    uint16_t bal, bah;

    bal = Fetch_Immediate(1);
    bah = Fetch_Immediate(2) << 8;
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

    bal = Fetch_Immediate(1);
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

    ial = Fetch_Immediate(1);
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


//===================================================================
//===================================================================
//
//   Function used to execute each opcode
//
//===================================================================
//===================================================================

uint16_t illegal_opcode()
{
    Serial.println("ERROR: Illegal instruction");
    run_mode = WAITING;

    return 0;
}

// -------------------------------------------------
// 0x0A - ASL A - Arithmetic Shift Left - Accumulator
// -------------------------------------------------
uint16_t opcode_0x0A()
{
    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    if (0x80 & register_a)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = register_a << 1;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x0A].length);
}

// -------------------------------------------------
// 0x4A - LSR A - Logical Shift Right - Accumulator
// -------------------------------------------------
uint16_t opcode_0x4A()
{
    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    if (0x01 & register_a)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = register_a >> 1;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x4A].length);
}

// -------------------------------------------------
// 0x6A - ROR A - Rotate Right - Accumulator
// -------------------------------------------------
uint16_t opcode_0x6A()
{

    uint8_t old_carry_flag = 0;

    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    old_carry_flag = register_flags << 7; // Shift the old carry flag to bit[8] to be rotated in

    if (0x01 & register_a)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = (old_carry_flag | (register_a >> 1));

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x6A].length);
}

// -------------------------------------------------
// 0x2A - ROL A - Rotate Left - Accumulator
// -------------------------------------------------
uint16_t opcode_0x2A()
{

    uint8_t old_carry_flag = 0;

    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    old_carry_flag = 0x1 & register_flags; // Store the old carry flag to be rotated in

    if (0x80 & register_a)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = (register_a << 1) | old_carry_flag;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x2A].length);
}

// -------------------------------------------------
// ADC
// -------------------------------------------------
void Calculate_ADC(uint16_t local_data)
{
    uint16_t total = 0;
    uint16_t bcd_low = 0;
    uint16_t bcd_high = 0;
    uint16_t bcd_total = 0;
    uint8_t operand0 = 0;
    uint8_t operand1 = 0;
    uint8_t result = 0;
    uint8_t low_carry = 0;
    uint8_t high_carry = 0;

    Begin_Fetch_Next_Opcode();

    if ((flag_d) == 1)
    {
        bcd_low = (0x0F & register_a) + (0x0F & local_data) + (flag_c);
        if (bcd_low > 0x9)
        {
            low_carry = 0x10;
            bcd_low = bcd_low - 0xA;
        }

        bcd_high = (0xF0 & register_a) + (0xF0 & local_data) + low_carry;
        if (bcd_high > 0x90)
        {
            high_carry = 1;
            bcd_high = bcd_high - 0xA0;
        }

        register_flags = register_flags & 0xFE; // Clear the C flag
        if ((0x00FF & bcd_total) > 0x09)
        {
            bcd_total = bcd_total + 0x010;
            bcd_total = bcd_total - 0x0A;
        }

        if (high_carry == 1)
        {
            bcd_total = bcd_total - 0xA0;
            register_flags = register_flags | 0x01;
        } // Set the C flag
        else
            register_flags = register_flags & 0xFE; // Clear the C flag

        total = (0xFF & (bcd_low + bcd_high));
    }
    else
    {
        total = register_a + local_data + (flag_c);

        if (total > 255)
            register_flags = register_flags | 0x01; // Set the C flag
        else
            register_flags = register_flags & 0xFE; // Clear the C flag
    }

    operand0 = (register_a & 0x80);
    operand1 = (local_data & 0x80);
    result = (total & 0x80);

    if (operand0 == 0 && operand1 == 0 && result != 0)
        register_flags = register_flags | 0x40; // Set the V flag
    else if (operand0 != 0 && operand1 != 0 && result == 0)
        register_flags = register_flags | 0x40;
    else
        register_flags = register_flags & 0xBF; // Clear the V flag

    register_a = (0xFF & total);
    Calc_Flags_NEGATIVE_ZERO(register_a);

    return;
}
uint16_t opcode_0x69()
{
    Calculate_ADC(Fetch_Immediate(1));
    return (register_pc + opcode_info[0x69].length);
} // 0x69 - ADC - Immediate - Binary
uint16_t opcode_0x65()
{
    Calculate_ADC(Fetch_ZeroPage());
    return (register_pc + opcode_info[0x65].length);
} // 0x65 - ADC - ZeroPage
uint16_t opcode_0x75()
{
    Calculate_ADC(Fetch_ZeroPage_X());
    return (register_pc + opcode_info[0x75].length);
} // 0x75 - ADC - ZeroPage , X
uint16_t opcode_0x6D()
{
    Calculate_ADC(Fetch_Absolute());
    return (register_pc + opcode_info[0x6D].length);
} // 0x6D - ADC - Absolute
uint16_t opcode_0x7D()
{
    Calculate_ADC(Fetch_Absolute_X(1));
    return (register_pc + opcode_info[0x7D].length);
} // 0x7D - ADC - Absolute , X
uint16_t opcode_0x79()
{
    Calculate_ADC(Fetch_Absolute_Y(1));
    return (register_pc + opcode_info[0x79].length);
} // 0x79 - ADC - Absolute , Y
uint16_t opcode_0x61()
{
    Calculate_ADC(Fetch_Indexed_Indirect_X());
    return (register_pc + opcode_info[0x61].length);
} // 0x61 - ADC - Indexed Indirect X
uint16_t opcode_0x71()
{
    Calculate_ADC(Fetch_Indexed_Indirect_Y(1));
    return (register_pc + opcode_info[0x71].length);
} // 0x71 - ADC - Indirect Indexed  Y

// -------------------------------------------------
// SBC
// -------------------------------------------------
void Calculate_SBC(uint16_t local_data)
{
    uint16_t total = 0;
    uint16_t bcd_low = 0;
    uint16_t bcd_high = 0;
    uint16_t bcd_total = 0;
    int16_t signed_total = 0;
    uint8_t operand0 = 0;
    uint8_t operand1 = 0;
    uint8_t result = 0;
    uint8_t flag_c_invert = 0;
    uint8_t low_carry = 0;
    uint8_t high_carry = 0;

    Begin_Fetch_Next_Opcode();

    if (flag_c != 0)
        flag_c_invert = 0;
    else
        flag_c_invert = 1;

    if ((flag_d) == 1)
    {
        bcd_low = (0x0F & register_a) - (0x0F & local_data) - flag_c_invert;
        if (bcd_low > 0x9)
        {
            low_carry = 0x10;
            bcd_low = bcd_low + 0xA;
        }

        bcd_high = (0xF0 & register_a) - (0xF0 & local_data) - low_carry;
        if (bcd_high > 0x90)
        {
            high_carry = 1;
            bcd_high = bcd_high + 0xA0;
        }

        register_flags = register_flags & 0xFE; // Clear the C flag
        if ((0x00FF & bcd_total) > 0x09)
        {
            bcd_total = bcd_total + 0x010;
            bcd_total = bcd_total - 0x0A;
        }

        if (high_carry == 0)
        {
            bcd_total = bcd_total - 0xA0;
            register_flags = register_flags | 0x01;
        } // Set the C flag
        else
            register_flags = register_flags & 0xFE; // Clear the C flag

        total = (0xFF & (bcd_low + bcd_high));
    }
    else
    {

        total = register_a - local_data - flag_c_invert;
        signed_total = (int16_t)register_a - (int16_t)(local_data)-flag_c_invert;

        if (signed_total >= 0)
            register_flags = register_flags | 0x01; // Set the C flag
        else
            register_flags = register_flags & 0xFE; // Clear the C flag
    }

    operand0 = (register_a & 0x80);
    operand1 = (local_data & 0x80);
    result = (total & 0x80);

    if (operand0 == 0 && operand1 != 0 && result != 0)
        register_flags = register_flags | 0x40; // Set the V flag
    else if (operand0 != 0 && operand1 == 0 && result == 0)
        register_flags = register_flags | 0x40;
    else
        register_flags = register_flags & 0xBF; // Clear the V flag

    register_a = (0xFF & total);
    Calc_Flags_NEGATIVE_ZERO(register_a);

    return;
}
uint16_t opcode_0xE9()
{
    Calculate_SBC(Fetch_Immediate(1));
    return (register_pc + opcode_info[0xE9].length);
} // 0xE9 - SBC - Immediate
uint16_t opcode_0xE5()
{
    Calculate_SBC(Fetch_ZeroPage());
    return (register_pc + opcode_info[0xE5].length);
} // 0xE5 - SBC - ZeroPage
uint16_t opcode_0xF5()
{
    Calculate_SBC(Fetch_ZeroPage_X());
    return (register_pc + opcode_info[0xF5].length);
} // 0xF5 - SBC - ZeroPage , X
uint16_t opcode_0xED()
{
    Calculate_SBC(Fetch_Absolute());
    return (register_pc + opcode_info[0xED].length);
} // 0xED - SBC - Absolute
uint16_t opcode_0xFD()
{
    Calculate_SBC(Fetch_Absolute_X(1));
    return (register_pc + opcode_info[0xFD].length);
} // 0xFD - SBC - Absolute , X
uint16_t opcode_0xF9()
{
    Calculate_SBC(Fetch_Absolute_Y(1));
    return (register_pc + opcode_info[0xF9].length);
} // 0xF9 - SBC - Absolute , Y
uint16_t opcode_0xE1()
{
    Calculate_SBC(Fetch_Indexed_Indirect_X());
    return (register_pc + opcode_info[0xE1].length);
} // 0xE1 - SBC - Indexed Indirect X
uint16_t opcode_0xF1()
{
    Calculate_SBC(Fetch_Indexed_Indirect_Y(1));
    return (register_pc + opcode_info[0xF1].length);
} // 0xF1 - SBC - Indirect Indexed  Y

// -------------------------------------------------
// Flag set/resets and NOP
// -------------------------------------------------
uint16_t opcode_0xEA()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xEA].length);
} // 0xEA - NOP
uint16_t opcode_0x18()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xFE;
    return (register_pc + opcode_info[0x18].length);
} // 0x18 - CLC - Clear Carry Flag
uint16_t opcode_0xD8()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xF7;
    return (register_pc + opcode_info[0xD8].length);
} // 0xD8 - CLD - Clear Decimal Mode
uint16_t opcode_0x58()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xFB;
    return (register_pc + opcode_info[0x58].length);
} // 0x58 - CLI - Clear Interrupt Flag
uint16_t opcode_0xB8()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xBF;
    return (register_pc + opcode_info[0xB8].length);
} // 0xB8 - CLV - Clear Overflow Flag
uint16_t opcode_0x38()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags | 0x01;
    return (register_pc + opcode_info[0x38].length);
} // 0x38 - SEC - Set Carry Flag
uint16_t opcode_0x78()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags | 0x04;
    return (register_pc + opcode_info[0x78].length);
} // 0x78 - SEI - Set Interrupt Flag
uint16_t opcode_0xF8()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags | 0x08;
    return (register_pc + opcode_info[0xF8].length);
} // 0xF8 - SED - Set Decimal Mode

// -------------------------------------------------
// Increment/Decrements
// -------------------------------------------------
uint16_t opcode_0xCA()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_x - 1;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xCA].length);
} // 0xCA - DEX - Decrement X
uint16_t opcode_0x88()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_y = register_y - 1;
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0x88].length);
} // 0x88 - DEY - Decrement Y
uint16_t opcode_0xE8()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_x + 1;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xE8].length);
} // 0xE8 - INX - Increment X
uint16_t opcode_0xC8()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_y = register_y + 1;
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xC8].length);
} // 0xC8 - INY - Increment Y

// -------------------------------------------------
// Transfers
// -------------------------------------------------
uint16_t opcode_0xAA()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_a;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xAA].length);
} // 0xAA - TAX - Transfer Accumulator to X
uint16_t opcode_0xA8()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_y = register_a;
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xA8].length);
} // 0xA8 - TAY - Transfer Accumulator to Y
uint16_t opcode_0xBA()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_sp;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xBA].length);
} // 0xBA - TSX - Transfer Stack Pointer to X
uint16_t opcode_0x8A()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_a = register_x;
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x8A].length);
} // 0x8A - TXA - Transfer X to Accumulator
uint16_t opcode_0x9A()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_sp = register_x;
    return (register_pc + opcode_info[0x9A].length);
} // 0x9A - TXS - Transfer X to Stack Pointer
uint16_t opcode_0x98()
{
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_a = register_y;
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x98].length);
} // 0x98 - TYA - Transfer Y to Accumulator

// -------------------------------------------------
// PUSH/POP Flags and Accumulator
// -------------------------------------------------
uint16_t opcode_0x08()
{
    read_byte(register_pc + 1);
    push(register_flags | 0x30);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x08].length);
} // 0x08 - PHP - Push Flags to Stack
uint16_t opcode_0x48()
{
    read_byte(register_pc + 1);
    push(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x48].length);
} // 0x48 - PHA - Push Accumulator to the stack
uint16_t opcode_0x28()
{
    read_byte(register_pc + 1);
    read_byte(register_sp_fixed);
    register_flags = (pop() | 0x30);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x28].length);
} // 0x28 - PLP - Pop Flags from Stack
uint16_t opcode_0x68()
{
    read_byte(register_pc + 1);
    read_byte(register_sp_fixed);
    register_a = pop();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x68].length);
} // 0x68 - PLA - Pop Accumulator from Stack

// -------------------------------------------------
// AND
// -------------------------------------------------
uint16_t opcode_0x29()
{
    register_a = register_a & (Fetch_Immediate(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x29].length);
} // 0x29 - AND - Immediate
uint16_t opcode_0x25()
{
    register_a = register_a & (Fetch_ZeroPage());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x25].length);
} // 0x25 - AND - ZeroPage
uint16_t opcode_0x35()
{
    register_a = register_a & (Fetch_ZeroPage_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x35].length);
} // 0x35 - AND - ZeroPage , X
uint16_t opcode_0x2D()
{
    register_a = register_a & (Fetch_Absolute());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x2D].length);
} // 0x2D - AND - Absolute
uint16_t opcode_0x3D()
{
    register_a = register_a & (Fetch_Absolute_X(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x3D].length);
} // 0x3D - AND - Absolute , X
uint16_t opcode_0x39()
{
    register_a = register_a & (Fetch_Absolute_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x39].length);
} // 0x19 - OR - Absolute , Y
uint16_t opcode_0x21()
{
    register_a = register_a & (Fetch_Indexed_Indirect_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x21].length);
} // 0x21 - AND - Indexed Indirect X
uint16_t opcode_0x31()
{
    register_a = register_a & (Fetch_Indexed_Indirect_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x31].length);
} // 0x31 - AND - Indirect Indexed  Y

// -------------------------------------------------
// ORA
// -------------------------------------------------
uint16_t opcode_0x09()
{
    register_a = register_a | (Fetch_Immediate(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x09].length);
} // 0x09 - OR - Immediate
uint16_t opcode_0x05()
{
    register_a = register_a | (Fetch_ZeroPage());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x05].length);
} // 0x05 - OR - ZeroPage
uint16_t opcode_0x15()
{
    register_a = register_a | (Fetch_ZeroPage_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x15].length);
} // 0x15 - OR - ZeroPage , X
uint16_t opcode_0x0D()
{
    register_a = register_a | (Fetch_Absolute());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x0D].length);
} // 0x0D - OR - Absolute
uint16_t opcode_0x1D()
{
    register_a = register_a | (Fetch_Absolute_X(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x1D].length);
} // 0x1D - OR - Absolute , X
uint16_t opcode_0x19()
{
    register_a = register_a | (Fetch_Absolute_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x19].length);
} // 0x19 - OR - Absolute , Y
uint16_t opcode_0x01()
{
    register_a = register_a | (Fetch_Indexed_Indirect_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x01].length);
} // 0x01 - OR - Indexed Indirect X
uint16_t opcode_0x11()
{
    register_a = register_a | (Fetch_Indexed_Indirect_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x11].length);
} // 0x11 - OR - Indirect Indexed  Y

// -------------------------------------------------
// EOR
// -------------------------------------------------
uint16_t opcode_0x49()
{
    register_a = register_a ^ (Fetch_Immediate(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x49].length);
} // 0x49 - EOR - Immediate
uint16_t opcode_0x45()
{
    register_a = register_a ^ (Fetch_ZeroPage());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x45].length);
} // 0x45 - EOR - ZeroPage
uint16_t opcode_0x55()
{
    register_a = register_a ^ (Fetch_ZeroPage_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x55].length);
} // 0x55 - EOR - ZeroPage , X
uint16_t opcode_0x4D()
{
    register_a = register_a ^ (Fetch_Absolute());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x4D].length);
} // 0x4D - EOR - Absolute
uint16_t opcode_0x5D()
{
    register_a = register_a ^ (Fetch_Absolute_X(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x5D].length);
} // 0x5D - EOR - Absolute , X
uint16_t opcode_0x59()
{
    register_a = register_a ^ (Fetch_Absolute_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x59].length);
} // 0x59 - EOR - Absolute , Y
uint16_t opcode_0x41()
{
    register_a = register_a ^ (Fetch_Indexed_Indirect_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x41].length);
} // 0x41 - EOR - Indexed Indirect X
uint16_t opcode_0x51()
{
    register_a = register_a ^ (Fetch_Indexed_Indirect_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0x51].length);
} // 0x51 - EOR - Indirect Indexed  Y

// -------------------------------------------------
// LDA
// -------------------------------------------------
uint16_t opcode_0xA9()
{
    register_a = Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xA9].length);
} // 0xA9 - LDA - Immediate
uint16_t opcode_0xA5()
{
    register_a = Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xA5].length);
} // 0xA5 - LDA - ZeroPage
uint16_t opcode_0xB5()
{
    register_a = Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xB5].length);
} // 0xB5 - LDA - ZeroPage , X
uint16_t opcode_0xAD()
{
    register_a = Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xAD].length);
} // 0xAD - LDA - Absolute
uint16_t opcode_0xBD()
{
    register_a = Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xBD].length);
} // 0xBD - LDA - Absolute , X
uint16_t opcode_0xB9()
{
    register_a = Fetch_Absolute_Y(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xB9].length);
} // 0xB9 - LDA - Absolute , Y
uint16_t opcode_0xA1()
{
    register_a = Fetch_Indexed_Indirect_X();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xA1].length);
} // 0xA1 - LDA - Indexed Indirect X
uint16_t opcode_0xB1()
{
    register_a = Fetch_Indexed_Indirect_Y(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xB1].length);
} // 0xB1 - LDA - Indirect Indexed  Y

// -------------------------------------------------
// LDX
// -------------------------------------------------
uint16_t opcode_0xA2()
{
    register_x = Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xA2].length);
} // 0xA2 - LDX - Immediate
uint16_t opcode_0xA6()
{
    register_x = Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xA6].length);
} // 0xA6 - LDX - ZeroPage
uint16_t opcode_0xB6()
{
    register_x = Fetch_ZeroPage_Y();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xB6].length);
} // 0xB6 - LDX - ZeroPage , Y
uint16_t opcode_0xAE()
{
    register_x = Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xAE].length);
} // 0xAE - LDX - Absolute
uint16_t opcode_0xBE()
{
    register_x = Fetch_Absolute_Y(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return (register_pc + opcode_info[0xBE].length);
} // 0xBE - LDX - Absolute , Y

// -------------------------------------------------
// LDY
// -------------------------------------------------
uint16_t opcode_0xA0()
{
    register_y = Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xA0].length);
} // 0xA0 - LDY - Immediate
uint16_t opcode_0xA4()
{
    register_y = Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xA4].length);
} // 0xA4 - LDY - ZeroPage
uint16_t opcode_0xB4()
{
    register_y = Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xB4].length);
} // 0xB4 - LDY - ZeroPage , X
uint16_t opcode_0xAC()
{
    register_y = Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xAC].length);
} // 0xAC - LDY - Absolute
uint16_t opcode_0xBC()
{
    register_y = Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return (register_pc + opcode_info[0xBC].length);
} // 0xBC - LDY - Absolute , X

// -------------------------------------------------
// BIT
// -------------------------------------------------
void Calculate_BIT(uint8_t local_data)
{
    uint8_t temp = 0;

    Begin_Fetch_Next_Opcode();

    register_flags = (register_flags & 0x3F) | (local_data & 0xC0); // Copy fetched memory[7:6] to C,V flags

    temp = local_data & register_a;
    if (temp == 0)
        register_flags = register_flags | 0x02; // Set the Z flag
    else
        register_flags = register_flags & 0xFD; // Clear the Z flag

    return;
}
uint16_t opcode_0x24()
{
    Calculate_BIT(Fetch_ZeroPage());
    return (register_pc + opcode_info[0x24].length);
} // 0x24 - BIT - ZeroPage
uint16_t opcode_0x2C()
{
    Calculate_BIT(Fetch_Absolute());
    return (register_pc + opcode_info[0x2C].length);
} // 0x2C - BIT - Absolute

// -------------------------------------------------
// CMP
// -------------------------------------------------
void Calculate_CMP(uint8_t local_data)
{
    int16_t temp = 0;

    Begin_Fetch_Next_Opcode();

    temp = register_a - local_data;

    if (register_a >= local_data)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    Calc_Flags_NEGATIVE_ZERO(temp);
    return;
}
uint16_t opcode_0xC9()
{
    Calculate_CMP(Fetch_Immediate(1));
    return (register_pc + opcode_info[0xC9].length);
} // 0xC9 - CMP - Immediate
uint16_t opcode_0xC5()
{
    Calculate_CMP(Fetch_ZeroPage());
    return (register_pc + opcode_info[0xC5].length);
} // 0xC5 - CMP - ZeroPage
uint16_t opcode_0xD5()
{
    Calculate_CMP(Fetch_ZeroPage_X());
    return (register_pc + opcode_info[0xD5].length);
} // 0xD5 - CMP - ZeroPage , X
uint16_t opcode_0xCD()
{
    Calculate_CMP(Fetch_Absolute());
    return (register_pc + opcode_info[0xCD].length);
} // 0xCD - CMP - Absolute
uint16_t opcode_0xDD()
{
    Calculate_CMP(Fetch_Absolute_X(1));
    return (register_pc + opcode_info[0xDD].length);
} // 0xDD - CMP - Absolute , X
uint16_t opcode_0xD9()
{
    Calculate_CMP(Fetch_Absolute_Y(1));
    return (register_pc + opcode_info[0xD9].length);
} // 0xD9 - CMP - Absolute , Y
uint16_t opcode_0xC1()
{
    Calculate_CMP(Fetch_Indexed_Indirect_X());
    return (register_pc + opcode_info[0xC1].length);
} // 0xC1 - CMP - Indexed Indirect X
uint16_t opcode_0xD1()
{
    Calculate_CMP(Fetch_Indexed_Indirect_Y(1));
    return (register_pc + opcode_info[0xD1].length);
} // 0xD1 - CMP - Indirect Indexed  Y

// -------------------------------------------------
// CPX
// -------------------------------------------------
void Calculate_CPX(uint8_t local_data)
{
    uint16_t temp = 0;

    Begin_Fetch_Next_Opcode();

    temp = register_x - local_data;

    if (register_x >= local_data)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    Calc_Flags_NEGATIVE_ZERO(temp);
    return;
}
uint16_t opcode_0xE0()
{
    Calculate_CPX(Fetch_Immediate(1));
    return (register_pc + opcode_info[0xE0].length);
} // 0xE0 - CPX - Immediate
uint16_t opcode_0xE4()
{
    Calculate_CPX(Fetch_ZeroPage());
    return (register_pc + opcode_info[0xE4].length);
} // 0xE4 - CPX - ZeroPage
uint16_t opcode_0xEC()
{
    Calculate_CPX(Fetch_Absolute());
    return (register_pc + opcode_info[0xEC].length);
} // 0xEC - CPX - Absolute

// -------------------------------------------------
// CPY
// -------------------------------------------------
void Calculate_CPY(uint8_t local_data)
{
    uint16_t temp = 0;

    Begin_Fetch_Next_Opcode();

    temp = register_y - local_data;

    if (register_y >= local_data)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    Calc_Flags_NEGATIVE_ZERO(temp);
    return;
}
uint16_t opcode_0xC0()
{
    Calculate_CPY(Fetch_Immediate(1));
    return (register_pc + opcode_info[0xC0].length);
} // 0xC0 - CPY - Immediate
uint16_t opcode_0xC4()
{
    Calculate_CPY(Fetch_ZeroPage());
    return (register_pc + opcode_info[0xC4].length);
} // 0xC4 - CPY - ZeroPage
uint16_t opcode_0xCC()
{
    Calculate_CPY(Fetch_Absolute());
    return (register_pc + opcode_info[0xCC].length);
} // 0xCC - CPY - Absolute

// -------------------------------------------------
// Store Operations
// -------------------------------------------------
uint16_t opcode_0x85()
{
    Write_ZeroPage(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x85].length);
} // 0x85 - STA - ZeroPage
uint16_t opcode_0x8D()
{
    Write_Absolute(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x8D].length);
} // 0x8D - STA - Absolute
uint16_t opcode_0x95()
{
    Write_ZeroPage_X(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x95].length);
} // 0x95 - STA - ZeroPage , X
uint16_t opcode_0x9D()
{
    Write_Absolute_X(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x9D].length);
} // 0x9D - STA - Absolute , X
uint16_t opcode_0x99()
{
    Write_Absolute_Y(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x99].length);
} // 0x99 - STA - Absolute , Y
uint16_t opcode_0x81()
{
    Write_Indexed_Indirect_X(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x81].length);
} // 0x81 - STA - Indexed Indirect X
uint16_t opcode_0x91()
{
    Write_Indexed_Indirect_Y(register_a);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x91].length);
} // 0x91 - STA - Indirect Indexed  Y
uint16_t opcode_0x86()
{
    Write_ZeroPage(register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x86].length);
} // 0x86 - STX - ZeroPage
uint16_t opcode_0x96()
{
    Write_ZeroPage_Y(register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x96].length);
} // 0x96 - STX - ZeroPage , Y
uint16_t opcode_0x8E()
{
    Write_Absolute(register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x8E].length);
} // 0x8E - STX - Absolute
uint16_t opcode_0x84()
{
    Write_ZeroPage(register_y);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x84].length);
} // 0x84 - STY - ZeroPage
uint16_t opcode_0x94()
{
    Write_ZeroPage_X(register_y);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x94].length);
} // 0x94 - STY - ZeroPage , X
uint16_t opcode_0x8C()
{
    Write_Absolute(register_y);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x8C].length);
} // 0x8C - STY - Absolute

// -------------------------------------------------
// ASL - Arithmetic Shift Left - Memory
// -------------------------------------------------
uint8_t Calculate_ASL(uint8_t local_data)
{

    if ((0x80 & local_data) == 0x80)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = ((local_data << 1) & 0xFE);

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}

// -------------------------------------------------
// ASL - Read-modify-write Operations
// -------------------------------------------------
uint16_t opcode_0x06()
{
    Double_WriteBack(Calculate_ASL(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x06].length);
} // 0x06 - ASL  - Arithmetic Shift Left - ZeroPage
uint16_t opcode_0x16()
{
    Double_WriteBack(Calculate_ASL(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x16].length);
} // 0x16 - ASL  - Arithmetic Shift Left - ZeroPage , X
uint16_t opcode_0x0E()
{
    Double_WriteBack(Calculate_ASL(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x0E].length);
} // 0x0E - ASL  - Arithmetic Shift Left - Absolute
uint16_t opcode_0x1E()
{
    Double_WriteBack(Calculate_ASL(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x1E].length);
} // 0x1E - ASL  - Arithmetic Shift Left - Absolute , X

// -------------------------------------------------
// INC - Memory
// -------------------------------------------------
uint8_t Calculate_INC(uint8_t local_data)
{

    local_data = local_data + 1;
    global_temp = local_data;
    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}

uint16_t opcode_0xE6()
{
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xE6].length);
} // 0xE6 - INC - ZeroPage
uint16_t opcode_0xF6()
{
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xF6].length);
} // 0xF6 - INC - ZeroPage , X
uint16_t opcode_0xEE()
{
    Double_WriteBack(Calculate_INC(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xEE].length);
} // 0xEE - INC - Absolute
uint16_t opcode_0xFE()
{
    Double_WriteBack(Calculate_INC(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xFE].length);
} // 0xFE - INC - Absolute , X

// -------------------------------------------------
// DEC - Memory
// -------------------------------------------------
uint8_t Calculate_DEC(uint8_t local_data)
{

    local_data = local_data - 1;
    global_temp = local_data;
    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}

uint16_t opcode_0xC6()
{
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xC6].length);
} // 0xC6 - DEC - ZeroPage
uint16_t opcode_0xD6()
{
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xD6].length);
} // 0xD6 - DEC - ZeroPage , X
uint16_t opcode_0xCE()
{
    Double_WriteBack(Calculate_DEC(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xCE].length);
} // 0xCE - DEC - Absolute
uint16_t opcode_0xDE()
{
    Double_WriteBack(Calculate_DEC(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xDE].length);
} // 0xDE - DEC - Absolute , X

// -------------------------------------------------
// LSR - Memory
// -------------------------------------------------
uint8_t Calculate_LSR(uint8_t local_data)
{

    if ((0x01 & local_data) == 0x01)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (0x7F & (local_data >> 1));

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}
uint16_t opcode_0x46()
{
    Double_WriteBack(Calculate_LSR(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x46].length);
} // 0x46 - LSR - Logical Shift Right - ZeroPage
uint16_t opcode_0x56()
{
    Double_WriteBack(Calculate_LSR(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x56].length);
} // 0x56 - LSR - Logical Shift Right - ZeroPage , X
uint16_t opcode_0x4E()
{
    Double_WriteBack(Calculate_LSR(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x4E].length);
} // 0x4E - LSR - Logical Shift Right - Absolute
uint16_t opcode_0x5E()
{
    Double_WriteBack(Calculate_LSR(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x5E].length);
} // 0x5E - LSR - Logical Shift Right - Absolute , X

// -------------------------------------------------
// ROR - Memory
// -------------------------------------------------
uint8_t Calculate_ROR(uint8_t local_data)
{

    uint8_t old_carry_flag = 0;

    old_carry_flag = register_flags << 7; // Shift the old carry flag to bit[8] to be rotated in

    if ((0x01 & local_data) == 0x01)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (old_carry_flag | (local_data >> 1));

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}
uint16_t opcode_0x66()
{
    Double_WriteBack(Calculate_ROR(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x66].length);
} // 0x66 - ROR - Rotate Right - ZeroPage
uint16_t opcode_0x76()
{
    Double_WriteBack(Calculate_ROR(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x76].length);
} // 0x76 - ROR - Rotate Right - ZeroPage , X
uint16_t opcode_0x6E()
{
    Double_WriteBack(Calculate_ROR(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x6E].length);
} // 0x6E - ROR - Rotate Right - Absolute
uint16_t opcode_0x7E()
{
    Double_WriteBack(Calculate_ROR(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x7E].length);
} // 0x7E - ROR - Rotate Right - Absolute , X

// -------------------------------------------------
// ROL - Memory
// -------------------------------------------------
uint8_t Calculate_ROL(uint8_t local_data)
{

    uint8_t old_carry_flag = 0;

    old_carry_flag = 0x1 & register_flags; // Store the old carry flag to be rotated in

    if (0x80 & local_data)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (local_data << 1) | old_carry_flag;

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}
uint16_t opcode_0x26()
{
    Double_WriteBack(Calculate_ROL(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x26].length);
} // 0x26 - ROL - Rotate Left - ZeroPage
uint16_t opcode_0x36()
{
    Double_WriteBack(Calculate_ROL(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x36].length);
} // 0x36 - ROL - Rotate Left - ZeroPage , X
uint16_t opcode_0x2E()
{
    Double_WriteBack(Calculate_ROL(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x2E].length);
} // 0x2E - ROL - Rotate Left - Absolute
uint16_t opcode_0x3E()
{
    Double_WriteBack(Calculate_ROL(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x3E].length);
} // 0x3E - ROL - Rotate Left - Absolute , X

// -------------------------------------------------
// Branches
// -------------------------------------------------
void Branch_Taken()
{

    effective_address = Sign_Extend16(Fetch_Immediate(1));
    effective_address = (register_pc + 2) + effective_address;

    if ((0xFF00 & register_pc) == (0xFF00 & effective_address))
    {
        Fetch_Immediate(2);
    } // Page boundary not crossed
    else
    {
        Fetch_Immediate(2);
        Fetch_Immediate(3);
    } // Page boundary crossed

    register_pc = effective_address;
    start_read(register_pc);
    return;
}
uint16_t opcode_0xB0()
{
    if ((flag_c) == 1)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0xB0].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0xB0 - BCS - Branch on Carry Set
uint16_t opcode_0x90()
{
    if ((flag_c) == 0)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0x90].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0x90 - BCC - Branch on Carry Clear
uint16_t opcode_0xF0()
{
    if ((flag_z) == 1)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0xF0].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0xF0 - BEQ - Branch on Zero Set
uint16_t opcode_0xD0()
{
    if ((flag_z) == 0)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0xD0].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0xD0 - BNE - Branch on Zero Clear
uint16_t opcode_0x70()
{
    if ((flag_v) == 1)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0x70].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0x70 - BVS - Branch on Overflow Set
uint16_t opcode_0x50()
{
    if ((flag_v) == 0)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0x50].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0x50 - BVC - Branch on Overflow Clear
uint16_t opcode_0x30()
{
    if ((flag_n) == 1)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0x30].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0x30 - BMI - Branch on Minus (N Flag Set)
uint16_t opcode_0x10()
{
    if ((flag_n) == 0)
        Branch_Taken();
    else
    {
        register_pc = register_pc + opcode_info[0x10].length;
        Fetch_Immediate(1);
        Begin_Fetch_Next_Opcode();
    }
    return (register_pc);
} // 0x10 - BPL - Branch on Plus  (N Flag Clear)

// -------------------------------------------------
// Jumps and Returns
// -------------------------------------------------
uint16_t opcode_0x4C()
{
    register_pc = Calculate_Absolute();
    start_read(register_pc);
    return (register_pc);
} // 0x4C - JMP - Jump Absolute

// -------------------------------------------------
// 0x6C - JMP - Jump Indirect
// -------------------------------------------------
uint16_t opcode_0x6C()
{
    uint16_t lal, lah;
    uint16_t adl, adh;

    lal = Fetch_Immediate(1);
    lah = Fetch_Immediate(2) << 8;
    adl = read_byte(lah + lal);
    adh = read_byte(lah + lal + 1) << 8;
    effective_address = adh + adl;
    register_pc = (0xFF00 & adh) + (0x00FF & effective_address); // 6502 page wrapping bug
    start_read(register_pc);
    return (register_pc);
}

// -------------------------------------------------
// 0x20 - JSR - Jump to Subroutine
// -------------------------------------------------
uint16_t opcode_0x20()
{
    uint16_t adl, adh;

    adl = Fetch_Immediate(1);
    adh = Fetch_Immediate(2) << 8;
    read_byte(register_sp_fixed);
    push((0xFF00 & register_pc) >> 8);

    push(0x00FF & register_pc);
    register_pc = adh + adl;
    start_read(register_pc);
    return (register_pc);
}

// -------------------------------------------------
// 0x40 - RTI - Return from Interrupt
// -------------------------------------------------
uint16_t opcode_0x40()
{
    uint16_t pcl, pch;

    Fetch_Immediate(1);
    read_byte(register_sp_fixed);
    register_flags = pop();
    pcl = pop();
    pch = pop() << 8;
    register_pc = pch + pcl;
    start_read(register_pc);
    return (register_pc);
}

// -------------------------------------------------
// 0x60 - RTS - Return from Subroutine
// -------------------------------------------------
uint16_t opcode_0x60()
{
    uint16_t pcl, pch;

    Fetch_Immediate(1);
    read_byte(register_sp_fixed);
    pcl = pop();
    pch = pop() << 8;
    register_pc = pch + pcl + 3;
    read_byte(register_pc);
    start_read(register_pc);
    return (register_pc);
}

// -------------------------------------------------
//
// *** Undocumented 6502 Opcodes ***
//
// -------------------------------------------------

// --------------------------------------------------------------------------------------------------
// SLO - Shift left one bit in memory, then OR accumulator with memory.
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_SLO(uint8_t local_data)
{

    if ((0x80 & local_data) == 0x80)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = ((local_data << 1) & 0xFE);

    register_a = register_a | local_data;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return local_data;
}
uint16_t opcode_0x07()
{
    Double_WriteBack(Calculate_SLO(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x07].length);
} // 0x07 - SLO - ZeroPage
uint16_t opcode_0x17()
{
    Double_WriteBack(Calculate_SLO(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x17].length);
} // 0x17 - SLO - ZeroPage , X
uint16_t opcode_0x03()
{
    Double_WriteBack(Calculate_SLO(Fetch_Indexed_Indirect_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x03].length);
} // 0x03 - SLO - Indexed Indirect X
uint16_t opcode_0x13()
{
    Double_WriteBack(Calculate_SLO(Fetch_Indexed_Indirect_Y(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x13].length);
} // 0x13 - SLO - Indirect Indexed  Y
uint16_t opcode_0x0F()
{
    Double_WriteBack(Calculate_SLO(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x0F].length);
} // 0x0F - SLO - Absolute
uint16_t opcode_0x1F()
{
    Double_WriteBack(Calculate_SLO(Fetch_Absolute_X(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x1F].length);
} // 0x1F - SLO - Absolute , X
uint16_t opcode_0x1B()
{
    Double_WriteBack(Calculate_SLO(Fetch_Absolute_Y(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x1B].length);
} // 0x1B - SLO - Absolute , Y

// --------------------------------------------------------------------------------------------------
// RLA - Rotate one bit left in memory, then AND accumulator with memory.
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_RLA(uint8_t local_data)
{
    uint8_t old_carry_flag = 0;

    old_carry_flag = 0x1 & register_flags; // Store the old carry flag to be rotated in

    if (0x80 & local_data)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (local_data << 1) | old_carry_flag;

    register_a = register_a & local_data;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return local_data;
}
uint16_t opcode_0x27()
{
    Double_WriteBack(Calculate_RLA(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x27].length);
} // 0x27 - RLA - ZeroPage
uint16_t opcode_0x37()
{
    Double_WriteBack(Calculate_RLA(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x37].length);
} // 0x37 - RLA - ZeroPage , X
uint16_t opcode_0x23()
{
    Double_WriteBack(Calculate_RLA(Fetch_Indexed_Indirect_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x23].length);
} // 0x23 - RLA - Indexed Indirect X
uint16_t opcode_0x33()
{
    Double_WriteBack(Calculate_RLA(Fetch_Indexed_Indirect_Y(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x33].length);
} // 0x33 - RLA - Indirect Indexed  Y
uint16_t opcode_0x2F()
{
    Double_WriteBack(Calculate_RLA(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x2F].length);
} // 0x2F - RLA - Absolute
uint16_t opcode_0x3F()
{
    Double_WriteBack(Calculate_RLA(Fetch_Absolute_X(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x3F].length);
} // 0x3F - RLA - Absolute , X
uint16_t opcode_0x3B()
{
    Double_WriteBack(Calculate_RLA(Fetch_Absolute_Y(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x3B].length);
} // 0x3B - RLA - Absolute , Y

// --------------------------------------------------------------------------------------------------
// SRE - Shift right one bit in memory, then EOR accumulator with memory.
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_SRE(uint8_t local_data)
{

    if ((0x01 & local_data) == 0x01)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (0x7F & (local_data >> 1));

    register_a = register_a ^ local_data;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return local_data;
}
uint16_t opcode_0x47()
{
    Double_WriteBack(Calculate_SRE(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x47].length);
} // 0x47 - SRE - ZeroPage
uint16_t opcode_0x57()
{
    Double_WriteBack(Calculate_SRE(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x57].length);
} // 0x57 - SRE - ZeroPage , X
uint16_t opcode_0x43()
{
    Double_WriteBack(Calculate_SRE(Fetch_Indexed_Indirect_X()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x43].length);
} // 0x43 - SRE - Indexed Indirect X
uint16_t opcode_0x53()
{
    Double_WriteBack(Calculate_SRE(Fetch_Indexed_Indirect_Y(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x53].length);
} // 0x53 - SRE - Indirect Indexed  Y
uint16_t opcode_0x4F()
{
    Double_WriteBack(Calculate_SRE(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x4F].length);
} // 0x4F - SRE - Absolute
uint16_t opcode_0x5F()
{
    Double_WriteBack(Calculate_SRE(Fetch_Absolute_X(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x5F].length);
} // 0x5F - SRE - Absolute , X
uint16_t opcode_0x5B()
{
    Double_WriteBack(Calculate_SRE(Fetch_Absolute_Y(1)));
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x5B].length);
} // 0x5B - SRE - Absolute , Y

// --------------------------------------------------------------------------------------------------
// RRA - Rotate one bit right in memory, then add memory to accumulator (with carry).
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_RRA(uint8_t local_data)
{

    if ((0x01 & local_data) == 0x01)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (0x7F & (local_data >> 1));

    global_temp = local_data;

    return local_data;
}
uint16_t opcode_0x67()
{
    Double_WriteBack(Calculate_RRA(Fetch_ZeroPage()));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x67].length);
} // 0x67 - RRA - ZeroPage
uint16_t opcode_0x77()
{
    Double_WriteBack(Calculate_RRA(Fetch_ZeroPage_X()));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x77].length);
} // 0x77 - RRA - ZeroPage , X
uint16_t opcode_0x63()
{
    Double_WriteBack(Calculate_RRA(Fetch_Indexed_Indirect_X()));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x63].length);
} // 0x63 - RRA - Indexed Indirect X
uint16_t opcode_0x73()
{
    Double_WriteBack(Calculate_RRA(Fetch_Indexed_Indirect_Y(1)));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x73].length);
} // 0x73 - RRA - Indirect Indexed  Y
uint16_t opcode_0x6F()
{
    Double_WriteBack(Calculate_RRA(Fetch_Absolute()));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x6F].length);
} // 0x6F - RRA - Absolute
uint16_t opcode_0x7F()
{
    Double_WriteBack(Calculate_RRA(Fetch_Absolute_X(1)));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x7F].length);
} // 0x7F - RRA - Absolute , X
uint16_t opcode_0x7B()
{
    Double_WriteBack(Calculate_RRA(Fetch_Absolute_Y(1)));
    Calculate_ADC(global_temp);
    return (register_pc + opcode_info[0x7B].length);
} // 0x7B - RRA - Absolute , Y

// --------------------------------------------------------------------------------------------------
// AND the contents of the A and X registers (without changing the contents of either register) and
// stores the result in memory.
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x87()
{
    Write_ZeroPage(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x87].length);
} // 0x87 - SAX - ZeroPage
uint16_t opcode_0x97()
{
    Write_ZeroPage_Y(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x97].length);
} // 0x97 - SAX - ZeroPage , Y
uint16_t opcode_0x83()
{
    Write_Indexed_Indirect_X(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x83].length);
} // 0x83 - SAX - Indexed Indirect X
uint16_t opcode_0x8F()
{
    Write_Absolute(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x8F].length);
} // 0x8F - SAX - Absolute

// --------------------------------------------------------------------------------------------------
// Load both the accumulator and the X register with the contents of a memory location.
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0xA7()
{
    register_a = Fetch_ZeroPage();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xA7].length);
} // 0xA7 - LAX - ZeroPage
uint16_t opcode_0xB7()
{
    register_a = Fetch_ZeroPage_Y();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xB7].length);
} // 0xB7 - LAX - ZeroPage , Y
uint16_t opcode_0xA3()
{
    register_a = Fetch_Indexed_Indirect_X();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xA3].length);
} // 0xA3 - LAX - Indexed Indirect X
uint16_t opcode_0xB3()
{
    register_a = Fetch_Indexed_Indirect_Y(1);
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xB3].length);
} // 0xB3 - LAX - Indirect Indexed  Y
uint16_t opcode_0xAF()
{
    register_a = Fetch_Absolute();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xAF].length);
} // 0xAF - LAX - Absolute
uint16_t opcode_0xBF()
{
    register_a = Fetch_Absolute_Y(1);
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xBF].length);
} // 0xBF - LAX - Absolute , Y

// --------------------------------------------------------------------------------------------------
// Decrement the contents of a memory location and then compare the result with the A register.
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0xC7()
{
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage()));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xC7].length);
} // 0xC7 - DCP - ZeroPage
uint16_t opcode_0xD7()
{
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage_X()));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xD7].length);
} // 0xD7 - DCP - ZeroPage , X
uint16_t opcode_0xC3()
{
    Double_WriteBack(Calculate_DEC(Fetch_Indexed_Indirect_X()));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xC3].length);
} // 0xC3 - DCP - Indexed Indirect X
uint16_t opcode_0xD3()
{
    Double_WriteBack(Calculate_DEC(Fetch_Indexed_Indirect_Y(0)));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xD3].length);
} // 0xD3 - DCP - Indirect Indexed  Y
uint16_t opcode_0xCF()
{
    Double_WriteBack(Calculate_DEC(Fetch_Absolute()));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xCF].length);
} // 0xCF - DCP - Absolute
uint16_t opcode_0xDF()
{
    Double_WriteBack(Calculate_DEC(Fetch_Absolute_X(0)));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xDF].length);
} // 0xDF - DCP - Absolute , X
uint16_t opcode_0xDB()
{
    Double_WriteBack(Calculate_DEC(Fetch_Absolute_Y(0)));
    Calculate_CMP(global_temp);
    return (register_pc + opcode_info[0xDB].length);
} // 0xDB - DCP - Absolute , Y

// --------------------------------------------------------------------------------------------------
// ISC - Increase memory by one, then subtract memory from accumulator (with borrow).
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0xE7()
{
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage()));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xE7].length);
} // 0xE7 - ISC - ZeroPage
uint16_t opcode_0xF7()
{
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage_X()));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xF7].length);
} // 0xF7 - ISC - ZeroPage , X
uint16_t opcode_0xE3()
{
    Double_WriteBack(Calculate_INC(Fetch_Indexed_Indirect_X()));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xE3].length);
} // 0xE3 - ISC - Indexed Indirect X
uint16_t opcode_0xF3()
{
    Double_WriteBack(Calculate_INC(Fetch_Indexed_Indirect_Y(0)));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xF3].length);
} // 0xF3 - ISC - Indirect Indexed  Y
uint16_t opcode_0xEF()
{
    Double_WriteBack(Calculate_INC(Fetch_Absolute()));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xEF].length);
} // 0xEF - ISC - Absolute
uint16_t opcode_0xFF()
{
    Double_WriteBack(Calculate_INC(Fetch_Absolute_X(0)));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xFF].length);
} // 0xFF - ISC - Absolute , X
uint16_t opcode_0xFB()
{
    Double_WriteBack(Calculate_INC(Fetch_Absolute_Y(0)));
    Calculate_SBC(global_temp);
    return (register_pc + opcode_info[0xFB].length);
} // 0xFB - ISC - Absolute , Y

// --------------------------------------------------------------------------------------------------
// ANC - ANDs the contents of the A register with an immediate value and then moves bit 7 of A
// into the Carry flag.
// --------------------------------------------------------------------------------------------------
void Calculate_ANC(uint8_t local_data)
{

    Begin_Fetch_Next_Opcode();

    register_a = register_a & local_data;

    if ((0x80 & register_a) == 0x80)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}
uint16_t opcode_0x0B()
{
    Calculate_ANC(Fetch_Immediate(1));
    return (register_pc + opcode_info[0x0B].length);
} // 0x0B - ANC - Immediate
uint16_t opcode_0x2B()
{
    Calculate_ANC(Fetch_Immediate(1));
    return (register_pc + opcode_info[0x2B].length);
} // 0x2B - ANC - Immediate

// --------------------------------------------------------------------------------------------------
// ALR - AND the contents of the A register with an immediate value and then LSRs the result.
// --------------------------------------------------------------------------------------------------
void Calculate_ALR(uint8_t local_data)
{

    Begin_Fetch_Next_Opcode();

    register_a = register_a & local_data;

    if ((0x01 & register_a) == 0x01)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = (0x7F & (register_a >> 1));

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}
uint16_t opcode_0x4B()
{
    Calculate_ALR(Fetch_Immediate(1));
    return (register_pc + opcode_info[0x4B].length);
} // 0x4B - ALR - Immediate

// --------------------------------------------------------------------------------------------------
// ARR - ANDs the accumulator with an immediate value and then rotates the content right.
// --------------------------------------------------------------------------------------------------
void Calculate_ARR(uint8_t local_data)
{

    Begin_Fetch_Next_Opcode();

    register_a = register_a & local_data;

    register_a = (0x7F & (register_a >> 1));

    register_flags = register_flags & 0xBE; // Pre-clear the C and V flags
    if ((0xC0 & register_a) == 0x40)
    {
        register_flags = register_flags | 0x40;
    } // Set the V flag
    if ((0xC0 & register_a) == 0x80)
    {
        register_flags = register_flags | 0x41;
    } // Set the C and V flags
    if ((0xC0 & register_a) == 0xC0)
    {
        register_flags = register_flags | 0x01;
    } // Set the C flag

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}
uint16_t opcode_0x6B()
{
    Calculate_ARR(Fetch_Immediate(1));
    return (register_pc + opcode_info[0x6B].length);
} // 0x6B - ARR - Immediate

// --------------------------------------------------------------------------------------------------
// SBX - ANDs the contents of the A and X registers (leaving the contents of A intact),
// subtracts an immediate value, and then stores the result in X.
// --------------------------------------------------------------------------------------------------
void Calculate_SBX(uint16_t local_data)
{
    int16_t signed_total = 0;

    Begin_Fetch_Next_Opcode();

    register_x = register_a & register_x;

    register_x = register_x - local_data;
    signed_total = (int16_t)register_x - (int16_t)(local_data);

    if (signed_total >= 0)
        register_flags = register_flags | 0x01; // Set the C flag
    else
        register_flags = register_flags & 0xFE; // Clear the C flag

    register_x = (0xFF & register_x);
    Calc_Flags_NEGATIVE_ZERO(register_x);

    return;
}
uint16_t opcode_0xCB()
{
    Calculate_SBX(Fetch_Immediate(1));
    return (register_pc + opcode_info[0xCB].length);
} // 0xCB - SBX - Immediate

// --------------------------------------------------------------------------------------------------
// LAS - AND memory with stack pointer, transfer result to accumulator, X register and stack pointer.
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0xBB()
{
    register_sp = (register_sp & Fetch_Absolute_Y(1));
    register_a = register_sp;
    register_x = register_sp;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return (register_pc + opcode_info[0xBB].length);
} // 0xBB - LAS - Absolute , Y

// --------------------------------------------------------------------------------------------------
// NOP - Fetch Immediate
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x80()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x80].length);
} // 0x80 - NOP - Immediate
uint16_t opcode_0x82()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x82].length);
} // 0x82 - NOP - Immediate
uint16_t opcode_0xC2()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xC2].length);
} // 0xC2 - NOP - Immediate
uint16_t opcode_0xE2()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xE2].length);
} // 0xE2 - NOP - Immediate
uint16_t opcode_0x89()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x89].length);
} // 0x89 - NOP - Immediate

// --------------------------------------------------------------------------------------------------
// NOP - Fetch ZeroPage
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x04()
{
    Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x04].length);
} // 0x04 - NOP - ZeroPage
uint16_t opcode_0x44()
{
    Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x44].length);
} // 0x44 - NOP - ZeroPage
uint16_t opcode_0x64()
{
    Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x64].length);
} // 0x64 - NOP - ZeroPage

// --------------------------------------------------------------------------------------------------
// NOP - Fetch ZeroPage , X
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x14()
{
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x14].length);
} // 0x14 - NOP - ZeroPage , X
uint16_t opcode_0x34()
{
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x34].length);
} // 0x34 - NOP - ZeroPage , X
uint16_t opcode_0x54()
{
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x54].length);
} // 0x54 - NOP - ZeroPage , X
uint16_t opcode_0x74()
{
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x74].length);
} // 0x74 - NOP - ZeroPage , X
uint16_t opcode_0xD4()
{
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xD4].length);
} // 0xD4 - NOP - ZeroPage , X
uint16_t opcode_0xF4()
{
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xF4].length);
} // 0xF4 - NOP - ZeroPage , X

// --------------------------------------------------------------------------------------------------
// NOP - Fetch Absolute
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x0C()
{
    Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x0C].length);
} // 0x0C - NOP - Absolute

// --------------------------------------------------------------------------------------------------
// NOP - Fetch Absolute , X
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x1C()
{
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x1C].length);
} // 0x1C - NOP - Absolute , X
uint16_t opcode_0x3C()
{
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x3C].length);
} // 0x3C - NOP - Absolute , X
uint16_t opcode_0x5C()
{
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x5C].length);
} // 0x5C - NOP - Absolute , X
uint16_t opcode_0x7C()
{
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x7C].length);
} // 0x7C - NOP - Absolute , X
uint16_t opcode_0xDC()
{
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xDC].length);
} // 0xDC - NOP - Absolute , X
uint16_t opcode_0xFC()
{
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xFC].length);
} // 0xFC - NOP - Absolute , X

// --------------------------------------------------------------------------------------------------
// JAM - Lock up the processor
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x02()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x02].length);
} // 0x02 - JAM
uint16_t opcode_0x12()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x12].length);
} // 0x12 - JAM
uint16_t opcode_0x22()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x22].length);
} // 0x22 - JAM
uint16_t opcode_0x32()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x32].length);
} // 0x32 - JAM
uint16_t opcode_0x42()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x42].length);
} // 0x42 - JAM
uint16_t opcode_0x52()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x52].length);
} // 0x52 - JAM
uint16_t opcode_0x62()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x62].length);
} // 0x62 - JAM
uint16_t opcode_0x72()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x72].length);
} // 0x72 - JAM
uint16_t opcode_0x92()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0x92].length);
} // 0x92 - JAM
uint16_t opcode_0xB2()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0xB2].length);
} // 0xB2 - JAM
uint16_t opcode_0xD2()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0xD2].length);
} // 0xD2 - JAM
uint16_t opcode_0xF2()
{
    Fetch_Immediate(1);
    while (1)
    {
    }
    return (register_pc + opcode_info[0xF2].length);
} // 0xF2 - JAM

// --------------------------------------------------------------------------------------------------
// Unstable 6502 opcodes
// --------------------------------------------------------------------------------------------------
uint16_t opcode_0x93()
{
    Fetch_ZeroPage_Y();
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x93].length);
} // 0x93 - SHA - ZeroPage , Y - Implelented here as a size 2 NOP
uint16_t opcode_0x9F()
{
    Fetch_Absolute_Y(0);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x9F].length);
} // 0x9F - SHA - Absolute , Y - Implelented here as a size 3 NOP
uint16_t opcode_0x9E()
{
    Fetch_Absolute_Y(0);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x9E].length);
} // 0x9E - SHX - Absolute , Y - Implelented here as a size 3 NOP
uint16_t opcode_0x9C()
{
    Fetch_Absolute_X(0);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x9C].length);
} // 0x9C - SHY - Absolute , X - Implelented here as a size 3 NOP
uint16_t opcode_0x9B()
{
    Fetch_Absolute_Y(0);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x9B].length);
} // 0x9B - TAS - Absolute , Y - Implelented here as a size 3 NOP
uint16_t opcode_0x8B()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0x8B].length);
} // 0x8B - ANE - Immediate    - Implelented here as a size 2 NOP
uint16_t opcode_0xAB()
{
    Fetch_Immediate(1);
    Begin_Fetch_Next_Opcode();
    return (register_pc + opcode_info[0xAB].length);
} // 0xAB - LAX - Immediate    - Implelented here as a size 2 NOP

uint16_t opcode_0x00()
{
    return (register_pc + opcode_info[0x00].length);
} // 0xAB - LAX - Immediate    - Implelented here as a size 2 NOP

//-------------------------------------------------------------------------
//  Initialize opcodes once the opcode-execution function has been defined
//
void initialize_opcode_info()
{

    for (int i = 0; i <= 255; i++)
        opcode_info[i] = {"---", "", 0, 1, 0, illegal_opcode};

    opcode_info[0x00] = {"BRK", "", "B", 7, 1, opcode_0x00};
    opcode_info[0x01] = {"ORA", "(ind,X)", "SZ", 6, 2, opcode_0x01};
    opcode_info[0x05] = {"ORA", "zpg", "SZ", 3, 2, opcode_0x05};
    opcode_info[0x06] = {"ASL", "zpg", "SZC", 5, 2, opcode_0x06};
    opcode_info[0x08] = {"PHP", "", "", 3, 1, opcode_0x08};
    opcode_info[0x09] = {"ORA", "#", "SZ", 2, 2, opcode_0x09};
    opcode_info[0x0a] = {"ASL", "A", "SZC", 2, 1, opcode_0x0A};
    opcode_info[0x0d] = {"ORA", "abs", "SZ", 4, 3, opcode_0x0D};
    opcode_info[0x0e] = {"ASL", "abs", "SZC", 6, 3, opcode_0x0E};
    opcode_info[0x10] = {"BPL", "rel", "", 2, 2, opcode_0x10};
    opcode_info[0x11] = {"ORA", "(ind),Y", "SZ", 5, 2, opcode_0x11};
    opcode_info[0x15] = {"ORA", "zpg,X", "SZ", 4, 2, opcode_0x15};
    opcode_info[0x16] = {"ASL", "zpg,X", "SZC", 6, 2, opcode_0x16};
    opcode_info[0x18] = {"CLC", "", "C", 2, 1, opcode_0x18};
    opcode_info[0x19] = {"ORA", "abs,Y", "SZ", 4, 3, opcode_0x19};
    opcode_info[0x1d] = {"ORA", "abs,X", "SZ", 4, 3, opcode_0x1D};
    opcode_info[0x1e] = {"ASL", "abs,X", "SZC", 7, 3, opcode_0x1E};
    opcode_info[0x20] = {"JSR", "abs", "", 6, 3, opcode_0x20};
    opcode_info[0x21] = {"AND", "(ind,X)", "SZ", 6, 2, opcode_0x21};
    opcode_info[0x24] = {"BIT", "zpg", "NVZ", 3, 2, opcode_0x24};
    opcode_info[0x25] = {"AND", "zpg", "SZ", 3, 2, opcode_0x25};
    opcode_info[0x26] = {"ROL", "zpg", "SZC", 5, 2, opcode_0x26};
    opcode_info[0x28] = {"PLP", "", "", 4, 1, opcode_0x28};
    opcode_info[0x29] = {"AND", "#", "SZ", 2, 2, opcode_0x29};
    opcode_info[0x2a] = {"ROL", "A", "SZC", 2, 1, opcode_0x2A};
    opcode_info[0x2c] = {"BIT", "abs", "NVZ", 4, 3, opcode_0x2C};
    opcode_info[0x2d] = {"AND", "abs", "SZ", 4, 3, opcode_0x2D};
    opcode_info[0x2e] = {"ROL", "abs", "SZC", 6, 3, opcode_0x2E};
    opcode_info[0x30] = {"BMI", "rel", "", 2, 2, opcode_0x30};
    opcode_info[0x31] = {"AND", "(ind),Y", "SZ", 5, 2, opcode_0x31};
    opcode_info[0x35] = {"AND", "zpg,X", "SZ", 4, 2, opcode_0x35};
    opcode_info[0x36] = {"ROL", "zpg,X", "SZC", 6, 2, opcode_0x36};
    opcode_info[0x38] = {"SEC", "", "C", 2, 1, opcode_0x38};
    opcode_info[0x39] = {"AND", "abs,Y", "SZ", 4, 3, opcode_0x39};
    opcode_info[0x3d] = {"AND", "abs,X", "SZ", 4, 3, opcode_0x3D};
    opcode_info[0x3e] = {"ROL", "abs,X", "SZC", 7, 3, opcode_0x3E};
    opcode_info[0x40] = {"RTI", "", "SZCDVIB", 6, 1, opcode_0x40};
    opcode_info[0x41] = {"EOR", "(ind,X)", "SZ", 6, 2, opcode_0x41};
    opcode_info[0x45] = {"EOR", "zpg", "SZ", 3, 2, opcode_0x45};
    opcode_info[0x46] = {"LSR", "zpg", "SZC", 5, 2, opcode_0x46};
    opcode_info[0x48] = {"PHA", "", "", 3, 1, opcode_0x48};
    opcode_info[0x49] = {"EOR", "#", "SZ", 2, 2, opcode_0x49};
    opcode_info[0x4a] = {"LSR", "A", "SZC", 2, 1, opcode_0x4A};
    opcode_info[0x4c] = {"JMP", "abs", "", 3, 3, opcode_0x4C};
    opcode_info[0x4d] = {"EOR", "abs", "SZ", 4, 3, opcode_0x4D};
    opcode_info[0x4e] = {"LSR", "abs", "SZC", 6, 3, opcode_0x4E};
    opcode_info[0x50] = {"BVC", "rel", "", 2, 2, opcode_0x50};
    opcode_info[0x51] = {"EOR", "(ind),Y", "SZ", 5, 2, opcode_0x51};
    opcode_info[0x55] = {"EOR", "zpg,X", "SZ", 4, 2, opcode_0x55};
    opcode_info[0x56] = {"LSR", "zpg,X", "SZC", 6, 2, opcode_0x56};
    opcode_info[0x58] = {"CLI", "", "I", 2, 1, opcode_0x58};
    opcode_info[0x59] = {"EOR", "abs,Y", "SZ", 4, 3, opcode_0x59};
    opcode_info[0x5d] = {"EOR", "abs,X", "SZ", 4, 3, opcode_0x5D};
    opcode_info[0x5e] = {"LSR", "abs,X", "SZC", 7, 3, opcode_0x5E};
    opcode_info[0x60] = {"RTS", "", "", 6, 1, opcode_0x60};
    opcode_info[0x61] = {"ADC", "(ind,X)", "SVZC", 6, 2, opcode_0x61};
    opcode_info[0x65] = {"ADC", "zpg", "SVZC", 3, 2, opcode_0x65};
    opcode_info[0x66] = {"ROR", "zpg", "SZC", 5, 2, opcode_0x66};
    opcode_info[0x68] = {"PLA", "", "", 4, 1, opcode_0x68};
    opcode_info[0x69] = {"ADC", "#", "SVZC", 2, 2, opcode_0x69};
    opcode_info[0x6a] = {"ROR", "A", "SZC", 2, 1, opcode_0x6A};
    opcode_info[0x6c] = {"JMP", "(ind)", "", 5, 3, opcode_0x6C};
    opcode_info[0x6d] = {"ADC", "abs", "SVZC", 4, 3, opcode_0x6D};
    opcode_info[0x6e] = {"ROR", "abs", "SZC", 6, 3, opcode_0x6E};
    opcode_info[0x70] = {"BVS", "rel", "", 4, 2, opcode_0x70};
    opcode_info[0x71] = {"ADC", "(ind),Y", "SVZC", 4, 2, opcode_0x71};
    opcode_info[0x75] = {"ADC", "zpg,X", "SVZC", 4, 2, opcode_0x75};
    opcode_info[0x76] = {"ROR", "zpg,X", "SZC", 6, 2, opcode_0x76};
    opcode_info[0x78] = {"SEI", "", "I", 2, 1, opcode_0x78};
    opcode_info[0x79] = {"ADC", "abs,Y", "SVZC", 4, 3, opcode_0x79};
    opcode_info[0x7d] = {"ADC", "abs,X", "SVZC", 4, 3, opcode_0x7D};
    opcode_info[0x7e] = {"ROR", "abs,X", "SZC", 7, 3, opcode_0x7E};
    opcode_info[0x81] = {"STA", "(ind,X)", "", 6, 2, opcode_0x81};
    opcode_info[0x84] = {"STY", "zpg", "", 3, 2, opcode_0x84};
    opcode_info[0x85] = {"STA", "zpg", "", 3, 2, opcode_0x85};
    opcode_info[0x86] = {"STX", "zpg", "", 3, 2, opcode_0x86};
    opcode_info[0x88] = {"DEY", "", "SZ", 2, 1, opcode_0x88};
    opcode_info[0x8a] = {"TXA", "", "SZ", 2, 1, opcode_0x8A};
    opcode_info[0x8c] = {"STY", "abs", "", 4, 3, opcode_0x8C};
    opcode_info[0x8d] = {"STA", "abs", "", 4, 3, opcode_0x8D};
    opcode_info[0x8e] = {"STX", "abs", "", 4, 3, opcode_0x8E};
    opcode_info[0x90] = {"BCC", "rel", "", 2, 2, opcode_0x90};
    opcode_info[0x91] = {"STA", "(ind),Y", "", 6, 2, opcode_0x91};
    opcode_info[0x94] = {"STY", "zpg,X", "", 4, 2, opcode_0x94};
    opcode_info[0x95] = {"STA", "zpg,X", "", 4, 2, opcode_0x95};
    opcode_info[0x96] = {"STX", "zpg,Y", "", 4, 2, opcode_0x96};
    opcode_info[0x98] = {"TYA", "", "SZ", 2, 1, opcode_0x98};
    opcode_info[0x99] = {"STA", "abs,Y", "", 5, 3, opcode_0x99};
    opcode_info[0x9a] = {"TXS", "", "", 2, 1, opcode_0x9A};
    opcode_info[0x9d] = {"STA", "abs,X", "", 5, 3, opcode_0x9D};
    opcode_info[0xa0] = {"LDY", "#", "SZ", 2, 2, opcode_0xA0};
    opcode_info[0xa1] = {"LDA", "(ind,X)", "SZ", 6, 2, opcode_0xA1};
    opcode_info[0xa2] = {"LDX", "#", "SZ", 2, 2, opcode_0xA2};
    opcode_info[0xa4] = {"LDY", "zpg", "SZ", 3, 2, opcode_0xA4};
    opcode_info[0xa5] = {"LDA", "zpg", "SZ", 3, 2, opcode_0xA5};
    opcode_info[0xa6] = {"LDX", "zpg", "SZ", 3, 2, opcode_0xA6};
    opcode_info[0xa8] = {"TAY", "", "SZ", 2, 1, opcode_0xA8};
    opcode_info[0xa9] = {"LDA", "#", "SZ", 2, 2, opcode_0xA9};
    opcode_info[0xaa] = {"TAX", "", "SZ", 2, 1, opcode_0xAA};
    opcode_info[0xac] = {"LDY", "abs", "SZ", 4, 3, opcode_0xAC};
    opcode_info[0xad] = {"LDA", "abs", "SZ", 4, 3, opcode_0xAD};
    opcode_info[0xae] = {"LDX", "abs", "SZ", 4, 3, opcode_0xAE};
    opcode_info[0xb0] = {"BCS", "rel", "", 2, 2, opcode_0xB0};
    opcode_info[0xb1] = {"LDA", "(ind),Y", "SZ", 5, 2, opcode_0xB1};
    opcode_info[0xb4] = {"LDY", "zpg,X", "SZ", 4, 2, opcode_0xB4};
    opcode_info[0xb5] = {"LDA", "zpg,X", "SZ", 4, 2, opcode_0xB5};
    opcode_info[0xb6] = {"LDX", "zpg,Y", "SZ", 4, 2, opcode_0xB6};
    opcode_info[0xb8] = {"CLV", "", "V", 2, 1, opcode_0xB8};
    opcode_info[0xb9] = {"LDA", "abs,Y", "SZ", 4, 3, opcode_0xB9};
    opcode_info[0xba] = {"TSX", "", "", 2, 1, opcode_0xBA};
    opcode_info[0xbc] = {"LDY", "abs,X", "SZ", 4, 3, opcode_0xBC};
    opcode_info[0xbd] = {"LDA", "abs,X", "SZ", 4, 3, opcode_0xBD};
    opcode_info[0xbe] = {"LDX", "abs,Y", "SZ", 4, 3, opcode_0xBE};
    opcode_info[0xc0] = {"CPY", "#", "SZC", 2, 2, opcode_0xC0};
    opcode_info[0xc1] = {"CMP", "(ind,X)", "SZC", 6, 2, opcode_0xC1};
    opcode_info[0xc4] = {"CPY", "zpg", "SZC", 3, 2, opcode_0xC4};
    opcode_info[0xc5] = {"CMP", "zpg", "SZC", 3, 2, opcode_0xC5};
    opcode_info[0xc6] = {"DEC", "zpg", "SZ", 5, 2, opcode_0xC6};
    opcode_info[0xc8] = {"INY", "", "", 2, 1, opcode_0xC8};
    opcode_info[0xc9] = {"CMP", "#", "SZC", 2, 2, opcode_0xC9};
    opcode_info[0xca] = {"DEX", "", "SZ", 2, 1, opcode_0xCA};
    opcode_info[0xcc] = {"CPY", "abs", "SZC", 4, 3, opcode_0xCC};
    opcode_info[0xcd] = {"CMP", "abs", "SZC", 4, 3, opcode_0xCD};
    opcode_info[0xce] = {"DEC", "abs", "SZ", 6, 3, opcode_0xCE};
    opcode_info[0xd0] = {"BNE", "rel", "", 2, 2, opcode_0xD0};
    opcode_info[0xd1] = {"CMP", "(ind),Y", "SZC", 5, 2, opcode_0xD1};
    opcode_info[0xd5] = {"CMP", "zpg,X", "SZC", 4, 2, opcode_0xD5};
    opcode_info[0xd6] = {"DEC", "zpg,X", "SZ", 6, 2, opcode_0xD6};
    opcode_info[0xd8] = {"CLD", "", "D", 2, 1, opcode_0xD8};
    opcode_info[0xd9] = {"CMP", "abs,Y", "SZC", 4, 3, opcode_0xD9};
    opcode_info[0xdd] = {"CMP", "abs,X", "SZC", 4, 3, opcode_0xDD};
    opcode_info[0xde] = {"DEC", "abs,X", "SZ", 7, 3, opcode_0xDE};
    opcode_info[0xe0] = {"CPX", "#", "SZC", 2, 2, opcode_0xE0};
    opcode_info[0xe1] = {"SBC", "(ind,X)", "SVZC", 6, 2, opcode_0xE1};
    opcode_info[0xe4] = {"CPX", "zpg", "SZC", 3, 2, opcode_0xE4};
    opcode_info[0xe5] = {"SBC", "zpg", "SVZC", 3, 2, opcode_0xE5};
    opcode_info[0xe6] = {"INC", "zpg", "SZ", 5, 2, opcode_0xE6};
    opcode_info[0xe8] = {"INX", "", "SZ", 2, 1, opcode_0xE8};
    opcode_info[0xe9] = {"SBC", "#", "SVZC", 2, 2, opcode_0xE9};
    opcode_info[0xea] = {"NOP", "", "", 2, 1, opcode_0xEA};
    opcode_info[0xec] = {"CPX", "abs", "SZC", 4, 3, opcode_0xEC};
    opcode_info[0xed] = {"SBC", "abs", "SVZC", 4, 3, opcode_0xED};
    opcode_info[0xee] = {"INC", "abs", "SZ", 6, 3, opcode_0xEE};
    opcode_info[0xf0] = {"BEQ", "rel", "", 2, 2, opcode_0xF0};
    opcode_info[0xf1] = {"SBC", "(ind),Y", "SVZC", 5, 2, opcode_0xF1};
    opcode_info[0xf5] = {"SBC", "zpg,X", "SVZC", 4, 2, opcode_0xF5};
    opcode_info[0xf6] = {"INC", "zpg,X", "SZ", 6, 2, opcode_0xF6};
    opcode_info[0xf8] = {"SED", "", "D", 2, 1, opcode_0xF8};
    opcode_info[0xf9] = {"SBC", "abs,Y", "SVZC", 4, 3, opcode_0xF9};
    opcode_info[0xfd] = {"SBC", "abs,X", "SVZC", 4, 3, opcode_0xFD};
    opcode_info[0xfe] = {"INC", "abs,X", "SZ", 7, 3, opcode_0xFE};
}
