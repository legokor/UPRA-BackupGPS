#ifndef _DIAG_PORT_H
#define _DIAG_PORT_H

#ifdef _DEBUG_MODE
       #define debugstr(x) soft_uart_write_text(x)
       #define debugchr(x) soft_uart_write(x)
       #define debuginttostr(x,y) inttostr(x,y)
#else
     #define debugstr(x)
     #define debugchr(x)
     #define debuginttostr(x,y)
#endif

void soft_uart_write_text(const char* t);

#endif