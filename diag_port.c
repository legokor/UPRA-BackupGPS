#include "diag_port.h"

void soft_uart_write_text(char *t){
     for( ; (*t)!=0; t++) soft_uart_write(*t);
}