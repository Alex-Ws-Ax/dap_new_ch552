
/********************************** (C) COPYRIGHT *******************************
* File Name          :CompatibilityHID.C
* Author             : WCH
* Version            : V1.2
* Date               : 2018/02/28
* Description        : CH554ģ��HID�����豸��֧���ж����´���֧�ֿ��ƶ˵����´���֧������ȫ�٣�����
*******************************************************************************/

#include "CH552.H"
#include "Debug.H"
#include "DAP.h"
#include "Uart.H"

#define Fullspeed 1
#define WINUSB 1
#define THIS_ENDP0_SIZE 64

UINT8X Ep0Buffer[THIS_ENDP0_SIZE] _at_ 0x0000;  //�˵�0 OUT&IN��������������ż��ַ
UINT8X Ep1BufferO[THIS_ENDP0_SIZE] _at_ 0x0040; //�˵�1 OUT˫������,������ż��ַ Not Change!!!!!!
UINT8X Ep1BufferI[THIS_ENDP0_SIZE] _at_ 0x0080; //�˵�1 IN˫������,������ż��ַ Not Change!!!!!!

//100,140,180,1C0
UINT8X Ep2BufferO[4 * THIS_ENDP0_SIZE] _at_ 0x0100; //�˵�2 OUT˫������,������ż��ַ
//200,240,280,2C0
UINT8X Ep3BufferI[4 * THIS_ENDP0_SIZE] _at_ 0x0200; //�˵�3 IN˫������,������ż��ַ


//UINT8I Endp1Busy;
//UINT8I USBByteCount = 0;       //����USB�˵���յ�������
//UINT8I USBBufOutPoint = 0;     //ȡ����ָ��
extern BOOL UART_TX_BUSY;
extern BOOL EP1_TX_BUSY;
//UINT8I UartByteCount = 0;      //��ǰ������ʣ���ȡ�ֽ���
//UINT8I Uart_Input_Point = 0;   //ѭ��������д��ָ�룬���߸�λ��Ҫ��ʼ��Ϊ0
//UINT8I Uart_Output_Point = 0;  //ѭ��������ȡ��ָ�룬���߸�λ��Ҫ��ʼ��Ϊ0
BOOL DAP_LED_BUSY;

UINT8I Ep2Oi, Ep2Oo;            //OUT ����
UINT8I Ep3Ii, Ep3Io;            //IN ����
UINT8I Ep3Is[DAP_PACKET_COUNT]; //���Ͱ���

PUINT8 pDescr; //USB���ñ�־
UINT8I Endp3Busy = 0;
UINT8I SetupReq, SetupLen, Ready, Count, UsbConfig;
#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

#if (WINUSB == 1)
UINT8C DevDesc[] =
{
    0x12, 0x01, 0x10, 0x02, 0xEF, 0x02, 0x01, THIS_ENDP0_SIZE,
    0x28, 0x0D, 0x04, 0x02, 0x00, 0x01, 0x01, 0x02,
    0x03, 0x01
};
UINT8C CfgDesc[] =
{
    0x09, 0x02, 0x62, 0x00, 0x03, 0x01, 0x00, 0x80, 0xfa, //����������
    //DAP
    0x09, 0x04, 0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x04, //�ӿ�������

    0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00, //�˵�������
    0x07, 0x05, 0x83, 0x02, 0x40, 0x00, 0x00,
    //CDC
    0x08, 0x0b, 0x02, 0x02, 0x02, 0x02, 0x01, 0x05,
    0x09, 0x04, 0x02, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,
    0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,
    0x09, 0x04, 0x03, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,
    0x05, 0x24, 0x01, 0x00, 0x01,
    0x04, 0x24, 0x02, 0x02,
    0x05, 0x24, 0x06, 0x03, 0x02,
    0x07, 0x05, 0x84, 0x03, 0x08, 0x00, 0xFF,
};
#else
#Err no USB interface define
#endif

UINT16I USB_STATUS = 0;
//cdc����
UINT8I LineCoding[7] = {0x00, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x08}; //��ʼ��������Ϊ57600��1ֹͣλ����У�飬8����λ��

/*�ַ��������� ��*/
// ����������
UINT8C MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// ������Ϣ:ARM
UINT8C MyManuInfo[] = {0x08, 0x03, 'A', 0x00, 'R', 0x00, 'M', 0x00};
// ��Ʒ��Ϣ: DAP-Link-II
UINT8C MyProdInfo[] =
{
    36,
    0x03,
    'D', 0, 'A', 0, 'P', 0, 'L', 0, 'i', 0, 'n', 0, 'k', 0, ' ', 0,
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0,
    'P', 0
};
// ���к�: 3V3-IO-12345
UINT8C MySerNumber[] =
{
    0x1A,
    0x03,
    '3', 0, 'V', 0, '3', 0, '-', 0, 'I', 0, 'O', 0, '-', 0, '1', 0,
    '2', 0, '3', 0, '4', 0, '5', 0
};
// �ӿ�: CMSIS-DAP
UINT8C MyInterface[] =
{
    26,
    0x03,
    'C', 0, 'M', 0, 'S', 0, 'I', 0, 'S', 0, '-', 0, 'D', 0, 'A', 0,
    'P', 0, ' ', 0, 'v', 0, '2', 0
};
//CDC
UINT8C CDC_String[] =
{
    30,
    0x03,
    'D', 0, 'A', 0, 'P', 0, 'L', 0, 'i', 0, 'n', 0, 'k', 0, '-', 0, 'C', 0, 'D', 0, 'C', 0, 'E', 0, 'x', 0, 't', 0
};

UINT8C USB_BOSDescriptor[] =
{
    0x05,                                      /* bLength */
    0x0F,                                      /* bDescriptorType */
    0x28, 0x00,                                /* wTotalLength */
    0x02,                                      /* bNumDeviceCaps */
    0x07, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x1C,                                      /* bLength */
    0x10,                                      /* bDescriptorType */
    0x05,                                      /* bDevCapabilityType */
    0x00,                                      /* bReserved */
    0xDF, 0x60, 0xDD, 0xD8,                    /* PlatformCapabilityUUID */
    0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D,
    0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06,    /* >= Win 8.1 *//* dwWindowsVersion*/
    0xAA, 0x00,                                /* wDescriptorSetTotalLength */
    0x20,                                      /* bVendorCode */
    0x00                                       /* bAltEnumCode */
};

UINT8C WINUSB_Descriptor[] =
{
    0x0A, 0x00,                                 /* wLength */
    0x00, 0x00,                                 /* wDescriptorType */
    0x00, 0x00, 0x03, 0x06,                     /* dwWindowsVersion*/
    0xAA, 0x00,                                 /* wDescriptorSetTotalLength */
    /* ... */
    0x08, 0x00,
    0x02, 0x00,
    0x00, 0x00,
    0xA0, 0x00,
    /* ... */
    0x14, 0x00,                                 /* wLength */
    0x03, 0x00,                                 /* wDescriptorType */
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,         /* CompatibleId*/
    0, 0, 0, 0, 0, 0, 0, 0,                     /* SubCompatibleId*/
    0x84, 0x00,                                 /* wLength */
    0x04, 0x00,                                 /* wDescriptorType */
    0x07, 0x00,                                 /* wPropertyDataType */
    0x2A, 0x00,                                 /* wPropertyNameLength */
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
    'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    0x50, 0x00,                                 /* wPropertyDataLength */
    '{', 0,
    'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0,
    '2', 0, '9', 0, '3', 0, 'B', 0, '-', 0,
    '4', 0, '6', 0, '6', 0, '3', 0, '-', 0,
    'A', 0, 'A', 0, '3', 0, '6', 0, '-',
    0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0, '6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0,
    '}', 0, 0, 0, 0, 0,
};


void Config_Uart1(UINT8 *cfg_uart);
void Config_Uart0(UINT8 *cfg_uart);

/*******************************************************************************
* Function Name  : USBDeviceInit()
* Description    : USB�豸ģʽ����,�豸ģʽ�������շ��˵����ã��жϿ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceInit()
{
    IE_USB = 0;
    USB_CTRL = 0x00;        // ���趨USB�豸ģʽ
    UDEV_CTRL = bUD_PD_DIS; // ��ֹDP/DM��������
#ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED; //ѡ�����1.5Mģʽ
    USB_CTRL |= bUC_LOW_SPEED;
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED; //ѡ��ȫ��12Mģʽ��Ĭ�Ϸ�ʽ
    USB_CTRL &= ~bUC_LOW_SPEED;
#endif

    UEP0_DMA = Ep0Buffer;                                      //�˵�0���ݴ����ַ
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                //�˵�0��64�ֽ��շ�������
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                 //OUT���񷵻�ACK��IN���񷵻�NAK

    UEP1_DMA = Ep1BufferO;                                     //�˵�1���ݴ����ַ
    UEP4_1_MOD |= bUEP1_TX_EN | bUEP1_RX_EN;                   //�˵�1���ͽ���ʹ��
    UEP4_1_MOD &= ~bUEP1_BUF_MOD;                              //�˵�1�շ���64�ֽڻ�����
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; //�˵�1�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK

    UEP2_DMA = Ep2BufferO;                                     //�˵�2���ݴ����ַ
    UEP3_DMA = Ep3BufferI;                                     //�˵�2���ݴ����ַ
    UEP2_3_MOD |= (bUEP3_TX_EN | bUEP2_RX_EN);                   //�˵�2���ͽ���ʹ��
    UEP2_3_MOD &= ~(bUEP2_BUF_MOD | bUEP3_BUF_MOD);            //�˵�2�շ���64�ֽڻ�����
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; //�˵�2�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_NAK;//�˵�3�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����NACK

    USB_DEV_AD = 0x00;
    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // ����USB�豸��DMA�����ж��ڼ��жϱ�־δ���ǰ�Զ�����NAK
    UDEV_CTRL |= bUD_PORT_EN;                              // ����USB�˿�
    USB_INT_FG = 0xFF;                                     // ���жϱ�־
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}
/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB�жϴ�������
*******************************************************************************/
void DeviceInterrupt(void) interrupt INT_NO_USB using 1 //USB�жϷ������,ʹ�üĴ�����1
{
    UINT8 len;
    if (UIF_TRANSFER) //USB������ɱ�־
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_OUT | 2: //endpoint 2# �˵������´� DAP-CMD
            if (U_TOG_OK)         // ��ͬ�������ݰ�������
            {
                Ep2Oi += 64;
                UEP2_DMA_L = Ep2Oi;
            }
            break;

        case UIS_TOKEN_IN | 3: //endpoint 3# �˵������ϴ� DAP_ASK
            Endp3Busy = 0;
            UEP3_T_LEN = 0;      //Ԥʹ�÷��ͳ���һ��Ҫ���
            UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; //Ĭ��Ӧ��NAK
            break;

        case UIS_TOKEN_IN | 1: //endpoint 1# �˵������ϴ� CDC
            UEP1_T_LEN = 0;      //Ԥʹ�÷��ͳ���һ��Ҫ���
            //Endp1Busy = 0;
						EP1_TX_BUSY = FALSE;
            UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; //Ĭ��Ӧ��NAK
            break;
        case UIS_TOKEN_OUT | 1: //endpoint 1# �˵������´� CDC
            if (U_TOG_OK)         // ��ͬ�������ݰ�������
            {
                //USBByteCount = USB_RX_LEN;
                //USBBufOutPoint = 0;                                             //ȡ����ָ�븴λ
                //UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_NAK;       //�յ�һ�����ݾ�NAK�������������꣬���������޸���Ӧ��ʽ
							UART_Get_USB_Data(USB_RX_LEN);
            }
            break;

        case UIS_TOKEN_SETUP | 0: //SETUP����
            len = USB_RX_LEN;
            if (len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = UsbSetupBuf->wLengthL;
                if (UsbSetupBuf->wLengthH)
                    SetupLen = 0xFF; // �����ܳ���
                len = 0;           // Ĭ��Ϊ�ɹ������ϴ�0����
                SetupReq = UsbSetupBuf->bRequest;
                switch (UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK)
                {
                case USB_REQ_TYP_STANDARD:
                    switch (SetupReq) //������
                    {
                    case USB_GET_DESCRIPTOR:
                        switch (UsbSetupBuf->wValueH)
                        {
                        case 1:             //�豸������
                            pDescr = DevDesc; //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(DevDesc);
                            break;
                        case 2:             //����������
                            pDescr = CfgDesc; //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(CfgDesc);
                            break;
                        case 3: // �ַ���������
                            switch (UsbSetupBuf->wValueL)
                            {
                            case 0:
                                pDescr = (PUINT8)(&MyLangDescr[0]);
                                len = sizeof(MyLangDescr);
                                break;
                            case 1:
                                pDescr = (PUINT8)(&MyManuInfo[0]);
                                len = sizeof(MyManuInfo);
                                break;
                            case 2:
                                pDescr = (PUINT8)(&MyProdInfo[0]);
                                len = sizeof(MyProdInfo);
                                break;
                            case 3:
                                pDescr = (PUINT8)(&MySerNumber[0]);
                                len = sizeof(MySerNumber);
                                break;
                            case 4:
                                pDescr = (PUINT8)(&MyInterface[0]);
                                len = sizeof(MyInterface);
                                break;
                            case 5:
                                pDescr = (PUINT8)(&CDC_String[0]);
                                len = sizeof(CDC_String);
                                break;
                            default:
                                len = 0xFF; // ��֧�ֵ��ַ���������
                                break;
                            }
                            break;
                        case 15:
                            pDescr = (PUINT8)(&USB_BOSDescriptor[0]);
                            len = sizeof(USB_BOSDescriptor);
                            break;
                        default:
                            len = 0xff; //��֧�ֵ�������߳���
                            break;
                        }
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL; //�ݴ�USB�豸��ַ
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if (SetupLen >= 1)
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        if (UsbConfig)
                        {
                            Ready = 1; //set config����һ�����usbö����ɵı�־
                        }
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                                       //Clear Feature
                        if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // �˵�
                        {
                            switch (UsbSetupBuf->wIndexL)
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF; // ��֧�ֵĶ˵�
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF; // ���Ƕ˵㲻֧��
                        }
                        break;
                    case USB_SET_FEATURE:                             /* Set Feature */
                        if ((UsbSetupBuf->bRequestType & 0x1F) == 0x00) /* �����豸 */
                        {
                            if ((((UINT16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01)
                            {
                                if (CfgDesc[7] & 0x20)
                                {
                                    /* ���û���ʹ�ܱ�־ */
                                }
                                else
                                {
                                    len = 0xFF; /* ����ʧ�� */
                                }
                            }
                            else
                            {
                                len = 0xFF; /* ����ʧ�� */
                            }
                        }
                        else if ((UsbSetupBuf->bRequestType & 0x1F) == 0x02) /* ���ö˵� */
                        {
                            if ((((UINT16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00)
                            {
                                switch (((UINT16)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL)
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* ���ö˵�2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* ���ö˵�2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* ���ö˵�1 IN STALL */
                                    break;
                                default:
                                    len = 0xFF; /* ����ʧ�� */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF; /* ����ʧ�� */
                            }
                        }
                        else
                        {
                            len = 0xFF; /* ����ʧ�� */
                        }
                        break;
                    case USB_GET_STATUS:
                        pDescr = (PUINT8)&USB_STATUS;
                        if (SetupLen >= 2)
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff; //����ʧ��
                        break;
                    }

                    break;
                case USB_REQ_TYP_CLASS: /*HID������*/
                    if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_INTERF)
                    {
                        switch (SetupReq)
                        {
                        case 0x20://Configure
                            break;
                        case 0x21://currently configured
                            pDescr = LineCoding;
                            len = sizeof(LineCoding);
                            break;
                        case 0x22://generates RS-232/V.24 style control signals
                            break;
                        default:
                            len = 0xFF; /*���֧��*/
                            break;
                        }
                    }
                    break;
                case USB_REQ_TYP_VENDOR:
                    if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                    {
                        switch (SetupReq)
                        {
                        case 0x20:                         //GetReport
                            if (UsbSetupBuf->wIndexL == 0x07)
                            {
                                pDescr = WINUSB_Descriptor; //���豸�������͵�Ҫ���͵Ļ�����
                                len = sizeof(WINUSB_Descriptor);
                            }
                            break;
                        default:
                            len = 0xFF; /*���֧��*/
                            break;
                        }
                    }
                    break;
                default:
                    len = 0xFF;
                    break;
                }
                if (len != 0 && len != 0xFF)
                {
                    if (SetupLen > len)
                    {
                        SetupLen = len; //�����ܳ���
                    }
                    len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //���δ��䳤��
                    memcpy(Ep0Buffer, pDescr, len);                                 //�����ϴ�����
                    SetupLen -= len;
                    pDescr += len;
                }
            }
            else
            {
                len = 0xff; //�����ȴ���
            }
            if (len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; //STALL
            }
            else if (len <= THIS_ENDP0_SIZE) //�ϴ����ݻ���״̬�׶η���0���Ȱ�
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //Ĭ�����ݰ���DATA1������Ӧ��ACK
            }
            else
            {
                UEP0_T_LEN = 0;                                                      //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //Ĭ�����ݰ���DATA1,����Ӧ��ACK
            }
            break;
        case UIS_TOKEN_IN | 0: //endpoint0 IN
            switch (SetupReq)
            {
            case USB_GET_DESCRIPTOR:
            case 0x20:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //���δ��䳤��
                memcpy(Ep0Buffer, pDescr, len);                                 //�����ϴ�����
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG; //ͬ����־λ��ת
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0; //״̬�׶�����жϻ�����ǿ���ϴ�0�������ݰ��������ƴ���
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0: // endpoint0 OUT
            len = USB_RX_LEN;
            if (SetupReq == 0x20) //���ô�������
            {
                if (U_TOG_OK)
                {
                    memcpy(LineCoding, UsbSetupBuf, USB_RX_LEN);
                    
#if USE_UART0 == 1	
									Config_Uart0(LineCoding);
#elif	USE_UART1 == 1
									Config_Uart1(LineCoding);
#endif
                    UEP0_T_LEN = 0;
                    UEP0_CTRL |= UEP_R_RES_ACK | UEP_T_RES_ACK;  // ׼���ϴ�0��
                }
            }
            else if (SetupReq == 0x09)
            {
                if (Ep0Buffer[0])
                {
                }
                else if (Ep0Buffer[0] == 0)
                {
                }
            }
            UEP0_CTRL ^= bUEP_R_TOG; //ͬ����־λ��ת
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0; //д0����ж�
    }
    if (UIF_BUS_RST) //�豸ģʽUSB���߸�λ�ж�
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        Endp3Busy = 0;
        UIF_BUS_RST = 0; //���жϱ�־
    }
    if (UIF_SUSPEND) //USB���߹���/�������
    {
        UIF_SUSPEND = 0;
        if (USB_MIS_ST & bUMS_SUSPEND) //����
        {
        }
    }
    else
    {
        //������ж�,�����ܷ��������
        USB_INT_FG = 0xFF; //���жϱ�־
    }
}


typedef void( *goISP)( void );
goISP ISP_ADDR=0x3800;	
UINT16 LED_Timer;

void main(void)
{
    UINT8 Uart_Timeout = 0;

    CfgFsys();   //CH559ʱ��ѡ������
    mDelaymS(5); //�޸���Ƶ�ȴ��ڲ������ȶ�,�ؼ�
   
    USBDeviceInit(); //USB�豸ģʽ��ʼ��
		UART_Setup();
	
    EA = 1;          //������Ƭ���ж�
    UEP1_T_LEN = 0;  //Ԥʹ�÷��ͳ���һ��Ҫ���
    UEP2_T_LEN = 0;  //Ԥʹ�÷��ͳ���һ��Ҫ���
    Ready = 0;

    Ep2Oi = 0;
    Ep2Oo = 0;
    Ep3Ii = 0;
    Ep3Io = 0;
    Endp3Busy = 0;
		DAP_LED_BUSY = 0;
		LED_Timer = 0;

    while (!UsbConfig) {;};

    while (1)
    {
        DAP_Thread();

        if (Endp3Busy != 1 && Ep3Ii != Ep3Io)
        {
            Endp3Busy = 1;
            UEP3_T_LEN = Ep3Is[0];//Ep3Io>>6];
            UEP3_DMA_L = Ep3Io;
            
            UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; //������ʱ�ϴ����ݲ�Ӧ��ACK
            Ep3Io += 64;
        }
				if(DAP_LED_BUSY)
				{
					LED = 0;
					LED_Timer = 0;
				}
				else
				{
					LED_Timer++;
					if(((UINT8*)&LED_Timer)[0]==0x10)
					{
						LED = 1;
					}							
					if(((UINT8*)&LED_Timer)[0]==0xC0)
					{
						LED_Timer = 0;
						LED = 0;
					}			
				}

				//Uart_Main_Service();
//        if (USBByteCount)
//        {
//            //CH552UART1SendByte(Ep1BufferO[USBBufOutPoint++]);
//						SBUF = Ep1BufferO[USBBufOutPoint++];	
//						while(TI == 0);
//						TI = 0;											
//            USBByteCount--;
//            if (USBByteCount == 0)
//                UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_R_RES | UEP_R_RES_ACK;
//        }
//        if (UartByteCount)
//        {
//            Uart_Timeout++;
//            if (UartByteCount > 39 || Uart_Timeout > 100)
//            {
//                if (!Endp1Busy)
//                {
//                    Uart_Timeout = 0;
//                    UEP1_T_LEN = UartByteCount;                                                    //Ԥʹ�÷��ͳ���һ��Ҫ���
//                    UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;            //Ӧ��ACK
//                    Endp1Busy = 1;
//                    UartByteCount = 0;
//                    Uart_Input_Point = 0;
//                }
//            }
//        }
				if(CLOCK_CFG&bRST)//isp
				{
					USB_CTRL=0;
					UDEV_CTRL=0x80;
					mDelaymS(10);			
					(ISP_ADDR)();
				}				
						
    }
}

void Config_Uart1(UINT8 *cfg_uart)
{
    UINT32 uart1_buad = 0;
    *((UINT8 *)&uart1_buad) = cfg_uart[3];
    *((UINT8 *)&uart1_buad + 1) = cfg_uart[2];
    *((UINT8 *)&uart1_buad + 2) = cfg_uart[1];
    *((UINT8 *)&uart1_buad + 3) = cfg_uart[0];
    IE_UART1 = 0;
//    SBAUD1 = 0 - (FREQ_SYS+UART_BUAD*8) / 16 / uart1_buad;
		SBAUD1 = 256 - FREQ_SYS/16/uart1_buad;
    IE_UART1 = 1;
}

//void Uart1_ISR(void) interrupt INT_NO_UART1
//{
//    if (U1RI)  //�յ�����
//    {
//        Ep1BufferI[Uart_Input_Point++] = SBUF1;
//        UartByteCount++;                    //��ǰ������ʣ���ȡ�ֽ���
//        if (Uart_Input_Point >= THIS_ENDP0_SIZE)
//            Uart_Input_Point = 0;           //д��ָ��
//        U1RI = 0;
//    }
//}
void Config_Uart0(UINT8 *cfg_uart)
{
    UINT32 uart0_buad = 0;
    *((UINT8 *)&uart0_buad) = cfg_uart[3];
    *((UINT8 *)&uart0_buad + 1) = cfg_uart[2];
    *((UINT8 *)&uart0_buad + 2) = cfg_uart[1];
    *((UINT8 *)&uart0_buad + 3) = cfg_uart[0];
    ES = 0;
		TH1 = 256 - FREQ_SYS/16/uart0_buad;
//    TH1 = 0 - ((FREQ_SYS+8*uart0_buad) / 16 / uart0_buad);
    ES = 1;
}

//void Uart0_ISR(void) interrupt INT_NO_UART0
//{
//    if (RI)  //�յ�����
//    {
//        Ep1BufferI[Uart_Input_Point++] = SBUF;
//        UartByteCount++;                    //��ǰ������ʣ���ȡ�ֽ���
//        if (Uart_Input_Point >= THIS_ENDP0_SIZE)
//            Uart_Input_Point = 0;           //д��ָ��
//        RI = 0;
//    }
//}