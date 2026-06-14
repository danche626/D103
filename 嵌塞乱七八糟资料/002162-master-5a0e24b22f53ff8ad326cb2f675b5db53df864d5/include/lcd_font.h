/*
 * Copyright (c) 2021 LOCKZHINER Electronic Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _LCD_FONT_H_
#define _LCD_FONT_H_

#define ARRAY_CNT(x)  sizeof(x)/sizeof(x[0])

/* 定义中文字符 12*12 */
typedef struct
{
    unsigned char Index[3];
    unsigned char Msk[24];
} typFNT_GB12;

/* 定义中文字符 16*16 */
typedef struct
{
    unsigned char Index[3];
    unsigned char Msk[32];
} typFNT_GB16;

/* 定义中文字符 24*24 */
typedef struct
{
    unsigned char Index[3];
    unsigned char Msk[72];
} typFNT_GB24;


/* 定义中文字符 32*32 */
typedef struct
{
    unsigned char Index[3];
    unsigned char Msk[128];
} typFNT_GB32;

/* 定义中文字符 48*48 */
typedef struct
{
    unsigned char Index[3];
    unsigned char Msk[288];
} typFNT_GB48;



extern const unsigned char ascii_1206[][12];
extern const unsigned char ascii_1608[][16];
extern const unsigned char ascii_2412[][48];
extern const unsigned char ascii_3216[][64];
extern const unsigned char ascii_4824[][144];
extern const typFNT_GB16 tfont16[];
extern const typFNT_GB12 tfont12[];
extern const typFNT_GB24 tfont24[];
extern const typFNT_GB32 tfont32[];
extern const typFNT_GB48 tfont48[];
extern unsigned int tfont12_num;
extern unsigned int  tfont16_num;
extern unsigned int  tfont24_num;
extern unsigned int  tfont32_num;
extern unsigned int  tfont48_num;
#endif
