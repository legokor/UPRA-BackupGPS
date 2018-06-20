#include "diag_port.h"

void soft_uart_write_text(char *t){
     while(*t != 0){
              soft_uart_write(*t);
              t++;
     }
}