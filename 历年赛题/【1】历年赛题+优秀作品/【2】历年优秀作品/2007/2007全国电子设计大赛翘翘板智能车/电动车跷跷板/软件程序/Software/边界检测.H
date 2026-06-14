#define FL BIT4    //P5
#define FM BIT3
#define FR BIT2
#define SEL_F BIT1 //P5
#define BL BIT5    //P1
#define BM BIT6
#define BR BIT7    //P1
#define SEL_B BIT1 //P3

extern void sensor_init();
extern void sel_on();
extern void sel_off();
extern void sensor_scan();
extern signed char fr_flag,fl_flag,br_flag,bl_flag;

extern signed char fl_state[5],fr_state[5],bl_state[5],br_state[5];    //存放四角近5次数据
