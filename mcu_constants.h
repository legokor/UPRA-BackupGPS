// Pin config
#define AnalogPortA ANSELA
#define AnalogPortC ANSELC
#define Comparator1ON C1EN_bit
#define Comparator2ON C2EN_bit
#define CCP1Reg CCP1CON
#define CCP2Reg CCP2CON

// Peripheral Pin Select (PPS)
#define UART1_Disabled UART1MD_bit
#define UART1_RX_pin RX1DTPPS
#define UART1_TX_function 0x0F

// Interrupts
#define GlobalInterruptEnable GIE_bit
#define PeripheralInterruptEnable PEIE_bit
#define UART_RX_Interrupt_Enable RC1IE_bit
#define UART_RX_Interrupt_Flag RC1IF_bit
#define UART_RX_Interrupt_Clear()           // interrupt is cleared by reading the data reg on 16F15325

//USART
#define UART_RX_Data RC1REG

// Watchdog timer
//Config registers
#define WatchdogConfig0 WDTCON0
#define WatchdogConfig1 WDTCON1

//WWDT
#define _WWDT_WindowMax 0b01110000

//Enable/Disable
#define _WDT_SoftEnable 0
#define _WDT_SoftDisable 1

//Timing constants
#define _WDT_256s  0b00100100
#define _WDT_128s  0b00100010
#define _WDT_64s   0b00100000
#define _WDT_32s   0b00011110
#define _WDT_16s   0b00011100
#define _WDT_8s    0b00011010
#define _WDT_4s    0b00011000
#define _WDT_2s    0b00010110
#define _WDT_1s    0b00010100
#define _WDT_512ms 0b00010010
#define _WDT_256ms 0b00010000
#define _WDT_128ms 0b00001110
#define _WDT_64ms  0b00001100
#define _WDT_32ms  0b00001010
#define _WDT_16ms  0b00001000
#define _WDT_8ms   0b00000110
#define _WDT_4ms   0b00000100
#define _WDT_2ms   0b00000010
#define _WDT_1ms   0b00000000

// ASM code to reset the WDT
#define ClearWDT() asm CLRWDT
