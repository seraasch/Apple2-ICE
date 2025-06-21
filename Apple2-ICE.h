

// Teensy 4.1 pin assignments
//
#define PIN_CLK0_INV 24
#define PIN_RESET 40
#define PIN_READY_n 26
#define PIN_IRQ 25
#define PIN_NMI 41
#define PIN_RDWR_n 12
#define PIN_SYNC 39

//  Return the ACTAUL state of the CLK0 pin
// Complexity vs Speed... hmmm...
#define CLK0 (((GPIO6_DR >> 12) & 0x1) == 0)
//  #define CLK0 (digitalReadFast(PIN_CLK0_INV) == LOW)

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

struct OpDecoder
{
    String opcode;
    String operands;
    String flags;
    uint8_t max_cycles;
    uint8_t length;
    uint16_t (*operation)(void);
}

extern opcode_info[];
