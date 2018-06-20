#include "sim808.h"
#include "mcu_constants.h"
#include "pin_config.h"
#include "diag_port.h"

volatile char USM = 0;        //UART RX state machine state
volatile char gnss_buff[_GNSS_BUFF_SIZE];
volatile char buff_end = 0;
volatile char PROMPT_FLAG = 0, OK_FLAG = 0, ERROR_FLAG = 0, RING_FLAG = 0;

char gsm_active = 0;

char gps_run_status = '0';
char gps_fix_status     = '0';
char timestamp[20] = "0";
char latitude[12]  = "0";
char longitude[12] = "0";
int altitude       = 0;

char phone_number[16];

char flag_timeout(char *flag){
    char i = 0;
    
    ClearWDT();
    
    while(!*flag && i < 20){
         if(!*flag){
              ++i;
              delay_ms(500);
         }
    }
    
    if(*flag) return 0;
    else return 1;
}

char command(char *cmd, char *flag){
    buff_end = 0;
    *flag = 0;
    uart1_write_text(cmd);
    
    if(flag_timeout(flag)) return 1;
    else return 0;
}

char fona_init(){
    char timeout;
    
    debugstr("fona_init()\r");
     
    GSM_DTR_dir = 0;
    GSM_DTR = 0;
    
    // PWRKEY IS HARDWIRED TO GND ON THE MODULE!
    GSM_PWRKEY_dir = 1;       // Set it to hi-Z!

    timeout = command("AT+CFUN=1\r", &OK_FLAG);             // Enable GSM functions
    if(timeout) return 1;
    debugstr("   +CFUN=1 OK\r");
    
    timeout = command("ATE0\r", &OK_FLAG);                 // Disable command echo
    if(timeout) return 2;
    debugstr("   ATE0 OK\r");

    timeout = command("AT+CGNSPWR=1\r", &OK_FLAG);         // Enable GNSS functions
    if(timeout) return 3;
    debugstr("   +CGNSPWR=1 OK\r");

    timeout = command("AT+CMGF=1\r", &OK_FLAG);            // Set SMS format to text mode
    if(timeout) return 4;
    debugstr("   +CMGF=1 OK\r");
    
    gsm_active = 1;

    return 0;
}

char gsm_poweron(){
     char timeout;
     
     debugstr("gsm_poweron()\r");
     
     timeout = command("AT+CFUN=1\r", &OK_FLAG);
     if(timeout) return 1;
     
     gsm_active = 1;
     return 0;
}

char gsm_poweroff(){
     char timeout;
     
     debugstr("gsm_poweroff()\r");

     timeout = command("AT+CFUN=0\r", &OK_FLAG);
     if(timeout) return 1;
     
     gsm_active = 0;
     return 0;
}

char get_gps_data(){
     char timeout;
     
     debugstr("get_gps_data()\r");
     
     timeout = command("AT+CGNSINF\r", &OK_FLAG);
     if(timeout) return 1;

     return 0;                                   //got data
}

char gps_parse(){
/*
     Updates GPS-related variables
             Returns 0 if the fix in the buffer is valid.
             Returns 1 if the fix is invalid
             
     ONLY PARAMETERS #1-6 ARE PARSED
     Example:
     +CGNSINF: 1,1,20170126184414.000,46.062122,20.049337,80.5,0.63,70.7,1,,1.2,1.5,0.9,,11,7,,,36,,
               1 2 3                  4         5         6    7    8    9  11  12  13   15 16  19

          ->    1 - GPS run status    <-
          ->    2 - Fix status        <-
          ->    3 - UTC timestamp     <-
          ->    4 - Latitude          <-
          ->    5 - Longitude         <-
          ->    6 - MSL Altitude      <-
                7 - Speed over ground
                8 - Course over ground
                9 - Fix mode
               10 - (reserved)
               11 - HDOP
               12 - PDOP
               13 - VDOP
               14 - (reserved)
               15 - GPS satellites in view
               16 - GNSS satellites used
               17 - GLONASS satellites in view
               18 - (reserved)
               19 - C/N0 max
               20 - HPA (reserved?)
               21 - VPA (reserved?)
*/
     char gps_fix_status_new = 0;
     char tmp[10];
     int i,j,k, found;
     char *inf;
     char infsize;

     debugstr("gps_parse()\r");

     // Find CGNSINF header in the buffer
     found = 0;
     for(i=0;i<buff_end-7 && found==0 ;++i){
          if(gnss_buff[i  ]=='C' && 
             gnss_buff[i+1]=='G' && 
             gnss_buff[i+2]=='N' &&
             gnss_buff[i+3]=='S' && 
             gnss_buff[i+4]=='I' && 
             gnss_buff[i+5]=='N' && 
             gnss_buff[i+6]=='F') found=1;
     }
     if(found==0) return 1;
     
     // Check if there's a full message
     found = 0;
     for(j=i; j<buff_end && found==0; ++j){
              if(gnss_buff[j]==CR) found=1;
     }
     if(found==0) return 1;

     i--;

     infsize = buff_end-i;
     inf = gnss_buff+i;

     for(i=8, j=1; i<infsize && j<=9; ++i){             //Check if the fix is valid
              if(inf[i]==',') j++;
              else if(j==2) gps_fix_status_new = inf[i];
     }

     if(gps_fix_status_new == '1'){
          char integerpart = 1;
          char millisec = 0;
          
          altitude = 0;
          
          for(i=8, j=1;i<infsize && j<=9;++i){
               if(inf[i]==','){
                    j++;
                    k=0;
                    integerpart = 1;
               }
               else{
                    if(j==1){
                        gps_run_status = inf[i];
                    }
                    else if(j==2){
                         gps_fix_status = inf[i];
                    }
                    else if(j==3){
                         if(inf[i]>='0' && inf[i]<='9' && millisec==0){
                              timestamp[k]=inf[i];
                              k++;
                         }
                         if(inf[i]=='.') millisec = 1;
                    }
                    else if(j==4){
                         latitude[k]=inf[i];
                         k++;
                    }
                    else if(j==5){
                         longitude[k]=inf[i];
                         k++;
                    }
                    else if(j==6){
                         if(inf[i]>='0' && inf[i]<='9' && integerpart){
                              altitude *= 10;
                              altitude += (unsigned int)(inf[i]-'0');
                         }
                         if(inf[i]=='.') integerpart = 0;
                    }
               }
          }

          debugstr("    NEW Run status: ");
          debugchr(gps_run_status);
          debugstr("\r    NEW Fix status: ");
          debugchr(gps_fix_status);
          debugstr("\r    NEW UTC time:   ");
          debugstr(timestamp);
          debugstr("\r    NEW Latitude:   ");
          debugstr(latitude);
          debugstr("\r    NEW Longitude:  ");
          debugstr(longitude);
          debugstr("\r    NEW Altitude:   ");
          debuginttostr(altitude,tmp);
          debugstr(tmp);
          debugchr(CR);

          return 0;    // NEW data parsed
     }
     else return 1;    // OLD data remains
}

char get_number(){
     /*
     Answer: +CLCC: <id>,<dir>,<stat>,<mode>,<mpty>[,<number>,<type>,<alphaID>]
                                                     --------
     */
     int i,j,k;
     char *inf;
     char infsize;
     char timeout;

     debugstr("get_number()\r");

     timeout = command("AT+CLCC\r", &OK_FLAG);
     if(timeout) return 1;

     j=0;
     for(i=0; j==0 && i<buff_end; ++i){
         if(gnss_buff[i]=='+' && gnss_buff[i+1]=='C' && gnss_buff[i+2]=='L' && gnss_buff[i+3]=='C' && gnss_buff[i+4]=='C') j = 1;
     }
     i--;

     infsize = buff_end-i;
     inf = gnss_buff+i;

     for(i=7, j=1;i<infsize && j<=6;++i){
         if(inf[i]==','){
             j++;
             k=0;
         }
         else{
             if(j==6){
                 phone_number[k] = inf[i];
                 k++;
                 phone_number[k] = 0;
             }
         }
     }

     debugstr("    Phone number: ");
     debugstr(phone_number);
     debugchr(CR);
     
     return 0;
}

char end_call(){
     char timeout;

     debugstr("end_call()");
     
     timeout = command("ATH\r", &OK_FLAG);
     if(timeout) return 1;
     else return 0;
}

char send_sms(){
     char altstring[8];
     char commandstring[32];
     char i;
     
     debugstr("send_sms()\r");
     
     // Convert altitude to string
     inttostr(altitude,altstring);
     ltrim(altstring);

     // Construct the command
     strcpy(commandstring, "AT+CMGS=");
     strncat(commandstring, phone_number, 20);
     strcat(commandstring, "\r");


     #ifndef _SMS_DISABLED
     //Send real SMS
        timeout = command(commandstring, &PROMPT_FLAG);
        if(timeout) return 1;
        else{
            OK_FLAG = 0;
            if(gps_fix_status =='1'){
                uart1_write_text("Last fix\r");
                uart1_write_text("UTC: ");
                uart1_write_text(timestamp);
                uart1_write_text("\rLAT: ");
                uart1_write_text(latitude);
                uart1_write_text("\rLON: ");
                uart1_write_text(longitude);
                uart1_write_text("\rALT: ");
                uart1_write_text(altstring);
                uart1_write(CR);
                uart1_write(SUB);
            }
            else{
                uart1_write_text("No valid fix yet\r");
                uart1_write(SUB);
            }
        
            timeout = flag_timeout(&OK_FLAG);
            if(timeout) return 2;
        }
     #endif
     
     // Write data to diag port
     if(gps_fix_status=='1'){
         debugstr("    SMS data sent ");
         debugstr(" UTC ");
         debugstr(timestamp);
         debugstr(" LAT ");
         debugstr(latitude);
         debugstr(" LON ");
         debugstr(longitude);
         debugstr(" ALT ");
         debugstr(altstring);
     }
     else debugstr("    SMS \"No valid fix\" sent\r");

     return 0;
}