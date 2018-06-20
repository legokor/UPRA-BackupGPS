#ifndef _SIM808_H
#define _SIM808_H

#define _SMS_DISABLED
#define _DEBUG_MODE



#ifndef _DEBUG_MODE
    #ifdef _SMS_DISABLED
        #error SMS disabled in release build
    #endif
#endif


#define CR 0x0D
#define SUB 0x1A

#define _GNSS_BUFF_SIZE 250

extern volatile char gnss_buff[_GNSS_BUFF_SIZE];
extern volatile char buff_end;
extern volatile char USM;
extern volatile char PROMPT_FLAG, OK_FLAG, ERROR_FLAG, RING_FLAG;

extern char gsm_active;
extern char phone_number[16];
extern char gps_fix_status;
extern char timestamp[20];
extern char latitude[12];
extern char longitude[12];
extern int altitude;


char fona_init();
char gsm_poweron();
char gsm_poweroff();
char get_gps_data();
char get_number();
char end_call();
char gps_parse();
char send_sms();
#endif