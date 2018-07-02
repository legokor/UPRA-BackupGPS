/*
  * UPRA Backup GPS tracker
  * Tamás Kiss
  * 2018

  * Hardware config
             MCU:    PIC16F18326
             OSC:    INTOSC 32MHz (2xPLL)

             Ext. modules: SIM808

  * PIN MAP:
              RA0       ICSP DATA
              RA1       ICSP CLK
              RA3       ICSP MCLR

              RA2       GSM_SLEEP
              RC0       GSM_PWRKEY
              RC1       GSM_RX
              RC2       GSM_TX

              RA4       LED2
              RA5       LED1
              RC3       Buzzer

              RC4       Diag TX
              RC5       Diag RX
*/


#include "sim808.h"
#include "mcu_constants.h"
#include "pin_config.h"
#include "diag_port.h"

#define _GSM_POWEROFF_ALTITUDE 5000   // meters
#define _GSM_POWERON_ALTITUDE 4500    // meters

#define _GPS_UPDATE_INTERVAL 20           // # of ticks (about 100 ms)
#define _RSSI_UPDATE_INTERVAL 20
#define _TEMP_UPDATE_INTERVAL 20
#define _VBAT_UPDATE_INTERVAL 20
#define _DIAG_REPORT_INTERVAL 20

void interrupt(){
     if(RCIF_bit && buff_end < _GNSS_BUFF_SIZE-1) {
          char buff_tmp;

          buff_tmp = UART_Read();
          gnss_buff[buff_end] = buff_tmp;

          switch(USM){
                        case 0:
                                        if     (buff_tmp=='O') USM = 1;                        // OK
                                        else if(buff_tmp=='E') USM = 2;                        // ERROR
                                        else if(buff_tmp=='R') USM = 6;                        // RING
                                        else if(buff_tmp=='>') PROMPT_FLAG = 1;                // SMS text prompt
                                        break;
                        case 1:                                                                        // "O" received, waiting for "K"
                                        if(buff_tmp=='K') OK_FLAG = 1;
                                        USM = 0;
                                        break;
                        case 2:                                                                        // "E" received, waiting for "RROR"
                                        if(buff_tmp=='R') USM = 3;
                                        else USM = 0;
                                        break;
                        case 3:                                                                        // "ER" received, waiting for "ROR"
                                        if(buff_tmp=='R') USM = 4;
                                        else USM = 0;
                                        break;
                        case 4:                                                                        // "ERR" received, waiting for "OR"
                                        if(buff_tmp=='O') USM = 5;
                                        else USM = 0;
                                        break;
                        case 5:                                                                        // "ERRO" received, waiting for "R"
                                        if(buff_tmp=='R') ERROR_FLAG = 1;
                                        USM = 0;
                                        break;
                        case 6:                                                                        // "R" received, waiting for "ING"
                                        if(buff_tmp=='I') USM = 7;
                                        else USM = 0;
                                        break;
                        case 7:                                                                        // "RI" received, waiting for "NG"
                                        if(buff_tmp=='N') USM = 8;
                                        else USM = 0;
                                        break;
                        case 8:                                                                        // "RIN" received, waiting for "G"
                                        if(buff_tmp=='G') RING_FLAG = 1;
                                        USM = 0;
                                        break;

                        default: USM = 0;
         }

         buff_end++;
         gnss_buff[buff_end] = 0;

         RCIF_bit = 0;
     }
}


// Inititalize internal modules
void mcu_init(){
     WDTCON = _WDT_SoftEnable_32s;      // Enable the watchdog timer
     
     GlobalInterruptEnable = 0;
     
     TRISA = 0xFF;         // High-Z all pins
     TRISC = 0xFF;
     LATA = 0x00;          // Clear output latches
     LATC = 0x00;
     AnalogPortA  = 0;          // Disable analog functionality
     AnalogPortC = 0;
     Comparator1ON = 0;         // Disable comparators
     Comparator2ON = 0;

     RED_LED_dir = 0;
     RED_LED = 0;
     YELLOW_LED_dir = 0;
     YELLOW_LED = 0;

     // Initialize UART module
     RXPIN_dir = 1;
     TXPIN_dir = 0;
     Unlock_IOLOCK();              // Unlock peripheral pin select
         UART1_Disabled = 0;       // Enable UART-PPS
         RXPPS = RXPIN;            // RX    = RXPIN
         TXPIN = 0b10100;          // TXPIN = TX
     Lock_IOLOCK();                // Lock PPS
     UART1_Remappable_Init(9600);
     delay_ms(100);

     // Enable interrupts
     UART_RX_Interrupt_Enable  = 1;      // enable UART RX interrupt
     UART_RX_Interrupt_Flag    = 0;      // clear UART RX IT flag

     // Initialize software uart on diag port
     #ifdef _DEBUG_MODE
            Soft_UART_Init(_DIAGPORT, _DIAG_RXPIN, _DIAG_TXPIN, _DIAG_BAUD,0); // (*port, rx, tx, baud, inverted)
            delay_ms(100);
     #endif

     // Enable interrupts
     PeripheralInterruptEnable = 1;
     GlobalInterruptEnable     = 1;
}


void main() {
     unsigned long tick = 0;
     char timed_out = 0;
     char error = 0;
     char altstring[8];
     
     mcu_init();
     debugstr("POWER ON\r");
     debugstr("\rData format:\r");
     debugstr("DATA:Time,Lat,Lon,Alt(m),GSM_state,RSSI,BER,Temp(C),V_Bat(mV)\r\r");

     RED_LED = 1;

     ClearWDT();

     timed_out = fona_init();
     while(timed_out){
                    debugstr("TIMEOUT\r");
                    timed_out = fona_init();
     }

     RED_LED = 0;

     ClearWDT();

     while(1){
              ClearWDT();

              if(RING_FLAG){
                            debugstr("RINGING\r");
          
                            debugstr("get_number");
                            timed_out = get_number();
                            
                            while(timed_out){
                                debugstr(" TIMEOUT");
                                debugstr("\rget_number");
                                timed_out = get_number();
                            }
                            
                            ClearWDT();
                            
                            debugstr(phone_number);
                            debugstr("\rend_call");
                            timed_out = end_call();
                            
                            while(timed_out){
                                debugstr(" TIMEOUT");
                                debugstr("\rend_call");
                                timed_out = end_call();
                            }
                            
                            ClearWDT();

                            debugstr("\rsend_sms");
                            timed_out = send_sms();
                            
                            while(timed_out){
                                debugstr(" TIMEOUT");
                                debugstr("\rsend_sms");
                                timed_out = send_sms();
                            }
                            debugchr(CR);

                            ClearWDT();
                            RING_FLAG = 0;
              }


              if(tick % _GPS_UPDATE_INTERVAL == 0){
                  debugstr("get_gps_data");
                  timed_out = get_gps_data();
                  
                  if(!timed_out){
                       debugstr("\rgps_parse");
                       error = gps_parse();
                       if(error) debugstr(" BUFFER INVALID\r");
                  }
                  else debugstr(" TIMEOUT\r");
              }
              
              if(tick % _RSSI_UPDATE_INTERVAL == 0){
                  debugstr("read_rssi");
                  timed_out = read_rssi();
                  if(timed_out) debugstr(" ERROR");
                  debugchr(CR);
              }
              
              if(tick % _TEMP_UPDATE_INTERVAL == 0){
                  debugstr("read_temp");
                  timed_out = read_temp();
                  if(timed_out) debugstr(" ERROR");
                  debugchr(CR);
              }
              
              if(tick % _RSSI_UPDATE_INTERVAL == 0){
                  debugstr("read_vbat");
                  timed_out = read_vbat();
                  if(timed_out) debugstr(" ERROR");
                  debugchr(CR);
              }
              
              if(gps_fix_status=='1'){
                   if(gsm_active && altitude>_GSM_POWEROFF_ALTITUDE){
                       debugstr("gsm_poweroff");
                       timed_out = gsm_poweroff();
                       while(timed_out){
                           debugstr(" TIMEOUT");
                           debugstr("\rgsm_poweroff");
                           timed_out = gsm_poweroff();
                       }
                       debugchr(CR);
                   }
                   if(!gsm_active && altitude<_GSM_POWERON_ALTITUDE){
                       debugstr("gsm_poweron");
                       timed_out = gsm_poweron();
                       while(timed_out){
                           debugstr(" TIMEOUT");
                           debugstr("\rgsm_poweron");
                           timed_out = gsm_poweron();
                       }
                       debugchr(CR);
                   }
              }
              
              if(tick % _GPS_UPDATE_INTERVAL == 0 ) RED_LED = 1;
              else RED_LED = 0;
              
              tick++;
              delay_ms(100);
              
              
              
              #ifdef _DEBUG_MODE
                  if(tick % _DIAG_REPORT_INTERVAL == 0){
                        debugstr("\rDATA:");

                        if(gps_fix_status=='1'){
                             char i;

                             inttostr(altitude,altstring);
                             ltrim(altstring);
                             for(i=0; timestamp[i]!=0; i++){
                                  if(i==4 || i==6 || i==8) debugchr('.');
                                  if(i==8) debugchr(' ');
                                  if(i==10 || i==12) debugchr(':');
                                  debugchr(timestamp[i]);
                             }
                             debugchr(',');
                             debugstr(latitude);
                             debugchr(',');
                             debugstr(longitude);
                             debugchr(',');
                             debugstr(altstring);
                        }
                        else debugstr(",,,");

                        if(gsm_active) debugstr(",ON");
                        else debugstr(",OFF");

                        debugchr(','); debugstr(rssi);
                        debugchr(','); debugstr(ber);
                        debugchr(','); debugstr(temp);
                        debugchr(','); debugstr(vbat);
                        debugstr("\r\r");
                  }
              #endif
     }
}