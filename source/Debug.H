/* ���� */
/* �ṩprintf�ӳ������ʱ���� */

#ifndef __DEBUG_H__
#define __DEBUG_H__

//���庯������ֵ
#ifndef SUCCESS
#define SUCCESS 0
#endif
#ifndef FAIL
#define FAIL 0xFF
#endif

//���嶨ʱ����ʼ
#ifndef START
#define START 1
#endif
#ifndef STOP
#define STOP 0
#endif

//#ifndef  DE_PRINTF
//#define  DE_PRINTF     1
//#endif
#define FREQ_SYS 16000000 //ϵͳ��Ƶ16MHz

void CfgFsys();                         //CH552ʱ��ѡ�������
void mDelayuS(UINT16 n);                // ��uSΪ��λ��ʱ
void mDelaymS(UINT16 n);                // ��mSΪ��λ��ʱ
void CH552UART0Alter();                 //CH552����0����ӳ�䵽P0.2/P0.3
void UART0Setup();                      //T1��Ϊ�����ʷ�����
UINT8 CH552UART0RcvByte();              //CH552 UART0��ѯ��ʽ����һ���ֽ�
void CH552UART0SendByte(UINT8 SendDat); //CH552UART0����һ���ֽ�

void UART1Setup();                      //
UINT8 CH552UART1RcvByte();              //CH552 UART1��ѯ��ʽ����һ���ֽ�
void CH552UART1SendByte(UINT8 SendDat); //CH552UART1����һ���ֽ�

void CH552WatchDog(UINT8 mode);    //CH552���Ź�ģʽ����
void CH552WatchDogFeed(UINT8 tim); //CH552���Ź�ι��
#endif