#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

// 指令格式常量定义
constexpr int NNN = 0;      // 无操作数
constexpr int REG = 1;      // 寄存器操作数
constexpr int MRG = 2;      // 内存/寄存器操作数
constexpr int IMM = 3;      // 立即数操作数
constexpr int MMA = 4;      // 仅内存操作数
constexpr int JOW = 5;      // 长跳转操作数（32位相对偏移）
constexpr int JOB = 6;      // 短跳转操作数（8位相对偏移）
constexpr int IMA = 7;      // 直接内存地址操作数
constexpr int RAC = 8;      // 累加器（EAX/AX/AL）
constexpr int RCM = 9;      // 通用寄存器操作数
constexpr int SCM = 10;     // 段寄存器操作数
constexpr int ONE = 11;     // 常数1（用于移位指令）

// 指令类型定义
constexpr int C_CMD = 0x0000;  // 普通指令
constexpr int C_PSH = 0x0100;  // PUSH类指令
constexpr int C_POP = 0x0200;  // POP类指令
constexpr int C_CAL = 0x0300;  // CALL类指令
constexpr int C_JMC = 0x0400;  // 条件跳转指令
constexpr int C_LOOP = 0x0500; // 循环指令类型 - 新增
constexpr int C_RARE = 0x0800; // 罕见/特殊指令

// 操作数或寄存器大小标志
constexpr int BB = 0;   // Byte（8位）操作数
constexpr int W16 = 1;   // Word（16）操作数
constexpr int W32 = 2;   // DWord（32位）操作数
constexpr int WW = 3;   // Word/DWord（16/32位）操作数
constexpr int QQ = 4;   // QWord（64位）操作数
constexpr int W3 = 5;   // 3位宽度的Word操作数
constexpr int SS = 6;   // 栈操作数
constexpr int CC = 7;   // 条件码操作数
constexpr int MM = 8;   // 内存操作数
constexpr int RR = 9;   // 寄存器操作数
constexpr int II = 10;   // 立即数操作数


// ModR/M字节的Mod字段定义
constexpr unsigned char MOD_INDIRECT = 0x00;    // 间接寻址模式（无位移）
constexpr unsigned char MOD_DISP8 = 0x40;       // 8位位移寻址模式
constexpr unsigned char MOD_DISP32 = 0x80;      // 32位位移寻址模式
constexpr unsigned char MOD_REGISTER = 0xC0;    // 寄存器直接寻址模式

// 指令前缀字节定义
constexpr unsigned char PREFIX_OPERAND = 0x66;  // 操作数大小前缀（切换16/32位）
constexpr unsigned char PREFIX_ADDRESS = 0x67;  // 地址大小前缀（切换16/32位寻址）
constexpr unsigned char PREFIX_LOCK = 0xF0;     // LOCK前缀（原子操作）
constexpr unsigned char PREFIX_REP = 0xF3;      // REP前缀（字符串操作重复）
constexpr unsigned char PREFIX_REPNE = 0xF2;    // REPNE前缀（字符串操作条件重复）

// 段寄存器前缀定义
constexpr unsigned char PREFIX_ES = 0x26;  // ES段前缀
constexpr unsigned char PREFIX_CS = 0x2E;  // CS段前缀
constexpr unsigned char PREFIX_SS = 0x36;  // SS段前缀
constexpr unsigned char PREFIX_DS = 0x3E;  // DS段前缀
constexpr unsigned char PREFIX_FS = 0x64;  // FS段前缀
constexpr unsigned char PREFIX_GS = 0x65;  // GS段前缀

// 指令编码数据结构
struct t_cmddata {
    unsigned int mask;       // 指令掩码（用于匹配操作码）
    unsigned int code;       // 基本操作码
    unsigned char len;       // 指令长度（字节数）
    unsigned char bits1;      // 操作数1大小标志
    unsigned char bits2;      // 操作数2大小标志
    unsigned char bits3;      // 操作数3大小标志
    unsigned char arg1;      // 第一个操作数类型
    unsigned char arg2;      // 第二个操作数类型
    unsigned char arg3;      // 第三个操作数类型
    unsigned int type;       // 指令类型标志
    const char* name;        // 指令助记符
    int template_id;         // 指令模板编号
    unsigned char opcode_bytes;     // 操作码字节数
    bool needs_modrm;              // 是否需要ModR/M字节
    unsigned char reg_field;        // ModR/M字节中的reg字段值
    bool has_immediate;            // 是否包含立即数
    unsigned char imm_bytes;       // 立即数的字节数
    bool reverse_operands;         // 是否需要反转操作数顺序
};

// 寄存器信息数据结构
struct RegInfo {
    const char* name;        // 寄存器名称
    unsigned char code;      // 寄存器编码
    int bits;                //寄存器位数
};

// 操作数数据结构
struct Operand {
    int type;               // 操作数类型
    int reg;               // 寄存器编号
    int base;              // 基址寄存器
    int index;             // 变址寄存器
    int scale;             // 比例因子
    int disp;              // 偏移量
    int bits;              // 操作数1大小标志
    std::string segment;   // 段前缀
    bool typeassign;        //是否都是寄存器，有强制指定为MRG

    Operand() : type(NNN), reg(-1), base(-1), index(-1), scale(1), disp(0),
        bits(0), segment(""), typeassign(false) {}
};

// 汇编器类定义
class Assembler {
public:
    // 构造和析构函数
    Assembler();
    ~Assembler() = default;

    // 主要功能函数
    std::vector<unsigned char> AssembleInstruction(const std::string& instruction, int currentTotalLength, int def_num_base = 0);  // 汇编单条指令
    std::string BytesToHexString(const std::vector<unsigned char>& bytes);  // 将字节序列转换为十六进制字符串
    std::string BytesToByteSet(const std::vector<unsigned char>& bytes);    // 将字节序列转换为字节集格式
    std::vector<unsigned char> HexStringToBytes(const std::string& hex);    // 将十六进制字符串转换为字节序列
    void setDebug(bool debug) { is_debug = debug; }                        // 设置调试模式
    std::string preprocessAsmCode(const std::string& code);                // 预处理整个汇编代码

private:
    // 内部辅助函数
    bool isCmdnameMatch(const char* cmdName, const std::string& opcode);//判断模版name和指令名字是否相同
    std::string toUpper(const std::string& str);                          // 转换为大写
    int parseRegister(const std::string& reg, int& Regbits);          // 解析寄存器
    int parseImmediate(const std::string& imm, int def_num_base = 0);
    Operand parseMemoryOperand(const std::string& operand, int def_num_base = 0);              // 解析内存操作数
    Operand parseOperand(const std::string& operandStr, int def_num_base);
    //unsigned char generateModRM(int mod, int reg, int rm);               // 生成ModR/M字节
    unsigned char generateSIB(int scale, int index, int base);           // 生成SIB字节
    std::vector<unsigned char> generatePrefixes(const Operand& op);      // 生成指令前缀
    bool needsSIB(const Operand& op);                                    // 检查是否需要SIB字节
    //bool isOperandMatchingArg_vague(const Operand& op, int argType);    // 模糊匹配操作数类型
    bool isOperandMatchingArg(const Operand& op, int argType, bool strict = false);          // 精确匹配操作数类型
    bool isOperandMatchingbits(const std::vector<Operand>& parsedOperands, const t_cmddata& cmd, int requiredOperands);//判断模版bits是否符合
    int checkBitRange(int num);//返回操作数位数
    std::string DebugTypeToStr(int type);//返回寄存器字符名称

    // 生成机器码的辅助函数
    std::vector<unsigned char> generateMachineCode(const t_cmddata& cmd,
        const std::vector<Operand>& parsedOperands, int currentTotalLength);

    // 内部数据成员
    std::map<std::string, int> labels;          // 标签表（地址映射）
    std::vector<unsigned char> currentCode;      // 当前生成的机器码
    int currentOffset;                          // 当前指令的偏移量
    int totalLength;                            // 已生成机器码的总长度
    bool is_debug;                              // 调试模式标志

    // 辅助函数
    Operand createOperand(int type = NNN, int reg = -1, int base = -1, int index = -1,
        int scale = 1, int disp = 0, int bits = 0, bool isExtended = false,
        const std::string& segment = "");  // 创建操作数对象

// 跳转相关的辅助函数
    bool needsModRM(const t_cmddata& cmd, const std::vector<Operand>& operands); // 判断是否需要ModR/M
    unsigned char generateModRMByte(const t_cmddata& cmd, const std::vector<Operand>& operands); // 生成ModR/M字节
};

// 外部数据声明
extern const t_cmddata cmddata[];           // 指令编码表
extern const RegInfo registers[];           // 寄存器信息表
extern const std::map<std::string, unsigned char> segmentPrefixes;  // 段前缀映射表