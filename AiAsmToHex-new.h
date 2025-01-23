#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

// ָ���ʽ��������
constexpr int NNN = 0;      // �޲�����
constexpr int REG = 1;      // �Ĵ���������
constexpr int MRG = 2;      // �ڴ�/�Ĵ���������
constexpr int IMM = 3;      // ������������
constexpr int MMA = 4;      // ���ڴ������
constexpr int JOW = 5;      // ����ת��������32λ���ƫ�ƣ�
constexpr int JOB = 6;      // ����ת��������8λ���ƫ�ƣ�
constexpr int IMA = 7;      // ֱ���ڴ��ַ������
constexpr int RAC = 8;      // �ۼ�����EAX/AX/AL��
constexpr int RCM = 9;      // ͨ�üĴ���������
constexpr int SCM = 10;     // �μĴ���������
constexpr int ONE = 11;     // ����1��������λָ�

// ָ�����Ͷ���
constexpr int C_CMD = 0x0000;  // ��ָͨ��
constexpr int C_PSH = 0x0100;  // PUSH��ָ��
constexpr int C_POP = 0x0200;  // POP��ָ��
constexpr int C_CAL = 0x0300;  // CALL��ָ��
constexpr int C_JMC = 0x0400;  // ������תָ��
constexpr int C_LOOP = 0x0500; // ѭ��ָ������ - ����
constexpr int C_RARE = 0x0800; // ����/����ָ��

// ��������Ĵ�����С��־
constexpr int BB = 0;   // Byte��8λ��������
constexpr int W16 = 1;   // Word��16��������
constexpr int W32 = 2;   // DWord��32λ��������
constexpr int WW = 3;   // Word/DWord��16/32λ��������
constexpr int QQ = 4;   // QWord��64λ��������
constexpr int W3 = 5;   // 3λ��ȵ�Word������
constexpr int SS = 6;   // ջ������
constexpr int CC = 7;   // �����������
constexpr int MM = 8;   // �ڴ������
constexpr int RR = 9;   // �Ĵ���������
constexpr int II = 10;   // ������������


// ModR/M�ֽڵ�Mod�ֶζ���
constexpr unsigned char MOD_INDIRECT = 0x00;    // ���Ѱַģʽ����λ�ƣ�
constexpr unsigned char MOD_DISP8 = 0x40;       // 8λλ��Ѱַģʽ
constexpr unsigned char MOD_DISP32 = 0x80;      // 32λλ��Ѱַģʽ
constexpr unsigned char MOD_REGISTER = 0xC0;    // �Ĵ���ֱ��Ѱַģʽ

// ָ��ǰ׺�ֽڶ���
constexpr unsigned char PREFIX_OPERAND = 0x66;  // ��������Сǰ׺���л�16/32λ��
constexpr unsigned char PREFIX_ADDRESS = 0x67;  // ��ַ��Сǰ׺���л�16/32λѰַ��
constexpr unsigned char PREFIX_LOCK = 0xF0;     // LOCKǰ׺��ԭ�Ӳ�����
constexpr unsigned char PREFIX_REP = 0xF3;      // REPǰ׺���ַ��������ظ���
constexpr unsigned char PREFIX_REPNE = 0xF2;    // REPNEǰ׺���ַ������������ظ���

// �μĴ���ǰ׺����
constexpr unsigned char PREFIX_ES = 0x26;  // ES��ǰ׺
constexpr unsigned char PREFIX_CS = 0x2E;  // CS��ǰ׺
constexpr unsigned char PREFIX_SS = 0x36;  // SS��ǰ׺
constexpr unsigned char PREFIX_DS = 0x3E;  // DS��ǰ׺
constexpr unsigned char PREFIX_FS = 0x64;  // FS��ǰ׺
constexpr unsigned char PREFIX_GS = 0x65;  // GS��ǰ׺

// ָ��������ݽṹ
struct t_cmddata {
    unsigned int mask;       // ָ�����루����ƥ������룩
    unsigned int code;       // ����������
    unsigned char len;       // ָ��ȣ��ֽ�����
    unsigned char bits1;      // ������1��С��־
    unsigned char bits2;      // ������2��С��־
    unsigned char bits3;      // ������3��С��־
    unsigned char arg1;      // ��һ������������
    unsigned char arg2;      // �ڶ�������������
    unsigned char arg3;      // ����������������
    unsigned int type;       // ָ�����ͱ�־
    const char* name;        // ָ�����Ƿ�
    int template_id;         // ָ��ģ����
    unsigned char opcode_bytes;     // �������ֽ���
    bool needs_modrm;              // �Ƿ���ҪModR/M�ֽ�
    unsigned char reg_field;        // ModR/M�ֽ��е�reg�ֶ�ֵ
    bool has_immediate;            // �Ƿ����������
    unsigned char imm_bytes;       // ���������ֽ���
    bool reverse_operands;         // �Ƿ���Ҫ��ת������˳��
};

// �Ĵ�����Ϣ���ݽṹ
struct RegInfo {
    const char* name;        // �Ĵ�������
    unsigned char code;      // �Ĵ�������
    int bits;                //�Ĵ���λ��
};

// ���������ݽṹ
struct Operand {
    int type;               // ����������
    int reg;               // �Ĵ������
    int base;              // ��ַ�Ĵ���
    int index;             // ��ַ�Ĵ���
    int scale;             // ��������
    int disp;              // ƫ����
    int bits;              // ������1��С��־
    std::string segment;   // ��ǰ׺
    bool typeassign;        //�Ƿ��ǼĴ�������ǿ��ָ��ΪMRG

    Operand() : type(NNN), reg(-1), base(-1), index(-1), scale(1), disp(0),
        bits(0), segment(""), typeassign(false) {}
};

// ������ඨ��
class Assembler {
public:
    // �������������
    Assembler();
    ~Assembler() = default;

    // ��Ҫ���ܺ���
    std::vector<unsigned char> AssembleInstruction(const std::string& instruction, int currentTotalLength, int def_num_base = 0);  // ��൥��ָ��
    std::string BytesToHexString(const std::vector<unsigned char>& bytes);  // ���ֽ�����ת��Ϊʮ�������ַ���
    std::string BytesToByteSet(const std::vector<unsigned char>& bytes);    // ���ֽ�����ת��Ϊ�ֽڼ���ʽ
    std::vector<unsigned char> HexStringToBytes(const std::string& hex);    // ��ʮ�������ַ���ת��Ϊ�ֽ�����
    void setDebug(bool debug) { is_debug = debug; }                        // ���õ���ģʽ
    std::string preprocessAsmCode(const std::string& code);                // Ԥ��������������

private:
    // �ڲ���������
    bool isCmdnameMatch(const char* cmdName, const std::string& opcode);//�ж�ģ��name��ָ�������Ƿ���ͬ
    std::string toUpper(const std::string& str);                          // ת��Ϊ��д
    int parseRegister(const std::string& reg, int& Regbits);          // �����Ĵ���
    int parseImmediate(const std::string& imm, int def_num_base = 0);
    Operand parseMemoryOperand(const std::string& operand, int def_num_base = 0);              // �����ڴ������
    Operand parseOperand(const std::string& operandStr, int def_num_base);
    //unsigned char generateModRM(int mod, int reg, int rm);               // ����ModR/M�ֽ�
    unsigned char generateSIB(int scale, int index, int base);           // ����SIB�ֽ�
    std::vector<unsigned char> generatePrefixes(const Operand& op);      // ����ָ��ǰ׺
    bool needsSIB(const Operand& op);                                    // ����Ƿ���ҪSIB�ֽ�
    //bool isOperandMatchingArg_vague(const Operand& op, int argType);    // ģ��ƥ�����������
    bool isOperandMatchingArg(const Operand& op, int argType, bool strict = false);          // ��ȷƥ�����������
    bool isOperandMatchingbits(const std::vector<Operand>& parsedOperands, const t_cmddata& cmd, int requiredOperands);//�ж�ģ��bits�Ƿ����
    int checkBitRange(int num);//���ز�����λ��
    std::string DebugTypeToStr(int type);//���ؼĴ����ַ�����

    // ���ɻ�����ĸ�������
    std::vector<unsigned char> generateMachineCode(const t_cmddata& cmd,
        const std::vector<Operand>& parsedOperands, int currentTotalLength);

    // �ڲ����ݳ�Ա
    std::map<std::string, int> labels;          // ��ǩ����ַӳ�䣩
    std::vector<unsigned char> currentCode;      // ��ǰ���ɵĻ�����
    int currentOffset;                          // ��ǰָ���ƫ����
    int totalLength;                            // �����ɻ�������ܳ���
    bool is_debug;                              // ����ģʽ��־

    // ��������
    Operand createOperand(int type = NNN, int reg = -1, int base = -1, int index = -1,
        int scale = 1, int disp = 0, int bits = 0, bool isExtended = false,
        const std::string& segment = "");  // ��������������

// ��ת��صĸ�������
    bool needsModRM(const t_cmddata& cmd, const std::vector<Operand>& operands); // �ж��Ƿ���ҪModR/M
    unsigned char generateModRMByte(const t_cmddata& cmd, const std::vector<Operand>& operands); // ����ModR/M�ֽ�
};

// �ⲿ��������
extern const t_cmddata cmddata[];           // ָ������
extern const RegInfo registers[];           // �Ĵ�����Ϣ��
extern const std::map<std::string, unsigned char> segmentPrefixes;  // ��ǰ׺ӳ���