#ifndef __UART_H__
#define __UART_H__

	#define USE_UART0 0
	#define USE_UART1 1
	
	#ifndef UART_BUAD
		#define UART_BUAD 115200
	#endif


void UART_Setup(void);
void UART_Get_USB_Data(UINT8 Nums);



#endif
