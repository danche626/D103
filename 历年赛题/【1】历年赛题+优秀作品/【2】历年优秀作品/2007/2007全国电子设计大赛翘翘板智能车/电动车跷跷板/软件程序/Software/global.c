 //计时寄存器
unsigned int time_start_to_bal;    //用于记录从起始点到平衡点附近的时间
unsigned int time_bal_search;      //记录查找平衡的时间
unsigned int time_bal_wait;        //记录平衡等待的时间
unsigned int time_bal_to_end;      //记录平衡点到末尾的时间
unsigned int time_end_wait;        //记录在末尾等待的时间
unsigned int time_end_to_start;    //记录从末尾回到起始点的时间
unsigned int time_next_bal;
unsigned int time_bal_insure;
unsigned int time_bal_search2=0;
unsigned int time_bal_wait2=0;


unsigned int glb_cnt=100;           //全局计数
unsigned char sys_mode;             //系统工作模式 1:基本要求 2:扩展
unsigned char work_mode;            //工作过程中，各阶段工作状态
signed char dir_flag;               //小车方向 1:正走 -1:倒走

signed int yy_avrg;                 //角度
signed int yy;                      //从传感器读出的当前倾角
unsigned char balance_flag;         //用于平衡状态指示
