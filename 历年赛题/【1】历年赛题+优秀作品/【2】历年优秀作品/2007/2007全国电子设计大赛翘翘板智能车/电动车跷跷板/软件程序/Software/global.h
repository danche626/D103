extern unsigned int glb_cnt;
extern signed char dir_flag;
extern signed int yy_avrg;
extern unsigned char sys_mode;    // 1：基本演示    2：扩展演示
extern signed int yy;                      //从传感器读出的当前倾角
extern unsigned char balance_flag;    //用于平衡状态指示
extern unsigned char work_mode;
#define START_TO_BAL 0      
#define BAL_SEARCH 1
#define BAL_WAIT 2
#define BAL_TO_END 3
#define END_WAIT 4
#define END_TO_START 5
#define OVER 6
#define NEXT_BAL 7

//各模式的计时
extern unsigned int time_start_to_bal;
extern unsigned int time_bal_search;
extern unsigned int time_bal_wait;
extern unsigned int time_bal_to_end;
extern unsigned int time_end_wait;
extern unsigned int time_end_to_start;
extern unsigned int time_next_bal;
extern unsigned int time_bal_insure;
extern unsigned int time_bal_search2;
extern unsigned int time_bal_wait2;
