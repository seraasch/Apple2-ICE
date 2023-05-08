
// -------------------------------------------------
//
//               6502 Opcodes
//
// -------------------------------------------------

// -------------------------------------------------
// 0x0A - ASL A - Arithmetic Shift Left - Accumulator
// -------------------------------------------------
void opcode_0x0A() {

    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    if (0x80 & register_a) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = register_a << 1;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}

// -------------------------------------------------
// 0x4A - LSR A - Logical Shift Right - Accumulator
// -------------------------------------------------
void opcode_0x4A() {

    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    if (0x01 & register_a) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = register_a >> 1;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}

// -------------------------------------------------
// 0x6A - ROR A - Rotate Right - Accumulator
// -------------------------------------------------
void opcode_0x6A() {

    uint8_t old_carry_flag = 0;

    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    old_carry_flag = register_flags << 7; // Shift the old carry flag to bit[8] to be rotated in

    if (0x01 & register_a) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = (old_carry_flag | (register_a >> 1));

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}

// -------------------------------------------------
// 0x2A - ROL A - Rotate Left - Accumulator
// -------------------------------------------------
void opcode_0x2A() {

    uint8_t old_carry_flag = 0;

    read_byte(register_pc);
    Begin_Fetch_Next_Opcode();

    old_carry_flag = 0x1 & register_flags; // Store the old carry flag to be rotated in

    if (0x80 & register_a) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = (register_a << 1) | old_carry_flag;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}

// -------------------------------------------------
// ADC 
// -------------------------------------------------
void Calculate_ADC(uint16_t local_data) {
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

    if ((flag_d) == 1) {
        bcd_low = (0x0F & register_a) + (0x0F & local_data) + (flag_c);
        if (bcd_low > 0x9) {
            low_carry = 0x10;
            bcd_low = bcd_low - 0xA;
        }

        bcd_high = (0xF0 & register_a) + (0xF0 & local_data) + low_carry;
        if (bcd_high > 0x90) {
            high_carry = 1;
            bcd_high = bcd_high - 0xA0;
        }

        register_flags = register_flags & 0xFE; // Clear the C flag
        if ((0x00FF & bcd_total) > 0x09) {
            bcd_total = bcd_total + 0x010;
            bcd_total = bcd_total - 0x0A;
        }

        if (high_carry == 1) {
            bcd_total = bcd_total - 0xA0;
            register_flags = register_flags | 0x01;
        } // Set the C flag
        else register_flags = register_flags & 0xFE; // Clear the C flag     

        total = (0xFF & (bcd_low + bcd_high));
    } else {
        total = register_a + local_data + (flag_c);

        if (total > 255) register_flags = register_flags | 0x01; // Set the C flag
        else register_flags = register_flags & 0xFE; // Clear the C flag
    }

    operand0 = (register_a & 0x80);
    operand1 = (local_data & 0x80);
    result = (total & 0x80);

    if (operand0 == 0 && operand1 == 0 && result != 0) register_flags = register_flags | 0x40; // Set the V flag
    else if (operand0 != 0 && operand1 != 0 && result == 0) register_flags = register_flags | 0x40;
    else register_flags = register_flags & 0xBF; // Clear the V flag

    register_a = (0xFF & total);
    Calc_Flags_NEGATIVE_ZERO(register_a);

    return;
}
void opcode_0x69() {
    Calculate_ADC(Fetch_Immediate());
    return;
} // 0x69 - ADC - Immediate - Binary
void opcode_0x65() {
    Calculate_ADC(Fetch_ZeroPage());
    return;
} // 0x65 - ADC - ZeroPage
void opcode_0x75() {
    Calculate_ADC(Fetch_ZeroPage_X());
    return;
} // 0x75 - ADC - ZeroPage , X
void opcode_0x6D() {
    Calculate_ADC(Fetch_Absolute());
    return;
} // 0x6D - ADC - Absolute
void opcode_0x7D() {
    Calculate_ADC(Fetch_Absolute_X(1));
    return;
} // 0x7D - ADC - Absolute , X
void opcode_0x79() {
    Calculate_ADC(Fetch_Absolute_Y(1));
    return;
} // 0x79 - ADC - Absolute , Y
void opcode_0x61() {
    Calculate_ADC(Fetch_Indexed_Indirect_X());
    return;
} // 0x61 - ADC - Indexed Indirect X
void opcode_0x71() {
    Calculate_ADC(Fetch_Indexed_Indirect_Y(1));
    return;
} // 0x71 - ADC - Indirect Indexed  Y

// -------------------------------------------------
// SBC 
// -------------------------------------------------
void Calculate_SBC(uint16_t local_data) {
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

    if (flag_c != 0) flag_c_invert = 0;
    else flag_c_invert = 1;

    if ((flag_d) == 1) {
        bcd_low = (0x0F & register_a) - (0x0F & local_data) - flag_c_invert;
        if (bcd_low > 0x9) {
            low_carry = 0x10;
            bcd_low = bcd_low + 0xA;
        }

        bcd_high = (0xF0 & register_a) - (0xF0 & local_data) - low_carry;
        if (bcd_high > 0x90) {
            high_carry = 1;
            bcd_high = bcd_high + 0xA0;
        }

        register_flags = register_flags & 0xFE; // Clear the C flag
        if ((0x00FF & bcd_total) > 0x09) {
            bcd_total = bcd_total + 0x010;
            bcd_total = bcd_total - 0x0A;
        }

        if (high_carry == 0) {
            bcd_total = bcd_total - 0xA0;
            register_flags = register_flags | 0x01;
        } // Set the C flag
        else register_flags = register_flags & 0xFE; // Clear the C flag     

        total = (0xFF & (bcd_low + bcd_high));
    } else {

        total = register_a - local_data - flag_c_invert;
        signed_total = (int16_t) register_a - (int16_t)(local_data) - flag_c_invert;

        if (signed_total >= 0) register_flags = register_flags | 0x01; // Set the C flag
        else register_flags = register_flags & 0xFE; // Clear the C flag  
    }

    operand0 = (register_a & 0x80);
    operand1 = (local_data & 0x80);
    result = (total & 0x80);

    if (operand0 == 0 && operand1 != 0 && result != 0) register_flags = register_flags | 0x40; // Set the V flag
    else if (operand0 != 0 && operand1 == 0 && result == 0) register_flags = register_flags | 0x40;
    else register_flags = register_flags & 0xBF; // Clear the V flag

    register_a = (0xFF & total);
    Calc_Flags_NEGATIVE_ZERO(register_a);

    return;
}
void opcode_0xE9() {
    Calculate_SBC(Fetch_Immediate());
    return;
} // 0xE9 - SBC - Immediate
void opcode_0xE5() {
    Calculate_SBC(Fetch_ZeroPage());
    return;
} // 0xE5 - SBC - ZeroPage
void opcode_0xF5() {
    Calculate_SBC(Fetch_ZeroPage_X());
    return;
} // 0xF5 - SBC - ZeroPage , X
void opcode_0xED() {
    Calculate_SBC(Fetch_Absolute());
    return;
} // 0xED - SBC - Absolute
void opcode_0xFD() {
    Calculate_SBC(Fetch_Absolute_X(1));
    return;
} // 0xFD - SBC - Absolute , X
void opcode_0xF9() {
    Calculate_SBC(Fetch_Absolute_Y(1));
    return;
} // 0xF9 - SBC - Absolute , Y
void opcode_0xE1() {
    Calculate_SBC(Fetch_Indexed_Indirect_X());
    return;
} // 0xE1 - SBC - Indexed Indirect X
void opcode_0xF1() {
    Calculate_SBC(Fetch_Indexed_Indirect_Y(1));
    return;
} // 0xF1 - SBC - Indirect Indexed  Y

// -------------------------------------------------
// Flag set/resets and NOP
// -------------------------------------------------
void opcode_0xEA() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0xEA - NOP   
void opcode_0x18() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xFE;
    return;
} // 0x18 - CLC - Clear Carry Flag  
void opcode_0xD8() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xF7;
    return;
} // 0xD8 - CLD - Clear Decimal Mode  
void opcode_0x58() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xFB;
    return;
} // 0x58 - CLI - Clear Interrupt Flag  
void opcode_0xB8() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags & 0xBF;
    return;
} // 0xB8 - CLV - Clear Overflow Flag  
void opcode_0x38() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags | 0x01;
    return;
} // 0x38 - SEC - Set Carry Flag  
void opcode_0x78() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags | 0x04;
    return;
} // 0x78 - SEI - Set Interrupt Flag  
void opcode_0xF8() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_flags = register_flags | 0x08;
    return;
} // 0xF8 - SED - Set Decimal Mode  

// -------------------------------------------------
// Increment/Decrements
// -------------------------------------------------
void opcode_0xCA() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_x - 1;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xCA - DEX - Decrement X  
void opcode_0x88() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_y = register_y - 1;
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0x88 - DEY - Decrement Y  
void opcode_0xE8() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_x + 1;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xE8 - INX - Increment X  
void opcode_0xC8() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_y = register_y + 1;
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xC8 - INY - Increment Y  

// -------------------------------------------------
// Transfers
// -------------------------------------------------
void opcode_0xAA() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_a;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xAA - TAX - Transfer Accumulator to X 
void opcode_0xA8() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_y = register_a;
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xA8 - TAY - Transfer Accumulator to Y
void opcode_0xBA() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_x = register_sp;
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xBA - TSX - Transfer Stack Pointer to X
void opcode_0x8A() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_a = register_x;
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x8A - TXA - Transfer X to Accumulator
void opcode_0x9A() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_sp = register_x;
    return;
} // 0x9A - TXS - Transfer X to Stack Pointer
void opcode_0x98() {
    read_byte(register_pc + 1);
    Begin_Fetch_Next_Opcode();
    register_a = register_y;
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x98 - TYA - Transfer Y to Accumulator

// -------------------------------------------------
// PUSH/POP Flags and Accumulator 
// -------------------------------------------------
void opcode_0x08() {
    read_byte(register_pc + 1);
    push(register_flags | 0x30);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x08 - PHP - Push Flags to Stack
void opcode_0x48() {
    read_byte(register_pc + 1);
    push(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x48 - PHA - Push Accumulator to the stack
void opcode_0x28() {
    read_byte(register_pc + 1);
    read_byte(register_sp_fixed);
    register_flags = (pop() | 0x30);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x28 - PLP - Pop Flags from Stack
void opcode_0x68() {
    read_byte(register_pc + 1);
    read_byte(register_sp_fixed);
    register_a = pop();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x68 - PLA - Pop Accumulator from Stack

// -------------------------------------------------
// AND
// -------------------------------------------------
void opcode_0x29() {
    register_a = register_a & (Fetch_Immediate());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x29 - AND - Immediate
void opcode_0x25() {
    register_a = register_a & (Fetch_ZeroPage());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x25 - AND - ZeroPage
void opcode_0x35() {
    register_a = register_a & (Fetch_ZeroPage_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x35 - AND - ZeroPage , X
void opcode_0x2D() {
    register_a = register_a & (Fetch_Absolute());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x2D - AND - Absolute
void opcode_0x3D() {
    register_a = register_a & (Fetch_Absolute_X(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x3D - AND - Absolute , X
void opcode_0x39() {
    register_a = register_a & (Fetch_Absolute_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x19 - OR - Absolute , Y
void opcode_0x21() {
    register_a = register_a & (Fetch_Indexed_Indirect_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x21 - AND - Indexed Indirect X
void opcode_0x31() {
    register_a = register_a & (Fetch_Indexed_Indirect_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x31 - AND - Indirect Indexed  Y

// -------------------------------------------------
// ORA
// -------------------------------------------------
void opcode_0x09() {
    register_a = register_a | (Fetch_Immediate());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x09 - OR - Immediate
void opcode_0x05() {
    register_a = register_a | (Fetch_ZeroPage());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x05 - OR - ZeroPage
void opcode_0x15() {
    register_a = register_a | (Fetch_ZeroPage_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x15 - OR - ZeroPage , X
void opcode_0x0D() {
    register_a = register_a | (Fetch_Absolute());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x0D - OR - Absolute
void opcode_0x1D() {
    register_a = register_a | (Fetch_Absolute_X(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x1D - OR - Absolute , X
void opcode_0x19() {
    register_a = register_a | (Fetch_Absolute_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x19 - OR - Absolute , Y
void opcode_0x01() {
    register_a = register_a | (Fetch_Indexed_Indirect_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x01 - OR - Indexed Indirect X
void opcode_0x11() {
    register_a = register_a | (Fetch_Indexed_Indirect_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x11 - OR - Indirect Indexed  Y

// -------------------------------------------------
// EOR
// -------------------------------------------------
void opcode_0x49() {
    register_a = register_a ^ (Fetch_Immediate());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x49 - EOR - Immediate
void opcode_0x45() {
    register_a = register_a ^ (Fetch_ZeroPage());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x45 - EOR - ZeroPage
void opcode_0x55() {
    register_a = register_a ^ (Fetch_ZeroPage_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x55 - EOR - ZeroPage , X
void opcode_0x4D() {
    register_a = register_a ^ (Fetch_Absolute());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x4D - EOR - Absolute
void opcode_0x5D() {
    register_a = register_a ^ (Fetch_Absolute_X(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x5D - EOR - Absolute , X
void opcode_0x59() {
    register_a = register_a ^ (Fetch_Absolute_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x59 - EOR - Absolute , Y
void opcode_0x41() {
    register_a = register_a ^ (Fetch_Indexed_Indirect_X());
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x41 - EOR - Indexed Indirect X
void opcode_0x51() {
    register_a = register_a ^ (Fetch_Indexed_Indirect_Y(1));
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0x51 - EOR - Indirect Indexed  Y

// -------------------------------------------------
// LDA
// -------------------------------------------------
void opcode_0xA9() {
    register_a = Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xA9 - LDA - Immediate
void opcode_0xA5() {
    register_a = Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xA5 - LDA - ZeroPage
void opcode_0xB5() {
    register_a = Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xB5 - LDA - ZeroPage , X
void opcode_0xAD() {
    register_a = Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xAD - LDA - Absolute
void opcode_0xBD() {
    register_a = Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xBD - LDA - Absolute , X
void opcode_0xB9() {
    register_a = Fetch_Absolute_Y(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xB9 - LDA - Absolute , Y
void opcode_0xA1() {
    register_a = Fetch_Indexed_Indirect_X();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xA1 - LDA - Indexed Indirect X
void opcode_0xB1() {
    register_a = Fetch_Indexed_Indirect_Y(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xB1 - LDA - Indirect Indexed  Y

// -------------------------------------------------
// LDX
// -------------------------------------------------
void opcode_0xA2() {
    register_x = Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xA2 - LDX - Immediate
void opcode_0xA6() {
    register_x = Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xA6 - LDX - ZeroPage
void opcode_0xB6() {
    register_x = Fetch_ZeroPage_Y();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xB6 - LDX - ZeroPage , Y
void opcode_0xAE() {
    register_x = Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xAE - LDX - Absolute
void opcode_0xBE() {
    register_x = Fetch_Absolute_Y(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_x);
    return;
} // 0xBE - LDX - Absolute , Y

// -------------------------------------------------          
// LDY                                                        
// -------------------------------------------------          
void opcode_0xA0() {
    register_y = Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xA0 - LDY - Immediate
void opcode_0xA4() {
    register_y = Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xA4 - LDY - ZeroPage
void opcode_0xB4() {
    register_y = Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xB4 - LDY - ZeroPage , X
void opcode_0xAC() {
    register_y = Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xAC - LDY - Absolute
void opcode_0xBC() {
    register_y = Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_y);
    return;
} // 0xBC - LDY - Absolute , X

// -------------------------------------------------
// BIT 
// -------------------------------------------------
void Calculate_BIT(uint8_t local_data) {
    uint8_t temp = 0;

    Begin_Fetch_Next_Opcode();

    register_flags = (register_flags & 0x3F) | (local_data & 0xC0); // Copy fetched memory[7:6] to C,V flags

    temp = local_data & register_a;
    if (temp == 0) register_flags = register_flags | 0x02; // Set the Z flag
    else register_flags = register_flags & 0xFD; // Clear the Z flag 

    return;
}
void opcode_0x24() {
    Calculate_BIT(Fetch_ZeroPage());
    return;
} // 0x24 - BIT - ZeroPage
void opcode_0x2C() {
    Calculate_BIT(Fetch_Absolute());
    return;
} // 0x2C - BIT - Absolute

// -------------------------------------------------
// CMP 
// -------------------------------------------------
void Calculate_CMP(uint8_t local_data) {
    int16_t temp = 0;

    Begin_Fetch_Next_Opcode();

    temp = register_a - local_data;

    if (register_a >= local_data) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag  

    Calc_Flags_NEGATIVE_ZERO(temp);
    return;
}
void opcode_0xC9() {
    Calculate_CMP(Fetch_Immediate());
    return;
} // 0xC9 - CMP - Immediate
void opcode_0xC5() {
    Calculate_CMP(Fetch_ZeroPage());
    return;
} // 0xC5 - CMP - ZeroPage
void opcode_0xD5() {
    Calculate_CMP(Fetch_ZeroPage_X());
    return;
} // 0xD5 - CMP - ZeroPage , X
void opcode_0xCD() {
    Calculate_CMP(Fetch_Absolute());
    return;
} // 0xCD - CMP - Absolute
void opcode_0xDD() {
    Calculate_CMP(Fetch_Absolute_X(1));
    return;
} // 0xDD - CMP - Absolute , X
void opcode_0xD9() {
    Calculate_CMP(Fetch_Absolute_Y(1));
    return;
} // 0xD9 - CMP - Absolute , Y
void opcode_0xC1() {
    Calculate_CMP(Fetch_Indexed_Indirect_X());
    return;
} // 0xC1 - CMP - Indexed Indirect X
void opcode_0xD1() {
    Calculate_CMP(Fetch_Indexed_Indirect_Y(1));
    return;
} // 0xD1 - CMP - Indirect Indexed  Y

// -------------------------------------------------
// CPX 
// -------------------------------------------------
void Calculate_CPX(uint8_t local_data) {
    uint16_t temp = 0;

    Begin_Fetch_Next_Opcode();

    temp = register_x - local_data;

    if (register_x >= local_data) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag  

    Calc_Flags_NEGATIVE_ZERO(temp);
    return;
}
void opcode_0xE0() {
    Calculate_CPX(Fetch_Immediate());
    return;
} // 0xE0 - CPX - Immediate
void opcode_0xE4() {
    Calculate_CPX(Fetch_ZeroPage());
    return;
} // 0xE4 - CPX - ZeroPage
void opcode_0xEC() {
    Calculate_CPX(Fetch_Absolute());
    return;
} // 0xEC - CPX - Absolute

// -------------------------------------------------
// CPY
// -------------------------------------------------
void Calculate_CPY(uint8_t local_data) {
    uint16_t temp = 0;

    Begin_Fetch_Next_Opcode();

    temp = register_y - local_data;

    if (register_y >= local_data) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag  

    Calc_Flags_NEGATIVE_ZERO(temp);
    return;
}
void opcode_0xC0() {
    Calculate_CPY(Fetch_Immediate());
    return;
} // 0xC0 - CPY - Immediate
void opcode_0xC4() {
    Calculate_CPY(Fetch_ZeroPage());
    return;
} // 0xC4 - CPY - ZeroPage
void opcode_0xCC() {
    Calculate_CPY(Fetch_Absolute());
    return;
} // 0xCC - CPY - Absolute

// -------------------------------------------------
// Store Operations
// -------------------------------------------------
void opcode_0x85() {
    Write_ZeroPage(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x85 - STA - ZeroPage
void opcode_0x8D() {
    Write_Absolute(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x8D - STA - Absolute
void opcode_0x95() {
    Write_ZeroPage_X(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x95 - STA - ZeroPage , X
void opcode_0x9D() {
    Write_Absolute_X(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x9D - STA - Absolute , X
void opcode_0x99() {
    Write_Absolute_Y(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x99 - STA - Absolute , Y
void opcode_0x81() {
    Write_Indexed_Indirect_X(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x81 - STA - Indexed Indirect X
void opcode_0x91() {
    Write_Indexed_Indirect_Y(register_a);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x91 - STA - Indirect Indexed  Y
void opcode_0x86() {
    Write_ZeroPage(register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x86 - STX - ZeroPage
void opcode_0x96() {
    Write_ZeroPage_Y(register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x96 - STX - ZeroPage , Y
void opcode_0x8E() {
    Write_Absolute(register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x8E - STX - Absolute
void opcode_0x84() {
    Write_ZeroPage(register_y);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x84 - STY - ZeroPage
void opcode_0x94() {
    Write_ZeroPage_X(register_y);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x94 - STY - ZeroPage , X
void opcode_0x8C() {
    Write_Absolute(register_y);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x8C - STY - Absolute

// -------------------------------------------------
// ASL - Arithmetic Shift Left - Memory
// -------------------------------------------------
uint8_t Calculate_ASL(uint8_t local_data) {

    if ((0x80 & local_data) == 0x80) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = ((local_data << 1) & 0xFE);

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}

// -------------------------------------------------
// ASL - Read-modify-write Operations
// -------------------------------------------------
void opcode_0x06() {
    Double_WriteBack(Calculate_ASL(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x06 - ASL  - Arithmetic Shift Left - ZeroPage
void opcode_0x16() {
    Double_WriteBack(Calculate_ASL(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x16 - ASL  - Arithmetic Shift Left - ZeroPage , X
void opcode_0x0E() {
    Double_WriteBack(Calculate_ASL(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x0E - ASL  - Arithmetic Shift Left - Absolute
void opcode_0x1E() {
    Double_WriteBack(Calculate_ASL(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x1E - ASL  - Arithmetic Shift Left - Absolute , X

// -------------------------------------------------
// INC - Memory
// -------------------------------------------------
uint8_t Calculate_INC(uint8_t local_data) {

    local_data = local_data + 1;
    global_temp = local_data;
    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}

void opcode_0xE6() {
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xE6 - INC - ZeroPage
void opcode_0xF6() {
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xF6 - INC - ZeroPage , X
void opcode_0xEE() {
    Double_WriteBack(Calculate_INC(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xEE - INC - Absolute
void opcode_0xFE() {
    Double_WriteBack(Calculate_INC(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xFE - INC - Absolute , X

// -------------------------------------------------
// DEC - Memory
// -------------------------------------------------
uint8_t Calculate_DEC(uint8_t local_data) {

    local_data = local_data - 1;
    global_temp = local_data;
    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}

void opcode_0xC6() {
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xC6 - DEC - ZeroPage
void opcode_0xD6() {
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xD6 - DEC - ZeroPage , X
void opcode_0xCE() {
    Double_WriteBack(Calculate_DEC(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xCE - DEC - Absolute
void opcode_0xDE() {
    Double_WriteBack(Calculate_DEC(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0xDE - DEC - Absolute , X

// -------------------------------------------------
// LSR - Memory
// -------------------------------------------------
uint8_t Calculate_LSR(uint8_t local_data) {

    if ((0x01 & local_data) == 0x01) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (0x7F & (local_data >> 1));

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}
void opcode_0x46() {
    Double_WriteBack(Calculate_LSR(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x46 - LSR - Logical Shift Right - ZeroPage
void opcode_0x56() {
    Double_WriteBack(Calculate_LSR(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x56 - LSR - Logical Shift Right - ZeroPage , X
void opcode_0x4E() {
    Double_WriteBack(Calculate_LSR(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x4E - LSR - Logical Shift Right - Absolute
void opcode_0x5E() {
    Double_WriteBack(Calculate_LSR(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x5E - LSR - Logical Shift Right - Absolute , X

// -------------------------------------------------
// ROR - Memory
// -------------------------------------------------
uint8_t Calculate_ROR(uint8_t local_data) {

    uint8_t old_carry_flag = 0;

    old_carry_flag = register_flags << 7; // Shift the old carry flag to bit[8] to be rotated in

    if ((0x01 & local_data) == 0x01) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (old_carry_flag | (local_data >> 1));

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}
void opcode_0x66() {
    Double_WriteBack(Calculate_ROR(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x66 - ROR - Rotate Right - ZeroPage
void opcode_0x76() {
    Double_WriteBack(Calculate_ROR(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x76 - ROR - Rotate Right - ZeroPage , X
void opcode_0x6E() {
    Double_WriteBack(Calculate_ROR(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x6E - ROR - Rotate Right - Absolute
void opcode_0x7E() {
    Double_WriteBack(Calculate_ROR(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x7E - ROR - Rotate Right - Absolute , X

// -------------------------------------------------
// ROL - Memory
// -------------------------------------------------
uint8_t Calculate_ROL(uint8_t local_data) {

    uint8_t old_carry_flag = 0;

    old_carry_flag = 0x1 & register_flags; // Store the old carry flag to be rotated in

    if (0x80 & local_data) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (local_data << 1) | old_carry_flag;

    Calc_Flags_NEGATIVE_ZERO(local_data);
    return local_data;
}
void opcode_0x26() {
    Double_WriteBack(Calculate_ROL(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x26 - ROL - Rotate Left - ZeroPage
void opcode_0x36() {
    Double_WriteBack(Calculate_ROL(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x36 - ROL - Rotate Left - ZeroPage , X
void opcode_0x2E() {
    Double_WriteBack(Calculate_ROL(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x2E - ROL - Rotate Left - Absolute
void opcode_0x3E() {
    Double_WriteBack(Calculate_ROL(Fetch_Absolute_X(0)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x3E - ROL - Rotate Left - Absolute , X

// -------------------------------------------------
// Branches
// -------------------------------------------------
void Branch_Taken() {

    effective_address = Sign_Extend16(Fetch_Immediate());
    effective_address = (register_pc + 1) + effective_address;

    if ((0xFF00 & register_pc) == (0xFF00 & effective_address)) {
        Fetch_Immediate();
    } // Page boundary not crossed
    else {
        Fetch_Immediate();
        Fetch_Immediate();
    } // Page boundary crossed

    register_pc = effective_address;
    assert_sync = 1;
    start_read(register_pc);
    return;
}
void opcode_0xB0() {
    if ((flag_c) == 1) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0xB0 - BCS - Branch on Carry Set
void opcode_0x90() {
    if ((flag_c) == 0) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0x90 - BCC - Branch on Carry Clear
void opcode_0xF0() {
    if ((flag_z) == 1) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0xF0 - BEQ - Branch on Zero Set
void opcode_0xD0() {
    if ((flag_z) == 0) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0xD0 - BNE - Branch on Zero Clear
void opcode_0x70() {
    if ((flag_v) == 1) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0x70 - BVS - Branch on Overflow Set
void opcode_0x50() {
    if ((flag_v) == 0) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0x50 - BVC - Branch on Overflow Clear
void opcode_0x30() {
    if ((flag_n) == 1) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0x30 - BMI - Branch on Minus (N Flag Set)
void opcode_0x10() {
    if ((flag_n) == 0) Branch_Taken();
    else {
        Fetch_Immediate();
        Begin_Fetch_Next_Opcode();
    }
    return;
} // 0x10 - BPL - Branch on Plus  (N Flag Clear)

// -------------------------------------------------
// Jumps and Returns
// -------------------------------------------------
void opcode_0x4C() {
    register_pc = Calculate_Absolute();
    assert_sync = 1;
    start_read(register_pc);
    return;
} // 0x4C - JMP - Jump Absolute

// -------------------------------------------------
// 0x6C - JMP - Jump Indirect
// -------------------------------------------------
void opcode_0x6C() {
    uint16_t lal, lah;
    uint16_t adl, adh;

    lal = Fetch_Immediate();
    lah = Fetch_Immediate() << 8;
    adl = read_byte(lah + lal);
    adh = read_byte(lah + lal + 1) << 8;
    effective_address = adh + adl;
    register_pc = (0xFF00 & adh) + (0x00FF & effective_address); // 6502 page wrapping bug 
    assert_sync = 1;
    start_read(register_pc);
    return;
}

// -------------------------------------------------
// 0x20 - JSR - Jump to Subroutine
// -------------------------------------------------
void opcode_0x20() {
    uint16_t adl, adh;

    adl = Fetch_Immediate();
    adh = Fetch_Immediate() << 8;
    read_byte(register_sp_fixed);
    push((0xFF00 & register_pc) >> 8);

    push(0x00FF & register_pc);
    register_pc = adh + adl;
    assert_sync = 1;
    start_read(register_pc);
    return;
}

// -------------------------------------------------
// 0x40 - RTI - Return from Interrupt
// -------------------------------------------------
void opcode_0x40() {
    uint16_t pcl, pch;

    Fetch_Immediate();
    read_byte(register_sp_fixed);
    register_flags = pop();
    pcl = pop();
    pch = pop() << 8;
    register_pc = pch + pcl;
    assert_sync = 1;
    start_read(register_pc);
    return;
}

// -------------------------------------------------
// 0x60 - RTS - Return from Subroutine
// -------------------------------------------------
void opcode_0x60() {
    uint16_t pcl, pch;

    Fetch_Immediate();
    read_byte(register_sp_fixed);
    pcl = pop();
    pch = pop() << 8;
    register_pc = pch + pcl + 1;
    read_byte(register_pc);
    assert_sync = 1;
    start_read(register_pc);
    return;
}

// -------------------------------------------------
//
// *** Undocumented 6502 Opcodes ***
//
// -------------------------------------------------

// --------------------------------------------------------------------------------------------------
// SLO - Shift left one bit in memory, then OR accumulator with memory.
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_SLO(uint8_t local_data) {

    if ((0x80 & local_data) == 0x80) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = ((local_data << 1) & 0xFE);

    register_a = register_a | local_data;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return local_data;
}
void opcode_0x07() {
    Double_WriteBack(Calculate_SLO(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x07 - SLO - ZeroPage
void opcode_0x17() {
    Double_WriteBack(Calculate_SLO(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x17 - SLO - ZeroPage , X
void opcode_0x03() {
    Double_WriteBack(Calculate_SLO(Fetch_Indexed_Indirect_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x03 - SLO - Indexed Indirect X
void opcode_0x13() {
    Double_WriteBack(Calculate_SLO(Fetch_Indexed_Indirect_Y(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x13 - SLO - Indirect Indexed  Y
void opcode_0x0F() {
    Double_WriteBack(Calculate_SLO(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x0F - SLO - Absolute
void opcode_0x1F() {
    Double_WriteBack(Calculate_SLO(Fetch_Absolute_X(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x1F - SLO - Absolute , X
void opcode_0x1B() {
    Double_WriteBack(Calculate_SLO(Fetch_Absolute_Y(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x1B - SLO - Absolute , Y

// --------------------------------------------------------------------------------------------------
// RLA - Rotate one bit left in memory, then AND accumulator with memory.
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_RLA(uint8_t local_data) {
    uint8_t old_carry_flag = 0;

    old_carry_flag = 0x1 & register_flags; // Store the old carry flag to be rotated in

    if (0x80 & local_data) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (local_data << 1) | old_carry_flag;

    register_a = register_a & local_data;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return local_data;
}
void opcode_0x27() {
    Double_WriteBack(Calculate_RLA(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x27 - RLA - ZeroPage
void opcode_0x37() {
    Double_WriteBack(Calculate_RLA(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x37 - RLA - ZeroPage , X
void opcode_0x23() {
    Double_WriteBack(Calculate_RLA(Fetch_Indexed_Indirect_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x23 - RLA - Indexed Indirect X
void opcode_0x33() {
    Double_WriteBack(Calculate_RLA(Fetch_Indexed_Indirect_Y(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x33 - RLA - Indirect Indexed  Y
void opcode_0x2F() {
    Double_WriteBack(Calculate_RLA(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x2F - RLA - Absolute
void opcode_0x3F() {
    Double_WriteBack(Calculate_RLA(Fetch_Absolute_X(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x3F - RLA - Absolute , X
void opcode_0x3B() {
    Double_WriteBack(Calculate_RLA(Fetch_Absolute_Y(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x3B - RLA - Absolute , Y

// --------------------------------------------------------------------------------------------------
// SRE - Shift right one bit in memory, then EOR accumulator with memory.
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_SRE(uint8_t local_data) {

    if ((0x01 & local_data) == 0x01) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (0x7F & (local_data >> 1));

    register_a = register_a ^ local_data;

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return local_data;
}
void opcode_0x47() {
    Double_WriteBack(Calculate_SRE(Fetch_ZeroPage()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x47 - SRE - ZeroPage
void opcode_0x57() {
    Double_WriteBack(Calculate_SRE(Fetch_ZeroPage_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x57 - SRE - ZeroPage , X
void opcode_0x43() {
    Double_WriteBack(Calculate_SRE(Fetch_Indexed_Indirect_X()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x43 - SRE - Indexed Indirect X
void opcode_0x53() {
    Double_WriteBack(Calculate_SRE(Fetch_Indexed_Indirect_Y(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x53 - SRE - Indirect Indexed  Y
void opcode_0x4F() {
    Double_WriteBack(Calculate_SRE(Fetch_Absolute()));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x4F - SRE - Absolute
void opcode_0x5F() {
    Double_WriteBack(Calculate_SRE(Fetch_Absolute_X(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x5F - SRE - Absolute , X
void opcode_0x5B() {
    Double_WriteBack(Calculate_SRE(Fetch_Absolute_Y(1)));
    Begin_Fetch_Next_Opcode();
    return;
} // 0x5B - SRE - Absolute , Y

// --------------------------------------------------------------------------------------------------
// RRA - Rotate one bit right in memory, then add memory to accumulator (with carry).
// --------------------------------------------------------------------------------------------------
uint8_t Calculate_RRA(uint8_t local_data) {

    if ((0x01 & local_data) == 0x01) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    local_data = (0x7F & (local_data >> 1));

    global_temp = local_data;

    return local_data;
}
void opcode_0x67() {
    Double_WriteBack(Calculate_RRA(Fetch_ZeroPage()));
    Calculate_ADC(global_temp);
    return;
} // 0x67 - RRA - ZeroPage
void opcode_0x77() {
    Double_WriteBack(Calculate_RRA(Fetch_ZeroPage_X()));
    Calculate_ADC(global_temp);
    return;
} // 0x77 - RRA - ZeroPage , X
void opcode_0x63() {
    Double_WriteBack(Calculate_RRA(Fetch_Indexed_Indirect_X()));
    Calculate_ADC(global_temp);
    return;
} // 0x63 - RRA - Indexed Indirect X
void opcode_0x73() {
    Double_WriteBack(Calculate_RRA(Fetch_Indexed_Indirect_Y(1)));
    Calculate_ADC(global_temp);
    return;
} // 0x73 - RRA - Indirect Indexed  Y
void opcode_0x6F() {
    Double_WriteBack(Calculate_RRA(Fetch_Absolute()));
    Calculate_ADC(global_temp);
    return;
} // 0x6F - RRA - Absolute
void opcode_0x7F() {
    Double_WriteBack(Calculate_RRA(Fetch_Absolute_X(1)));
    Calculate_ADC(global_temp);
    return;
} // 0x7F - RRA - Absolute , X
void opcode_0x7B() {
    Double_WriteBack(Calculate_RRA(Fetch_Absolute_Y(1)));
    Calculate_ADC(global_temp);
    return;
} // 0x7B - RRA - Absolute , Y

// --------------------------------------------------------------------------------------------------
// AND the contents of the A and X registers (without changing the contents of either register) and 
// stores the result in memory.
// --------------------------------------------------------------------------------------------------
void opcode_0x87() {
    Write_ZeroPage(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x87 - SAX - ZeroPage
void opcode_0x97() {
    Write_ZeroPage_Y(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x97 - SAX - ZeroPage , Y
void opcode_0x83() {
    Write_Indexed_Indirect_X(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x83 - SAX - Indexed Indirect X
void opcode_0x8F() {
    Write_Absolute(register_a & register_x);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x8F - SAX - Absolute

// --------------------------------------------------------------------------------------------------
// Load both the accumulator and the X register with the contents of a memory location.
// --------------------------------------------------------------------------------------------------
void opcode_0xA7() {
    register_a = Fetch_ZeroPage();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xA7 - LAX - ZeroPage
void opcode_0xB7() {
    register_a = Fetch_ZeroPage_Y();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xB7 - LAX - ZeroPage , Y
void opcode_0xA3() {
    register_a = Fetch_Indexed_Indirect_X();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xA3 - LAX - Indexed Indirect X
void opcode_0xB3() {
    register_a = Fetch_Indexed_Indirect_Y(1);
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xB3 - LAX - Indirect Indexed  Y
void opcode_0xAF() {
    register_a = Fetch_Absolute();
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xAF - LAX - Absolute
void opcode_0xBF() {
    register_a = Fetch_Absolute_Y(1);
    register_x = register_a;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xBF - LAX - Absolute , Y

// --------------------------------------------------------------------------------------------------
// Decrement the contents of a memory location and then compare the result with the A register.
// --------------------------------------------------------------------------------------------------
void opcode_0xC7() {
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage()));
    Calculate_CMP(global_temp);
    return;
} // 0xC7 - DCP - ZeroPage
void opcode_0xD7() {
    Double_WriteBack(Calculate_DEC(Fetch_ZeroPage_X()));
    Calculate_CMP(global_temp);
    return;
} // 0xD7 - DCP - ZeroPage , X
void opcode_0xC3() {
    Double_WriteBack(Calculate_DEC(Fetch_Indexed_Indirect_X()));
    Calculate_CMP(global_temp);
    return;
} // 0xC3 - DCP - Indexed Indirect X
void opcode_0xD3() {
    Double_WriteBack(Calculate_DEC(Fetch_Indexed_Indirect_Y(0)));
    Calculate_CMP(global_temp);
    return;
} // 0xD3 - DCP - Indirect Indexed  Y
void opcode_0xCF() {
    Double_WriteBack(Calculate_DEC(Fetch_Absolute()));
    Calculate_CMP(global_temp);
    return;
} // 0xCF - DCP - Absolute
void opcode_0xDF() {
    Double_WriteBack(Calculate_DEC(Fetch_Absolute_X(0)));
    Calculate_CMP(global_temp);
    return;
} // 0xDF - DCP - Absolute , X
void opcode_0xDB() {
    Double_WriteBack(Calculate_DEC(Fetch_Absolute_Y(0)));
    Calculate_CMP(global_temp);
    return;
} // 0xDB - DCP - Absolute , Y

// --------------------------------------------------------------------------------------------------
// ISC - Increase memory by one, then subtract memory from accumulator (with borrow).
// --------------------------------------------------------------------------------------------------
void opcode_0xE7() {
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage()));
    Calculate_SBC(global_temp);
    return;
} // 0xE7 - ISC - ZeroPage
void opcode_0xF7() {
    Double_WriteBack(Calculate_INC(Fetch_ZeroPage_X()));
    Calculate_SBC(global_temp);
    return;
} // 0xF7 - ISC - ZeroPage , X
void opcode_0xE3() {
    Double_WriteBack(Calculate_INC(Fetch_Indexed_Indirect_X()));
    Calculate_SBC(global_temp);
    return;
} // 0xE3 - ISC - Indexed Indirect X
void opcode_0xF3() {
    Double_WriteBack(Calculate_INC(Fetch_Indexed_Indirect_Y(0)));
    Calculate_SBC(global_temp);
    return;
} // 0xF3 - ISC - Indirect Indexed  Y
void opcode_0xEF() {
    Double_WriteBack(Calculate_INC(Fetch_Absolute()));
    Calculate_SBC(global_temp);
    return;
} // 0xEF - ISC - Absolute
void opcode_0xFF() {
    Double_WriteBack(Calculate_INC(Fetch_Absolute_X(0)));
    Calculate_SBC(global_temp);
    return;
} // 0xFF - ISC - Absolute , X
void opcode_0xFB() {
    Double_WriteBack(Calculate_INC(Fetch_Absolute_Y(0)));
    Calculate_SBC(global_temp);
    return;
} // 0xFB - ISC - Absolute , Y

// --------------------------------------------------------------------------------------------------
// ANC - ANDs the contents of the A register with an immediate value and then moves bit 7 of A
// into the Carry flag.
// --------------------------------------------------------------------------------------------------
void Calculate_ANC(uint8_t local_data) {

    Begin_Fetch_Next_Opcode();

    register_a = register_a & local_data;

    if ((0x80 & register_a) == 0x80) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}
void opcode_0x0B() {
    Calculate_ANC(Fetch_Immediate());
    return;
} // 0x0B - ANC - Immediate
void opcode_0x2B() {
    Calculate_ANC(Fetch_Immediate());
    return;
} // 0x2B - ANC - Immediate

// --------------------------------------------------------------------------------------------------
// ALR - AND the contents of the A register with an immediate value and then LSRs the result.
// --------------------------------------------------------------------------------------------------
void Calculate_ALR(uint8_t local_data) {

    Begin_Fetch_Next_Opcode();

    register_a = register_a & local_data;

    if ((0x01 & register_a) == 0x01) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag

    register_a = (0x7F & (register_a >> 1));

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}
void opcode_0x4B() {
    Calculate_ALR(Fetch_Immediate());
    return;
} // 0x4B - ALR - Immediate

// --------------------------------------------------------------------------------------------------
// ARR - ANDs the accumulator with an immediate value and then rotates the content right.
// --------------------------------------------------------------------------------------------------
void Calculate_ARR(uint8_t local_data) {

    Begin_Fetch_Next_Opcode();

    register_a = register_a & local_data;

    register_a = (0x7F & (register_a >> 1));

    register_flags = register_flags & 0xBE; // Pre-clear the C and V flags   
    if ((0xC0 & register_a) == 0x40) {
        register_flags = register_flags | 0x40;
    } // Set the V flag 
    if ((0xC0 & register_a) == 0x80) {
        register_flags = register_flags | 0x41;
    } // Set the C and V flags 
    if ((0xC0 & register_a) == 0xC0) {
        register_flags = register_flags | 0x01;
    } // Set the C flag 

    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
}
void opcode_0x6B() {
    Calculate_ARR(Fetch_Immediate());
    return;
} // 0x6B - ARR - Immediate

// --------------------------------------------------------------------------------------------------
// SBX - ANDs the contents of the A and X registers (leaving the contents of A intact),
// subtracts an immediate value, and then stores the result in X.
// --------------------------------------------------------------------------------------------------
void Calculate_SBX(uint16_t local_data) {
    int16_t signed_total = 0;

    Begin_Fetch_Next_Opcode();

    register_x = register_a & register_x;

    register_x = register_x - local_data;
    signed_total = (int16_t) register_x - (int16_t)(local_data);

    if (signed_total >= 0) register_flags = register_flags | 0x01; // Set the C flag
    else register_flags = register_flags & 0xFE; // Clear the C flag  

    register_x = (0xFF & register_x);
    Calc_Flags_NEGATIVE_ZERO(register_x);

    return;
}
void opcode_0xCB() {
    Calculate_SBX(Fetch_Immediate());
    return;
} // 0xCB - SBX - Immediate

// --------------------------------------------------------------------------------------------------
// LAS - AND memory with stack pointer, transfer result to accumulator, X register and stack pointer.
// --------------------------------------------------------------------------------------------------
void opcode_0xBB() {
    register_sp = (register_sp & Fetch_Absolute_Y(1));
    register_a = register_sp;
    register_x = register_sp;
    Begin_Fetch_Next_Opcode();
    Calc_Flags_NEGATIVE_ZERO(register_a);
    return;
} // 0xBB - LAS - Absolute , Y

// --------------------------------------------------------------------------------------------------
// NOP - Fetch Immediate
// --------------------------------------------------------------------------------------------------
void opcode_0x80() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x80 - NOP - Immediate
void opcode_0x82() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x82 - NOP - Immediate
void opcode_0xC2() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0xC2 - NOP - Immediate
void opcode_0xE2() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0xE2 - NOP - Immediate
void opcode_0x89() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x89 - NOP - Immediate

// --------------------------------------------------------------------------------------------------
// NOP - Fetch ZeroPage
// --------------------------------------------------------------------------------------------------
void opcode_0x04() {
    Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x04 - NOP - ZeroPage
void opcode_0x44() {
    Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x44 - NOP - ZeroPage
void opcode_0x64() {
    Fetch_ZeroPage();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x64 - NOP - ZeroPage

// --------------------------------------------------------------------------------------------------
// NOP - Fetch ZeroPage , X
// --------------------------------------------------------------------------------------------------
void opcode_0x14() {
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x14 - NOP - ZeroPage , X
void opcode_0x34() {
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x34 - NOP - ZeroPage , X
void opcode_0x54() {
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x54 - NOP - ZeroPage , X
void opcode_0x74() {
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x74 - NOP - ZeroPage , X
void opcode_0xD4() {
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return;
} // 0xD4 - NOP - ZeroPage , X
void opcode_0xF4() {
    Fetch_ZeroPage_X();
    Begin_Fetch_Next_Opcode();
    return;
} // 0xF4 - NOP - ZeroPage , X

// --------------------------------------------------------------------------------------------------
// NOP - Fetch Absolute
// --------------------------------------------------------------------------------------------------
void opcode_0x0C() {
    Fetch_Absolute();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x0C - NOP - Absolute

// --------------------------------------------------------------------------------------------------
// NOP - Fetch Absolute , X
// --------------------------------------------------------------------------------------------------
void opcode_0x1C() {
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x1C - NOP - Absolute , X
void opcode_0x3C() {
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x3C - NOP - Absolute , X
void opcode_0x5C() {
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x5C - NOP - Absolute , X
void opcode_0x7C() {
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x7C - NOP - Absolute , X
void opcode_0xDC() {
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0xDC - NOP - Absolute , X
void opcode_0xFC() {
    Fetch_Absolute_X(1);
    Begin_Fetch_Next_Opcode();
    return;
} // 0xFC - NOP - Absolute , X

// --------------------------------------------------------------------------------------------------
// JAM - Lock up the processor
// --------------------------------------------------------------------------------------------------
void opcode_0x02() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x02 - JAM
void opcode_0x12() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x12 - JAM
void opcode_0x22() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x22 - JAM
void opcode_0x32() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x32 - JAM
void opcode_0x42() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x42 - JAM
void opcode_0x52() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x52 - JAM
void opcode_0x62() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x62 - JAM
void opcode_0x72() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x72 - JAM
void opcode_0x92() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0x92 - JAM
void opcode_0xB2() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0xB2 - JAM
void opcode_0xD2() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0xD2 - JAM
void opcode_0xF2() {
    Fetch_Immediate();
    while (1) {}
    return;
} // 0xF2 - JAM

// --------------------------------------------------------------------------------------------------
// Unstable 6502 opcodes
// --------------------------------------------------------------------------------------------------
void opcode_0x93() {
    Fetch_ZeroPage_Y();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x93 - SHA - ZeroPage , Y - Implelented here as a size 2 NOP
void opcode_0x9F() {
    Fetch_Absolute_Y(0);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x9F - SHA - Absolute , Y - Implelented here as a size 3 NOP
void opcode_0x9E() {
    Fetch_Absolute_Y(0);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x9E - SHX - Absolute , Y - Implelented here as a size 3 NOP
void opcode_0x9C() {
    Fetch_Absolute_X(0);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x9C - SHY - Absolute , X - Implelented here as a size 3 NOP
void opcode_0x9B() {
    Fetch_Absolute_Y(0);
    Begin_Fetch_Next_Opcode();
    return;
} // 0x9B - TAS - Absolute , Y - Implelented here as a size 3 NOP
void opcode_0x8B() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0x8B - ANE - Immediate    - Implelented here as a size 2 NOP
void opcode_0xAB() {
    Fetch_Immediate();
    Begin_Fetch_Next_Opcode();
    return;
} // 0xAB - LAX - Immediate    - Implelented here as a size 2 NOP
