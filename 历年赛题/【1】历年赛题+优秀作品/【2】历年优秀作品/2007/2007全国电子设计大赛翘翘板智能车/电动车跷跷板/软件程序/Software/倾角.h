//spi.h

extern void SPI_Init();
extern signed int SPI_GetX( void );
extern signed int SPI_GetY( void ); 

#define MEAS 0x00 //测量模式
#define RWTR 0x08 //读写温度数据寄存器
#define STX  0x0E //x通道自检
#define STY  0x0F //Y通道自检
#define RDAX 0x10 //读Ｘ通道加速度值
#define RDAY 0x11 //读Ｙ通道加速度值

#define SCK  BIT1
#define MISO BIT4
#define MOSI BIT0
#define CSB  BIT2
