#include "api_os.h"
#include "i2c_oled.h"
#include "api_hal_i2c.h"
#include "codetab.h"


#define OLED_ADDR       0x3c//0x78
#define I2C_OLED I2C2
#define I2C_TIMEOUT         10000

#define OLED_TWI_SCL_PIN		16
#define OLED_TWI_SDA_PIN		15

bool i2c_init()
{
    I2C_Config_t config;

    config.freq = I2C_FREQ_400K;
    return I2C_Init(I2C_OLED, config);
}

void WriteCmd(unsigned char I2C_Command)//д����
{
    I2C_WriteMem(I2C_OLED,OLED_ADDR,0x00,1,&I2C_Command,1,I2C_TIMEOUT);
}

void WriteDat(unsigned char I2C_Data)//д����
{
    I2C_WriteMem(I2C_OLED,OLED_ADDR,0x40,1,&I2C_Data,1,I2C_TIMEOUT);
}

void OLED_Init(void)
{
    OS_Sleep(100); //�������ʱ����Ҫ

    WriteCmd(DISPLAYOFF); //display off
    WriteCmd(MEMORYMODE); //Set Memory Addressing Mode
    WriteCmd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    WriteCmd(0xb0); //Set Page Start Address for Page Addressing Mode,0-7
    WriteCmd(COMSCANINC); //Set COM Output Scan Direction
    WriteCmd(SETLOWCOLUMN);  //---set low column address
    WriteCmd(SETHIGHCOLUMN); //---set high column address
    WriteCmd(SETSTARTLINE);  //--set start line address
    WriteCmd(SETCONTRAST);   //--set contrast control register
    WriteCmd(0x8f); //���ȵ��� 0x00~0xff
    WriteCmd(SEGREMAP);      //--set segment re-map 0 to 127
    WriteCmd(NORMALDISPLAY); //--set normal display
    WriteCmd(SETMULTIPLEX);  //--set multiplex ratio(1 to 64)
    WriteCmd(0x3F); //
    WriteCmd(0xa4); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    WriteCmd(SETDISPLAYOFFSET); //-set display offset
    WriteCmd(0x00); //-not offset
    WriteCmd(SETDISPLAYCLOCKDIV); //--set display clock divide ratio/oscillator frequency
    WriteCmd(0xf0); //--set divide ratio
    WriteCmd(SETPRECHARGE); //--set pre-charge period
    WriteCmd(0x22); //
    WriteCmd(SETCOMPINS); //--set com pins hardware configuration
    WriteCmd(0x02);
    WriteCmd(SETVCOMDETECT); //--set vcomh
    WriteCmd(0x20); //0x20,0.77xVcc
    WriteCmd(CHARGEPUMP); //--set DC-DC enable
    WriteCmd(0x14); //
    WriteCmd(DISPLAYON); //--turn on oled panel
}

void OLED_SetPos(unsigned char x, unsigned char y) //������ʼ������
{
    WriteCmd(0xb0+y);
    WriteCmd(((x&0xf0)>>4)|0x10);
    WriteCmd((x&0x0f)|0x01);
}

void OLED_Fill(unsigned char fill_Data)//ȫ�����
{
    unsigned char m,n;
    for(m=0;m<8;m++)
    {
        WriteCmd(0xb0+m);       //page0-page1
        WriteCmd(0x00);     //low column start address
        WriteCmd(0x10);     //high column start address
        for(n=0;n<128;n++)
        {
            WriteDat(fill_Data);
        }
    }
}

void OLED_CLS(void)//����
{
    OLED_Fill(0x00);
}

void OLED_ON(void)
{
    WriteCmd(0X8D);  //���õ�ɱ�
    WriteCmd(0X14);  //�����ɱ�
    WriteCmd(0XAF);  //OLED����
}

void OLED_OFF(void)
{
    WriteCmd(0X8D);  //���õ�ɱ�
    WriteCmd(0X10);  //�رյ�ɱ�
    WriteCmd(0XAE);  //OLED����
}

void OLED_ShowStr(unsigned char x, unsigned char y, char ch[], unsigned char TextSize)
{
    unsigned char c = 0,i = 0,j = 0;
    switch(TextSize)
    {
      case 1:
        {
            while(ch[j] != '\0')
            {
                c = ch[j] - 32;
                if(x > 126)
                {
                    x = 0;
                    y++;
                }
                OLED_SetPos(x,y);
                for(i=0;i<6;i++)
                    WriteDat(F6x8[c][i]);
                x += 6;
                j++;
            }
        }
        break;

      case 2:
        {
            while(ch[j] != '\0')
            {
                c = ch[j] - 32;
                if(x > 120)
                {
                    x = 0;
                    y++;
                }
                OLED_SetPos(x,y);
                for(i=0;i<8;i++)
                    WriteDat(F8X16[c*16+i]);
                OLED_SetPos(x,y+1);
                for(i=0;i<8;i++)
                    WriteDat(F8X16[c*16+i+8]);
                x += 8;
                j++;
            }
        }
        break;
    }
}
