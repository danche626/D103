extern void LCD_init();
extern void LCD_clear();
extern void gotoXY(unsigned char x, unsigned char y);
extern void print(unsigned char *str);
extern void print_num(signed int num,unsigned char bitnum);
extern void print_time(unsigned int num);
extern void light_off();
extern void light_on();

extern unsigned char count;
