#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <vector>
#include <regex>
#include <string>

#include "RegexWrapper.h"

using namespace std;

class Parser
{
private:
    static Parser *instance;

    static int symbolId;
    static int sectionId;

    const string UNDEFINED = "UNDEFINED";
    const string ABSOLUTE = "ABSOLUTE";

    const string R_H_16 = "R_H_16";
    const string R_H_16_PC = "R_H_16_PC";

    const string HALT = "halt";
    const string IRET = "iret";
    const string RET = "ret";

    const string PSW = "psw";
    const string INT = "int";
    const string NOT = "not";
    const string PUSH = "push";
    const string POP = "pop";

    const string CALL = "call";
    const string JMP = "jmp";
    const string JEQ = "jeq";
    const string JNE = "jne";
    const string JGT = "jgt";

    const string LDR = "ldr";
    const string STR = "str";

    const string XCHG = "xchg";
    const string ADD = "add";
    const string SUB = "sub";
    const string MUL = "mul";
    const string DIV = "div";
    const string CMP = "cmp";
    const string AND = "and";
    const string OR = "or";
    const string XOR = "xor";
    const string TEST = "test";
    const string SHL = "shl";
    const string SHR = "shr";

    struct AssemblerError
    {
        string message;
        int lineNumber;
        AssemblerError(string m, int l) : message(m), lineNumber(l) {}
    };
    struct Symbol
    {
        int symbolId, offset;
        bool isLocal, isDefined, isExtern;
        string section, name;
        Symbol(int id, int o, bool local, bool defined, bool ex, string s, string n) : symbolId(id), offset(o), isLocal(local), isDefined(defined), isExtern(ex), section(s), name(n) {}
    };
    struct Section
    {
        int sectionId, sectionSize;
        string sectionName;
        vector<char> data;
        vector<int> offsets;
        Section(int id, int size, string name) : sectionId(id), sectionSize(size), sectionName(name) {}
    };
    struct RelocationValue
    {
        bool isData;
        string sectionName, type, symbolName;
        int offset, addend;
        RelocationValue(bool data, string section, string t, string symbol, int o, int a) : isData(data), sectionName(section), type(t), symbolName(symbol), offset(o), addend(a) {}
    };

    string inputFilePath, outputFilePath, currentSection;
    int currentLine, locationCounter;
    vector<string> inputFileWithClearedLines;
    vector<AssemblerError> errors;
    vector<Symbol> symbolTable;
    vector<Section> sectionTable;
    vector<RelocationValue> relocationTable;
    map<int, int> lineNumberBeforeProcessing;
    RegexWrapper *regexWrapper;

    bool removeBlankLinesComments();
    bool firstPass();
    bool secondPass();
    void addError(string message, int lineNumber);
    void addSymbol(int o, bool local, bool defined, bool ext, string s, string n);
    void addSection(int s, string n);
    void addRelocationValue(bool data, string section, string t, string symbol, int o, int a);
    int convertToDecimalValueFromLiteral(string literal);
    void increaseSectionSizeAndCounter(int size, string name);
    void printErrors();
    void createTxtFile();
    bool handleLabel(string symbolName);
    void updateAbsoluteSection(int value);
    void insertWordDataInCurrentSection(int value);
    void insertJumpDataInCurrentSection(int instrDescr, int regDescr, int adrMode, int value, bool includeValue);

public:
    static Parser *getInstance();

    Parser();
    ~Parser();
    void setFilesPath(string iFile, string oFile);
    void compile();
};

#endif
