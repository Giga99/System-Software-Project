#ifndef REGEX_WRAPPER_H
#define REGEX_WRAPPER_H

#include <regex>
#include <string>

using namespace std;

class RegexWrapper
{
private:
    const regex regexComment = regex("([^#]*)#.*");
    const regex regexSpaces = regex(" {2,}");
    const regex regexTabs = regex("\\t");
    const regex regexBoundarySpaces = regex("^( *)([^ ].*[^ ])( *)$");
    const regex regexCommaSpaces = regex(" ?, ?");
    const regex regexColumnsSpaces = regex(" ?: ?");

    const regex regexDecimal = regex("^(-?[0-9]+)$");
    const regex regexHexaDecimal = regex("^(0x[0-9A-F]+)$");

    const regex regexGlobalDirective = regex("^\\.global ([a-zA-Z][a-zA-Z0-9_]*(,[a-zA-Z][a-zA-Z0-9_]*)*)$");
    const regex regexExternalDirective = regex("^\\.extern ([a-zA-Z][a-zA-Z0-9_]*(,[a-zA-Z][a-zA-Z0-9_]*)*)$");
    const regex regexSectionDirective = regex("^\\.section ([a-zA-Z][a-zA-Z0-9_]*)$");
    const regex regexWordDirective = regex("^\\.word (([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)(,([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+))*)$");
    const regex regexSkipDirective = regex("^\\.skip (-?[0-9]+|0x[0-9A-F]+)$");
    const regex regexEquDirective = regex("^\\.equ ([a-zA-Z][a-zA-Z0-9_]*),(-?[0-9]+|0x[0-9A-F]+)$");
    const regex regexEndDirective = regex("^\\.end$");
    const regex regexLabel = regex("^([a-zA-Z][a-zA-Z0-9_]*):$");
    const regex regexLabelWithInstruction = regex("^([a-zA-Z][a-zA-Z0-9_]*):(.*)$");
    const regex regexSymbol = regex("^([a-zA-Z][a-zA-Z0-9_]*)$");

    const regex regexNoOperandInstruction = regex("^(halt|iret|ret)$");
    const regex regexOneOperandRegisterInstruction = regex("^(push|pop|int|not) (r[0-7]|psw)$");
    const regex regexTwoOperandRegisterInstruction = regex("^(xchg|add|sub|mul|div|cmp|and|or|xor|test|shl|shr) (r[0-7]|psw),(r[0-7]|psw)$");
    const regex regexOneOperandJump = regex("^(call|jmp|jeq|jne|jgt) (.*)$");
    const regex regexTwoOperandLoadStore = regex("^(ldr|str) (r[0-7]|psw),(.*)$");

    const regex regexJumpAbsolute = regex("^([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)$");
    const regex regexJumpMemDir = regex("^\\*([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)$");
    const regex regexJumPCRelative = regex("^%([a-zA-Z][a-zA-Z0-9_]*)$");
    const regex regexJumpRegDir = regex("^\\*(r[0-7]|psw)$");
    const regex regexJumpRegInd = regex("^\\*\\[(r[0-7]|psw)\\]$");
    const regex regexJumpRegIndWithDisplacement = regex("^\\*\\[(r[0-7]|psw) \\+ ([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)\\]$");

    const regex regexLoadStoreAbsolute = regex("^\\$([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)$");
    const regex regexLoadStoreMemDir = regex("^([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)$");
    const regex regexLoadStorePCRelative = regex("^%([a-zA-Z][a-zA-Z0-9_]*)$");
    const regex regexLoadStoreRegDir = regex("^(r[0-7]|psw)$");
    const regex regexLoadStoreRegInd = regex("^\\[(r[0-7]|psw)\\]$");
    const regex regexLoadStoreRegIndWithDisplacement = regex("^\\[(r[0-7]|psw) \\+ ([a-zA-Z][a-zA-Z0-9_]*|-?[0-9]+|0x[0-9A-F]+)\\]$");

public:
    enum DirectiveType
    {
        GLOBAL,
        EXTERNAL,
        SECTION,
        WORD,
        SKIP,
        EQU,
        END,
        LABEL,
        LABEL_WITH_INSTRUCTION,
        INSTRUCTION
    };

    struct Directive
    {
        string param1, param2;
        DirectiveType type;
        Directive(string p1, string p2, DirectiveType t) : param1(p1), param2(p2), type(t) {}
    };

    enum InstructionType
    {
        NO_OPERAND,
        ONE_OPERAND,
        TWO_OPERAND,
        ONE_OPERAND_JUMP,
        TWO_OPERAND_LOAD_STORE,
        BAD_INSTRUCTION
    };

    struct Instruction
    {
        string param1, param2, param3;
        InstructionType type;
        Instruction(string p1, string p2, string p3, InstructionType t) : param1(p1), param2(p2), param3(p3), type(t) {}
    };

    enum JumpType
    {
        JUMP_ABS,
        JUMP_MEM_DIR,
        JUMP_PC_RELATIVE,
        JUMP_REG_DIR,
        JUMP_REG_IND,
        JUMP_REG_IND_DISPL,
        BAD_JUMP
    };

    struct Jump
    {
        string param1, param2;
        JumpType type;
        Jump(string p1, string p2, JumpType t) : param1(p1), param2(p2), type(t) {}
    };

    enum LoadStoreType
    {
        LOAD_STORE_ABS_SYMBOL,
        LOAD_STORE_ABS_VALUE,
        LOAD_STORE_MEM_DIR_SYMBOL,
        LOAD_STORE_MEM_DIR_VALUE,
        LOAD_STORE_PC_RELATIVE,
        LOAD_STORE_REG_DIR,
        LOAD_STORE_REG_IND,
        LOAD_STORE_REG_IND_DISPL_SYMBOL,
        LOAD_STORE_REG_IND_DISPL_VALUE,
        BAD_LOAD_STORE
    };

    struct LoadStore
    {
        string param1, param2;
        LoadStoreType type;
        LoadStore(string p1, string p2, LoadStoreType t) : param1(p1), param2(p2), type(t) {}
    };

    enum LiteralType
    {
        DECIMAL,
        HEXA_DECIMAL,
        ERROR
    };

    struct Literal
    {
        string param1;
        LiteralType type;
        Literal(string p1, LiteralType t) : param1(p1), type(t) {}
    };

    Directive searchLine(string line);
    Instruction searchInstruction(string line);
    Jump searchJump(string operand);
    LoadStore searchLoadStore(string operand);
    Literal searchLiteral(string literal);
    bool isSymbol(string operand);
    string removeBlankLinesComments(string line);
    RegexWrapper();
    ~RegexWrapper();
};

#endif
