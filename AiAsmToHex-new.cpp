#include "AiAsmToHex-new.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>

std::string debug_pause_str = "";
int debug_pause_num = -1;

// 段寄存器前缀映射表
const std::map<std::string, unsigned char> segmentPrefixes = {
	{"ES", PREFIX_ES}, {"CS", PREFIX_CS}, {"SS", PREFIX_SS},
	{"DS", PREFIX_DS}, {"FS", PREFIX_FS}, {"GS", PREFIX_GS}
};

// 指令编码表
const t_cmddata cmddata[] = {
	//----------------------------------------以下区分RAC专用模板和通用模板的指令
		// CMP指令(1000+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x00003C,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"CMP",1000,1,false,0,true,1,false }, // CMP RAC8,imm8
		{ 0x0000FF,0x00003D,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"CMP",1001,1,false,0,true,4,false }, // CMP RAC16,imm16
		{ 0x0000FF,0x00003D,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"CMP",1002,1,false,0,true,4,false }, // CMP RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x000038,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"CMP",1003,1,true,0,false,0,false }, // CMP r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"CMP",1004,1,true,7,true,1,false }, // CMP r8,imm8
		{ 0x0000FE,0x000038,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"CMP",1005,1,true,0,false,0,false }, // CMP m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"CMP",1006,1,true,7,true,1,false }, // CMP m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"CMP",1007,1,true,7,true,1,false }, // CMP r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"CMP",1008,1,true,7,true,1,false }, // CMP m32,imm8

		{ 0x0000FE,0x00003B,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"CMP",1009,1,true,0,false,0,false }, // CMP r16/32,m16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"CMP",1010,1,true,7,true,0,false }, // CMP r16/32,imm16/32
		{ 0x0000FE,0x000039,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"CMP",1011,1,true,0,false,0,false }, // CMP m16/32,r16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"CMP",1012,1,true,7,true,0,false }, // CMP m16/32,imm16/32

		// MOV指令 (1100+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x0000B0,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"MOV",1100,1,false,0,true,1,false }, // MOV RAC8,imm8
		{ 0x0000FF,0x0000B8,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"MOV",1101,1,false,0,true,4,false }, // MOV RAC16,imm16
		{ 0x0000FF,0x0000B8,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"MOV",1102,1,false,0,true,4,false }, // MOV RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x00008A,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"MOV",1103,1,true,0,false,0,false }, // MOV r8,m8
		{ 0x0000F8,0x0000B0,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"MOV",1104,1,false,0,true,1,false }, // MOV r8,imm8
		{ 0x0000FE,0x000088,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"MOV",1105,1,true,0,false,0,false }, // MOV m8,r8
		{ 0x0000FF,0x0000C6,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"MOV",1106,1,true,0,true,1,false }, // MOV m8,imm8

		{ 0x0000FF,0x0000B8,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"MOV",1107,1,false,0,true,4,false }, // MOV r32, imm32
		{ 0x0000FF,0x0000C7,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"MOV",1108,1,true,0,true,4,false }, // MOV m32,imm8

		{ 0x0000FE,0x00008B,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"MOV",1109,1,true,0,false,0,false }, // MOV r16,m16
		{ 0x0000FF,0x0000C7,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"MOV",1110,1,true,0,true,0,false }, // MOV r16,imm16
		{ 0x0000FE,0x000089,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"MOV",1111,1,true,0,false,0,false }, // MOV m16,r16
		{ 0x0000FF,0x0000C7,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"MOV",1112,1,true,0,true,0,false }, // MOV m16,imm16

		// XOR指令 (1200+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x000034,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"XOR",1200,1,false,0,true,1,false }, // XOR RAC8,imm8
		{ 0x0000FF,0x000035,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"XOR",1201,1,false,0,true,4,false }, // XOR RAC16,imm16
		{ 0x0000FF,0x000035,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"XOR",1202,1,false,0,true,4,false }, // XOR RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x000032,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"XOR",1203,1,true,0,false,0,false }, // XOR r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"XOR",1204,1,true,6,true,1,false }, // XOR r8,imm8
		{ 0x0000FE,0x000030,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"XOR",1205,1,true,0,false,0,false }, // XOR m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"XOR",1206,1,true,6,true,1,false }, // XOR m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"XOR",1207,1,true,6,true,1,false }, // XOR r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"XOR",1207,1,true,6,true,1,false }, // XOR m32,imm8


		{ 0x0000FE,0x000033,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"XOR",1208,1,true,0,false,0,false }, // XOR r16/32,m16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"XOR",1209,1,true,6,true,0,false }, // XOR r16/32,imm16/32
		{ 0x0000FE,0x000031,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"XOR",1210,1,true,0,false,0,false }, // XOR m16/32,r16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"XOR",1211,1,true,6,true,0,false }, // XOR m16/32,imm16/32


		// ADD指令 (1300+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x000004,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"ADD",1300,1,false,0,true,1,false }, // ADD RAC8,imm8
		{ 0x0000FF,0x000005,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"ADD",1301,1,false,0,true,4,false }, // ADD RAC16,imm16
		{ 0x0000FF,0x000005,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"ADD",1302,1,false,0,true,4,false }, // ADD RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x000002,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"ADD",1303,1,true,0,false,0,false }, // ADD r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"ADD",1304,1,true,0,true,1,false }, // ADD r8,imm8
		{ 0x0000FE,0x000000,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"ADD",1305,1,true,0,false,0,false }, // ADD m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"ADD",1306,1,true,0,true,1,false }, // ADD m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"ADD",1307,1,true,0,true,1,false }, // ADD r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"ADD",1308,1,true,0,true,1,false }, // ADD m32,imm8

		{ 0x0000FE,0x000003,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"ADD",1309,1,true,0,false,0,false }, // ADD r16/32,m16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"ADD",1310,1,true,0,true,0,false }, // ADD r16/32,imm16/32
		{ 0x0000FE,0x000001,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"ADD",1311,1,true,0,false,0,false }, // ADD m16/32,r16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"ADD",1312,1,true,0,true,0,false }, // ADD m16/32,imm16/32


		// OR指令 (1400+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x00000C,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"OR",1400,1,false,0,true,1,false }, // OR RAC8,imm8
		{ 0x0000FF,0x00000D,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"OR",1401,1,false,0,true,4,false }, // OR RAC16,imm16
		{ 0x0000FF,0x00000D,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"OR",1402,1,false,0,true,4,false }, // OR RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x00000A,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"OR",1403,1,true,0,false,0,false }, // OR r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"OR",1404,1,true,1,true,1,false }, // OR r8,imm8
		{ 0x0000FE,0x000008,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"OR",1405,1,true,0,false,0,false }, // OR m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"OR",1406,1,true,1,true,1,false }, // OR m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"OR",1407,1,true,1,true,0,false }, // OR m32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"OR",1407,1,true,0,true,1,false }, // OR m32,imm8

		{ 0x0000FE,0x00000B,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"OR",1408,1,true,0,false,0,false }, // OR r16,m16
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"OR",1409,1,true,1,true,0,false }, // OR r16,imm16
		{ 0x0000FE,0x000009,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"OR",1410,1,true,0,false,0,false }, // OR m16,r16
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"OR",1411,1,true,1,true,0,false }, // OR m16,imm16

		// AND指令 (1500+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x000024,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"AND",1500,1,false,0,true,1,false }, // AND RAC8,imm8
		{ 0x0000FF,0x000025,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"AND",1501,1,false,0,true,4,false }, // AND RAC16,imm16
		{ 0x0000FF,0x000025,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"AND",1502,1,false,0,true,4,false }, // AND RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x000022,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"AND",1503,1,true,0,false,0,false }, // AND r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"AND",1504,1,true,4,true,1,false }, // AND r8,imm8
		{ 0x0000FE,0x000020,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"AND",1505,1,true,0,false,0,false }, // AND m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"AND",1506,1,true,4,true,1,false }, // AND m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"AND",1507,1,true,4,true,1,false }, // AND r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"AND",1507,1,true,4,true,1,false }, // AND m32,imm8


		{ 0x0000FE,0x000023,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"AND",1508,1,true,0,false,0,false }, // AND r16,m16
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"AND",1509,1,true,4,true,4,false }, // AND r16,imm16
		{ 0x0000FE,0x000021,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"AND",1510,1,true,0,false,0,false }, // AND m16,r16
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"AND",1511,1,true,4,true,4,false }, // AND m16,imm16

		// SUB指令 (1600+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x00002C,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"SUB",1600,1,false,0,true,1,false }, // SUB RAC8,imm8
		{ 0x0000FF,0x00002D,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"SUB",1601,1,false,0,true,4,false }, // SUB RAC16,imm16
		{ 0x0000FF,0x00002D,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"SUB",1602,1,false,0,true,4,false }, // SUB RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x00002A,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"SUB",1603,1,true,0,false,0,false }, // SUB r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"SUB",1604,1,true,5,true,1,false }, // SUB r8,imm8
		{ 0x0000FE,0x000028,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"SUB",1605,1,true,0,false,0,false }, // SUB m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"SUB",1606,1,true,5,true,1,false }, // SUB m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"SUB",1607,1,true,5,true,1,false }, // AND r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"SUB",1607,1,true,5,true,1,false }, // AND m32,imm8

		{ 0x0000FE,0x00002B,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"SUB",1608,1,true,0,false,0,false }, // SUB r16,m16
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"SUB",1609,1,true,5,true,2,false }, // SUB r16,imm16
		{ 0x0000FE,0x000029,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"SUB",1610,1,true,0,false,0,false }, // SUB m16,r16
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"SUB",1611,1,true,5,true,2,false }, // SUB m16,imm16

		//TEST指令(1700+)																			
		// RAC 专用模板																			
		{ 0x0000FF,0x0000A8,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"TEST",1700,1,false,0,true,1,false }, // TEST RAC8,imm8
		{ 0x0000FF,0x0000A9,1,W16,W16,BB,RAC,IMM,NNN,C_CMD,"TEST",1701,1,false,0,true,4,false }, // TEST RAC16,imm16
		{ 0x0000FF,0x0000A9,1,W32,W32,BB,RAC,IMM,NNN,C_CMD,"TEST",1702,1,false,0,true,4,false }, // TEST RAC32,imm32
		// 通用模板																			
		{ 0x0000FE,0x000084,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"TEST",1703,1,true,0,false,0,false }, // TEST r8,m8
		{ 0x0000F8,0x0000F6,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"TEST",1704,1,true,0,true,1,false }, // TEST r8,imm8
		{ 0x0000FE,0x000084,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"TEST",1705,1,true,0,false,0,false }, // TEST m8,r8
		{ 0x0000FF,0x0000F6,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"TEST",1706,1,true,0,true,1,false }, // TEST m8,imm8

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"TEST",1707,1,true,0,true,0,false }, // TEST r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"TEST",1707,1,true,0,true,0,false }, // TEST m32,imm8

		{ 0x0000FE,0x000085,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"TEST",1708,1,true,0,false,0,false }, // TEST r16,m16
		{ 0x0000FF,0x0000F7,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"TEST",1709,1,true,0,true,2,false }, // TEST r16,imm16
		{ 0x0000FE,0x000085,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"TEST",1710,1,true,0,false,0,false }, // TEST m16,r16
		{ 0x0000FF,0x0000F7,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"TEST",1711,1,true,0,true,2,false }, // TEST m16,imm16

		// ADC 指令模板(1800+)
		{ 0x0000FF,0x000014,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"ADC",1800,1,false,0,true,1,false }, // ADC RAC8,imm8
		{ 0x0000FF,0x000015,1,WW,WW,BB,RAC,IMM,NNN,C_CMD,"ADC",1801,1,false,0,true,0,false }, // ADC RAC16/32,imm16/32
		{ 0x0000FE,0x000012,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"ADC",1802,1,true,0,false,0,false }, // ADC r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"ADC",1803,1,true,2,true,1,false }, // ADC r8,imm8
		{ 0x0000FE,0x000010,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"ADC",1804,1,true,0,false,0,false }, // ADC m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"ADC",1805,1,true,2,true,1,false }, // ADC m8,imm8


		{ 0x0000FE,0x000013,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"ADC",1806,1,true,0,false,0,false }, // ADC r16/32,m16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"ADC",1807,1,true,2,true,0,false }, // ADC r16/32,imm16/32
		{ 0x0000FE,0x000011,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"ADC",1808,1,true,0,false,0,false }, // ADC m16/32,r16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"ADC",1809,1,true,2,true,0,false }, // ADC m16/32,imm16/32
		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"ADC",1810,1,true,2,true,1,false }, // ADC r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"ADC",1810,1,true,2,true,1,false }, // ADC m32,imm8

		// SBB 指令模板(1900+)
		{ 0x0000FF,0x00001C,1,BB,BB,BB,RAC,IMM,NNN,C_CMD,"SBB",1900,1,false,0,true,1,false }, // SBB RAC8,imm8
		{ 0x0000FF,0x00001D,1,WW,WW,BB,RAC,IMM,NNN,C_CMD,"SBB",1901,1,false,0,true,0,false }, // SBB RAC16/32,imm16/32
		{ 0x0000FE,0x00001A,1,BB,BB,BB,REG,MRG,NNN,C_CMD,"SBB",1902,1,true,0,false,0,false }, // SBB r8,m8
		{ 0x0000F8,0x000080,1,BB,BB,BB,REG,IMM,NNN,C_CMD,"SBB",1903,1,true,3,true,1,false }, // SBB r8,imm8
		{ 0x0000FE,0x000018,1,BB,BB,BB,MRG,REG,NNN,C_CMD,"SBB",1904,1,true,0,false,0,false }, // SBB m8,r8
		{ 0x0000FF,0x000080,1,BB,BB,BB,MRG,IMM,NNN,C_CMD,"SBB",1905,1,true,3,true,1,false }, // SBB m8,imm8

		{ 0x0000FE,0x00001B,1,WW,WW,BB,REG,MRG,NNN,C_CMD,"SBB",1906,1,true,0,false,0,false }, // SBB r16/32,m16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,REG,IMM,NNN,C_CMD,"SBB",1907,1,true,3,true,0,false }, // SBB r16/32,imm16/32
		{ 0x0000FE,0x000019,1,WW,WW,BB,MRG,REG,NNN,C_CMD,"SBB",1908,1,true,0,false,0,false }, // SBB m16/32,r16/32
		{ 0x0000FF,0x000081,1,WW,WW,BB,MRG,IMM,NNN,C_CMD,"SBB",1909,1,true,3,true,0,false }, // SBB m16/32,imm16/32

		{ 0x0000FF,0x000083,1,WW,BB,BB,REG,IMM,NNN,C_CMD,"SBB",1910,1,true,3,true,1,false }, // SBB r32,imm8
		{ 0x0000FF,0x000083,1,WW,BB,BB,MRG,IMM,NNN,C_CMD,"SBB",1910,1,true,3,true,1,false }, // SBB m32,imm8


//----------------------------------------以下C_JMC-C_CAL-C_LOOP需要计算跳转的指令

		// Jcc指令 (2000+)
		{ 0x0000FF, 0x000074, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JE/JZ",2000, 1, false, 0, true, 1, false },   // JE/JZ rel8
		{ 0x00FFFF, 0x00840F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JE/JZ",2001, 2, false, 0, true, 4, false },    // JE/JZ rel32
		{ 0x0000FF, 0x000075, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JNE/JNZ",2002, 1, false, 0, true, 1, false },  // JNE/JNZ rel8
		{ 0x00FFFF, 0x00850F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JNE/JNZ",2003, 2, false, 0, true, 4, false },   // JNE/JNZ rel32
		{ 0x0000FF, 0x00007C, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JL/JNGE",2004, 1, false, 0, true, 1, false },   // JL/JNGE rel8
		{ 0x00FFFF, 0x008C0F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JL/JNGE",2005, 2, false, 0, true, 4, false },    // JL/JNGE rel32
		{ 0x0000FF, 0x00007D, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JGE/JNL",2006, 1, false, 0, true, 1, false },  // JGE/JNL rel8
		{ 0x00FFFF, 0x008D0F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JGE/JNL",2007, 2, false, 0, true, 4, false },   // JGE/JNL rel32
		{ 0x0000FF, 0x00007E, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JLE/JNG",2008, 1, false, 0, true, 1, false },  // JLE/JNG rel8
		{ 0x00FFFF, 0x008E0F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JLE/JNG",2009, 2, false, 0, true, 4, false },   // JLE/JNG rel32
		{ 0x0000FF, 0x00007F, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JG/JNLE",2010, 1, false, 0, true, 1, false },   // JG/JNLE rel8
		{ 0x00FFFF, 0x008F0F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JG/JNLE",2011, 2, false, 0, true, 4, false },    // JG/JNLE rel32
		{ 0x0000FF, 0x000072, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JB/JNAE",2012, 1, false, 0, true, 1, false },   // JB/JNAE rel8
		{ 0x00FFFF, 0x00820F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JB/JNAE",2013, 2, false, 0, true, 4, false },    // JB/JNAE rel32
		{ 0x0000FF, 0x000073, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JAE/JNB",2014, 1, false, 0, true, 1, false },  // JAE/JNB rel8
		{ 0x00FFFF, 0x00830F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JAE/JNB",2015, 2, false, 0, true, 4, false },   // JAE/JNB rel32
		{ 0x0000FF, 0x000076, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JBE/JNA",2016, 1, false, 0, true, 1, false },  // JBE/JNA rel8
		{ 0x00FFFF, 0x00860F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JBE/JNA",2017, 2, false, 0, true, 4, false },   // JBE/JNA rel32
		{ 0x0000FF, 0x000077, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JA/JNBE",2018, 1, false, 0, true, 1, false },   // JA/JNBE rel8
		{ 0x00FFFF, 0x00870F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JA/JNBE",2019, 2, false, 0, true, 4, false },    // JA/JNBE rel32

		// jcc指令1(2100+)
		{ 0x0000FF, 0x000078, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JS",2100, 1, false, 0, true, 1, false },   // JS rel8
		{ 0x00FFFF, 0x00880F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JS",2101, 2, false, 0, true, 4, false },    // JS rel32
		{ 0x0000FF, 0x000079, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JNS",2102, 1, false, 0, true, 1, false },  // JNS rel8
		{ 0x00FFFF, 0x00890F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JNS",2103, 2, false, 0, true, 4, false },   // JNS rel32
		{ 0x0000FF, 0x000070, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JO",2104, 1, false, 0, true, 1, false },   // JO rel8
		{ 0x00FFFF, 0x00800F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JO",2105, 2, false, 0, true, 4, false },    // JO rel32
		{ 0x0000FF, 0x000071, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JNO",2106, 1, false, 0, true, 1, false },  // JNO rel8
		{ 0x00FFFF, 0x00810F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JNO",2107, 2, false, 0, true, 4, false },   // JNO rel32
		{ 0x0000FF, 0x00007A, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JP/JPE",2108, 1, false, 0, true, 1, false },   // JP/JPE rel8
		{ 0x00FFFF, 0x008A0F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JP/JPE",2109, 2, false, 0, true, 4, false },    // JP/JPE rel32
		{ 0x0000FF, 0x00007B, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JNP/JPO",2110, 1, false, 0, true, 1, false },  // JNP/JPO rel8
		{ 0x00FFFF, 0x008B0F, 2, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JNP/JPO",2111, 2, false, 0, true, 4, false },   // JNP/JPO rel32
		{ 0x0000FF, 0x0000E3, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JCXZ/JECXZ",2112, 1, false, 0, true, 1, false }, // JCXZ/JECXZ rel8

		// JMP指令(2200+)
		{ 0x0000FF, 0x0000EB, 1, CC,BB,BB, JOB,NNN,NNN, C_JMC,"JMP",2200, 1, false, 0, true, 1, false }, // JMP rel8
		{ 0x0000FF, 0x0000E9, 1, BB,BB,BB, JOW,NNN,NNN, C_JMC,"JMP",2201, 1, false, 0, true, 4, false },  // JMP rel32

		// CALL指令 (2300+)
		{ 0x0000FF, 0x0000E8, 1, BB,BB,BB, JOW,NNN,NNN, C_CAL,"CALL",2300, 1, false, 0, true, 4, false },  // CALL rel32
		{ 0x0000FF, 0x0000FF, 1, WW,BB,BB, MRG,NNN,NNN, C_CAL,"CALL",2301, 1, true, 2, false, 0, false },  // CALL m32

		// LOOP指令 (2400+)
		{ 0x0000FF, 0x0000E2, 1, BB,BB,BB, JOB,NNN,NNN, C_LOOP,"LOOPD",2400, 1, false, 0, true, 1, false },  // LOOPD rel8
		{ 0x0000FF, 0x0000E1, 1, BB,BB,BB, JOB,NNN,NNN, C_LOOP,"LOOPE",2401, 1, false, 0, true, 1, false },  // LOOPE rel8
		{ 0x0000FF, 0x0000E0, 1, BB,BB,BB, JOB,NNN,NNN, C_LOOP,"LOOPNE",2402, 1, false, 0, true, 1, false }, // LOOPNE rel8
		{ 0x0000FF, 0x0000E2, 1, BB,BB,BB, JOB,NNN,NNN, C_LOOP,"LOOP",2403, 1, false, 0, true, 1, false },   // LOOP rel8

	//----------------------------------------以下无须区分W16,W32的指令，仅使用WW
		// LEA指令 (2500+)
		{ 0x0000FE, 0x00008D, 1, WW, WW, BB, REG, MRG, NNN, C_CMD, "LEA", 2500, 1, true, 0, false, 0, false },  // LEA r16, m16|LEA r32, m32

		// NOT指令 (2600+)
		{ 0x0000FE, 0x0000F6, 1, BB,BB,BB, MRG,NNN,NNN, C_CMD,"NOT",2600, 1, true, 2, false, 0, false },  // NOT m8
		{ 0x0000FE, 0x0000F7, 1, WW,BB,BB, MRG,NNN,NNN, C_CMD,"NOT",2601, 1, true, 2, false, 0, false }, // NOT m32

		// INC指令(2700+)
		{ 0x0000F8, 0x000040, 1, WW, WW, BB, REG, NNN, NNN, C_CMD, "INC", 2700, 1, false, 0, false, 0, false },
		{ 0x0000FE, 0x0000FE, 1, BB, BB, BB, MRG, NNN, NNN, C_CMD, "INC", 2701, 1, true, 0, false, 0, false },
		{ 0x0000FF, 0x0000FF, 1, WW, WW, BB, MRG, NNN, NNN, C_CMD, "INC", 2702, 1, true, 0, false, 0, false },

		// DEC指令(2800+)
		{ 0x0000F8, 0x000048, 1, WW, WW, BB, REG, NNN, NNN, C_CMD, "DEC", 2800, 1, false, 0, false, 0, false },
		{ 0x0000FE, 0x0000FE, 1, BB, BB, BB, MRG, NNN, NNN, C_CMD, "DEC", 2801, 1, true, 0, false, 0, false },
		{ 0x0000FF, 0x0000FF, 1, WW, WW, BB, MRG, NNN, NNN, C_CMD, "DEC", 2802, 1, true, 0, false, 0, false },

		// SHL指令 (2900+)
		{ 0x0000FF, 0x0000D0, 1, BB, BB, BB, MRG, ONE, NNN, C_CMD, "SHL/SAL", 2901, 1, true, 4, false, 0, false },  // SHL/SAL m8,1
		{ 0x0000FF, 0x0000D1, 1, WW, BB, BB, MRG, ONE, NNN, C_CMD, "SHL/SAL", 2902, 1, true, 4, false, 0, false }, // SHL/SAL m32,1
		{ 0x0000FF, 0x0000C0, 1, BB, BB, BB, MRG, IMM, NNN, C_CMD, "SHL/SAL", 2903, 1, true, 4, true, 1, false },   // SHL/SAL m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW, BB, BB, MRG, IMM, NNN, C_CMD, "SHL/SAL", 2904, 1, true, 4, true, 1, false },  // SHL/SAL m32,imm8
		{ 0x0000FF, 0x0000D2, 1, BB, WW, BB, MRG, RCM, NNN, C_CMD, "SHL/SAL", 2905, 1, true, 4, false, 0, false },  // SHL/SAL m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW, WW, BB, MRG, RCM, NNN, C_CMD, "SHL/SAL", 2906, 1, true, 4, false, 0, false }, // SHL/SAL m32,CL


		// RETN指令(3000+)
		{ 0x0000FF, 0x0000C2, 1, BB,BB,BB, IMM,NNN,NNN, C_CMD,"RETN",3000, 1, false, 0, true, 2, false }, // RETN imm16
		{ 0x0000FF, 0x0000C2, 1, WW,BB,BB, IMM,NNN,NNN, C_CMD,"RETN",3001, 1, false, 0, true, 4, false }, // RETN imm32
		{ 0x0000FF, 0x0000C3, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"RETN",3002, 1, false, 0, false, 0, false }, // RETN


		// PUSH指令 (3100+)
		{ 0x0000FF, 0x000050, 1, WW,BB,BB, RCM,NNN,NNN, C_PSH,"PUSH",3100, 1, false, 0, false, 0, false }, // PUSH r32
		{ 0x0000FF, 0x0000FF, 1, WW,BB,BB, MRG,NNN,NNN, C_PSH,"PUSH",3101, 1, true, 6, false, 0, false }, // PUSH m32
		{ 0x0000FF, 0x00006A, 1, BB,BB,BB, IMM,NNN,NNN, C_PSH,"PUSH",3102, 1, false, 0, true, 1, false }, // PUSH imm8
		{ 0x0000FF, 0x00009C, 1, BB,BB,BB, NNN,NNN,NNN, C_PSH,"PUSHFD",3103,1, false, 0, false, 0, false },

		// POP指令 (3200+)
		{ 0x0000FF, 0x000058, 1, WW,BB,BB, RCM,NNN,NNN, C_POP,"POP",3200, 1, false, 0, false, 0, false }, // POP r32
		{ 0x0000FF, 0x00008F, 1, WW,BB,BB, MRG,NNN,NNN, C_POP,"POP",3201, 1, true, 0, false, 0, false }, // POP m32
		{ 0x0000FF, 0x00009D, 1, BB,BB,BB, NNN,NNN,NNN, C_POP,"POPFD",3202, 1, false, 0, false, 0, false }, // POPFD

		// IMUL指令 (3300+)
		{ 0x00FFFF, 0x00AF0F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"IMUL",3300, 2, true, 0, false, 0, false },    // IMUL r32,m32
		{ 0x0000FE, 0x000069, 1, WW,WW,WW, REG,MRG,IMM, C_CMD,"IMUL",3301, 1, true, 0, true, 4, false },     // IMUL r32,m32,imm32
		{ 0x0000FE, 0x00006B, 1, WW,BB,BB, REG,MRG,IMM, C_CMD,"IMUL",3302, 1, true, 0, true, 1, false },     // IMUL r32,m32,imm8

		// SAR指令 (3400+)
		{ 0x0000FF, 0x0000D0, 1, BB,BB,BB, MRG,ONE,NNN, C_CMD,"SAR",3402, 1, true, 7, false, 0, false },  // SAR m8,1
		{ 0x0000FF, 0x0000D1, 1, WW,BB,BB, MRG,ONE,NNN, C_CMD,"SAR",3403, 1, true, 7, false, 0, false }, // SAR m32,1

		{ 0x0000FF, 0x0000C0, 1, BB,BB,BB, MRG,IMM,NNN, C_CMD,"SAR",3400, 1, true, 7, true, 1, false },   // SAR m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW,BB,BB, MRG,IMM,NNN, C_CMD,"SAR",3401, 1, true, 7, true, 1, false },  // SAR m32,imm8

		{ 0x0000FF, 0x0000D2, 1, BB,BB,BB, MRG,RCM,NNN, C_CMD,"SAR",3404, 1, true, 7, false, 0, false },  // SAR m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW,WW,BB, MRG,RCM,NNN, C_CMD,"SAR",3405, 1, true, 7, false, 0, false }, // SAR m32,CL

		// SHR指令 (3500+)
		{ 0x0000FF, 0x0000D0, 1, BB,BB,BB, MRG,ONE,NNN, C_CMD,"SHR",3502, 1, true, 5, false, 0, false },  // SHR m8,1
		{ 0x0000FF, 0x0000D1, 1, WW,WW,BB, MRG,ONE,NNN, C_CMD,"SHR",3503, 1, true, 5, false, 0, false }, // SHR m32,1

		{ 0x0000FF, 0x0000C0, 1, BB,BB,BB, MRG,IMM,NNN, C_CMD,"SHR",3500, 1, true, 5, true, 1, false },   // SHR m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW,WW,BB, MRG,IMM,NNN, C_CMD,"SHR",3501, 1, true, 5, true, 1, false },  // SHR m32,imm8
		{ 0x0000FF, 0x0000D2, 1, BB,BB,BB, MRG,RCM,NNN, C_CMD,"SHR",3504, 1, true, 5, false, 0, false },  // SHR m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW,WW,BB, MRG,RCM,NNN, C_CMD,"SHR",3505, 1, true, 5, false, 0, false }, // SHR m32,CL

		// SHLD指令 (3600+)
		{ 0x00FFFF, 0x00A40F, 2, WW,WW,BB, MRG,REG,IMM, C_CMD,"SHLD",3600, 2, true, 0, true, 1, false },  // SHLD m32,r32,imm8
		{ 0x00FFFF, 0x00A50F, 2, WW,WW,BB, MRG,REG,RCM, C_CMD,"SHLD",3601, 2, true, 0, false, 0, false }, // SHLD m32,r32,CL

		// SHRD指令 (3700+)
		{ 0x00FFFF, 0x00AC0F, 2, WW,WW,BB, MRG,REG,IMM, C_CMD,"SHRD",3700, 2, true, 0, true, 1, false },  // SHRD m32,r32,imm8
		{ 0x00FFFF, 0x00AD0F, 2, WW,WW,BB, MRG,REG,RCM, C_CMD,"SHRD",3701, 2, true, 0, false, 0, false }, // SHRD m32,r32,CL

		// NEG指令 (3800+)
		{ 0x0000FE, 0x0000F6, 1, BB,BB,BB, MRG,NNN,NNN, C_CMD,"NEG",3800, 1, true, 3, false, 0, false },  // NEG m8
		{ 0x0000FE, 0x0000F7, 1, WW,WW,BB, MRG,NNN,NNN, C_CMD,"NEG",3801, 1, true, 3, false, 0, false }, // NEG m32

		// DIV指令 (3900+)
		{ 0x0000FE, 0x0000F6, 1, BB,BB,BB, MRG,NNN,NNN, C_CMD,"DIV",3900, 1, true, 6, false, 0, false },  // DIV m8
		{ 0x0000FE, 0x0000F7, 1, WW,BB,BB, MRG,NNN,NNN, C_CMD,"DIV",3901, 1, true, 6, false, 0, false },  // DIV m32

		// IDIV指令 (4000+)
		{ 0x0000FE, 0x0000F6, 1, BB,BB,BB, MRG,NNN,NNN, C_CMD,"IDIV",4000, 1, true, 7, false, 0, false }, // IDIV m8
		{ 0x0000FE, 0x0000F7, 1, WW,BB,BB, MRG,NNN,NNN, C_CMD,"IDIV",4001, 1, true, 7, false, 0, false }, // IDIV m32

		// MUL指令 (4100+)
		{ 0x0000FE, 0x0000F6, 1, BB,BB,BB, MRG,NNN,NNN, C_CMD,"MUL",4100, 1, true, 4, false, 0, false },  // MUL m8
		{ 0x0000FE, 0x0000F7, 1, WW,BB,BB, MRG,NNN,NNN, C_CMD,"MUL",4101, 1, true, 4, false, 0, false },  // MUL m32

		// MOVSX指令 (4200+)
		{ 0x00FFFF, 0x00BE0F, 2, WW,BB,BB, REG,MRG,NNN, C_CMD,"MOVSX",4200, 2, true, 0, false, 0, false }, // MOVSX r32,m8
		{ 0x00FFFF, 0x00BF0F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"MOVSX",4201, 2, true, 0, false, 0, false }, // MOVSX r32,m16

		// MOVZX指令 (4300+)
		{ 0x00FFFF, 0x00B60F, 2, WW,BB,BB, REG,MRG,NNN, C_CMD,"MOVZX",4300, 2, true, 0, false, 0, false }, // MOVZX r32,m8
		{ 0x00FFFF, 0x00B70F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"MOVZX",4301, 2, true, 0, false, 0, false }, // MOVZX r32,m16

		// XCHG指令 (4400+)
		{ 0x0000FF, 0x000090, 1, WW,BB,BB, RAC,REG,NNN, C_CMD,"XCHG",4400, 1, false, 0, false, 0, false },  // XCHG EAX,r32
		{ 0x0000FE, 0x000086, 1, BB,BB,BB, MRG,REG,NNN, C_CMD,"XCHG",4401, 1, true, 0, false, 0, false },   // XCHG m8,r8
		{ 0x0000FE, 0x000087, 1, WW,WW,BB, MRG,REG,NNN, C_CMD,"XCHG",4402, 1, true, 0, false, 0, false },  // XCHG m32,r32

		// BT指令 (4500+)
		{ 0x00FFFF, 0x00A30F, 2, WW,WW,BB, MRG,REG,NNN, C_CMD,"BT",4500, 2, true, 0, false, 0, false },    // BT m32,r32
		{ 0x00FFFF, 0x00BA0F, 2, WW,BB,BB, MRG,IMM,NNN, C_CMD,"BT",4501, 2, true, 4, true, 1, false },     // BT m32,imm8

		// BTS指令 (4500+)
		{ 0x00FFFF, 0x00AB0F, 2, WW,WW,BB, MRG,REG,NNN, C_CMD,"BTS",4500, 2, true, 0, false, 0, false },   // BTS m32,r32
		{ 0x00FFFF, 0x00BA0F, 2, WW,BB,BB, MRG,IMM,NNN, C_CMD,"BTS",4501, 2, true, 5, true, 1, false },    // BTS m32,imm8

		// BTR指令 (4600+)
		{ 0x00FFFF, 0x00B30F, 2, WW,WW,BB, MRG,REG,NNN, C_CMD,"BTR",4600, 2, true, 0, false, 0, false },   // BTR m32,r32
		{ 0x00FFFF, 0x00BA0F, 2, WW,BB,BB, MRG,IMM,NNN, C_CMD,"BTR",4601, 2, true, 6, true, 1, false },    // BTR m32,imm8

		// BTC指令 (4700+)
		{ 0x00FFFF, 0x00BA0F, 2, WW,BB,BB, MRG,IMM,NNN, C_CMD,"BTC",4700, 2, true, 7, true, 1, false },   // BTC m32,imm8
		{ 0x00FFFF, 0x00BB0F, 2, WW,WW,BB, MRG,REG,NNN, C_CMD,"BTC",4701, 2, true, 0, false, 0, false },  // BTC m32,r32

		// BSF指令 (4800+)
		{ 0x00FFFF, 0x00BC0F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"BSF",4800, 2, true, 0, false, 0, false },  // BSF r32,m32

		// BSR指令 (4900+)
		{ 0x00FFFF, 0x00BD0F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"BSR",4900, 2, true, 0, false, 0, false },  // BSR r32,m32

		// BSWAP指令 (5000+)
		{ 0x00FFFF, 0x00C80F, 2, WW,BB,BB, REG,NNN,NNN, C_CMD,"BSWAP",5000, 2, false, 0, false, 0, false },  // BSWAP r32

		// ROL指令 (5100+)
		{ 0x0000FF, 0x0000D0, 1, BB,BB,BB, MRG,ONE,NNN, C_CMD,"ROL",5102, 1, true, 0, false, 0, false },   // ROL m8,1
		{ 0x0000FF, 0x0000D1, 1, WW,BB,BB, MRG,ONE,NNN, C_CMD,"ROL",5103, 1, true, 0, false, 0, false },  // ROL m32,1

		{ 0x0000FF, 0x0000C0, 1, BB,BB,BB, MRG,IMM,NNN, C_CMD,"ROL",5100, 1, true, 0, true, 1, false },    // ROL m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW,BB,BB, MRG,IMM,NNN, C_CMD,"ROL",5101, 1, true, 0, true, 1, false },   // ROL m32,imm8
		{ 0x0000FF, 0x0000D2, 1, BB,BB,BB, MRG,RCM,NNN, C_CMD,"ROL",5104, 1, true, 0, false, 0, false },   // ROL m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW,BB,BB, MRG,RCM,NNN, C_CMD,"ROL",5105, 1, true, 0, false, 0, false },  // ROL m32,CL

		// ROR指令 (5200+)
		{ 0x0000FF, 0x0000D0, 1, BB,BB,BB, MRG,ONE,NNN, C_CMD,"ROR",5202, 1, true, 1, false, 0, false },   // ROR m8,1
		{ 0x0000FF, 0x0000D1, 1, WW,BB,BB, MRG,ONE,NNN, C_CMD,"ROR",5203, 1, true, 1, false, 0, false },  // ROR m32,1

		{ 0x0000FF, 0x0000C0, 1, BB,BB,BB, MRG,IMM,NNN, C_CMD,"ROR",5200, 1, true, 1, true, 1, false },    // ROR m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW,BB,BB, MRG,IMM,NNN, C_CMD,"ROR",5201, 1, true, 1, true, 1, false },   // ROR m32,imm8
		{ 0x0000FF, 0x0000D2, 1, BB,BB,BB, MRG,RCM,NNN, C_CMD,"ROR",5204, 1, true, 1, false, 0, false },   // ROR m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW,BB,BB, MRG,RCM,NNN, C_CMD,"ROR",5205, 1, true, 1, false, 0, false },  // ROR m32,CL

		// RCL指令 (5300+)
		{ 0x0000FF, 0x0000D0, 1, BB,BB,BB, MRG,ONE,NNN, C_CMD,"RCL",5302, 1, true, 2, false, 0, false },   // RCL m8,1
		{ 0x0000FF, 0x0000D1, 1, WW,BB,BB, MRG,ONE,NNN, C_CMD,"RCL",5303, 1, true, 2, false, 0, false },  // RCL m32,1

		{ 0x0000FF, 0x0000C0, 1, BB,BB,BB, MRG,IMM,NNN, C_CMD,"RCL",5300, 1, true, 2, true, 1, false },    // RCL m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW,BB,BB, MRG,IMM,NNN, C_CMD,"RCL",5301, 1, true, 2, true, 1, false },   // RCL m32,imm8
		{ 0x0000FF, 0x0000D2, 1, BB,BB,BB, MRG,RCM,NNN, C_CMD,"RCL",5304, 1, true, 2, false, 0, false },   // RCL m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW,BB,BB, MRG,RCM,NNN, C_CMD,"RCL",5305, 1, true, 2, false, 0, false },  // RCL m32,CL

		// RCR指令 (5400+)
		{ 0x0000FF, 0x0000D0, 1, BB,BB,BB, MRG,ONE,NNN, C_CMD,"RCR",5402, 1, true, 3, false, 0, false },   // RCR m8,1
		{ 0x0000FF, 0x0000D1, 1, WW,BB,BB, MRG,ONE,NNN, C_CMD,"RCR",5403, 1, true, 3, false, 0, false },  // RCR m32,1

		{ 0x0000FF, 0x0000C0, 1, BB,BB,BB, MRG,IMM,NNN, C_CMD,"RCR",5400, 1, true, 3, true, 1, false },    // RCR m8,imm8
		{ 0x0000FF, 0x0000C1, 1, WW,BB,BB, MRG,IMM,NNN, C_CMD,"RCR",5401, 1, true, 3, true, 1, false },   // RCR m32,imm8
		{ 0x0000FF, 0x0000D2, 1, BB,BB,BB, MRG,RCM,NNN, C_CMD,"RCR",5404, 1, true, 3, false, 0, false },   // RCR m8,CL
		{ 0x0000FF, 0x0000D3, 1, WW,BB,BB, MRG,RCM,NNN, C_CMD,"RCR",5405, 1, true, 3, false, 0, false },  // RCR m32,CL

		// SETcc指令 (5500+)
		{ 0x00FFFF, 0x00940F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETE",5500, 2, true, 0, false, 0, false },   // SETE m8
		{ 0x00FFFF, 0x00950F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETNE",5501, 2, true, 0, false, 0, false },  // SETNE m8
		{ 0x00FFFF, 0x009C0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETL",5502, 2, true, 0, false, 0, false },   // SETL m8
		{ 0x00FFFF, 0x009D0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETGE",5503, 2, true, 0, false, 0, false },  // SETGE m8
		{ 0x00FFFF, 0x009E0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETLE",5504, 2, true, 0, false, 0, false },  // SETLE m8
		{ 0x00FFFF, 0x009F0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETG",5505, 2, true, 0, false, 0, false },   // SETG m8
		{ 0x00FFFF, 0x00920F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETB",5506, 2, true, 0, false, 0, false },   // SETB m8
		{ 0x00FFFF, 0x00930F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETAE",5507, 2, true, 0, false, 0, false },  // SETAE m8
		{ 0x00FFFF, 0x00960F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETBE",5508, 2, true, 0, false, 0, false },  // SETBE m8
		{ 0x00FFFF, 0x00970F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETA",5509, 2, true, 0, false, 0, false },   // SETA m8
		{ 0x00FFFF, 0x00980F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETS",5510, 2, true, 0, false, 0, false },   // SETS m8
		{ 0x00FFFF, 0x00990F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETNS",5511, 2, true, 0, false, 0, false },  // SETNS m8
		{ 0x00FFFF, 0x009A0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETP",5512, 2, true, 0, false, 0, false },   // SETP m8
		{ 0x00FFFF, 0x009B0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETNP",5513, 2, true, 0, false, 0, false },  // SETNP m8

		// 字符串操作指令 (5600+)
		{ 0x0000FF, 0x0000A4, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"MOVSB",5600, 1, false, 0, false, 0, false }, // MOVSB
		{ 0x0000FF, 0x0000A5, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"MOVSD",5601, 1, false, 0, false, 0, false }, // MOVSD
		{ 0x0000FF, 0x0000AA, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"STOSB",5602, 1, false, 0, false, 0, false }, // STOSB
		{ 0x0000FF, 0x0000AB, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"STOSD",5603, 1, false, 0, false, 0, false }, // STOSD
		{ 0x0000FF, 0x0000AC, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"LODSB",5604, 1, false, 0, false, 0, false }, // LODSB
		{ 0x0000FF, 0x0000AD, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"LODSD",5605, 1, false, 0, false, 0, false }, // LODSD
		{ 0x0000FF, 0x0000AE, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"SCASB",5606, 1, false, 0, false, 0, false }, // SCASB
		{ 0x0000FF, 0x0000AF, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"SCASD",5607, 1, false, 0, false, 0, false }, // SCASD
		{ 0x0000FF, 0x0000A6, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CMPSB",5608, 1, false, 0, false, 0, false }, // CMPSB
		{ 0x0000FF, 0x0000A7, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"CMPSD",5609, 1, false, 0, false, 0, false }, // CMPSD

		// REP/REPE/REPNE前缀 5700+)
		{ 0x0000FF, 0x0000F3, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "REP/REPE", 5700, 1, false, 0, false, 0, false },
		{ 0x0000FF, 0x0000F2, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"REPNE",5701, 1, false, 0, false, 0, false }, // REPNE

		// 标志位操作指令 (5800+)
		{ 0x0000FF, 0x0000F8, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CLC",5800, 1, false, 0, false, 0, false },   // CLC
		{ 0x0000FF, 0x0000F9, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"STC",5801, 1, false, 0, false, 0, false },   // STC
		{ 0x0000FF, 0x0000F5, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CMC",5802, 1, false, 0, false, 0, false },   // CMC
		{ 0x0000FF, 0x0000FC, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CLD",5803, 1, false, 0, false, 0, false },   // CLD
		{ 0x0000FF, 0x0000FD, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"STD",5804, 1, false, 0, false, 0, false },   // STD
		{ 0x0000FF, 0x0000FA, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CLI",5805, 1, false, 0, false, 0, false },   // CLI
		{ 0x0000FF, 0x0000FB, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"STI",5806, 1, false, 0, false, 0, false },   // STI

		// ENTELEAVE指令 (5900+)
		{ 0x0000FF, 0x0000C8, 1, WW,BB,BB, IMM,IMM,NNN, C_CMD,"ENTER",5900, 1, false, 0, true, 3, false },  // ENTER imm16,imm8
		{ 0x0000FF, 0x0000C9, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"LEAVE",5901, 1, false, 0, false, 0, false }, // LEAVE

		// 其他系统指令 (6000+)
		{ 0x0000FF, 0x0000CC, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"INT3",6000, 1, false, 0, false, 0, false },  // INT3
		{ 0x0000FF, 0x0000CD, 1, BB,BB,BB, IMM,NNN,NNN, C_CMD,"INT",6001, 1, false, 0, true, 1, false },    // INT imm8
		{ 0x0000FF, 0x0000CE, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"INTO",6002, 1, false, 0, false, 0, false },  // INTO
		{ 0x0000FF, 0x0000CF, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"IRET",6003, 1, false, 0, false, 0, false },  // IRET
		{ 0x0000FF, 0x00009B, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"WAIT",6004, 1, false, 0, false, 0, false },  // WAIT
		{ 0x0000FF, 0x0000F4, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"HLT",6005, 1, false, 0, false, 0, false },   // HLT
		{ 0x0000FF, 0x0000F0, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"LOCK",6006, 1, false, 0, false, 0, false },  // LOCK

		// CBW/CWDE(6100+)
		{ 0x0000FF, 0x000098, 1, WW, WW, BB, RAC, NNN, NNN, C_CMD, "CBW/CWDE", 6100, 1, false, 0, false, 0, false },

		// CMOVcc指令 (6200+)
		{ 0x00FFFF, 0x00400F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVO", 6200, 2, true, 0, false, 0, false },  // CMOVO r32, m32
		{ 0x00FFFF, 0x00410F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVNO", 6201, 2, true, 0, false, 0, false }, // CMOVNO r32, m32
		{ 0x00FFFF, 0x00420F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVB", 6202, 2, true, 0, false, 0, false },  // CMOVB r32, m32
		{ 0x00FFFF, 0x00430F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVAE", 6203, 2, true, 0, false, 0, false }, // CMOVAE r32, m32
		{ 0x00FFFF, 0x00440F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVE", 6204, 2, true, 0, false, 0, false },  // CMOVE r32, m32
		{ 0x00FFFF, 0x00450F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVNE", 6205, 2, true, 0, false, 0, false }, // CMOVNE r32, m32
		{ 0x00FFFF, 0x00460F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVBE", 6206, 2, true, 0, false, 0, false }, // CMOVBE r32, m32
		{ 0x00FFFF, 0x00470F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVA", 6207, 2, true, 0, false, 0, false },  // CMOVA r32, m32
		{ 0x00FFFF, 0x00480F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVS", 6208, 2, true, 0, false, 0, false },  // CMOVS r32, m32
		{ 0x00FFFF, 0x00490F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVNS", 6209, 2, true, 0, false, 0, false }, // CMOVNS r32, m32
		{ 0x00FFFF, 0x004A0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVP", 6210, 2, true, 0, false, 0, false },  // CMOVP r32, m32
		{ 0x00FFFF, 0x004B0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVNP", 6211, 2, true, 0, false, 0, false }, // CMOVNP r32, m32
		{ 0x00FFFF, 0x004C0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVL", 6212, 2, true, 0, false, 0, false },  // CMOVL r32, m32
		{ 0x00FFFF, 0x004D0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVGE", 6213, 2, true, 0, false, 0, false }, // CMOVGE r32, m32
		{ 0x00FFFF, 0x004E0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVLE", 6214, 2, true, 0, false, 0, false }, // CMOVLE r32, m32
		{ 0x00FFFF, 0x004F0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVG", 6215, 2, true, 0, false, 0, false },  // CMOVG r32, m32
		{ 0x00FFFF, 0x00420F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVC", 6216, 2, true, 0, false, 0, false },  // CMOVC r32, m32
		{ 0x00FFFF, 0x00430F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "CMOVNC", 6217, 2, true, 0, false, 0, false }, // CMOVNC r32, m32

		// XADD指令 (6300+)
		{ 0x00FFFF, 0x00C00F, 2, BB, BB, BB, MRG, REG, NNN, C_CMD, "XADD", 6300, 2, true, 0, false, 0, false },  // XADD m8, r8
		{ 0x00FFFF, 0x00C10F, 2, WW, WW, BB, MRG, REG, NNN, C_CMD, "XADD", 6301, 2, true, 0, false, 0, false }, // XADD m32, r32
		{ 0x00FFFF, 0x00C00F, 2, BB, BB, BB, MRG, REG, NNN, C_CMD, "XADD", 6302, 2, true, 0, false, 0, false },  // XADD m8, r8

		// CMPXCHG指令 (6400+)
		{ 0x00FFFF, 0x00B00F, 2, BB, BB, BB, MRG, REG, NNN, C_CMD, "CMPXCHG", 6400, 2, true, 0, false, 0, false },  // CMPXCHG m8, r8
		{ 0x00FFFF, 0x00B10F, 2, WW, WW, BB, MRG, REG, NNN, C_CMD, "CMPXCHG", 6401, 2, true, 0, false, 0, false }, // CMPXCHG m32, r32

		// CMPXCHG8B指令 (6500+)
		{ 0x00FFFF, 0x00C70F, 2, W32, W32, BB, MRG, NNN, NNN, C_CMD, "CMPXCHG8B", 6500, 2, true, 0, false, 0, false }, // CMPXCHG8B m64

		// PAUSE指令 (6600+)
		{ 0x0000FF, 0x0000F3, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "PAUSE", 6600, 1, false, 0, false, 0, false }, // PAUSE

		// LFENCE / MFENCE / SFENCE指令 (6700+)
		{ 0x00FFFF, 0x00E80F, 2, BB, BB, BB, NNN, NNN, NNN, C_CMD, "LFENCE", 6700, 2, false, 0, false, 0, false }, // LFENCE
		{ 0x00FFFF, 0x00F00F, 2, BB, BB, BB, NNN, NNN, NNN, C_CMD, "MFENCE", 6701, 2, false, 0, false, 0, false }, // MFENCE
		{ 0x00FFFF, 0x00F80F, 2, BB, BB, BB, NNN, NNN, NNN, C_CMD, "SFENCE", 6702, 2, false, 0, false, 0, false }, // SFENCE

		// RDTSC指令 (6800+)
		{ 0x0000FF, 0x000031, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "RDTSC", 6800, 1, false, 0, false, 0, false }, // RDTSC

		// CPUID指令 (6900+)
		{ 0x00FFFF, 0x00A20F, 2, BB, BB, BB, NNN, NNN, NNN, C_CMD, "CPUID", 6900, 2, false, 0, false, 0, false }, // CPUID

		// IN / OUT指令 (7000+)
		{ 0x0000FF, 0x0000E4, 1, BB, BB, BB, IMM, NNN, NNN, C_CMD, "IN", 7000, 1, false, 0, true, 1, false },  // IN AL, imm8
		{ 0x0000FF, 0x0000E5, 1, W16, W16, BB, IMM, NNN, NNN, C_CMD, "IN", 7001, 1, false, 0, true, 2, false }, // IN AX, imm8
		{ 0x0000FF, 0x0000E6, 1, BB, BB, BB, IMM, NNN, NNN, C_CMD, "OUT", 7002, 1, false, 0, true, 1, false }, // OUT imm8, AL
		{ 0x0000FF, 0x0000E7, 1, W16, W16, BB, IMM, NNN, NNN, C_CMD, "OUT", 7003, 1, false, 0, true, 2, false }, // OUT imm8, AX

		//补充指令(7200+)
		{ 0x0000FF, 0x0000A5, 1, WW, WW, BB, NNN, NNN, NNN, C_CMD, "MOVSW", 7200, 1, false, 0, false, 0, false }, // MOVSW
		{ 0x0000FF, 0x0000AB, 1, WW, WW, BB, NNN, NNN, NNN, C_CMD, "STOSW", 7201, 1, false, 0, false, 0, false }, // STOSW
		{ 0x0000FF, 0x0000AD, 1, WW, WW, BB, NNN, NNN, NNN, C_CMD, "LODSW", 7202, 1, false, 0, false, 0, false }, // LODSW
		{ 0x0000FF, 0x0000AF, 1, WW, WW, BB, NNN, NNN, NNN, C_CMD, "SCASW", 7203, 1, false, 0, false, 0, false }, // SCASW
		{ 0x0000FF, 0x0000A7, 1, WW, WW, BB, NNN, NNN, NNN, C_CMD, "CMPSW", 7204, 1, false, 0, false, 0, false }, // CMPSW
		{ 0x00FFFF, 0x00B80F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "POPCNT", 7205, 2, true, 0, false, 0, false }, // POPCNT r32, m32
		{ 0x00FFFF, 0x00BC0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "TZCNT", 7206, 2, true, 0, false, 0, false }, // TZCNT r32, m32
		{ 0x00FFFF, 0x00010F, 2, WW, WW, BB, MRG, NNN, NNN, C_CMD, "LGDT", 7207, 2, true, 2, false, 0, false }, // LGDT m32
		{ 0x00FFFF, 0x00010F, 2, WW, WW, BB, MRG, NNN, NNN, C_CMD, "SGDT", 7208, 2, true, 0, false, 0, false }, // SGDT m32
		{ 0x00FFFF, 0x00010F, 2, WW, WW, BB, MRG, NNN, NNN, C_CMD, "LIDT", 7209, 2, true, 3, false, 0, false }, // LIDT m32
		{ 0x00FFFF, 0x00010F, 2, WW, WW, BB, MRG, NNN, NNN, C_CMD, "SIDT", 7210, 2, true, 1, false, 0, false }, // SIDT m32
		{ 0x00FFFF, 0x00010F, 2, WW, WW, BB, MRG, NNN, NNN, C_CMD, "LMSW", 7211, 2, true, 6, false, 0, false }, // LMSW m32
		{ 0x00FFFF, 0x00010F, 2, WW, WW, BB, MRG, NNN, NNN, C_CMD, "SMSW", 7212, 2, true, 4, false, 0, false }, // SMSW m32
		{ 0x0000FF, 0x000008, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "INVD", 7213, 2, false, 0, false, 0, false }, // INVD
		{ 0x0000FF, 0x000009, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "WBINVD", 7214, 1, false, 0, false, 0, false }, // WBINVD
		{ 0x0000FF, 0x000032, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "RDMSR", 7215, 1, false, 0, false, 0, false }, // RDMSR
		{ 0x0000FF, 0x000030, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "WRMSR", 7216, 1, false, 0, false, 0, false }, // WRMSR
		{ 0x0000FF, 0x000034, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "SYSENTER", 7217, 1, false, 0, false, 0, false }, // SYSENTER
		{ 0x0000FF, 0x000035, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "SYSEXIT", 7218, 1, false, 0, false, 0, false }, // SYSEXIT
		{ 0x0000FF, 0x00009F, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "LAHF", 7219, 1, false, 0, false, 0, false }, // LAHF
		{ 0x0000FF, 0x00009E, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "SAHF", 7220, 1, false, 0, false, 0, false }, // SAHF
		{ 0x0000FF, 0x0000D7, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "XLAT", 7221, 1, false, 0, false, 0, false }, // XLAT
		{ 0x0000FF, 0x000090, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "NOP", 7222, 1, false, 0, false, 0, false }, // NOP
		{ 0x0000FF, 0x00000B, 1, BB, BB, BB, NNN, NNN, NNN, C_CMD, "UD2", 7223, 1, false, 0, false, 0, false }, // UD2
		{ 0x0000FF, 0x000062, 1, WW, WW, BB, REG, MRG, NNN, C_CMD, "BOUND", 7224, 1, true, 0, false, 0, false }, // BOUND r32, m32
		{ 0x0000FF, 0x000063, 1, WW, WW, BB, REG, MRG, NNN, C_CMD, "ARPL", 7225, 1, true, 0, false, 0, false }, // ARPL r32, m32
		{ 0x00FFFF, 0x00D40F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "PADDQ", 7226, 2, true, 0, false, 0, false }, // PADDQ r32, m32
		{ 0x00FFFF, 0x00FB0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "PSUBQ", 7227, 2, true, 0, false, 0, false }, // PSUBQ r32, m32
		{ 0x00FFFF, 0x00400F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "PMULLD", 7228, 2, true, 0, false, 0, false }, // PMULLD r32, m32
		{ 0x00FFFF, 0x002B0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "PACKUSDW", 7229, 2, true, 0, false, 0, false }, // PACKUSDW r32, m32
		{ 0x00FFFF, 0x006D0F, 2, WW, WW, BB, REG, MRG, NNN, C_CMD, "PUNPCKHQDQ", 7230, 2, true, 0, false, 0, false }, // PUNPCKHQDQ r32, m32

//----------------------------------------以下必须区分W16,W32的指令，无法仅使用WW
//暂无

		// BCD算术指令 (8000+)
		{ 0x0000FF, 0x000037, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"AAA",8000, 1, false, 0, false, 0, false },  // AAA
		{ 0x0000FF, 0x0000D5, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"AAD",8001, 1, false, 0, false, 0, false },  // AAD
		{ 0x0000FF, 0x0000D4, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"AAM",8002, 1, false, 0, false, 0, false },  // AAM
		{ 0x0000FF, 0x00003F, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"AAS",8003, 1, false, 0, false, 0, false },  // AAS
		{ 0x0000FF, 0x000027, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"DAA",8004, 1, false, 0, false, 0, false },  // DAA
		{ 0x0000FF, 0x00002F, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"DAS",8005, 1, false, 0, false, 0, false },  // DAS

		// 系统指令 (8010+)
		{ 0x00FFFF, 0x00060F, 2, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CLTS",8010, 2, false, 0, false, 0, false },  // CLTS
		{ 0x00FFFF, 0x00010F, 2, BB,BB,BB, MMA,NNN,NNN, C_CMD,"INVLPG",8011, 2, true, 7, false, 0, false },  // INVLPG

		// 条件移动指令 (8020+)
		{ 0x00FFFF, 0x00430F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"CMOVNB",8020, 2, true, 0, false, 0, false },  // CMOVNB r32,r/m32
		{ 0x00FFFF, 0x004A0F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"CMOVPE",8021, 2, true, 0, false, 0, false },  // CMOVPE r32,r/m32
		{ 0x00FFFF, 0x004B0F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"CMOVPO",8022, 2, true, 0, false, 0, false },  // CMOVPO r32,r/m32

		// 字符串操作指令 (8030+)
		{ 0x0000FF, 0x0000A6, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"CMPS",8030, 1, false, 0, false, 0, false },  // CMPS
		{ 0x0000FF, 0x00006C, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"INS",8031, 1, false, 0, false, 0, false },   // INS
		{ 0x0000FF, 0x00006E, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"OUTS",8032, 1, false, 0, false, 0, false },  // OUTS

		// 数据转换指令 (8040+)
		{ 0x0000FF, 0x000099, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"CWD",8040, 1, false, 0, false, 0, false },    // CWD
		{ 0x0000FF, 0x000099, 1, WW,BB,BB, NNN,NNN,NNN, C_CMD,"CDQ",8041, 1, false, 0, false, 0, false },    // CDQ

		// 段寄存器加载指令 (8050+)
		{ 0x0000FF, 0x0000C5, 1, WW,WW,BB, REG,MMA,NNN, C_CMD,"LDS",8050, 1, true, 0, false, 0, false },     // LDS r32,m16:32
		{ 0x0000FF, 0x0000C4, 1, WW,WW,BB, REG,MMA,NNN, C_CMD,"LES",8051, 1, true, 0, false, 0, false },     // LES r32,m16:32
		{ 0x00FFFF, 0x00B40F, 2, WW,WW,BB, REG,MMA,NNN, C_CMD,"LFS",8052, 2, true, 0, false, 0, false },     // LFS r32,m16:32
		{ 0x00FFFF, 0x00B50F, 2, WW,WW,BB, REG,MMA,NNN, C_CMD,"LGS",8053, 2, true, 0, false, 0, false },     // LGS r32,m16:32

		// 系统表操作指令 (8060+)
		{ 0x00FFFF, 0x00020F, 2, WW,WW,BB, REG,MRG,NNN, C_CMD,"LAR",8060, 2, true, 0, false, 0, false },     // LAR r32,r/m32
		{ 0x00FFFF, 0x00000F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"LLDT",8061, 2, true, 2, false, 0, false },    // LLDT r/m16
		{ 0x00FFFF, 0x00000F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SLDT",8062, 2, true, 0, false, 0, false },    // SLDT r/m16
		{ 0x00FFFF, 0x00000F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"STR",8063, 2, true, 1, false, 0, false },     // STR r/m16
		{ 0x00FFFF, 0x00000F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"VERR",8064, 2, true, 4, false, 0, false },    // VERR r/m16
		{ 0x00FFFF, 0x00000F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"VERW",8065, 2, true, 5, false, 0, false },    // VERW r/m16

		// 标志寄存器操作指令 (8070+)
		{ 0x0000FF, 0x000061, 1, BB,BB,BB, NNN,NNN,NNN, C_POP,"POPA",8070, 1, false, 0, false, 0, false },   // POPA
		{ 0x0000FF, 0x000061, 1, WW,BB,BB, NNN,NNN,NNN, C_POP,"POPAD",8071, 1, false, 0, false, 0, false },  // POPAD
		{ 0x0000FF, 0x00009D, 1, BB,BB,BB, NNN,NNN,NNN, C_POP,"POPF",8072, 1, false, 0, false, 0, false },   // POPF
		{ 0x0000FF, 0x00009D, 1, WW,BB,BB, NNN,NNN,NNN, C_POP,"POPFD",8073, 1, false, 0, false, 0, false },  // POPFD
		{ 0x0000FF, 0x000060, 1, BB,BB,BB, NNN,NNN,NNN, C_PSH,"PUSHA",8074, 1, false, 0, false, 0, false },  // PUSHA
		{ 0x0000FF, 0x000060, 1, WW,BB,BB, NNN,NNN,NNN, C_PSH,"PUSHAD",8075, 1, false, 0, false, 0, false }, // PUSHAD
		{ 0x0000FF, 0x00009C, 1, BB,BB,BB, NNN,NNN,NNN, C_PSH,"PUSHF",8076, 1, false, 0, false, 0, false },  // PUSHF
		{ 0x0000FF, 0x00009C, 1, WW,BB,BB, NNN,NNN,NNN, C_PSH,"PUSHFD",8077, 1, false, 0, false, 0, false }, // PUSHFD

		// 系统管理指令 (8080+)
		{ 0x00FFFF, 0x00AA0F, 2, BB,BB,BB, NNN,NNN,NNN, C_CMD,"RSM",8080, 2, false, 0, false, 0, false },    // RSM
		{ 0x0000FF, 0x0000D6, 1, BB,BB,BB, NNN,NNN,NNN, C_CMD,"SALC",8081, 1, false, 0, false, 0, false },   // SALC

		// 条件设置指令 (8090+)
		{ 0x00FFFF, 0x00930F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETNB",8090, 2, true, 0, false, 0, false },   // SETNB r/m8
		{ 0x00FFFF, 0x00910F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETNO",8091, 2, true, 0, false, 0, false },   // SETNO r/m8
		{ 0x00FFFF, 0x00900F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETO",8092, 2, true, 0, false, 0, false },    // SETO r/m8
		{ 0x00FFFF, 0x009A0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETPE",8093, 2, true, 0, false, 0, false },   // SETPE r/m8
		{ 0x00FFFF, 0x009B0F, 2, BB,BB,BB, MRG,NNN,NNN, C_CMD,"SETPO",8094, 2, true, 0, false, 0, false },   // SETPO r/m8

		// 结束标记
		{ 0x000000, 0x000000, 0, BB,BB,BB, NNN,NNN,NNN, 0,"",0, 0, false, 0, false, 0, false }  // 结束标记
};

// 寄存器表
const RegInfo registers[] = {
	{"EAX", 0, W32}, {"ECX", 1, W32}, {"EDX", 2, W32}, {"EBX", 3, W32},
	{"ESP", 4, W32}, {"EBP", 5, W32}, {"ESI", 6, W32}, {"EDI", 7, W32},
	{"AX", 0, W16}, {"CX", 1, W16}, {"DX", 2, W16}, {"BX", 3, W16},
	{"SP", 4, W16}, {"BP", 5, W16}, {"SI", 6, W16}, {"DI", 7, W16},
	{"AL", 0, BB}, {"CL", 1, BB}, {"DL", 2, BB}, {"BL", 3, BB},
	{"AH", 4, BB}, {"CH", 5, BB}, {"DH", 6, BB}, {"BH", 7, BB}
};

// Assembler类实现
Assembler::Assembler() : currentOffset(0), totalLength(0), is_debug(false) {}

std::string Assembler::toUpper(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

int Assembler::parseRegister(const std::string& reg, int& Regbits) {
	std::string upperReg = toUpper(reg);
	for (const auto& r : registers) {
		if (upperReg == r.name) {
			Regbits = r.bits;
			return r.code;
		}
	}
	return -1;
}


int Assembler::parseImmediate(const std::string& imm, int def_num_base) {
	std::string str = imm;
	bool isNegative = false;
	bool isHex = false;

	// 移除所有空格
	str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());

	// 移除SHORT关键字
	size_t shortPos = str.find("SHORT");
	if (shortPos != std::string::npos) {
		str.erase(shortPos, 5);
	}

	if (str.empty()) {
		return 0;
	}

	// 处理正负号
	if (str[0] == '-') {
		isNegative = true;
		str = str.substr(1);
	}
	else if (str[0] == '+') {
		str = str.substr(1);
	}

	// 获取数字部分
	std::string numStr = str;

	// 检查是否是十六进制数
	if (str.length() >= 2 && str.substr(0, 2) == "0X") {
		numStr = str.substr(2);
		isHex = true;
	}
	else if (str.back() == 'H' || str.back() == 'h') {
		numStr = str.substr(0, str.length() - 1);
		isHex = true;
	}
	else if (def_num_base == 0) {
		isHex = true;
	}

	if (is_debug) {
		std::cout << "[DEBUG] 默认解析方式: " << (def_num_base == 0 ? "十六进制" : def_num_base == 1 ? "十进制" : "十六进制") << std::endl;
		std::cout << "[DEBUG] 处理后的数字串: " << numStr << std::endl;
		std::cout << "[DEBUG] 是否为负数: " << (isNegative ? "是" : "否") << std::endl;
		std::cout << "[DEBUG] 是否为十六进制: " << (isHex ? "是" : "否") << std::endl;
	}

	try {
		unsigned long value;
		if (isHex) {
			// 十六进制处理
			value = std::stoul(numStr, nullptr, 16);
		}
		else {
			// 十进制处理
			value = std::stoul(numStr, nullptr, 10);
		}

		// 处理负数
		if (isNegative) {
			value = static_cast<unsigned long>(-static_cast<long>(value));
		}

		if (is_debug) {
			std::cout << "[DEBUG] 最终值: 0x" << std::hex << value
				<< " (" << std::dec << static_cast<long>(value) << ")" << std::endl;
		}

		return static_cast<int>(value);
	}
	catch (const std::exception& e) {
		if (is_debug) {
			std::cout << "[DEBUG] 数值转换异常: " << e.what() << std::endl;
		}
		throw;
	}
}


bool Assembler::needsSIB(const Operand& op) {
	return op.index != -1 || op.scale != 1 || op.base == 4;  // ESP的编码是4
}

std::vector<unsigned char> Assembler::generatePrefixes(const Operand& op) {
	std::vector<unsigned char> prefixes;

	// 添加段前缀
	if (!op.segment.empty()) {
		auto it = segmentPrefixes.find(op.segment);
		if (it != segmentPrefixes.end()) {
			prefixes.push_back(it->second);
		}
	}

	// 只在非字符串指令且非8位操作数时添加操作数大小前缀
	if (op.bits == W32 && op.type != MRG && op.reg >= 8) {  // reg >= 8 表示不是8位寄存器
		prefixes.push_back(PREFIX_OPERAND);
	}

	return prefixes;
}

Operand Assembler::parseMemoryOperand(const std::string& operand, int def_num_base) {
	Operand result;
	result.type = MRG;
	result.base = -1;
	result.index = -1;
	result.scale = 1;
	result.disp = 0;
	result.bits = W32; // 默认为32位操作数

	if (is_debug) {
		std::cout << "[DEBUG] 解析内存操作数: " << operand << std::endl;
	}

	// 处理大小指示符
	std::string str = operand;
	if (str.find("BYTE PTR") != std::string::npos) {
		result.bits = BB;
		str = str.substr(str.find("PTR") + 3);
	}
	else if (str.find("DWORD PTR") != std::string::npos) {
		result.bits = W32;
		str = str.substr(str.find("PTR") + 3);
	}
	else if (str.find("WORD PTR") != std::string::npos) {
		result.bits = W16;
		str = str.substr(str.find("PTR") + 3);
	}

	// 处理段前缀
	size_t segPos = str.find(":");
	if (segPos != std::string::npos) {
		std::string segPrefix = str.substr(0, segPos);
		// 移除空格
		segPrefix.erase(std::remove_if(segPrefix.begin(), segPrefix.end(), ::isspace), segPrefix.end());
		result.segment = segPrefix;
		str = str.substr(segPos + 1);
		if (is_debug) {
			std::cout << "[DEBUG] 检测到段前缀: " << segPrefix << std::endl;
		}
	}

	// 移除方括号和空格
	str = str.substr(str.find('[') + 1);
	str = str.substr(0, str.find(']'));
	str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end());

	if (is_debug) {
		std::cout << "[DEBUG] 处理后的表达式: " << str << std::endl;
	}

	// 分析表达式
	std::string token;
	size_t pos = 0;
	char lastOp = '+';

	while (pos < str.length()) {
		// 找到下一个操作符或字符串结束
		size_t nextOp = str.find_first_of("+-", pos);
		if (nextOp == std::string::npos) {
			token = str.substr(pos);
			pos = str.length();
		}
		else {
			token = str.substr(pos, nextOp - pos);
			lastOp = str[nextOp];
			pos = nextOp + 1;
		}

		if (!token.empty()) {
			// 检查是否是比例变址寻址 (index*scale)
			size_t multPos = token.find('*');
			if (multPos != std::string::npos) {
				std::string indexReg = token.substr(0, multPos);
				std::string scaleStr = token.substr(multPos + 1);
				int regBits;
				result.index = parseRegister(indexReg, regBits);
				result.scale = std::stoi(scaleStr);
			}
			// 检查是否是寄存器
			else {
				int regBits;
				int reg = parseRegister(token, regBits);
				if (reg != -1) {
					if (result.base == -1) {
						result.base = reg;
					}
					else if (result.index == -1) {
						result.index = reg;
					}
				}
				// 如果不是寄存器，处理为立即数
				else {
					std::string dispStr = (lastOp == '-' ? "-" : "") + token;
					int disp = parseImmediate(dispStr, def_num_base);
					result.disp += disp;
					if (is_debug) {
						std::cout << "[DEBUG] 解析位移: " << dispStr << " = " << std::hex << disp << std::endl;
					}
				}
			}
		}
	}

	if (is_debug) {
		std::cout << "[DEBUG] 内存操作数解析结果: "
			<< "base=" << result.base
			<< ", index=" << result.index
			<< ", scale=" << result.scale
			<< ", disp=" << std::hex << result.disp
			<< ", bits=" << result.bits
			<< std::endl;
	}

	return result;
}

Operand Assembler::parseOperand(const std::string& str, int def_num_base) {
	Operand operand;
	if (is_debug) {
		std::cout << "[DEBUG] 解析操作数: " << str << std::endl;
	}

	// 检查是否是寄存器
	int REGbits;
	int regCode = parseRegister(str, REGbits);
	if (regCode != -1) {
		operand.type = REG;
		operand.reg = regCode;
		operand.bits = REGbits;
		if (is_debug) {
			std::cout << "[DEBUG] 识别为寄存器: code=" << regCode
				<< ", bits=" << REGbits << std::endl;
		}
		return operand;
	}

	// 检查是否是内存操作数（带前缀）
	if (str.find("BYTE PTR") != std::string::npos) {
		operand.bits = BB;
	}
	else if (str.find("WORD PTR") != std::string::npos) {
		operand.bits = W16;
	}
	else if (str.find("DWORD PTR") != std::string::npos) {
		operand.bits = W32;
	}

	if (str.find("PTR") != std::string::npos) {
		if (is_debug) {
			std::cout << "[DEBUG] 识别为带前缀的内存操作数" << std::endl;
		}
		size_t bracketStart = str.find('[');
		if (bracketStart != std::string::npos) {
			return parseMemoryOperand(str, def_num_base);
		}
	}

	// 检查是否是内存操作数（不带前缀）
	if (str[0] == '[') {
		if (is_debug) {
			std::cout << "[DEBUG] 识别为不带前缀的内存操作数" << std::endl;
		}
		return parseMemoryOperand(str, def_num_base);
	}

	// 检查是否是带位移的内存操作数
	size_t plusPos = str.find('+');
	if (plusPos != std::string::npos) {
		std::string baseStr = str.substr(0, plusPos);
		std::string dispStr = str.substr(plusPos + 1);

		// 解析基址寄存器
		int basebits;
		int baseReg = parseRegister(baseStr, basebits);
		if (baseReg != -1) {
			operand.type = MRG;
			operand.base = baseReg;
			operand.disp = parseImmediate(dispStr);
			operand.bits = basebits;
			return operand;
		}
	}

	// 如果都不是，则认为是立即数
	operand.type = IMM;
	operand.disp = parseImmediate(str, def_num_base);  // 传递def_num_base
	// 根据立即数的大小和SHORT关键字设置isExtended
	if (str.find("SHORT") != std::string::npos) {
		operand.bits = BB;  // SHORT跳转使用8位偏移
	}
	else {
		operand.bits = checkBitRange(operand.disp);
	}
	if (is_debug) {
		std::cout << "[DEBUG] 识别为立即数: 值=0x" << std::dec << operand.disp
			<< ", bits=" << operand.bits << std::endl;
	}
	return operand;
}

//操作数位数匹配(操作数数据,模版数据，比较几个,是否严格)
bool Assembler::isOperandMatchingbits(const std::vector<Operand>& parsedOperands, const t_cmddata& cmd, int requiredOperands, unsigned int jmpstart, int currentTotalLength) {
	bool cmd_bits_match_ok = true;
	int nums[3];
	nums[0] = cmd.bits1;
	nums[1] = cmd.bits2;
	nums[2] = cmd.bits3;
	if (cmd.type == C_JMC || cmd.type == C_CAL || cmd.type == C_LOOP) {
		if (cmd.arg1 == 6 && parsedOperands[0].type == IMM) {
			int offset = parsedOperands[0].disp - jmpstart - (currentTotalLength + 2);
			if (offset < -128 || offset > 127) {
				std::cout << "不在短跳转范围" << std::endl;
				cmd_bits_match_ok = false;
			}
		}
		return cmd_bits_match_ok;
	}
	if (requiredOperands >= 2 && cmd.arg1 == MRG && cmd.arg2 != IMM && parsedOperands[0].type == MRG) {
		for (int i = 1; i < requiredOperands; ++i) {
			if (parsedOperands[i].type == REG || parsedOperands[i].type == MRG) {
				if (nums[i] != parsedOperands[i].bits) {
					if (nums[i] == BB || nums[i] == W16 || nums[i] == W32) {
						if (is_debug) {
							std::cout << "[DEBUG] 模板bits和操作数 " << i << "不匹配！"
								<< std::endl;
						}
						cmd_bits_match_ok = false;
						break;
					}
					else if (nums[i] == WW && parsedOperands[i].bits == BB)
					{
						cmd_bits_match_ok = false;
						break;
					}
				}
			}
		}
	}
	else {
		for (int i = 0; i < requiredOperands; ++i) {
			if (parsedOperands[i].type == REG || parsedOperands[i].type == MRG) {
				if (nums[i] != parsedOperands[i].bits) {
					if (nums[i] == BB) {
						if (is_debug) {
							std::cout << "[DEBUG] 模板bits和操作数 " << i << "不匹配！"
								<< std::endl;
						}
						cmd_bits_match_ok = false;
						break;
					}
					else if (nums[i] == WW || nums[i] == W16 || nums[i] == W32)
					{
						if (parsedOperands[i].bits == BB) {
							cmd_bits_match_ok = false;
							break;
						}
					}
				}
			}
			else if (parsedOperands[i].type == IMM) {
				if (nums[i] != parsedOperands[i].bits) {
					if (nums[i] == WW || nums[i] == W16 || nums[i] == W32)
					{
						if (parsedOperands[i].bits == BB) {
							if (is_debug) {
								std::cout << "[DEBUG] 模板bits和操作数 " << i << "不匹配！"
									<< std::endl;
							}
							cmd_bits_match_ok = false;
							break;
						}
					}
					else if (nums[i] == BB && parsedOperands[i].bits != BB)
					{
						if (is_debug) {
							std::cout << "[DEBUG] 模板bits和操作数 " << i << "不匹配！"
								<< std::endl;
						}
						cmd_bits_match_ok = false;
						break;
					}
				}
			}
		}
	}


	return cmd_bits_match_ok;
}

std::string Assembler::DebugTypeToStr(int type) {
	// 指令格式常量定义
	//constexpr int NNN = 0;      // 无操作数
	//constexpr int REG = 1;      // 寄存器操作数
	//constexpr int MRG = 2;      // 内存/寄存器操作数
	//constexpr int IMM = 3;      // 立即数操作数
	//constexpr int MMA = 4;      // 仅内存操作数
	//constexpr int JOW = 5;      // 长跳转操作数（32位相对偏移）
	//constexpr int JOB = 6;      // 短跳转操作数（8位相对偏移）
	//constexpr int IMA = 7;      // 直接内存地址操作数
	//constexpr int RAC = 8;      // 累加器（EAX/AX/AL）
	//constexpr int RCM = 9;      // 通用寄存器操作数
	//constexpr int SCM = 10;     // 段寄存器操作数
	//constexpr int ONE = 11;     // 常数1（用于移位指令）

	switch (type) {
	case 0:
		return "NNN";
	case 1:
		return "REG";
	case 2:
		return "MRG";
	case 3:
		return "IMM";
	case 4:
		return "MMA";
	case 5:
		return "JOW";
	case 6:
		return "JOB";
	case 7:
		return "IMA";
	case 8:
		return "RAC";
	case 9:
		return "RCM";
	case 10:
		return "SCM";
	case 11:
		return "ONE";
	default:
		return "NNN";
	}
}

//判断模版汇编指令名称和某字符串是否相同
bool Assembler::isCmdnameMatch(const char* cmdName, const std::string& opcode) {
	if (!cmdName || opcode.empty() || cmdName[0] == '\0') {
		return false;
	}

	// 查找 opcode 在 cmdName 中的位置
	const char* pos = strstr(cmdName, opcode.c_str());
	while (pos) {
		// 检查前面是否是'/'或者cmdName的开始
		bool isStartValid = (pos == cmdName || *(pos - 1) == '/');

		// 检查后面是否是'/'或者cmdName的结束
		bool isEndValid = (pos[opcode.length()] == '/' || pos[opcode.length()] == '\0');

		// 如果匹配的开始和结束位置都有效，则匹配成功
		if (isStartValid && isEndValid) {
			return true;
		}

		// 查找下一个匹配
		pos = strstr(pos + 1, opcode.c_str());
	}

	return false;
}

//instruction:汇编语句;currentTotalLength:累计指令长度;def_num_base:立即数默认进制:0为16进制,1为10进制,其他为16进制;jmpstart:跳转立即数的起始基址
std::vector<unsigned char> Assembler::AssembleInstruction(const std::string& instruction, int currentTotalLength, int def_num_base, unsigned int jmpstart) {
	std::vector<unsigned char> machineCode;
	std::string upperInst = toUpper(instruction);
	bool foundMatch = false;

	// Debug output for current offset and machine code length
	if (is_debug) {
		std::cout << "\n[DEBUG] ========== 开始处理指令 ==========" << std::endl;
		std::cout << "[DEBUG] 原始指令: " << instruction << std::endl;
		std::cout << "[DEBUG] 转换后指令: " << upperInst << std::endl;
		std::cout << "[DEBUG] 当前偏移: 0x" << std::hex << currentTotalLength << std::endl;
	}

	// 检查并处理REP/REPE/REPNE前缀
	bool hasRepPrefix = false;
	bool hasRepnePrefix = false;
	if (upperInst.find("REP ") == 0 || upperInst.find("REPE ") == 0) {
		hasRepPrefix = true;
		if (upperInst.find("REPE ") == 0) {
			upperInst = upperInst.substr(5); // 移除"REPE "前缀
		}
		else {
			upperInst = upperInst.substr(4); // 移除"REP "前缀
		}
		if (is_debug) {
			std::cout << "[DEBUG] 检测到REP/REPE前缀，剩余指令: " << upperInst << std::endl;
		}
	}
	else if (upperInst.find("REPNE ") == 0) {
		hasRepnePrefix = true;
		upperInst = upperInst.substr(6); // 移除"REPNE "前缀
		if (is_debug) {
			std::cout << "[DEBUG] 检测到REPNE前缀，剩余指令: " << upperInst << std::endl;
		}
	}

	// Split instruction and operands
	std::istringstream iss(upperInst);
	std::string opcode;
	std::vector<std::string> operands;

	// Read opcode
	iss >> opcode;
	if (is_debug) {
		std::cout << "[DEBUG] 提取操作码: " << opcode << std::endl;
	}

	// 如果是MOVS指令，根据操作数大小选择正确的操作码
	bool isStringInstruction = false;
	if (opcode == "MOVS") {
		isStringInstruction = true;
		if (upperInst.find("BYTE PTR") != std::string::npos) {
			opcode = "MOVSB";
		}
		else if (upperInst.find("WORD PTR") != std::string::npos) {
			opcode = "MOVSW";
		}
		else if (upperInst.find("DWORD PTR") != std::string::npos) {
			opcode = "MOVSD";
		}
		if (is_debug) {
			std::cout << "[DEBUG] MOVS指令转换为: " << opcode << std::endl;
		}
	}

	// 如果是SCAS指令，根据操作数大小选择正确的操作码
	else if (opcode == "SCAS") {
		isStringInstruction = true;
		if (upperInst.find("BYTE PTR") != std::string::npos) {
			opcode = "SCASB";
		}
		else if (upperInst.find("WORD PTR") != std::string::npos) {
			opcode = "SCASW";
		}
		else if (upperInst.find("DWORD PTR") != std::string::npos) {
			opcode = "SCASD";
		}
		if (is_debug) {
			std::cout << "[DEBUG] SCAS指令转换为: " << opcode << std::endl;
		}
	}

	// 如果是STOS指令，根据操作数大小选择正确的操作码
	else if (opcode == "STOS") {
		isStringInstruction = true;
		if (upperInst.find("BYTE PTR") != std::string::npos) {
			opcode = "STOSB";
		}
		else if (upperInst.find("WORD PTR") != std::string::npos) {
			opcode = "STOSW";
		}
		else if (upperInst.find("DWORD PTR") != std::string::npos) {
			opcode = "STOSD";
		}
		if (is_debug) {
			std::cout << "[DEBUG] STOS指令转换为: " << opcode << std::endl;
		}
	}

	// 如果是LODS指令，根据操作数大小选择正确的操作码
	else if (opcode == "LODS") {
		isStringInstruction = true;
		if (upperInst.find("BYTE PTR") != std::string::npos) {
			opcode = "LODSB";
		}
		else if (upperInst.find("WORD PTR") != std::string::npos) {
			opcode = "LODSW";
		}
		else if (upperInst.find("DWORD PTR") != std::string::npos) {
			opcode = "LODSD";
		}
		if (is_debug) {
			std::cout << "[DEBUG] LODS指令转换为: " << opcode << std::endl;
		}
	}

	// 如果是CMPS指令，根据操作数大小选择正确的操作码
	else if (opcode == "CMPS") {
		isStringInstruction = true;
		if (upperInst.find("BYTE PTR") != std::string::npos) {
			opcode = "CMPSB";
		}
		else if (upperInst.find("WORD PTR") != std::string::npos) {
			opcode = "CMPSW";
		}
		else if (upperInst.find("DWORD PTR") != std::string::npos) {
			opcode = "CMPSD";
		}
		if (is_debug) {
			std::cout << "[DEBUG] CMPS指令转换为: " << opcode << std::endl;
		}
	}

	// Read remaining part as operand string
	std::string operandPart;
	std::getline(iss, operandPart);

	// 如果是字符串指令，直接生成机器码
	if (isStringInstruction) {
		// 字符串指令（MOVS/SCAS）默认就使用相应的段寄存器，不需要显式的段前缀

		// 添加REP/REPNE前缀
		if (hasRepPrefix) {
			machineCode.push_back(PREFIX_REP);
			if (is_debug) {
				std::cout << "[DEBUG] 添加REP前缀: F3H" << std::endl;
			}
		}
		else if (hasRepnePrefix) {
			machineCode.push_back(PREFIX_REPNE);
			if (is_debug) {
				std::cout << "[DEBUG] 添加REPNE前缀: F2H" << std::endl;
			}
		}

		// 添加字符串指令操作码
		if (opcode == "MOVSB") {
			machineCode.push_back(0xA4);
			if (is_debug) {
				std::cout << "[DEBUG] 添加MOVSB操作码: A4H" << std::endl;
			}
		}
		else if (opcode == "MOVSW") {
			machineCode.push_back(PREFIX_OPERAND);
			machineCode.push_back(0xA5);
			if (is_debug) {
				std::cout << "[DEBUG] 添加MOVSW操作码: 66 A5H" << std::endl;
			}
		}
		else if (opcode == "MOVSD") {
			machineCode.push_back(0xA5);
			if (is_debug) {
				std::cout << "[DEBUG] 添加MOVSD操作码: A5H" << std::endl;
			}
		}
		else if (opcode == "SCASB") {
			machineCode.push_back(0xAE);
			if (is_debug) {
				std::cout << "[DEBUG] 添加SCASB操作码: AEH" << std::endl;
			}
		}
		else if (opcode == "SCASW") {
			//machineCode.push_back(0x66); // 操作数大小前缀
			machineCode.push_back(0xAF);
			if (is_debug) {
				std::cout << "[DEBUG] 添加SCASW操作码: 66 AFH" << std::endl;
			}
		}
		else if (opcode == "SCASD") {
			machineCode.push_back(0xAF);
			if (is_debug) {
				std::cout << "[DEBUG] 添加SCASD操作码: AFH" << std::endl;
			}
		}
		else if (opcode == "STOSB") {
			machineCode.push_back(0xAA);
			if (is_debug) {
				std::cout << "[DEBUG] 添加STOSB操作码: AAH" << std::endl;
			}
		}
		else if (opcode == "STOSW") {
			//machineCode.push_back(0x66);
			machineCode.push_back(0xAB);
			if (is_debug) {
				std::cout << "[DEBUG] 添加STOSW操作码: 66 ABH" << std::endl;
			}
		}
		else if (opcode == "STOSD") {
			machineCode.push_back(0xAB);
			if (is_debug) {
				std::cout << "[DEBUG] 添加STOSD操作码: ABH" << std::endl;
			}
		}
		else if (opcode == "LODSB") {
			machineCode.push_back(0xAC);
			if (is_debug) {
				std::cout << "[DEBUG] 添加LODSB操作码: ACH" << std::endl;
			}
		}
		else if (opcode == "LODSW") {
			//machineCode.push_back(0x66);
			machineCode.push_back(0xAD);
			if (is_debug) {
				std::cout << "[DEBUG] 添加LODSW操作码: 66 ADH" << std::endl;
			}
		}
		else if (opcode == "LODSD") {
			machineCode.push_back(0xAD);
			if (is_debug) {
				std::cout << "[DEBUG] 添加LODSD操作码: ADH" << std::endl;
			}
		}
		else if (opcode == "CMPSB") {
			machineCode.push_back(0xA6);
			if (is_debug) {
				std::cout << "[DEBUG] 添加CMPSB操作码: A6H" << std::endl;
			}
		}
		else if (opcode == "CMPSW") {
			//machineCode.push_back(0x66);
			machineCode.push_back(0xA7);
			if (is_debug) {
				std::cout << "[DEBUG] 添加CMPSW操作码: 66 A7H" << std::endl;
			}
		}
		else if (opcode == "CMPSD") {
			machineCode.push_back(0xA7);
			if (is_debug) {
				std::cout << "[DEBUG] 添加CMPSD操作码: A7H" << std::endl;
			}
		}

		if (is_debug) {
			std::cout << "[DEBUG] 添加字符串指令操作码" << std::endl;
		}

		return machineCode;
	}

	// 处理其他非字符串指令
	// ... existing code for normal instructions ...

	// Parse operands
	if (!operandPart.empty()) {
		std::string formattedOperands;
		bool inBrackets = false;
		std::string currentToken;

		// Format operands
		for (size_t i = 0; i < operandPart.length(); ++i) {
			char c = operandPart[i];
			if (c == '[') inBrackets = true;
			else if (c == ']') inBrackets = false;

			if (c == ',' && !inBrackets) {
				if (!currentToken.empty()) {
					formattedOperands += currentToken + " , ";
					currentToken.clear();
				}
			}
			else {
				currentToken += c;
			}
		}
		if (!currentToken.empty()) {
			formattedOperands += currentToken;
		}

		// Split operands
		std::istringstream operandStream(formattedOperands);
		std::string token;
		std::string currentOperand;

		while (operandStream >> token) {
			if (token != ",") {
				if (!currentOperand.empty()) currentOperand += " ";
				currentOperand += token;
			}
			else if (!currentOperand.empty()) {
				operands.push_back(currentOperand);
				currentOperand.clear();
			}
		}
		if (!currentOperand.empty()) {
			operands.push_back(currentOperand);
		}
	}

	// Parse each operand
	std::vector<Operand> parsedOperands;
	for (const auto& op : operands) {
		parsedOperands.push_back(parseOperand(op, def_num_base));
	}

	if (instruction == debug_pause_str) {
		std::cout << "临时暂停" << std::endl;
	}

	//转换两个REG操作数指令为1个一个MRG 一个REG,;
	if (parsedOperands.size() >= 2) {
		if (parsedOperands[0].type == 1 && parsedOperands[1].type == 1) {
			if (isCmdnameMatch("XOR/ADD/CMP/MOV/SUB/OR", opcode))
			{
				parsedOperands[1].type = 2;
				parsedOperands[1].typeassign = true;
			}
			else if (parsedOperands[0].bits == BB) {
				parsedOperands[1].type = 2;
				parsedOperands[1].typeassign = true;
			}
			else {
				parsedOperands[0].type = 2;
				parsedOperands[0].typeassign = true;
			}

		}
	}

	if (is_debug) {
		std::cout << "[DEBUG] 解析完成，操作数数量: " << parsedOperands.size() << std::endl;
		for (size_t i = 0; i < parsedOperands.size(); ++i) {
			const auto& op = parsedOperands[i];
			std::cout << "[DEBUG] 操作数 " << i + 1 << ": "
				<< "类型=" << DebugTypeToStr(op.type)
				<< ", 寄存器=" << op.reg
				<< ", 位数=" << op.bits
				//<< ", 基址=" << op.base
				//<< ", 变址=" << op.index
				//<< ", 比例=" << op.scale
				//<< ", 偏移=0x" << std::hex << op.disp
				<< std::endl;
		}
	}

	if (is_debug) {
		std::cout << "[DEBUG] 开始严格匹配..." << std::endl;
	}

	// 严格匹配开始
	const t_cmddata* bestMatch = nullptr;
	for (const auto& cmd : cmddata) {
		if (!isCmdnameMatch(cmd.name, opcode)) continue;

		bool match = true;
		// Check number of operands
		int requiredOperands = 0;
		if (cmd.arg1 != NNN) requiredOperands++;
		if (cmd.arg2 != NNN) requiredOperands++;
		if (cmd.arg3 != NNN) requiredOperands++;

		if (parsedOperands.size() != requiredOperands) {
			match = false;
			continue;
		}

		if (instruction == debug_pause_str && cmd.template_id == debug_pause_num) {
			std::cout << "临时暂停" << std::endl;
		}

		if (is_debug) {
			std::cout << "[DEBUG] 找到模版详情 "
				<< "模版编号=" << std::dec << cmd.template_id
				//<< " 指令名称=" << cmd.name
				//<< " 指令掩码=" << std::hex << cmd.mask
				<< " 基本操作码=" << std::hex << cmd.code
				//<< " 指令类型=" << std::dec << cmd.type
				//<< " 指令长度=" << std::dec << static_cast<int>(cmd.len)
				<< " 寄存器1=" << DebugTypeToStr(static_cast<int>(cmd.arg1))
				<< " 寄存器2=" << DebugTypeToStr(static_cast<int>(cmd.arg2))
				<< " 寄存器3=" << DebugTypeToStr(static_cast<int>(cmd.arg3))
				<< " 位数1=" << std::dec << static_cast<int>(cmd.bits1)
				<< " 位数2=" << std::dec << static_cast<int>(cmd.bits2)
				<< " 位数3=" << std::dec << static_cast<int>(cmd.bits3)
				<< std::endl;
		}

		if (match && cmd.arg1 != NNN && !isOperandMatchingArg(parsedOperands[0], cmd.arg1, true)) {
			if (is_debug) {
				std::cout << "[DEBUG] 严格匹配: 第一个操作数不匹配, 模板 " << std::dec << cmd.template_id << std::endl;
			}
			match = false;
		}
		if (match && cmd.arg2 != NNN && !isOperandMatchingArg(parsedOperands[1], cmd.arg2, true)) {
			if (is_debug) {
				std::cout << "[DEBUG] 严格匹配: 第二个操作数不匹配, 模板 " << std::dec << cmd.template_id << std::endl;
			}
			match = false;
		}
		if (match && cmd.arg3 != NNN && !isOperandMatchingArg(parsedOperands[2], cmd.arg3, true)) {
			if (is_debug) {
				std::cout << "[DEBUG] 严格匹配: 第三个操作数不匹配, 模板 " << std::dec << cmd.template_id << std::endl;
			}
			match = false;
		}
		if (match) {
			//检查操作数位数匹配
			if (!isOperandMatchingbits(parsedOperands, cmd, requiredOperands, jmpstart, currentTotalLength)) {
				if (is_debug) {
					std::cout << "[DEBUG] 严格匹配，操作数不匹配，跳过模板 " << std::dec << cmd.template_id
						<< std::endl;
				}
				match = false;
				continue;
			}
		}
		if (match) {
			if (is_debug) {
				std::cout << "[DEBUG] 严格匹配，找到匹配的指令模板: " << std::dec << cmd.template_id
					<< ", bits=" << (int)cmd.bits1 << std::endl;
			}
			bestMatch = &cmd;
			break;
		}
	}

	// If no exact match found, try fuzzy matching
	if (!bestMatch) {
		if (is_debug) {
			std::cout << "[DEBUG] 严格匹配失败，尝试模糊匹配..." << std::endl;
		}
		for (const auto& cmd : cmddata) {
			if (isCmdnameMatch(cmd.name, opcode)) {
				bool matches = true;
				int requiredOperands = 0;
				if (cmd.arg1 != NNN) requiredOperands++;
				if (cmd.arg2 != NNN) requiredOperands++;
				if (cmd.arg3 != NNN) requiredOperands++;

				if (parsedOperands.size() != requiredOperands) {
					matches = false;
					continue;
				}

				if (matches && cmd.arg1 != NNN && !isOperandMatchingArg(parsedOperands[0], cmd.arg1)) {
					if (is_debug) {
						std::cout << "[DEBUG] 模糊匹配: 第一个操作数不匹配, 模板 " << std::dec << cmd.template_id << std::endl;
					}
					matches = false;
				}
				if (matches && cmd.arg2 != NNN && !isOperandMatchingArg(parsedOperands[1], cmd.arg2)) {
					if (is_debug) {
						std::cout << "[DEBUG] 模糊匹配: 第二个操作数不匹配, 模板 " << std::dec << cmd.template_id << std::endl;
					}
					matches = false;
				}
				if (matches && cmd.arg3 != NNN && !isOperandMatchingArg(parsedOperands[2], cmd.arg3)) {
					if (is_debug) {
						std::cout << "[DEBUG] 模糊匹配: 第三个操作数不匹配, 模板 " << std::dec << cmd.template_id << std::endl;
					}
					matches = false;
				}

				if (matches) {
					//检查操作数位数匹配
					if (!isOperandMatchingbits(parsedOperands, cmd, 1, jmpstart, currentTotalLength)) {
						if (is_debug) {
							std::cout << "[DEBUG] 模糊匹配，操作数不匹配，跳过模板 " << std::dec << cmd.template_id
								<< std::endl;
						}
						matches = false;
						continue;
					}
				}
				if (matches) {
					if (is_debug) {
						std::cout << "[DEBUG] 模糊匹配成功: 找到模板 " << std::dec << cmd.template_id
							<< ", bits=" << (int)cmd.bits1 << std::endl;
					}
					bestMatch = &cmd;
					break;
				}
			}
		}
	}
	if (instruction == debug_pause_str) {
		std::cout << "临时暂停" << std::endl;
	}

	if (bestMatch) {
		return generateMachineCode(*bestMatch, parsedOperands, currentTotalLength, jmpstart);
	}

	if (is_debug) {
		std::cout << "[DEBUG] 没有找到匹配的指令模板" << std::endl;
	}
	return {};
}

int Assembler::checkBitRange(int num) {
	if (num <= 0xFF) { // 8位范围：0 ~ 255
		return 0;
	}
	else if (num <= 0xFFFF) { // 16位范围：0 ~ 65535
		return 1;
	}
	else if (num <= 0xFFFFFF) { // 24位范围：0 ~ 16777215
		return 2;
	}
	else if (num <= 0xFFFFFFFF) { // 32位范围：0 ~ 4294967295
		return 3;
	}
	else {
		return 3; // 超过32位
	}
}

// 生成机器码
std::vector<unsigned char> Assembler::generateMachineCode(const t_cmddata& cmd, const std::vector<Operand>& parsedOperands, int currentTotalLength, unsigned int jmpstart) {
	std::vector<unsigned char> machineCode;
	if (is_debug) {
		std::cout << "[DEBUG] 开始生成机器码..." << std::endl;
	}

	// 1. 生成前缀
	if (!parsedOperands.empty()) {
		// 对于16位操作数，添加操作数大小前缀
		if (parsedOperands[0].bits == W16) {
			machineCode.push_back(PREFIX_OPERAND);
			if (is_debug) {
				std::cout << "[DEBUG] 添加操作数大小前缀: 66H" << std::endl;
			}
		}
		else if (parsedOperands.size() > 1) {
			if (parsedOperands[0].type == MRG && parsedOperands[1].bits == W16) {
				machineCode.push_back(PREFIX_OPERAND);
				if (is_debug) {
					std::cout << "[DEBUG] 添加操作数大小前缀: 66H" << std::endl;
				}
			}
		}

		// 处理段前缀
		if (!parsedOperands[0].segment.empty()) {
			auto it = segmentPrefixes.find(parsedOperands[0].segment);
			if (it != segmentPrefixes.end()) {
				machineCode.push_back(it->second);
				if (is_debug) {
					std::cout << "[DEBUG] 添加段前缀: " << std::hex << static_cast<int>(it->second) << "H" << std::endl;
				}
			}
		}

		auto prefixes = generatePrefixes(parsedOperands[0]);
		machineCode.insert(machineCode.end(), prefixes.begin(), prefixes.end());
	}

	// 2. 生成基本操作码
	if (cmd.len == 2) {
		// 处理两字节操作码
		unsigned char opcode1 = static_cast<unsigned char>(cmd.code & 0xFF);
		unsigned char opcode2 = static_cast<unsigned char>((cmd.code >> 8) & 0xFF);
		machineCode.push_back(opcode1);
		machineCode.push_back(opcode2);
		if (is_debug) {
			std::cout << "[DEBUG] 添加两字节操作码: " << std::hex
				<< static_cast<int>(opcode1) << " "
				<< static_cast<int>(opcode2) << "H" << std::endl;
		}
	}
	else {
		// 处理单字节操作码
		unsigned char opcode = static_cast<unsigned char>(cmd.code & 0xFF);
		if (!parsedOperands.empty() && cmd.needs_modrm == false && parsedOperands[0].type == REG) {
			opcode += parsedOperands[0].reg & 0x07;  // 只使用低3位作为寄存器编号
		}
		machineCode.push_back(opcode);
		if (is_debug) {
			std::cout << "[DEBUG] 添加单字节操作码: " << std::hex
				<< static_cast<int>(opcode) << "H" << std::endl;
		}
	}


	// 3. 生成ModR/M字节（如果需要）
	if (needsModRM(cmd, parsedOperands)) {
		unsigned char modrm = generateModRMByte(cmd, parsedOperands);
		machineCode.push_back(modrm);
		if (is_debug) {
			std::cout << "[DEBUG] 添加ModR/M字节: " << std::hex << static_cast<int>(modrm) << "H" << std::endl;
		}

		// 4. 生成SIB字节（如果需要）
		const Operand& rmOperand = (cmd.arg2 == MRG) ? parsedOperands[1] : parsedOperands[0];
		if (needsSIB(rmOperand)) {
			unsigned char sib = generateSIB(rmOperand.scale, rmOperand.index, rmOperand.base);
			machineCode.push_back(sib);
			if (is_debug) {
				std::cout << "[DEBUG] 添加SIB字节: " << std::hex << static_cast<int>(sib) << "H" << std::endl;
			}
		}

		// 5. 生成位移字节
		if (rmOperand.type == MRG) {
			// 确定是否需要位移以及位移的大小
			bool needDisp = false;
			bool shortDisp = false;

			// 根据ModR/M的mod字段决定位移大小
			unsigned char mod = (modrm >> 6) & 0x03;

			if (mod == 0x00 && rmOperand.base == 5) {  // [EBP] 需要显式的位移
				needDisp = true;
				shortDisp = false;  // 32位位移
			}
			else if (mod == 0x01) {  // 8位位移
				needDisp = true;
				shortDisp = true;
			}
			else if (mod == 0x02) {  // 32位位移
				needDisp = true;
				shortDisp = false;
			}

			if (needDisp) {
				if (shortDisp) {
					// 8位位移
					machineCode.push_back(static_cast<unsigned char>(rmOperand.disp & 0xFF));
					if (is_debug) {
						std::cout << "[DEBUG] 添加8位位移: " << std::hex
							<< static_cast<int>(rmOperand.disp & 0xFF) << "H" << std::endl;
					}
				}
				else {
					// 32位位移
					for (int i = 0; i < 4; ++i) {
						machineCode.push_back(static_cast<unsigned char>((rmOperand.disp >> (i * 8)) & 0xFF));
					}
					if (is_debug) {
						std::cout << "[DEBUG] 添加32位位移: " << std::hex << rmOperand.disp << "H" << std::endl;
					}
				}
			}
		}
	}

	// 6. 生成立即数（如果有）
	if (cmd.arg1 != ONE && cmd.arg2 != ONE && cmd.arg3 != ONE) {
		for (size_t i = 0; i < parsedOperands.size(); ++i) {
			if (parsedOperands[i].type == IMM) {
				int immValue = parsedOperands[i].disp;
				int immSize;
				if (cmd.imm_bytes > 0) {
					// 修改这里：根据操作数实际大小确定立即数大小
					if (parsedOperands[0].bits == W16) {  // 检查目标操作数是否是16位
						immSize = 2;  // 16位立即数
					}
					else {
						immSize = cmd.imm_bytes;  // 使用模板定义的大小
					}
				}
				else {
					immSize = checkBitRange(immValue) + 1;
				}


				if (cmd.type == C_JMC || cmd.type == C_CAL || cmd.type == C_LOOP) {
					// 计算相对偏移：目标地址 - (当前地址 + 指令长度)
					int currentInstrLength = static_cast<int>(machineCode.size()) + immSize;
					immValue = immValue - jmpstart - (currentTotalLength + currentInstrLength);

					if (is_debug) {
						std::cout << "[DEBUG] 计算跳转偏移: 目标=0x" << std::hex << parsedOperands[i].disp
							<< " 当前=0x" << currentTotalLength
							<< " 指令长度=0x" << currentInstrLength
							<< " 相对偏移=0x" << immValue << std::endl;
					}
					//immValue = immValue - (currentTotalLength + 2);
				}

				// 生成立即数字节
				for (int j = 0; j < immSize; ++j) {
					machineCode.push_back(static_cast<unsigned char>((immValue >> (j * 8)) & 0xFF));
				}

				if (is_debug) {
					std::cout << "[DEBUG] 添加" << std::dec << (immSize * 8) << "位立即数: "
						<< std::hex << immValue << "H" << std::endl;
				}
			}
		}
	}


	//if (is_debug) {
	//	std::cout << "[DEBUG] 机器码生成完成，总长度: "
	//		<< std::dec << machineCode.size() << " 字节" << std::endl;
	//	std::cout << "[DEBUG] 生成的机器码: ";
	//	for (unsigned char byte : machineCode) {
	//		std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
	//			<< static_cast<int>(byte) << " ";
	//	}
	//	std::cout << std::endl;
	//}

	return machineCode;
}

std::string Assembler::BytesToHexString(const std::vector<unsigned char>& bytes) {
	std::stringstream ss;
	for (size_t i = 0; i < bytes.size(); ++i) {
		ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
			<< static_cast<int>(bytes[i]);
		if (i < bytes.size() - 1) {
			ss << " ";
		}
	}
	return ss.str();
}

// Add new function: converting byte array to Easy Language byte set format
std::string Assembler::BytesToByteSet(const std::vector<unsigned char>& bytes) {
	std::ostringstream oss;
	oss << "{";
	for (size_t i = 0; i < bytes.size(); ++i) {
		oss << static_cast<int>(bytes[i]);
		if (i < bytes.size() - 1) {
			oss << ",";
		}
	}
	oss << "}";
	return oss.str();
}

std::vector<unsigned char> Assembler::HexStringToBytes(const std::string& hex) {
	std::vector<unsigned char> bytes;
	for (size_t i = 0; i < hex.length(); i += 3) {
		std::string byteString = hex.substr(i, 2);
		unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
		bytes.push_back(byte);
	}
	return bytes;
}

// Add generateSIB function implementation
unsigned char Assembler::generateSIB(int scale, int index, int base) {
	int scaleField = 0;
	switch (scale) {
	case 2: scaleField = 1; break;
	case 4: scaleField = 2; break;
	case 8: scaleField = 3; break;
	}
	int index_no = (index == -1) ? 4 : index;
	return static_cast<unsigned char>((scaleField << 6) | ((index_no & 0x07) << 3) | (base & 0x07));
}

bool Assembler::isOperandMatchingArg(const Operand& op, int argType, bool strict) {
	if (is_debug) {
		std::cout << "[DEBUG] 检查操作数匹配: 操作数类型=" << DebugTypeToStr(op.type)
			<< ", 模版类型=" << argType
			<< ", 寄存器=" << op.reg << std::endl;
	}

	switch (argType) {
	case REG: // 寄存器		
		if (strict == true) {
			return op.type == REG;
		}
		return op.typeassign == true || op.type == REG;

	case MRG: // 内存/寄存器
		// 内存操作数或寄存器操作数都可以
		if (strict == true) {
			return op.type == MRG;
		}
		return op.type == MRG || op.type == REG;
	case IMM: // 立即数
		return op.type == IMM;
	case RCM: // 通用寄存器
		//if (strict == true) {
		//	return op.type == REG && op.reg != 0;
		//}
		return op.type == REG;
	case RAC: // 累加器 (EAX, AX, AL)
		return op.type == REG && op.reg == 0;
	case JOB: // 短跳转
		return op.type == JOB || op.type == IMM;
	case JOW: // 长跳转
		return op.type == JOW || op.type == IMM;
	case IMA: // 直接内存地址
		return op.type == IMM;
	case MMA: // 仅内存
		return op.type == MRG;
	case ONE: // 常数1
		return op.type == IMM && op.disp == 1;
	case NNN: // 无操作数
		return true;
	default:
		return false;
	}
}


// Preprocess entire assembly code
std::string Assembler::preprocessAsmCode(const std::string& code) {
	std::string result = code;

	// 将整个代码转换为大写
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);

	// 移除开头和结尾的空白字符
	result.erase(0, result.find_first_not_of(" \t\r\n"));
	result.erase(result.find_last_not_of(" \t\r\n") + 1);

	// 将多个连续的换行符替换为单个换行符
	result.erase(std::unique(result.begin(), result.end(),
		[](char a, char b) { return a == '\n' && b == '\n'; }), result.end());

	return result;
}

// 创建一个新的操作数对象
Operand Assembler::createOperand(int type, int reg, int base, int index,
	int scale, int bits, int disp, bool isExtended,
	const std::string& segment) {
	Operand op;
	op.type = type;          // 操作数类型
	op.reg = reg;            // 寄存器编号
	op.base = base;          // 基址寄存器
	op.index = index;        // 变址寄存器
	op.scale = scale;        // 比例因子
	op.disp = disp;          // 偏移量
	op.bits = 0;
	op.segment = segment;    // 段前缀
	return op;
}




// 判断指令是否需要ModR/M字节
bool Assembler::needsModRM(const t_cmddata& cmd, const std::vector<Operand>& operands) {
	if (cmd.needs_modrm) return true;  // 指令定义要求ModR/M
	if (cmd.needs_modrm == false) return false;  // 指令定义要求ModR/M
	if (operands.empty()) return false; // 没有操作数

	// 检查是否存在寄存器或内存操作数
	for (const auto& op : operands) {
		if (op.type == REG || op.type == MRG) return true;
	}
	return false;
}

// 生成ModR/M字节
unsigned char Assembler::generateModRMByte(const t_cmddata& cmd, const std::vector<Operand>& operands) {
	if (operands.empty()) return 0;

	// 初始化ModR/M字节的各个字段
	unsigned char mod = 0;
	unsigned char reg = -1;
	unsigned char rm = 0;
	bool rm_set = false;  // 标记rm是否已经被设置

	if (is_debug) {
		std::cout << "[DEBUG] 生成ModR/M字节，指令: " << cmd.name
			<< ", 操作数数量: " << operands.size() << std::endl;
	}

	if (operands.size() == 1) {
		// 单操作数指令
		if (cmd.reg_field >= 0) {
			// 使用指令定义中的reg字段（用于扩展操作码）
			reg = cmd.reg_field;
		}
		else {
			reg = operands[0].reg;
		}
	}
	else if (operands.size() >= 2) {
		if (cmd.reg_field >= 0) {
			// 使用指令定义中的reg字段（用于扩展操作码）
			reg = cmd.reg_field;
		}

		if (reg == 0 && cmd.arg1 == MRG && cmd.arg2 == REG) {
			// MRG,REG格式：reg字段放源寄存器
						//本来是REG,REG,第一个REG被强制指定为MRG
			if (operands[0].typeassign) {
				reg = operands[1].reg;
			}
			else {
				if (operands[0].reg >= 0) {
					reg = operands[0].reg;
				}
				else if (operands[1].reg >= 0) {
					reg = operands[1].reg;
				}
				else
				{
					reg = 0;
				}
			}
		}
		else if (reg == 0 && cmd.arg1 == REG && cmd.arg2 == MRG) {
			if (operands[1].typeassign) {
				//例如MOV AL,DL; MOV EDX,ECX
				if (isCmdnameMatch("XOR/ADD/CMP/MOV/SUB/OR/", cmd.name)) {
					reg = operands[0].reg;
				}
				else {
					reg = operands[1].reg;
				}

			}
			else {
				// REG,MRG格式：reg字段放目标寄存器
				reg = operands[0].reg;
			}

		}
	}

	// 确定mod和rm字段（如果还没设置）
	if (!rm_set) {
		const Operand& rmOperand = (cmd.arg2 == MRG) ? operands[1] : operands[0];

		if (rmOperand.type == REG) {
			// 寄存器直接寻址
			mod = 0b11;
			rm = rmOperand.reg;
		}
		else if (rmOperand.type == MRG) {
			// 内存操作数
			if (rmOperand.base != -1) {
				rm = rmOperand.base;
				if (rmOperand.disp == 0 && rmOperand.base != 5) { // [EBP] 需要显式的 0 位移
					mod = 0b00;
				}
				else if (rmOperand.disp >= -128 && rmOperand.disp <= 127) {
					//if (operands[0].type == MRG && operands[0].bits == 2 && operands[0].typeassign==false) {
					//	mod = 0b10;  // 32位位移
					//}else{
					mod = 0b01;  // 8位位移
				//}						
				}
				else {

					mod = 0b10;  // 32位位移
				}
			}
			else {
				mod = 0b11;
				if (operands[1].typeassign) {
					//例如MOV AL,DL ; MOV EDX,ECX
					if (isCmdnameMatch("XOR/ADD/CMP/MOV/SUB/OR/", cmd.name)) {
						rm = operands[1].reg;
					}
					else {
						rm = operands[0].reg;
					}
				}
				else {
					rm = rmOperand.reg;  // 直接寻址			
				}

			}

			// 检查是否需要SIB字节
			if (needsSIB(rmOperand)) {
				rm = 0b100;  // 表示使用SIB
			}
		}
	}



	// 组合ModR/M字节
	unsigned char modrm = (mod << 6) | ((reg & 0x07) << 3) | (rm & 0x07);

	if (is_debug) {
		std::cout << "[DEBUG] ModR/M字节组成: mod=" << static_cast<int>(mod)
			<< ", reg=" << static_cast<int>(reg)
			<< ", rm=" << static_cast<int>(rm) << std::endl;
		std::cout << "[DEBUG] 最终ModR/M字节: 0x" << std::hex
			<< static_cast<int>(modrm) << std::endl;
	}

	return modrm;
}

int main() {
	// 设置调试开关
	bool is_debug = true;  // 控制调试输出的开关
	bool is_debug_line = true; // 控制单行输出的开关
	Assembler assembler;
	assembler.setDebug(is_debug);  // 设置汇编器的调试状态
	unsigned int jmpstart = 0;  //跳转指令起始地址

	// 测试用的汇编代码
	std::string samstr = R"(
MOV EDX,[EBP+8]
MOV EDX,[EDX]
XOR EAX,EAX
TEST EDX,EDX
JE SHORT 00000058
PUSH ECX
MOV CL,[EDX]
PUSH EBX
XOR BL,BL
XOR EAX,EAX
CMP CL,20
JNZ SHORT 00000021
MOV CL,[EDX+1]
INC EDX
CMP CL,20
JE SHORT 00000018
MOV CL,[EDX]
CMP CL,2D
JNZ SHORT 0000002C
MOV BL,1
JMP SHORT 00000031
CMP CL,2B
JNZ SHORT 00000032
INC EDX
MOV CL,[EDX]
CMP CL,30
JL SHORT 00000050
CMP CL,39
JG SHORT 00000050
MOVSX ECX,CL
LEA EAX,[EAX+EAX*4]
INC EDX
LEA EAX,[ECX+EAX*2-30]
MOV CL,[EDX]
CMP CL,30
JGE SHORT 00000039
TEST BL,BL
POP EBX
POP ECX
JE SHORT 00000058
NEG EAX
POP EBP
RETN 4

)";

	std::string verifystr = "8B 55 08 8B 12 33 C0 85 D2 74 4D 51 8A 0A 53 32 DB 33 C0 80 F9 20 75 09 8A 4A 01 42 80 F9 20 74 F7 8A 0A 80 F9 2D 75 04 B3 01 EB 05 80 F9 2B 75 01 42 8A 0A 80 F9 30 7C 17 80 F9 39 7F 12 0F BE C9 8D 04 80 42 8D 44 41 D0 8A 0A 80 F9 30 7D E9 84 DB 5B 59 74 02 F7 D8 5D C2 04 00";

	// 预处理汇编代码（转换大写，清理空白等）
	samstr = assembler.preprocessAsmCode(samstr);

	// 用于存储所有指令的机器码
	std::vector<unsigned char> totalMachineCode;
	std::istringstream iss(samstr);
	std::string line;
	int lineNumber = 0;//当前处理汇编语句行数
	int totalLength = 0;  // 记录累积的机器码长度
	std::vector <ResultInfo> LineResult;
	// 逐行处理汇编代码
	while (std::getline(iss, line))
	{
		// 跳过空行
		if (line.empty()) continue;

		lineNumber++;

		// 输出调试信息：显示当前处理的指令
		if (is_debug) {
			std::cout << "\n[DEBUG] 处理第 " << std::dec << lineNumber << " 行: " << line << std::endl;
			std::cout << "[DEBUG] 当前累积机器码长度: 0x" << std::hex << totalLength << std::endl;
			std::cout << "[DEBUG] 当前指令偏移: 0x" << std::hex << totalLength << std::endl;
		}

		debug_pause_str = "";
		debug_pause_num = 1410;

		// 解析指令并生成机器码()
		auto instrBytes = assembler.AssembleInstruction(line, totalLength, 0);

		// 将当前指令的机器码添加到总机器码中
		totalMachineCode.insert(totalMachineCode.end(), instrBytes.begin(), instrBytes.end());

		// 更新累积的机器码长度
		totalLength += static_cast<int>(instrBytes.size());

		// 创建新的ResultInfo对象并添加到向量中
		ResultInfo result;
		result.asmstr = line;
		result.hexstr = assembler.BytesToHexString(instrBytes);
		result.hexlen = static_cast<int>(instrBytes.size());
		result.Bytestr = assembler.BytesToByteSet(instrBytes);
		LineResult.push_back(result);

		// 输出调试信息：显示生成的机器码
		if (is_debug) {
			std::cout << "[DEBUG] 生成的机器码: " << result.hexstr << std::endl;
			std::cout << "[DEBUG] 生成的字节集: " << result.Bytestr << std::endl;
			std::cout << "[DEBUG] 更新后的累积机器码长度: 0x" << std::hex << totalLength << std::endl;
			std::cout << "[DEBUG] 当前总机器码: " << assembler.BytesToHexString(totalMachineCode) << std::endl;
		}
		else if (is_debug_line) {
			std::cout << "\n处理第 " << std::dec << lineNumber << " 行: " << std::endl;
			std::cout << "汇编语句：" << result.asmstr << std::endl;
			std::cout << "机器码：" << result.hexstr << std::endl;
			std::cout << "字节集：" << result.Bytestr << std::endl;
			std::cout << "机器码长度：" << result.hexlen << std::endl;
		}
	}


	// Output final result
	std::string resulthstr = assembler.BytesToHexString(totalMachineCode);
	std::cout << "\n生成机器码：" << resulthstr << std::endl;
	std::cout << "\n效验机器码：" << verifystr << std::endl;
	std::cout << "\n字节集格式：" << assembler.BytesToByteSet(totalMachineCode) << std::endl;
	std::cout << "\n总长度：0x" << std::hex << totalMachineCode.size() << " ("
		<< std::dec << totalMachineCode.size() << " 字节)" << std::endl;
	std::cout << "\n" << std::endl;


	if (verifystr != resulthstr)
	{
		// 将两个机器码字符串分割成字节数组
		std::vector<std::string> verifyBytes;
		std::vector<std::string> generatedBytes;
		std::string temp;
		std::istringstream verifyStream(verifystr);
		std::istringstream generatedStream(resulthstr);

		while (verifyStream >> temp) verifyBytes.push_back(temp);
		while (generatedStream >> temp) generatedBytes.push_back(temp);

		// 首先检查两个字节数组是否真的不同
		bool isDifferent = false;
		if (verifyBytes.size() != generatedBytes.size()) {
			isDifferent = true;
		}
		else {
			for (size_t i = 0; i < verifyBytes.size(); ++i) {
				if (verifyBytes[i] != generatedBytes[i]) {
					isDifferent = true;
					break;
				}
			}
		}

		if (isDifferent) 
		{
			// 找出第一个不同的字节位置
			size_t diffIndex = 0;
			for (size_t i = 0; i < verifyBytes.size() && i < generatedBytes.size(); ++i) {
				if (verifyBytes[i] != generatedBytes[i]) {
					diffIndex = i;
					break;
				}
			}

			// 计算这个字节属于哪条汇编指令
			size_t currentLength = 0;
			size_t instructionIndex = 0;
			for (size_t i = 0; i < LineResult.size(); ++i) {
				if (currentLength + LineResult[i].hexlen > diffIndex) {
					instructionIndex = i;
					break;
				}
				currentLength += LineResult[i].hexlen;
			}

			std::cout << "第" << diffIndex + 1 << "个机器码不同，需要检查！" << std::endl;
			std::cout << "效验机器码：" << verifyBytes[diffIndex] << " (长度: " << verifyBytes[diffIndex].length() << ")" << std::endl;
			std::cout << "生成机器码：" << generatedBytes[diffIndex] << " (长度: " << generatedBytes[diffIndex].length() << ")" << std::endl;
			std::cout << "对应的汇编语句：" << LineResult[instructionIndex].asmstr << std::endl;
		}
		else
		{
			std::cout << "生成的机器码和效验机器码完全相同" << std::endl;

		}
	}
	else {
		std::cout << "生成的机器码和效验机器码完全相同" << std::endl;
	}

	return 0;
}

