#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "lz_hardware.h"
#include "Send_Printf.h"
#include "los_task.h"

void Usart_init( unsigned int BaudRate)
{
	unsigned int ret;
	UartAttribute attr;

	HAL_UART_DeInit(UART0);
	LzUartInit(UART0_ID, &attr);
	LzUartDeinit(UART0_ID);

	attr.baudRate = BaudRate;
	attr.dataBits = UART_DATA_BIT_8;
	attr.pad      = FLOW_CTRL_NONE;
	attr.parity   = UART_PARITY_NONE;
	attr.rxBlock  = UART_BLOCK_STATE_NONE_BLOCK;
	attr.stopBits = UART_STOP_BIT_1;
	attr.txBlock  = UART_BLOCK_STATE_NONE_BLOCK;

	PinctrlSet(GPIO0_PB6, MUX_FUNC2, PULL_UP, DRIVE_LEVEL2);   //rx  接su03T的tx	
	PinctrlSet(GPIO0_PB7, MUX_FUNC2, PULL_KEEP, DRIVE_LEVEL2); //tx

	ret = LzUartInit(UART0_ID, &attr);
	if (ret != LZ_HARDWARE_SUCCESS)
	{
		printf("%s, %d: LzUartInit(%d) failed!\n", __FILE__, __LINE__, ret);
	}
}


void su03t_play(uint8_t *msg, int len)
{
    static int init = 0;
    if(init == 1) return;
    init = 1;
    LzUartWrite(UART0_ID, msg, len);
    LOS_Msleep(10);
    init = 0;
}

///**************芯片设置命令*********************/
// unsigned char SYN_StopCom[] = {0xFD, 0X00, 0X02, 0X02, 0XFD}; //停止合成
//  unsigned char SYN_SuspendCom[] = {0XFD, 0X00, 0X02, 0X03, 0XFC}; //暂停合成
// unsigned char SYN_RecoverCom[] = {0XFD, 0X00, 0X02, 0X04, 0XFB}; //恢复合成
///* 返回0x4E表名芯片任在合成播音中，返回0x4F表名芯片处于空闲状态*/
// unsigned char SYN_ChackCom[] = {0XFD, 0X00, 0X02, 0X21, 0XDE}; //状态查询
// unsigned char SYN_PowerDownCom[] = {0XFD, 0X00, 0X02, 0X88, 0X77}; //进入POWER DOWN 状态命令

///* 修改波特率，有3中模式 命令参数的低3位 0x00  0x01 0x02 分别对应9600 ,19200 38400*/
// unsigned char SYN_SetBond[] = {0xFD,0x00,0x03,0x31,0x00,0xCF}; // 修改波特率 9600
////uchar SYN_SetBond[] = {0xFD,0x00,0x03,0x31,0x01,0xCE}; // 修改波特率 19200
////uchar SYN_SetBond[] = {0xFD,0x00,0x03,0x31,0x02,0xCD}; // 修改波特率  38400
//Music:  0:无背景音乐  1~15:选择背景音乐
/* 文本控制标记列表 
设置前景文本播放音量（含提示音效） 全局 [v?] ?为音量值 取值0-16 0 为静音  默认[v10]
设置背景音乐音量 全局 [m?] ? 为音量值，取值0-16  0为静音  默认[m4]
设置词语语速  全局 [t?]  ? 为词语语速值，取值0-5  默认[t4]
设置数字处理策略  全局 [n?] ? 为0，自动判断 为1 数字作号码处理 为2 数字作数值处理  默认[n0]
*/
// SYN_FrameInfo(0, “[v16][m1][t5]你好[n2]123444444”);
// 表示设置文本背景音乐无音量16背景音量1播放你好，数字按数值处理
void SYN_FrameInfo(char Music, char HZdata[], char leng)
{
    /****************需要发送的文本**********************************/
    unsigned char Frame_Info[128];
    unsigned char ecc = 0; //定义校验字节
    unsigned int i = 0;
    // unsigned char HZ_Length,out1[100],len1 ;
    //SwitchToGbk(HZdata,strlen(HZdata),out1,&len1);
    // HZ_Length = strlen((char *)HZdata); //需要发送文本的长度
    //LzUartWrite(UART0_ID, HZdata, leng);
    // /*****************帧固定配置信息**************************************/
    Frame_Info[0] = 0xFD;			   //构造帧头FD
    Frame_Info[1] = 0x00;			   //构造数据区长度的高字节
    Frame_Info[2] = leng + 3;	       //构造数据区长度的低字节
    Frame_Info[3] = 0x01;			   //构造命令字：合成播放命令
    Frame_Info[4] = 0x01 | Music << 4; //构造命令参数：背景音乐设定

    // /*******************校验码计算***************************************/
    for (i = 0; i < 5; i++) //依次发送构造好的5个帧头字节
    {
        ecc = ecc ^ (Frame_Info[i]); //对发送的字节进行异或校验
    }

    for (i = 0; i < leng; i++) //依次发送待合成的文本数据
    {
        ecc = ecc ^ (HZdata[i]); //对发送的字节进行异或校验
    }
    // /*******************发送帧信息***************************************/
    memcpy(&Frame_Info[5], HZdata, leng);
    Frame_Info[5 + leng] = ecc;
    printf("uart:%d\r\n", leng);
    // printf_hex(printf, Frame_Info, 5 + leng + 1);
    LzUartWrite(UART0_ID, Frame_Info, 5 + leng + 1);
}
/* 声音提示音列表  25段声音提示音
 sounda  出错声
 soundb  刷卡成功
 soundc  刷卡成功
 soundd  刷卡成功
 sounde  刷卡成功
 soundf   激光声
 soundg  门铃声
 soundh  门铃声
 soundi   电话铃声
 soundj   电话铃声
 soundk  广播提示音
 soundl   提示音
 soundm  提示音
 soundn   警报
 soundo   警报
 soundp   警报
 soundq  紧急警报
 soundr   紧急警报
 sounds  布谷声
 soundt   提示音
 soundu  提示音
 soundv  提示音
 soundw 提示音
 soundx  提示音
 soundy  提示音

 SYN_FrameInfo(0,"[v16][m1][t5]sounda");
*/
/* 和弦提示音列表  8首
msga   1s
msgb   1s
msgc   1s
msgd   1s
msge   2s
msgf    3s
msgg   4s
msgh   5s
*/

/* 和弦铃声   15首
ringa    60s   //
ringb    70s   //军港之夜
ringc    27s   //玫瑰花
ringd    65a   //绿岛之夜
ringe    60s   //同桌的你
ringf    57s   //
ringg    60s   //
ringh    53s
ringi     35s  //生日快乐
ringj     25s  //梦里水乡
ringk    18s   // 找朋友
ringl     38s  // 挪威森林
ringm   60s    
ringn    23s   //一闪一闪亮晶晶
ringo    5s
SYN_FrameInfo(0,"[v16][m1][t5]ringa");
*/

