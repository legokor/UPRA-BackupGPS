// INTERRUPTS
    #define GlobalInterruptEnable GIE_bit
    #define PeripheralInterruptEnable PEIE_bit
    #define UART_RX_Interrupt_Enable RCIE_bit
    #define UART_RX_Interrupt_Flag RCIF_bit

// WATCHDOG TIMER
    //Timing constants
    #define _WDT_SoftEnable_256s   0b00100101
    #define _WDT_SoftEnable_128s   0b00100011
    #define _WDT_SoftEnable_64s    0b00100001
    #define _WDT_SoftEnable_32s    0b00011111
    #define _WDT_SoftEnable_16s    0b00011101
    #define _WDT_SoftEnable_8s     0b00011011
    #define _WDT_SoftEnable_4s     0b00011001
    #define _WDT_SoftEnable_2s     0b00010111
    #define _WDT_SoftEnable_1s     0b00010101
    #define _WDT_SoftEnable_512ms  0b00010011
    #define _WDT_SoftEnable_256ms  0b00010001
    #define _WDT_SoftEnable_128ms  0b00001111
    #define _WDT_SoftEnable_64ms   0b00001101
    #define _WDT_SoftEnable_32ms   0b00001011
    #define _WDT_SoftEnable_16ms   0b00001001
    #define _WDT_SoftEnable_8ms    0b00000111
    #define _WDT_SoftEnable_4ms    0b00000101
    #define _WDT_SoftEnable_2ms    0b00000011
    #define _WDT_SoftEnable_1ms    0b00000001
    #define _WDT_SoftDisable_256s  0b00100100
    #define _WDT_SoftDisable_128s  0b00100010
    #define _WDT_SoftDisable_64s   0b00100000
    #define _WDT_SoftDisable_32s   0b00011110
    #define _WDT_SoftDisable_16s   0b00011100
    #define _WDT_SoftDisable_8s    0b00011010
    #define _WDT_SoftDisable_4s    0b00011000
    #define _WDT_SoftDisable_2s    0b00010110
    #define _WDT_SoftDisable_1s    0b00010100
    #define _WDT_SoftDisable_512ms 0b00010010
    #define _WDT_SoftDisable_256ms 0b00010000
    #define _WDT_SoftDisable_128ms 0b00001110
    #define _WDT_SoftDisable_64ms  0b00001100
    #define _WDT_SoftDisable_32ms  0b00001010
    #define _WDT_SoftDisable_16ms  0b00001000
    #define _WDT_SoftDisable_8ms   0b00000110
    #define _WDT_SoftDisable_4ms   0b00000100
    #define _WDT_SoftDisable_2ms   0b00000010
    #define _WDT_SoftDisable_1ms   0b00000000

    // Default WDT preset is 2s
    #define _WDT_SoftEnable _WDT_SoftEnable_2s
    #define _WDT_SoftDisable _WDT_SoftDisable_2s

    // ASM code to reset the WDT
    #define ClearWDT() asm CLRWDT

  
