#include"global.h"
#include"LCD1602.H"
#include"边界检测.h"

void display()
{
  
  LCD_clear();
  /*  if(fl_flag!=0)          //指示边界状态
   {
    gotoXY(0,0);
    print("FL");            //前左
   }
   if(fr_flag!=0)
   {
    gotoXY(0,14);
    print("FR");            //前右
   }
    if(bl_flag!=0)
   {
    gotoXY(1,0);
    print("BL");            //后左
   }
   if(br_flag!=0)
   {
    gotoXY(1,14);
    print("BR");            //后右
   } 
*/

 switch(work_mode)       //根据不同模式进行显示
 {
   
  case START_TO_BAL:                       //从起始点到平衡点附近，显示"A_TO_C",计时
   gotoXY(0,4);
   print("A_TO_C");
   gotoXY(1,6);
   print_time(time_start_to_bal);
  break;
  
  case BAL_SEARCH:                        //在起始点附近进入查找平衡模式，显示"C_SEA",查找时间，当前角度，上一阶段的时间
   gotoXY(0,3);
   print("C_SEA");
   gotoXY(0,9);
   print_num(yy_avrg,3);
   gotoXY(1,4);
   print_time(time_start_to_bal);
   gotoXY(1,8);
   print_time(time_bal_search);
  break;
  
  case BAL_WAIT:                          //到达平衡点，显示"BAL",平衡等待时间,平衡角度，平衡查找时间
   gotoXY(0,4);
   print("BAL");
   gotoXY(0,9);
   print_num(yy_avrg,3);
   gotoXY(1,4);
   print_time(time_bal_search);
   gotoXY(1,8);
   print_time(time_bal_wait);
  break;
  
  case BAL_TO_END:                        //从平衡点到末尾，显示 "C TO B",计时
   gotoXY(0,4);
   print("C_TO_B");
   gotoXY(1,3);
   print_time(time_bal_wait);
   gotoXY(1,8);
   print_time(time_bal_to_end);
  break;
  
  case END_WAIT:                           //到达末尾，显示"B_WT"，等待时间，上阶段时间
   gotoXY(0,5);
   print("B_WT");
   gotoXY(1,3);
   print_time(time_bal_to_end);
   gotoXY(1,8);
   print_time(time_end_wait);
  break;
  
  case END_TO_START:                       //从末尾到起始点，显示"B_TO_A",计时，上阶段时间
   gotoXY(0,4);
   print("B_T0_A");
   gotoXY(1,3);
   print_time(time_end_wait);
   gotoXY(1,8);
   print_time(time_end_to_start);
  break;
  
  case OVER:                               //全过程结束，显示"OVER"，上阶段时间和总时间
   gotoXY(0,4);
   print("OVER:");
   gotoXY(0,9);
   print_time(time_end_to_start);
   gotoXY(1,2);   
   print("Total:");   
   gotoXY(1,9);
   print_time((time_bal_wait2+time_bal_search2+time_start_to_bal+time_bal_search+time_bal_wait+time_bal_to_end+time_end_wait+time_end_to_start)/10);
   gotoXY(1,13);   
   print("sec");   

  break;
  
  case NEXT_BAL:                           //发挥要求，等待放置重物
   gotoXY(0,2);
   print("Place Object");
   gotoXY(1,2);
   print("In Three Sec.");
  break;
  
  default:break;
 }
}
