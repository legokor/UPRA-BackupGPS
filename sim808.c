#include "sim808.h"
#include "pin_config.h"
#include "diag_port.h"

volatile char USM = 0;        //UART RX state machine state
volatile char gnss_buff[_GNSS_BUFF_SIZE];
volatile char buff_end = 0;
volatile char PROMPT_FLAG = 0, OK_FLAG = 0, ERROR_FLAG = 0, RING_FLAG = 0;

char gsm_active = 0;

char gps_run_status = '0';
char gps_fix_status = '0';
char timestamp[20] = "0";
char latitude[12] = "0";
char longitude[12] = "0";
int altitude = 0;
char rssi[5] = "-1";
char ber[5] = "-1";
char vbat[8] = "-1";
char temp[8] = "-1";

char phone_number[16];

char flag_timeout(char *flag) {
    char i = 0;

    while (!*flag && i < _COMMAND_TIMEOUT) {
        if (!*flag) {
            ++i;
            delay_ms(100);
        }
    }

    if (*flag)
        return 0;
    else
        return 1;
}

char command(const char *cmd, char *flag) {
    buff_end = 0;
    *flag = 0;
    uart1_write_text(cmd);

    if (flag_timeout(flag))
        return 1;
    else
        return 0;
}

char fona_init() {
    char timeout;

    debugstr("fona_init()");

    GSM_DTR_dir = 0;
    GSM_DTR = 0;

    // PWRKEY IS HARDWIRED TO GND ON THE MODULE!
    GSM_PWRKEY_dir = 1;       // Set it to hi-Z!

    timeout = command("AT+CFUN=1\r", &OK_FLAG);         // Enable GSM functions
    if (timeout)
        return 1;
    debugstr("\r\n   +CFUN=1 OK");

    timeout = command("ATE0\r", &OK_FLAG);              // Disable command echo
    if (timeout)
        return 2;
    debugstr("\r\n   ATE0 OK");

    timeout = command("AT+CGNSPWR=1\r", &OK_FLAG);      // Enable GNSS functions
    if (timeout)
        return 3;
    debugstr("\r\n   +CGNSPWR=1 OK");

    timeout = command("AT+CMGF=1\r", &OK_FLAG);         // Set SMS format to text mode
    if (timeout)
        return 4;
    debugstr("   +CMGF=1 OK\r\n");

    gsm_active = 1;

    return 0;
}

char gsm_poweron() {
    char timeout;

    timeout = command("AT+CFUN=1\r", &OK_FLAG);
    if (timeout)
        return 1;

    gsm_active = 1;
    return 0;
}

char gsm_poweroff() {
    char timeout;

    timeout = command("AT+CFUN=0\r", &OK_FLAG);
    if (timeout)
        return 1;

    gsm_active = 0;
    return 0;
}

char get_gps_data() {
    char timeout;

    timeout = command("AT+CGNSINF\r", &OK_FLAG);
    if (timeout)
        return 1;

    return 0;                                   //got data
}

/**
 * Updates GPS-related variables
 * @return 0 if the fix in the buffer is valid
 *         1 if the fix is invalid
 */
char gps_parse() {
    /*
     * ONLY PARAMETERS #1-6 ARE PARSED
     * Example:
     * +CGNSINF: 1,1,20170126184414.000,46.062122,20.049337,80.5,0.63,70.7,1,,1.2,1.5,0.9,,11,7,,,36,,
     *           1|2|3                 |4        |5        |6   |7   |8   |9||11 |12 |13 ||15|16||19||
     *
     * 1 - GPS run status
     * 2 - Fix status
     * 3 - UTC timestamp
     * 4 - Latitude
     * 5 - Longitude
     * 6 - MSL Altitude
     * 7 - Speed over ground
     * 8 - Course over ground
     * 9 - Fix mode
     * 10 - (reserved)
     * 11 - HDOP
     * 12 - PDOP
     * 13 - VDOP
     * 14 - (reserved)
     * 15 - GPS satellites in view
     * 16 - GNSS satellites used
     * 17 - GLONASS satellites in view
     * 18 - (reserved)
     * 19 - C/N0 max
     * 20 - HPA (reserved?)
     * 21 - VPA (reserved?)
     */
    char gps_fix_status_new = 0;
    char tmp[10];
    int i, j, k, found;
    char *inf;
    char infsize;

    // Find CGNSINF header in the buffer
    found = 0;
    for (i = 0; i < buff_end - 7 && found == 0; ++i) {
        if (gnss_buff[i    ] == 'C' &&
            gnss_buff[i + 1] == 'G' &&
            gnss_buff[i + 2] == 'N' &&
            gnss_buff[i + 3] == 'S' &&
            gnss_buff[i + 4] == 'I' &&
            gnss_buff[i + 5] == 'N' &&
            gnss_buff[i + 6] == 'F')
            found = 1;
    }
    if (found == 0)
        return 1;

    // Check if there's a full message
    found = 0;
    for (j = i; j < buff_end && found == 0; ++j) {
        if (gnss_buff[j] == CR)
            found = 1;
    }
    if (found == 0)
        return 1;

    i--;

    infsize = buff_end - i;
    inf = gnss_buff + i;

    for (i = 8, j = 1; i < infsize && j <= 9; ++i) { //Check if the fix is valid
        if (inf[i] == ',')
            j++;
        else if (j == 2)
            gps_fix_status_new = inf[i];
    }

    if (gps_fix_status_new == '1') {
        char integerpart = 1;
        char millisec = 0;

        altitude = 0;

        for (i = 8, j = 1; i < infsize && j <= 9; ++i) {
            if (inf[i] == ',') {
                j++;
                k = 0;
                integerpart = 1;
            } else {
                if (j == 1) {
                    gps_run_status = inf[i];
                } else if (j == 2) {
                    gps_fix_status = inf[i];
                } else if (j == 3) {
                    if (inf[i] >= '0' && inf[i] <= '9' && millisec == 0) {
                        timestamp[k] = inf[i];
                        k++;
                    }
                    if (inf[i] == '.')
                        millisec = 1;
                } else if (j == 4) {
                    latitude[k] = inf[i];
                    k++;
                } else if (j == 5) {
                    longitude[k] = inf[i];
                    k++;
                } else if (j == 6) {
                    if (inf[i] >= '0' && inf[i] <= '9' && integerpart) {
                        altitude *= 10;
                        altitude += (unsigned int) (inf[i] - '0');
                    }
                    if (inf[i] == '.')
                        integerpart = 0;
                }
            }
        }
        return 0;    // NEW data parsed
    } else
        return 1;    // OLD data remains
}

char get_number() {
    /*
     Answer: +CLCC: <id>,<dir>,<stat>,<mode>,<mpty>[,<number>,<type>,<alphaID>]
     --------
     */
    int i, j, k;
    char *inf;
    char infsize;
    char timeout;

    timeout = command("AT+CLCC\r", &OK_FLAG);
    if (timeout)
        return 1;

    j = 0;
    for (i = 0; j == 0 && i < buff_end; ++i) {
        if (gnss_buff[i] == '+' && gnss_buff[i + 1] == 'C'
                && gnss_buff[i + 2] == 'L' && gnss_buff[i + 3] == 'C'
                && gnss_buff[i + 4] == 'C')
            j = 1;
    }
    i--;

    infsize = buff_end - i;
    inf = gnss_buff + i;

    for (i = 7, j = 1; i < infsize && j <= 6; ++i) {
        if (inf[i] == ',') {
            j++;
            k = 0;
        } else {
            if (j == 6) {
                phone_number[k] = inf[i];
                k++;
                phone_number[k] = 0;
            }
        }
    }

    return 0;
}

char end_call() {
    char timeout;

    timeout = command("ATH\r", &OK_FLAG);
    if (timeout)
        return 1;
    else
        return 0;
}

char send_sms() {
    char altstring[8];
    char commandstring[32];
    char timeout;

    // Convert altitude to string
    inttostr(altitude, altstring);
    ltrim(altstring);

    // Construct the command
    strcpy(commandstring, "AT+CMGS=");
    strncat(commandstring, phone_number, 20);
    strcat(commandstring, "\r");

#ifndef _SMS_DISABLED
    //Send real SMS
    timeout = command(commandstring, &PROMPT_FLAG);
    if (timeout)
        return 1;
    else {
        OK_FLAG = 0;
        if (gps_fix_status == '1') {
            uart1_write_text((const char*) "Last fix\r");
            uart1_write_text((const char*) "UTC: ");
            uart1_write_text(timestamp);
            uart1_write_text((const char*) "\rLAT: ");
            uart1_write_text(latitude);
            uart1_write_text((const char*) "\rLON: ");
            uart1_write_text(longitude);
            uart1_write_text((const char*) "\rALT: ");
            uart1_write_text(altstring);
            uart1_write(CR);
            uart1_write(SUB);
        } else {
            uart1_write_text((const char*) "No valid fix yet\r");
            uart1_write(SUB);
        }

        timeout = flag_timeout(&OK_FLAG);
        if (timeout)
            return 2;
    }
#else
    // Write data to diag port
    if(gps_fix_status=='1') {
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
    else debugstr("    SMS \"No valid fix\" sent\r\n");
#endif

    return 0;
}

char read_rssi() {
    char timeout;
    int i, msgpos, found = 0;

    // Read data into the buffer
    timeout = command("AT+CSQ\r", &OK_FLAG);
    if (timeout)
        return 1;

    // Locate data in buffer
    for (i = 0; !found && i < buff_end - 11; i++) {
        if (gnss_buff[i] == 'C' && gnss_buff[i + 1] == 'S'
                && gnss_buff[i + 2] == 'Q' && gnss_buff[i + 3] == ':') {
            msgpos = i + 5;
            found = 1;
        }
    }
    if (!found)
        return 2;

    // Copy RSSI string
    for (i = 0; i < 3 && gnss_buff[msgpos + i] != ','; i++)
        rssi[i] = gnss_buff[msgpos + i];
    rssi[i] = 0;
    msgpos += i + 1;

    // Copy BER string
    for (i = 0; i < 3 && gnss_buff[msgpos + i] != CR; i++)
        ber[i] = gnss_buff[msgpos + i];
    ber[i] = 0;

    return 0;
}

char read_vbat() {
    char timeout;
    int i, msgpos, found = 0;

    // Read data into the buffer
    timeout = command("AT+CBC\r", &OK_FLAG);
    if (timeout)
        return 1;

    // Locate data in buffer
    for (i = 0; !found && i < buff_end - 14; i++) {
        if (gnss_buff[i] == 'C' && gnss_buff[i + 1] == 'B'
                && gnss_buff[i + 2] == 'C' && gnss_buff[i + 3] == ':') {
            msgpos = i + 5;
            found = 1;
        }
    }
    if (!found)
        return 2;

    // Jump over two commas
    for (; msgpos < buff_end && gnss_buff[msgpos] != ','; msgpos++)
        ;
    if (msgpos > buff_end - 7)
        return 3;

    for (msgpos++; msgpos < buff_end && gnss_buff[msgpos] != ','; msgpos++)
        ;
    if (msgpos > buff_end - 5)
        return 4;

    msgpos++;

    // Copy VBAT string
    for (i = 0; i < 4 && gnss_buff[msgpos + i] != CR; i++)
        vbat[i] = gnss_buff[msgpos + i];
    vbat[i] = 0;

    return 0;
}

char read_temp() {
    char timeout;
    int i, msgpos, found = 0;

    // Read data into the buffer
    timeout = command("AT+CMTE?\r", &OK_FLAG);
    if (timeout)
        return 1;

    // Locate data in buffer
    for (i = 0; !found && i < buff_end - 13; i++) {
        if (gnss_buff[i] == 'C' && gnss_buff[i + 1] == 'M'
                && gnss_buff[i + 2] == 'T' && gnss_buff[i + 3] == 'E'
                && gnss_buff[i + 4] == ':') {
            msgpos = i + 6;
            found = 1;
        }
    }
    if (!found)
        return 2;

    // Jump over a comma
    for (; msgpos < buff_end && gnss_buff[msgpos] != ','; msgpos++)
        ;
    if (msgpos > buff_end - 6)
        return 2;
    msgpos++;

    // Copy temperature string
    for (i = 0; i < 5 && gnss_buff[msgpos + i] != CR; i++)
        temp[i] = gnss_buff[msgpos + i];
    temp[i] = 0;

    return 0;
}
