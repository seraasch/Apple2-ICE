#line 1 "C:\\Users\\sraas\\Repositories\\Apple2-ICE\\opcode_decoder.h"


// OPCODE,MNEMONIC,operands ==FLAGS,CYCLES,LENGTH

struct OpDecoder {
	String opcode;
	String operands;
	String flags;
	uint8_t max_cycles;
	uint8_t length;
};


OpDecoder opcode_info[256];

String decode_opcode(uint8_t op, uint8_t op1, uint8_t op2) {
	OpDecoder *instr = &opcode_info[op];

	String s;
	
	if (instr->length == 2) {
		String operand = String(op1, HEX);
		if (instr->operands == "#") {
			s = "#$" + operand;
		}
		if ((instr->operands == "zpg")
			|| (instr->operands == "rel")) {
			s = "$" + operand;
		}
		if (instr->operands == "(ind,X)") {
			s = "($" + operand + ",X)"; 
		}
		if (instr->operands == "(ind),Y)") {
			s = "($" + operand + "),Y"; 
		}
		if (instr->operands == "zpg,X") {
			s = "$" + operand + ",X"; 
		}
		if (s.length() == 0) {
			s = "<unknown_format: " + instr->operands + ">";
		}
	}
	
	if (instr->length == 3) {
		String operand1 = String(op1, HEX);
		String operand2 = String(op2, HEX);
	
		if (instr->operands == "abs") {
			s = "$" + operand2 + operand1; 
		}
		if (instr->operands == "abs,X") {
			s = "$" + operand2 + operand1 + ",X"; 
		}
		if (instr->operands == "abs,Y") {
			s = "$" + operand2 + operand1 + ",Y"; 
		}
		if (instr->operands == "(ind)") {
			s = "($" + operand2 + operand1 + ")"; 
		}
		if (s.length() == 0) {
			s = "<unknown_format: " + instr->operands + ">";
		}
	}
	
	return(instr->opcode + " " + s);
}

void initialize_opcode_info() {

	for (int i=0; i<=255; i++)
		opcode_info[i] = {"---","",0,1};
		
	opcode_info[0x00] = {"BRK","","B",7,1};
	opcode_info[0x01] = {"ORA","(ind,X)","SZ",6,2};
	opcode_info[0x05] = {"ORA","zpg","SZ",3,2};
	opcode_info[0x06] = {"ASL","zpg","SZC",5,2};
	opcode_info[0x08] = {"PHP","","",3,1};
	opcode_info[0x09] = {"ORA","#","SZ",2,2};
	opcode_info[0x0a] = {"ASL","A","SZC",2,1};
	opcode_info[0x0d] = {"ORA","abs","SZ",4,3};
	opcode_info[0x0e] = {"ASL","abs","SZC",6,3};
	opcode_info[0x10] = {"BPL","rel","",2,2};
	opcode_info[0x11] = {"ORA","(ind),Y","SZ",5,2};
	opcode_info[0x15] = {"ORA","zpg,X","SZ",4,2};
	opcode_info[0x16] = {"ASL","zpg,X","SZC",6,2};
	opcode_info[0x18] = {"CLC","","C",2,1};
	opcode_info[0x19] = {"ORA","abs,Y","SZ",4,3};
	opcode_info[0x1d] = {"ORA","abs,X","SZ",4,3};
	opcode_info[0x1e] = {"ASL","abs,X","SZC",7,3};
	opcode_info[0x20] = {"JSR","abs","",6,3};
	opcode_info[0x21] = {"AND","(ind,X)","SZ",6,2};
	opcode_info[0x24] = {"BIT","zpg","NVZ",3,2};
	opcode_info[0x25] = {"AND","zpg","SZ",3,2};
	opcode_info[0x26] = {"ROL","zpg","SZC",5,2};
	opcode_info[0x28] = {"PLP","","",4,1};
	opcode_info[0x29] = {"AND","#","SZ",2,2};
	opcode_info[0x2a] = {"ROL","A","SZC",2,1};
	opcode_info[0x2c] = {"BIT","abs","NVZ",4,3};
	opcode_info[0x2d] = {"AND","abs","SZ",4,3};
	opcode_info[0x2e] = {"ROL","abs","SZC",6,3};
	opcode_info[0x30] = {"BMI","rel","",2,2};
	opcode_info[0x31] = {"AND","(ind),Y","SZ",5,2};
	opcode_info[0x35] = {"AND","zpg,X","SZ",4,2};
	opcode_info[0x36] = {"ROL","zpg,X","SZC",6,2};
	opcode_info[0x38] = {"SEC","","C",2,1};
	opcode_info[0x39] = {"AND","abs,Y","SZ",4,3};
	opcode_info[0x3d] = {"AND","abs,X","SZ",4,3};
	opcode_info[0x3e] = {"ROL","abs,X","SZC",7,3};
	opcode_info[0x40] = {"RTI","","SZCDVIB",6,1};
	opcode_info[0x41] = {"EOR","(ind,X)","SZ",6,2};
	opcode_info[0x45] = {"EOR","zpg","SZ",3,2};
	opcode_info[0x46] = {"LSR","zpg","SZC",5,2};
	opcode_info[0x48] = {"PHA","","",3,1};
	opcode_info[0x49] = {"EOR","#","SZ",2,2};
	opcode_info[0x4a] = {"LSR","A","SZC",2,1};
	opcode_info[0x4c] = {"JMP","abs","",3,3};
	opcode_info[0x4d] = {"EOR","abs","SZ",4,3};
	opcode_info[0x4e] = {"LSR","abs","SZC",6,3};
	opcode_info[0x50] = {"BVC","rel","",2,2};
	opcode_info[0x51] = {"EOR","(ind),Y","SZ",5,2};
	opcode_info[0x55] = {"EOR","zpg,X","SZ",4,2};
	opcode_info[0x56] = {"LSR","zpg,X","SZC",6,2};
	opcode_info[0x58] = {"CLI","","I",2,1};
	opcode_info[0x59] = {"EOR","abs,Y","SZ",4,3};
	opcode_info[0x5d] = {"EOR","abs,X","SZ",4,3};
	opcode_info[0x5e] = {"LSR","abs,X","SZC",7,3};
	opcode_info[0x60] = {"RTS","","",6,1};
	opcode_info[0x61] = {"ADC","(ind,X)","SVZC",6,2};
	opcode_info[0x65] = {"ADC","zpg","SVZC",3,2};
	opcode_info[0x66] = {"ROR","zpg","SZC",5,2};
	opcode_info[0x68] = {"PLA","","",4,1};
	opcode_info[0x69] = {"ADC","#","SVZC",2,2};
	opcode_info[0x6a] = {"ROR","A","SZC",2,1};
	opcode_info[0x6c] = {"JMP","(ind)","",5,3};
	opcode_info[0x6d] = {"ADC","abs","SVZC",4,3};
	opcode_info[0x6e] = {"ROR","abs","SZC",6,3};
	opcode_info[0x70] = {"BVS","rel","",4,2};
	opcode_info[0x71] = {"ADC","(ind),Y","SVZC",4,2};
	opcode_info[0x75] = {"ADC","zpg,X","SVZC",4,2};
	opcode_info[0x76] = {"ROR","zpg,X","SZC",6,2};
	opcode_info[0x78] = {"SEI","","I",2,1};
	opcode_info[0x79] = {"ADC","abs,Y","SVZC",4,3};
	opcode_info[0x7d] = {"ADC","abs,X","SVZC",4,3};
	opcode_info[0x7e] = {"ROR","abs,X","SZC",7,3};
	opcode_info[0x81] = {"STA","(ind,X)","",6,2};
	opcode_info[0x84] = {"STY","zpg","",3,2};
	opcode_info[0x85] = {"STA","zpg","",3,2};
	opcode_info[0x86] = {"STX","zpg","",3,2};
	opcode_info[0x88] = {"DEY","","SZ",2,1};
	opcode_info[0x8a] = {"TXA","","SZ",2,1};
	opcode_info[0x8c] = {"STY","abs","",4,3};
	opcode_info[0x8d] = {"STA","abs","",4,3};
	opcode_info[0x8e] = {"STX","abs","",4,3};
	opcode_info[0x90] = {"BCC","rel","",2,2};
	opcode_info[0x91] = {"STA","(ind),Y","",6,2};
	opcode_info[0x94] = {"STY","zpg,X","",4,2};
	opcode_info[0x95] = {"STA","zpg,X","",4,2};
	opcode_info[0x96] = {"STX","zpg,Y","",4,2};
	opcode_info[0x98] = {"TYA","","SZ",2,1};
	opcode_info[0x99] = {"STA","abs,Y","",5,3};
	opcode_info[0x9a] = {"TXS","","",2,1};
	opcode_info[0x9d] = {"STA","abs,X","",5,3};
	opcode_info[0xa0] = {"LDY","#","SZ",2,2};
	opcode_info[0xa1] = {"LDA","(ind,X)","SZ",6,2};
	opcode_info[0xa2] = {"LDX","#","SZ",2,2};
	opcode_info[0xa4] = {"LDY","zpg","SZ",3,2};
	opcode_info[0xa5] = {"LDA","zpg","SZ",3,2};
	opcode_info[0xa6] = {"LDX","zpg","SZ",3,2};
	opcode_info[0xa8] = {"TAY","","SZ",2,1};
	opcode_info[0xa9] = {"LDA","#","SZ",2,2};
	opcode_info[0xaa] = {"TAX","","SZ",2,1};
	opcode_info[0xac] = {"LDY","abs","SZ",4,3};
	opcode_info[0xad] = {"LDA","abs","SZ",4,3};
	opcode_info[0xae] = {"LDX","abs","SZ",4,3};
	opcode_info[0xb0] = {"BCS","rel","",2,2};
	opcode_info[0xb1] = {"LDA","(ind),Y","SZ",5,2};
	opcode_info[0xb4] = {"LDY","zpg,X","SZ",4,2};
	opcode_info[0xb5] = {"LDA","zpg,X","SZ",4,2};
	opcode_info[0xb6] = {"LDX","zpg,Y","SZ",4,2};
	opcode_info[0xb8] = {"CLV","","V",2,1};
	opcode_info[0xb9] = {"LDA","abs,Y","SZ",4,3};
	opcode_info[0xba] = {"TSX","","",2,1};
	opcode_info[0xbc] = {"LDY","abs,X","SZ",4,3};
	opcode_info[0xbd] = {"LDA","abs,X","SZ",4,3};
	opcode_info[0xbe] = {"LDX","abs,Y","SZ",4,3};
	opcode_info[0xc0] = {"CPY","#","SZC",2,2};
	opcode_info[0xc1] = {"CMP","(ind,X)","SZC",6,2};
	opcode_info[0xc4] = {"CPY","zpg","SZC",3,2};
	opcode_info[0xc5] = {"CMP","zpg","SZC",3,2};
	opcode_info[0xc6] = {"DEC","zpg","SZ",5,2};
	opcode_info[0xc8] = {"INY","","",2,1};
	opcode_info[0xc9] = {"CMP","#","SZC",2,2};
	opcode_info[0xca] = {"DEX","","SZ",2,1};
	opcode_info[0xcc] = {"CPY","abs","SZC",4,3};
	opcode_info[0xcd] = {"CMP","abs","SZC",4,3};
	opcode_info[0xce] = {"DEC","abs","SZ",6,3};
	opcode_info[0xd0] = {"BNE","rel","",2,2};
	opcode_info[0xd1] = {"CMP","(ind),Y","SZC",5,2};
	opcode_info[0xd5] = {"CMP","zpg,X","SZC",4,2};
	opcode_info[0xd6] = {"DEC","zpg,X","SZ",6,2};
	opcode_info[0xd8] = {"CLD","","D",2,1};
	opcode_info[0xd9] = {"CMP","abs,Y","SZC",4,3};
	opcode_info[0xdd] = {"CMP","abs,X","SZC",4,3};
	opcode_info[0xde] = {"DEC","abs,X","SZ",7,3};
	opcode_info[0xe0] = {"CPX","#","SZC",2,2};
	opcode_info[0xe1] = {"SBC","(ind,X)","SVZC",6,2};
	opcode_info[0xe4] = {"CPX","zpg","SZC",3,2};
	opcode_info[0xe5] = {"SBC","zpg","SVZC",3,2};
	opcode_info[0xe6] = {"INC","zpg","SZ",5,2};
	opcode_info[0xe8] = {"INX","","SZ",2,1};
	opcode_info[0xe9] = {"SBC","#","SVZC",2,2};
	opcode_info[0xea] = {"NOP","","",2,1};
	opcode_info[0xec] = {"CPX","abs","SZC",4,3};
	opcode_info[0xed] = {"SBC","abs","SVZC",4,3};
	opcode_info[0xee] = {"INC","abs","SZ",6,3};
	opcode_info[0xf0] = {"BEQ","rel","",2,2};
	opcode_info[0xf1] = {"SBC","(ind),Y","SVZC",5,2};
	opcode_info[0xf5] = {"SBC","zpg,X","SVZC",4,2};
	opcode_info[0xf6] = {"INC","zpg,X","SZ",6,2};
	opcode_info[0xf8] = {"SED","","D",2,1};
	opcode_info[0xf9] = {"SBC","abs,Y","SVZC",4,3};
	opcode_info[0xfd] = {"SBC","abs,X","SVZC",4,3};
	opcode_info[0xfe] = {"INC","abs,X","SZ",7,3};
}