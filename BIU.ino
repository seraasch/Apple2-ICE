// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------
//
// Begin 6502 Bus Interface Unit
//
// --------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------

// was used externally
// inline uint8_t finish_read_byte();

#include "Apple2-ICE.h"

uint16_t current_address = 0; // address last sent to address buss
uint8_t direct_datain = 0;    // data bus value at last rising edge

uint8_t last_access_internal_RAM = 0;
uint8_t rx_byte_state = 0;

uint8_t internal_RAM[65536];

// ----------------------------------------------------------
// Address range check
//  Return: 0x0 - All exernal memory accesses
//          0x1 - Reads use cycle accurate internal memory and writes pass through to motherboard
//          0x2 - Reads accelerated using internal memory and writes pass through to motherboard
//          0x3 - All read and write accesses use accelerated internal memory
// ----------------------------------------------------------
inline uint8_t internal_address_check(int32_t local_address)
{

    if ((local_address >= 0x0000) && (local_address < 0x0400))
        return addr_mode; //  6502 ZeroPage and Stack
    if ((local_address >= 0x0400) && (local_address < 0x0C00))
        return Read_Internal_Write_External; //  Apple II Plus Text Page 1 and 2
    if ((local_address >= 0x0C00) && (local_address < 0x2000))
        return addr_mode; //  Apple II Plus RAM
    if ((local_address >= 0x2000) && (local_address < 0x6000))
        return addr_mode; //  Apple IIPlus  HIRES Page 1 and 2
    if ((local_address >= 0x6000) && (local_address < 0xC000))
        return addr_mode; //  Apple II Plus RAM
    if ((local_address >= 0xD000) && (local_address <= 0xFFFF))
        return All_External; //  Apple II Plus ROMs
    //    Bank switching does not currently work, so set to 0x0 to use the Language card and have 64KB total memory
    //    and to accelerate ROM but run with only 48KB of memory, set to 'mode'

    return 0x0;
}

//====================================================================================================
// Clock edge detection.
//      REMEMBER: the CLK0 macro gives the value of the Phase-0 pin on the 6502 socket
//      Note: CLK1 is the inverted value of CLK0 (and is an output from the 6502)
//            CLK2 is the same as CLK0 (and is an output from the 6502)
inline void wait_for_CLK0_falling_edge() {
    if (debug_mode) return;

    while (! CLK0) {}    // Wait here until CLK0 is high
    while (CLK0) {}      // Wait here until CLK0 goes low
}

inline void wait_for_CLK0_rising_edge() {
    if (debug_mode) return;

    while (CLK0) {}         // Wait here until CLK0 is low
    while (! CLK0) {}       // Wait here until CLK0 goes high
}


// -------------------------------------------------
// Wait for the CLK1 rising edge and sample signals
// -------------------------------------------------
inline void sample_at_CLK0_falling_edge() {
    uint32_t GPIO6_data = 0;
    // uint32_t GPIO6_data_d1 = 0;
    // uint32_t d10, d2, d3, d4, d5, d76;

    if (debug_mode)
        return;

    //===============================================================================
	//   Wait for the end of the MPU-access period (indicated by falling CLK0 edge),
	//   then grab the data and control bits
	//
	//   Refer to the comments above ("Clock edge detection") for details
	//
    wait_for_CLK0_falling_edge();
	GPIO6_data = GPIO6_DR;

    // do {
        // GPIO6_data_d1 = GPIO6_DR;
    // } while (((GPIO6_data_d1 >> 12) & 0x1) == 0); // This method needed to support Apple-II+ DRAM read data setup time
    // GPIO6_data = GPIO6_data_d1;

	// This ugliness can be removed by doing a smarter job of routing data bits to the Teensy
#define bits_d10 ((GPIO6_data & 0x000C0000) >> 18)  // Teensy 4.1 Pin-14  GPIO6_DR[19:18]  D1:D0
#define bits_d2  ((GPIO6_data & 0x00800000) >> 21)  // Teensy 4.1 Pin-16  GPIO6_DR[23]     D2
#define bits_d3  ((GPIO6_data & 0x00400000) >> 19)  // Teensy 4.1 Pin-17  GPIO6_DR[22]     D3
#define bits_d4  ((GPIO6_data & 0x00020000) >> 13)  // Teensy 4.1 Pin-18  GPIO6_DR[17]     D4
#define bits_d5  ((GPIO6_data & 0x00010000) >> 11)  // Teensy 4.1 Pin-19  GPIO6_DR[16]     D5
#define bits_d76 ((GPIO6_data & 0x0C000000) >> 20)  // Teensy 4.1 Pin-20  GPIO6_DR[27:26]  D7:D6
    direct_datain = bits_d76 | bits_d5 | bits_d4 | bits_d3 | bits_d2 | bits_d10;

    // why are these synchronous?
    direct_irq     = (GPIO6_data & 0x00002000) >> 13; // Teensy 4.1 Pin-25  GPIO6_DR[13]     IRQ
    direct_ready_n = (GPIO6_data & 0x40000000) >> 30; // Teensy 4.1 Pin-26  GPIO6_DR[30]     READY
    direct_reset   = (GPIO6_data & 0x00100000) >> 20; // Teensy 4.1 Pin-40  GPIO6_DR[20]     RESET
    direct_nmi     = (GPIO6_data & 0x00200000) >> 21; // Teensy 4.1 Pin-41  GPIO6_DR[21]     NMI

    return;
}


// -------------------------------------------------
// Drive the 6502 Address pins
// -------------------------------------------------
inline void send_address(uint32_t local_address)
{
    uint32_t writeback_data = 0;

    writeback_data = (0x6DFFFFF3 & GPIO6_DR);                         // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x8000) << 10; // 6502_Address[15]   TEENSY_PIN23   GPIO6_DR[25]
    writeback_data = writeback_data | (local_address & 0x2000) >> 10; // 6502_Address[13]   TEENSY_PIN0    GPIO6_DR[3]
    writeback_data = writeback_data | (local_address & 0x1000) >> 10; // 6502_Address[12]   TEENSY_PIN1    GPIO6_DR[2]
    writeback_data = writeback_data | (local_address & 0x0002) << 27; // 6502_Address[1]    TEENSY_PIN38   GPIO6_DR[28]
    GPIO6_DR = writeback_data | (local_address & 0x0001) << 31;       // 6502_Address[0]    TEENSY_PIN27   GPIO6_DR[31]

    writeback_data = (0xCFF3EFFF & GPIO7_DR);                         // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x0400) << 2;  // 6502_Address[10]   TEENSY_PIN32   GPIO7_DR[12]
    writeback_data = writeback_data | (local_address & 0x0200) << 20; // 6502_Address[9]    TEENSY_PIN34   GPIO7_DR[29]
    writeback_data = writeback_data | (local_address & 0x0080) << 21; // 6502_Address[7]    TEENSY_PIN35   GPIO7_DR[28]
    writeback_data = writeback_data | (local_address & 0x0020) << 13; // 6502_Address[5]    TEENSY_PIN36   GPIO7_DR[18]
    GPIO7_DR = writeback_data | (local_address & 0x0008) << 16;       // 6502_Address[3]    TEENSY_PIN37   GPIO7_DR[19]

    writeback_data = (0xFF3BFFFF & GPIO8_DR);                         // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x0100) << 14; // 6502_Address[8]    TEENSY_PIN31   GPIO8_DR[22]
    writeback_data = writeback_data | (local_address & 0x0040) << 17; // 6502_Address[6]    TEENSY_PIN30   GPIO8_DR[23]
    GPIO8_DR = writeback_data | (local_address & 0x0004) << 16;       // 6502_Address[2]    TEENSY_PIN28   GPIO8_DR[18]

    writeback_data = (0x7FFFFF6F & GPIO9_DR);                         // Read in current GPIOx register value and clear the bits we intend to update
    writeback_data = writeback_data | (local_address & 0x4000) >> 10; // 6502_Address[14]   TEENSY_PIN2    GPIO9_DR[4]
    writeback_data = writeback_data | (local_address & 0x0800) >> 4;  // 6502_Address[11]   TEENSY_PIN33   GPIO9_DR[7]
    GPIO9_DR = writeback_data | (local_address & 0x0010) << 27;       // 6502_Address[4]    TEENSY_PIN29   GPIO9_DR[31]

    return;
}

// -------------------------------------------------
// Send the address for a read cyle
// -------------------------------------------------
inline void start_read(uint32_t local_address)
{

    current_address = local_address;

    if (internal_address_check(current_address) > 0x1)
    {
        // last_access_internal_RAM=1;
    }
    else
    {
        if (last_access_internal_RAM == 1)
            wait_for_CLK0_rising_edge();
        last_access_internal_RAM = 0;

        digitalWriteFast(PIN_RDWR_n, 0x1);
        // digitalWriteFast(PIN_SYNC, 0);
        send_address(local_address);
    }
    return;
}

// -------------------------------------------------
// On the rising CLK edge, read in the data
// -------------------------------------------------
inline uint8_t finish_read_byte()
{

    if (internal_address_check(current_address) > 0x1)
    {
        last_access_internal_RAM = 1;
        return internal_RAM[current_address];
    }
    else
    {
        if (last_access_internal_RAM == 1)
            wait_for_CLK0_rising_edge();
        last_access_internal_RAM = 0;
        digitalWriteFast(PIN_SYNC, 0x0);

        do
        {
            wait_for_CLK0_rising_edge();
        } while (direct_ready_n == 0x1); // Delay a clock cycle until ready is active

        if (internal_address_check(current_address) > 0x0)
        {
            return internal_RAM[current_address];
        }
        else
        {
            return direct_datain;
        }
    }
}

// -------------------------------------------------
// Full read cycle with address and data read in
// -------------------------------------------------
inline uint8_t read_byte(uint16_t local_address)
{

    if (internal_address_check(local_address) > 0x1)
    {
        last_access_internal_RAM = 1;
        return internal_RAM[local_address];
    }
    else
    {
        if (last_access_internal_RAM == 1)
            wait_for_CLK0_rising_edge();
        last_access_internal_RAM = 0;

        start_read(local_address);
        do
        {
            wait_for_CLK0_rising_edge();
        } while (direct_ready_n == 0x1); // Delay a clock cycle until ready is active

        // Set Acceleration using Apple II keystrokes
        // For level 0 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   0
        // For level 1 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   1
        // For level 2 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   2
        // For level 3 acceleration enter the following key sequence:  left_arrow  right_arrow  left_arrow   3
        //
        // These sequences can be entered at any time, however they only work when the Apple II software is polling for keystrokes.
        // If the software is not polling for keystrokes, then the UART RX character receiver can be used to set the acceleration mode.
        //
        if (local_address == 0xC000)
        {
            if (rx_byte_state == 0 && direct_datain == 0x88)
                rx_byte_state = 1;
            if (rx_byte_state == 1 && direct_datain == 0x95)
                rx_byte_state = 2;
            if (rx_byte_state == 2 && direct_datain == 0x88)
                rx_byte_state = 3;
            if (rx_byte_state == 3)
            {
                if (direct_datain == 0xB0)
                {
                    addr_mode = All_External;
                    rx_byte_state = 0;
                }
                if (direct_datain == 0xB1)
                {
                    addr_mode = Read_Internal_Write_External;
                    rx_byte_state = 0;
                }
                if (direct_datain == 0xB2)
                {
                    addr_mode = Read_Fast_Internal_Write_External;
                    rx_byte_state = 0;
                }
                if (direct_datain == 0xB3)
                {
                    addr_mode = All_Fast_Internal;
                    rx_byte_state = 0;
                }
            }
        }

        if (internal_address_check(current_address) > 0x0)
        {
            return internal_RAM[current_address];
        }
        else
        {
            return direct_datain;
        }
    }
}

// -------------------------------------------------
// Full write cycle with address and data written
// -------------------------------------------------
inline void write_byte(uint16_t local_address, uint8_t local_write_data)
{

    // Always mirror writes to internal RAM, except ROM range at 0xD000 - 0xFFFF
    //
    if (local_address < 0xC000)
        internal_RAM[local_address] = local_write_data;

    // Internal RAM
    //
    if (internal_address_check(local_address) > 0x2)
    {
        last_access_internal_RAM = 1;
    }
    else
    {
        if (last_access_internal_RAM == 1)
            wait_for_CLK0_rising_edge();
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
        wait_for_CLK0_falling_edge();
        digitalWriteFast(PIN_DATAOUT_OE_n, 0x0);

        wait_for_CLK0_rising_edge();
        digitalWriteFast(PIN_DATAOUT_OE_n, 0x1);
    }
    return;
}

// ======================================
// End 6502 Bus Interface Unit
// ======================================

// ======================================
//   Support for internal memory image
// ======================================

uint8_t AppleIIP_ROM_D0[0x0800] = {
    0x6f,
    0xd8,
    0x65,
    0xd7,
    0xf8,
    0xdc,
    0x94};
uint8_t AppleIIP_ROM_D8[0x0800] = {
    0xb8,
    0x90,
    0x2,
    0xe6,
    0xb9,
    0x24,
    0xf2};

uint8_t AppleIIP_ROM_E0[0x0800] = {
    0x4c,
    0x28,
    0xf1,
    0x4c,
    0x3c,
    0xd4,
    0x0};
uint8_t AppleIIP_ROM_E8[0x0800] = {
    0xe0,
    0xa5,
    0xf0,
    0x2,
    0xa0,
    0xa5,
    0x38};

uint8_t AppleIIP_ROM_F0[0x0800] = {
    0x20,
    0x23,
    0xec,
    0xa9,
    0x0,
    0x85,
    0xab};
uint8_t AppleIIP_ROM_F8[0x0800] = {
    0x4a,
    0x8,
    0x20,
    0x47,
    0xf8,
    0x28,
    0xa9};

void initialize_roms()
{
    // Copy ROM contents into the Teensy's internal RAM
    //
    for (uint32_t u = 0; u <= 0x07FF; u++)
    {
        internal_RAM[0xD000 + u] = AppleIIP_ROM_D0[u];
    }
    for (uint32_t u = 0; u <= 0x07FF; u++)
    {
        internal_RAM[0xD800 + u] = AppleIIP_ROM_D8[u];
    }

    for (uint32_t u = 0; u <= 0x07FF; u++)
    {
        internal_RAM[0xE000 + u] = AppleIIP_ROM_E0[u];
    }
    for (uint32_t u = 0; u <= 0x07FF; u++)
    {
        internal_RAM[0xE800 + u] = AppleIIP_ROM_E8[u];
    }

    for (uint32_t u = 0; u <= 0x07FF; u++)
    {
        internal_RAM[0xF000 + u] = AppleIIP_ROM_F0[u];
    }
    for (uint32_t u = 0; u <= 0x07FF; u++)
    {
        internal_RAM[0xF800 + u] = AppleIIP_ROM_F8[u];
    }
}