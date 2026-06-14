#include<msp430x14x.h>
#include"倾角.h"


#define SETB(BIT) P1OUT|=BIT
#define CLRB(BIT) P1OUT&=~BIT

unsigned int SPI_ReadBits( unsigned char n );
void SPI_WriteByte( unsigned char wdata );
void SPI_Start(void);
void SPI_Stop(void);
void SPI_Delay(unsigned int);
signed int SPI_GetX( void );
signed int SPI_GetY( void );


void SPI_Init()
{
  P1DIR|=(CSB+MOSI+SCK);
  P1DIR&=~MISO;
}


void SPI_Stop( void )
{
    CLRB(SCK);
    SETB(CSB);    
//    SPI_Delay(100);
}

void SPI_Delay(unsigned int n)
{
    while(n--);
}

void SPI_Start(void)
{
    CLRB(SCK);
    CLRB(CSB);
//    SPI_Delay(100);
}


//主机写一个BYTE到器件
//最终状态:CLK　=　LOW
void SPI_WriteByte( unsigned char wdata )
{
    unsigned char i;    
    for( i = 0 ; i < 8 ; ++i )
    {
        CLRB(SCK);
        SPI_Delay(10);
        if((wdata & 0x80)!=0)SETB(MOSI);
        else CLRB(MOSI);
        wdata = wdata << 1;
        SPI_Delay(10);
        SETB(SCK);
        SPI_Delay(10);
    }
}


//主机读入N个比特
//n 限制为16及其以下
//结束状态　CLK　=　0
unsigned int SPI_ReadBits( unsigned char n ){
    unsigned int rtemp=0,i;
    for( i = 0 ; i < n ; ++i )
    {
        rtemp <<= 1;
        SETB(SCK);
        SPI_Delay(10);
        CLRB(SCK);
        SPI_Delay(10);
        if((P1IN&MISO)!=0)
        {
            rtemp |= 0x0001;
        }
        else
        {
            rtemp &= 0xFFFE;
        }

    }
    return rtemp;
}

signed int SPI_GetX( void )
{
    unsigned int result;
    SPI_Start();
    SPI_WriteByte( RDAX );
//    SPI_Delay(10000);            //>150US
    result = SPI_ReadBits(11);
    SPI_Stop();
    return (result-1024);
}

signed int SPI_GetY( void )
{
    unsigned int result;
    SPI_Start();
    SPI_WriteByte( RDAY );
//    SPI_Delay(10000);             //>150US
    result = SPI_ReadBits(11);
    SPI_Stop();
    return (result-1024);
}
