#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

#include "../inc/Parser.h"
#include "../inc/FileReader.h"
#include "../inc/FileWriter.h"

using namespace std;

Parser *Parser::instance = 0;
int Parser::symbolId = 0;
int Parser::sectionId = 0;

Parser *Parser::getInstance()
{
    if (!instance)
    {
        instance = new Parser();
    }
    return instance;
}

Parser::Parser() : inputFilePath(""), outputFilePath(""), currentSection(""), locationCounter(0)
{
    addSection(0, UNDEFINED);
    addSymbol(0, true, true, false, UNDEFINED, UNDEFINED);

    addSection(0, ABSOLUTE);
    addSymbol(0, true, true, false, ABSOLUTE, ABSOLUTE);

    regexWrapper = new RegexWrapper();
}

Parser::~Parser()
{
    delete regexWrapper;
}

void Parser::setFilesPath(string iFile, string oFile)
{
    inputFilePath = iFile;
    outputFilePath = oFile;
}

void Parser::compile()
{
    if (inputFilePath == "" || outputFilePath == "")
    {
        cout << "Set path to files first!" << endl;
        return;
    }

    if (!removeBlankLinesComments())
    {
        cout << "Code cleanup error" << endl;
        return;
    }

    if (!firstPass())
    {
        printErrors();
        return;
    }

    if (!secondPass())
    {
        printErrors();
        return;
    }

    createTxtFile();
}

bool Parser::removeBlankLinesComments()
{
    FileReader *fr = new FileReader(inputFilePath);
    if (!fr->isFileOpened())
    {
        cout << "Cannot open the file with path: " + inputFilePath << endl;
        return false;
    }

    string line = fr->getNextLine();
    int lineBeforeProcessing = 0;
    int lineAfterProcessing = 0;
    while (!fr->isEndOfFile())
    {
        lineBeforeProcessing++;
        line = regexWrapper->removeBlankLinesComments(line);

        if (line.empty() || line == " ")
        {
            line = fr->getNextLine();
            continue;
        }

        lineAfterProcessing++;
        lineNumberBeforeProcessing[lineAfterProcessing] = lineBeforeProcessing;
        inputFileWithClearedLines.push_back(line);
        line = fr->getNextLine();
    }

    delete fr;
    return true;
}

bool Parser::firstPass()
{
    currentLine = 0;
    bool hasError = false;

    for (string line : inputFileWithClearedLines)
    {
        currentLine++;
        RegexWrapper::Directive directive = regexWrapper->searchLine(line);

        if (directive.type == RegexWrapper::LABEL)
        {
            if (!handleLabel(directive.param1))
            {
                hasError = true;
                continue;
            }
        }
        else
        {
            if (directive.type == RegexWrapper::LABEL_WITH_INSTRUCTION)
            {
                if (!handleLabel(directive.param1))
                {
                    hasError = true;
                    continue;
                }
                directive = regexWrapper->searchLine(directive.param2);
            }

            switch (directive.type)
            {
            case RegexWrapper::SECTION:
            {
                locationCounter = 0;
                currentSection = directive.param1;
                addSymbol(locationCounter, true, true, false, currentSection, currentSection);
                addSection(0, currentSection);
                break;
            }

            case RegexWrapper::EQU:
            {
                bool hasSymbol = false;
                string symbolName = directive.param1;
                int value = convertToDecimalValueFromLiteral(directive.param2);
                hasError = value == -1 ? true : hasError;
                if (hasError)
                    continue;
                for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                {
                    if (symbol->name == symbolName)
                    {
                        hasSymbol = true;
                        if (symbol->isExtern)
                        {
                            addError("EQU directive cannot define extern symbol", currentLine);
                            hasError = true;
                            break;
                        }

                        if (symbol->isDefined)
                        {
                            addError("EQU directive cannot define an absolute symbol that is already defined", currentLine);
                            hasError = true;
                            break;
                        }

                        symbol->isDefined = true;
                        symbol->section = ABSOLUTE;
                        symbol->offset = value;
                        updateAbsoluteSection(value);
                    }
                }

                if (!hasSymbol)
                {
                    addSymbol(value, true, true, false, ABSOLUTE, symbolName);
                    updateAbsoluteSection(value);
                }
                break;
            }

            case RegexWrapper::SKIP:
            {
                if (currentSection == "")
                {
                    addError("Skip has to be in section!", currentLine);
                    hasError = true;
                    continue;
                }

                int skipValue = convertToDecimalValueFromLiteral(directive.param1);
                increaseSectionSizeAndCounter(skipValue, currentSection);
                break;
            }

            case RegexWrapper::END:
            {
                return !hasError;
            }

            case RegexWrapper::GLOBAL:
            {
                string symbolName;
                stringstream ss(directive.param1);
                while (getline(ss, symbolName, ','))
                {
                    bool symbolExist = false;
                    for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                    {
                        if (symbol->name == symbolName)
                        {
                            symbolExist = true;
                            symbol->isLocal = false;
                        }
                    }
                    if (!symbolExist)
                    {
                        addSymbol(0, false, false, false, UNDEFINED, symbolName);
                    }
                }
                break;
            }

            case RegexWrapper::EXTERNAL:
            {
                string symbolName;
                stringstream ss(directive.param1);
                while (getline(ss, symbolName, ','))
                {
                    bool symbolExist = false;
                    for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                    {
                        symbolExist = symbol->name == symbolName;
                        if (symbolExist && symbol->isDefined)
                        {
                            addError("External redefinition of defined symbol", currentLine);
                            hasError = true;
                            break;
                        }
                    }
                    if (!symbolExist)
                    {
                        addSymbol(0, false, false, true, UNDEFINED, symbolName);
                    }
                }
                break;
            }
            case RegexWrapper::WORD:
            {
                stringstream ss(directive.param1);
                string symbol;
                while (getline(ss, symbol, ','))
                {
                    if (currentSection == "")
                    {
                        addError("Word directive has to be in a section!", currentLine);
                        hasError = true;
                        continue;
                    }

                    increaseSectionSizeAndCounter(2, currentSection);
                }
                break;
            }

            default:
            {
                if (currentSection == "")
                {
                    addError("Instruction has to be in a section!", currentLine);
                    hasError = true;
                    continue;
                }

                RegexWrapper::Instruction instruction = regexWrapper->searchInstruction(line);

                switch (instruction.type)
                {
                case RegexWrapper::NO_OPERAND:
                    increaseSectionSizeAndCounter(1, currentSection);
                    break;

                case RegexWrapper::ONE_OPERAND:
                {
                    string operation = instruction.param1;
                    if (operation == INT || operation == NOT)
                    {
                        increaseSectionSizeAndCounter(2, currentSection);
                    }
                    else if (operation == PUSH || operation == POP)
                    {
                        increaseSectionSizeAndCounter(3, currentSection);
                    }
                    break;
                }

                case RegexWrapper::ONE_OPERAND_JUMP:
                {
                    string operand = instruction.param2;
                    RegexWrapper::Jump jump = regexWrapper->searchJump(operand);

                    switch (jump.type)
                    {
                    case RegexWrapper::JUMP_ABS:
                    case RegexWrapper::JUMP_PC_RELATIVE:
                    case RegexWrapper::JUMP_REG_IND_DISPL:
                    case RegexWrapper::JUMP_MEM_DIR:
                        increaseSectionSizeAndCounter(5, currentSection);
                        break;

                    case RegexWrapper::JUMP_REG_DIR:
                    case RegexWrapper::JUMP_REG_IND:
                        increaseSectionSizeAndCounter(3, currentSection);
                        break;

                    default:
                        addError("Addressing type is invalid", currentLine);
                        hasError = true;
                        continue;
                    }

                    break;
                }

                case RegexWrapper::TWO_OPERAND_LOAD_STORE:
                {
                    string operand = instruction.param3;
                    RegexWrapper::LoadStore loadStore = regexWrapper->searchLoadStore(operand);

                    switch (loadStore.type)
                    {
                    case RegexWrapper::LOAD_STORE_ABS_SYMBOL:
                    case RegexWrapper::LOAD_STORE_ABS_VALUE:
                    case RegexWrapper::LOAD_STORE_PC_RELATIVE:
                    case RegexWrapper::LOAD_STORE_REG_IND_DISPL_SYMBOL:
                    case RegexWrapper::LOAD_STORE_REG_IND_DISPL_VALUE:
                    case RegexWrapper::LOAD_STORE_MEM_DIR_SYMBOL:
                    case RegexWrapper::LOAD_STORE_MEM_DIR_VALUE:
                        increaseSectionSizeAndCounter(5, currentSection);
                        break;

                    case RegexWrapper::LOAD_STORE_REG_DIR:
                    case RegexWrapper::LOAD_STORE_REG_IND:
                        increaseSectionSizeAndCounter(3, currentSection);
                        break;

                    default:
                        addError("Addressing type is invalid", currentLine);
                        hasError = true;
                        continue;
                    }

                    break;
                }

                case RegexWrapper::TWO_OPERAND:
                    increaseSectionSizeAndCounter(2, currentSection);
                    break;

                default:
                    addError("Instruction does not exists", currentLine);
                    hasError = true;
                    continue;
                }

                break;
            }
            }
        }
    }

    return !hasError;
}

bool Parser::secondPass()
{
    currentLine = 0;
    currentSection = "";
    locationCounter = 0;
    bool hasError = false;

    for (string line : inputFileWithClearedLines)
    {
        currentLine++;
        RegexWrapper::Directive directive = regexWrapper->searchLine(line);

        if (directive.type == RegexWrapper::LABEL)
        {
            continue;
        }
        else
        {
            if (directive.type == RegexWrapper::LABEL_WITH_INSTRUCTION)
            {
                directive = regexWrapper->searchLine(directive.param2);
            }

            switch (directive.type)
            {
            case RegexWrapper::GLOBAL:
            case RegexWrapper::EXTERNAL:
            case RegexWrapper::EQU:
                continue;

            case RegexWrapper::SECTION:
                locationCounter = 0;
                currentSection = directive.param1;
                break;

            case RegexWrapper::SKIP:
            {
                string stringValue = directive.param1;
                int skipValue = convertToDecimalValueFromLiteral(stringValue);

                for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
                {
                    if (section->sectionName == currentSection)
                    {
                        section->offsets.push_back(locationCounter);
                        for (int i = 0; i < skipValue; i++)
                        {
                            section->data.push_back(0);
                        }
                    }
                }
                locationCounter += skipValue;
                break;
            }

            case RegexWrapper::END:
            {
                return !hasError;
            }

            case RegexWrapper::WORD:
            {
                stringstream ss(directive.param1);
                string symbolOrNumber;
                while (getline(ss, symbolOrNumber, ','))
                {
                    if (regexWrapper->isSymbol(symbolOrNumber))
                    {
                        bool symbolExist = false;
                        for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                        {
                            if (symbol->name == symbolOrNumber)
                            {
                                symbolExist = true;
                                int value = symbol->section == ABSOLUTE || symbol->isDefined && symbol->isLocal ? symbol->offset : 0;
                                insertWordDataInCurrentSection(value);
                                if (symbol->section != ABSOLUTE)
                                {
                                    string name = symbol->isDefined && symbol->isLocal ? symbol->section : symbol->name;
                                    addRelocationValue(true, currentSection, R_H_16, name, locationCounter, 0);
                                }
                            }
                        }

                        if (!symbolExist)
                        {
                            addError(".word used with undefined symbol!", currentLine);
                            hasError = true;
                            continue;
                        }
                    }
                    else
                    {
                        char c = convertToDecimalValueFromLiteral(symbolOrNumber);
                        insertWordDataInCurrentSection(c);
                    }

                    locationCounter += 2;
                }

                break;
            }

            default:
            {
                RegexWrapper::Instruction instruction = regexWrapper->searchInstruction(line);
                string operation = instruction.param1;
                switch (instruction.type)
                {
                case RegexWrapper::NO_OPERAND:
                {
                    int operationCode = operation == HALT ? 0x00 : (operation == IRET ? 0x20 : 0x40);
                    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
                    {
                        if (section->sectionName == currentSection)
                        {
                            section->offsets.push_back(locationCounter);
                            section->data.push_back(operationCode);
                        }
                    }
                    locationCounter += 1;

                    break;
                }

                case RegexWrapper::ONE_OPERAND:
                {
                    string reg = instruction.param2;
                    int registerNumber = reg == PSW ? 8 : reg.at(1) - '0';
                    int operationCode;
                    int registerOffset;
                    if (operation == INT)
                    {
                        operationCode = 0x10;
                        registerOffset = 15;
                    }
                    else if (operation == PUSH)
                    {
                        operationCode = 0xB0;
                        registerOffset = 6;
                    }
                    else if (operation == POP)
                    {
                        operationCode = 0xA0;
                        registerOffset = 6;
                    }
                    else
                    {
                        operationCode = 0x80;
                        registerOffset = 15;
                    }

                    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
                    {
                        if (section->sectionName == currentSection)
                        {
                            section->offsets.push_back(locationCounter);
                            section->data.push_back(operationCode);
                            section->data.push_back((registerNumber << 4) + registerOffset);

                            if (operation == PUSH)
                                section->data.push_back(0x12);
                            else if (operation == POP)
                                section->data.push_back(0x42);
                        }
                    }

                    locationCounter += operation == INT || operation == NOT ? 2 : 3;

                    break;
                }

                case RegexWrapper::ONE_OPERAND_JUMP:
                {
                    string operation = instruction.param1;
                    string operand = instruction.param2;
                    int instrDescr, regDescr = 0xF0, adrMode;
                    RegexWrapper::Jump jump = regexWrapper->searchJump(operand);

                    if (operation == CALL)
                        instrDescr = 0x30;
                    else if (operation == JMP)
                        instrDescr = 0x50;
                    else if (operation == JEQ)
                        instrDescr = 0x51;
                    else if (operation == JNE)
                        instrDescr = 0x52;
                    else if (operation == JGT)
                        instrDescr = 0x53;

                    switch (jump.type)
                    {
                    case RegexWrapper::JUMP_ABS:
                    {
                        if (regexWrapper->isSymbol(operand))
                        {
                            regDescr += 0xF;
                            adrMode = 0;

                            int value;
                            bool symbolExist = false;
                            for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                            {
                                if (symbol->name == operand)
                                {
                                    symbolExist = true;

                                    if (symbol->section == ABSOLUTE)
                                        value = symbol->offset;
                                    else
                                    {
                                        addRelocationValue(false, currentSection, R_H_16, (!symbol->isLocal || symbol->isExtern) ? symbol->name : symbol->section, locationCounter + 4, 0);
                                        value = (!symbol->isLocal || symbol->isExtern) ? 0 : symbol->offset;
                                    }
                                }
                            }
                            if (!symbolExist)
                            {
                                addError("Symbol is not in symbol table", currentLine);
                                hasError = true;
                                continue;
                            }

                            insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                            locationCounter += 5;
                        }
                        else
                        {
                            int value = convertToDecimalValueFromLiteral(operand);
                            regDescr += 0xF;
                            adrMode = 0;
                            insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                            locationCounter += 5;
                        }

                        break;
                    }

                    case RegexWrapper::JUMP_PC_RELATIVE:
                    {
                        operand = jump.param1;
                        regDescr += 0x7;
                        adrMode = 0x05;

                        int value;
                        bool symbolExist = false;
                        for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                        {
                            if (symbol->name == operand)
                            {
                                symbolExist = true;

                                if (symbol->section == ABSOLUTE)
                                {
                                    value = -2;
                                    addRelocationValue(false, currentSection, R_H_16_PC, symbol->name, locationCounter + 4, 0);
                                }
                                else
                                {
                                    addRelocationValue(false, currentSection, R_H_16_PC, (!symbol->isLocal || symbol->isExtern) ? symbol->name : (currentSection == symbol->section ? "" : symbol->section), locationCounter + 4, 0);
                                    value = (!symbol->isLocal || symbol->isExtern) ? -2 : (currentSection == symbol->section ? symbol->offset - 2 - (locationCounter + 3) : symbol->offset - 2);
                                }
                            }
                        }
                        if (!symbolExist)
                        {
                            addError("Symbol is not in symbol table", currentLine);
                            hasError = true;
                            continue;
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }
                    case RegexWrapper::JUMP_REG_DIR:
                    {
                        regDescr += (jump.param1 == PSW ? 8 : jump.param1.at(1) - '0');
                        adrMode = 0x01;
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, -1, false);
                        locationCounter += 3;

                        break;
                    }

                    case RegexWrapper::JUMP_REG_IND:
                    {
                        regDescr += (jump.param1 == PSW ? 8 : jump.param1.at(1) - '0');
                        adrMode = 0x02;
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, -1, false);
                        locationCounter += 3;

                        break;
                    }
                    case RegexWrapper::JUMP_REG_IND_DISPL:
                    {
                        string displacement = jump.param2;
                        regDescr += (jump.param1 == PSW ? 8 : jump.param1.at(1) - '0');
                        adrMode = 0x03;
                        int value;

                        if (regexWrapper->isSymbol(displacement))
                        {
                            bool symbolExist = false;
                            for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                            {
                                if (symbol->name == displacement)
                                {
                                    symbolExist = true;

                                    if (symbol->section == ABSOLUTE)
                                        value = symbol->offset;
                                    else
                                    {
                                        addRelocationValue(false, currentSection, R_H_16, (!symbol->isLocal || symbol->isExtern) ? symbol->name : symbol->section, locationCounter + 4, 0);
                                        value = (!symbol->isLocal || symbol->isExtern) ? 0 : symbol->offset;
                                    }
                                }
                            }

                            if (!symbolExist)
                            {
                                addError("Symbol is not in symbol table", currentLine);
                                hasError = true;
                                continue;
                            }
                        }
                        else
                        {
                            value = convertToDecimalValueFromLiteral(displacement);
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }
                    case RegexWrapper::JUMP_MEM_DIR:
                    {
                        operand = jump.param1;
                        regDescr += 0xF;
                        adrMode = 0x04;
                        int value;

                        if (regexWrapper->isSymbol(operand))
                        {
                            bool symbolExist = false;
                            for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                            {
                                if (symbol->name == operand)
                                {
                                    symbolExist = true;

                                    if (symbol->section == ABSOLUTE)
                                        value = symbol->offset;
                                    else
                                    {
                                        addRelocationValue(false, currentSection, R_H_16, (!symbol->isLocal || symbol->isExtern) ? symbol->name : symbol->section, locationCounter + 4, 0);
                                        value = (!symbol->isLocal || symbol->isExtern) ? 0 : symbol->offset;
                                    }
                                }
                            }

                            if (!symbolExist)
                            {
                                addError("Symbol is not in symbol table", currentLine);
                                hasError = true;
                                continue;
                            }
                        }
                        else
                        {
                            value = convertToDecimalValueFromLiteral(operand);
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    default:
                        addError("Addressing type is invalid", currentLine);
                        hasError = true;
                        continue;
                    }

                    break;
                }

                case RegexWrapper::TWO_OPERAND_LOAD_STORE:
                {
                    string operation = instruction.param1;
                    string regD = instruction.param2;
                    string operand = instruction.param3;
                    RegexWrapper::LoadStore loadStore = regexWrapper->searchLoadStore(operand);

                    int instrDescr, regDescr, adrMode;
                    if (operation == LDR)
                    {
                        instrDescr = 0xA0;
                    }
                    else if (operation == STR)
                    {
                        instrDescr = 0xB0;
                    }
                    regDescr = (regD == PSW ? 0x8 : regD.at(1) - '0') << 4;

                    switch (loadStore.type)
                    {
                    case RegexWrapper::LOAD_STORE_ABS_SYMBOL:
                    {
                        operand = loadStore.param1;
                        regDescr += 0xF;
                        adrMode = 0;
                        int value;
                        bool symbolExist = false;
                        for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                        {
                            if (symbol->name == operand)
                            {
                                symbolExist = true;

                                if (symbol->section == ABSOLUTE)
                                    value = symbol->offset;
                                else
                                {
                                    addRelocationValue(false, currentSection, R_H_16, (!symbol->isLocal || symbol->isExtern) ? symbol->name : symbol->section, locationCounter + 4, 0);
                                    value = (!symbol->isLocal || symbol->isExtern) ? 0 : symbol->offset;
                                }
                            }
                        }

                        if (!symbolExist)
                        {
                            addError("Symbol is not in symbol table", currentLine);
                            hasError = true;
                            continue;
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    case RegexWrapper::LOAD_STORE_ABS_VALUE:
                    {
                        operand = loadStore.param1;
                        int value = convertToDecimalValueFromLiteral(operand);
                        regDescr += 0xF;
                        adrMode = 0;
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    case RegexWrapper::LOAD_STORE_PC_RELATIVE:
                    {
                        operand = loadStore.param1;
                        regDescr += 0x7;
                        adrMode = 0x03;

                        int value;
                        bool symbolExist = false;
                        for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                        {
                            if (symbol->name == operand)
                            {
                                symbolExist = true;

                                if (symbol->section == ABSOLUTE)
                                {
                                    value = -2;
                                    addRelocationValue(false, currentSection, R_H_16_PC, symbol->name, locationCounter + 4, 0);
                                }
                                else
                                {
                                    addRelocationValue(false, currentSection, R_H_16_PC, (!symbol->isLocal || symbol->isExtern) ? symbol->name : (currentSection == symbol->section ? "" : symbol->section), locationCounter + 4, 0);
                                    value = (!symbol->isLocal || symbol->isExtern) ? -2 : (currentSection == symbol->section ? symbol->offset - 2 - (locationCounter + 3) : symbol->offset - 2);
                                }
                            }
                        }
                        if (!symbolExist)
                        {
                            addError("Symbol is not in symbol table", currentLine);
                            hasError = true;
                            continue;
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    case RegexWrapper::LOAD_STORE_REG_DIR:
                    {
                        regDescr += (loadStore.param1 == PSW ? 8 : loadStore.param1.at(1) - '0');
                        adrMode = 0x01;
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, -1, false);
                        locationCounter += 3;
                        break;
                    }

                    case RegexWrapper::LOAD_STORE_REG_IND:
                    {
                        regDescr += (loadStore.param1 == PSW ? 8 : loadStore.param1.at(1) - '0');
                        adrMode = 0x02;
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, -1, false);
                        locationCounter += 3;
                        break;
                    }

                    case RegexWrapper::LOAD_STORE_REG_IND_DISPL_SYMBOL:
                    {
                        string displacement = loadStore.param2;
                        regDescr += (loadStore.param1 == PSW ? 8 : loadStore.param1.at(1) - '0');
                        adrMode = 0x03;
                        int value;
                        bool symbolExist = false;
                        for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                        {
                            if (symbol->name == displacement)
                            {
                                symbolExist = true;

                                if (symbol->section == ABSOLUTE)
                                    value = symbol->offset;
                                else
                                {
                                    addRelocationValue(false, currentSection, R_H_16, (!symbol->isLocal || symbol->isExtern) ? symbol->name : symbol->section, locationCounter + 4, 0);
                                    value = (!symbol->isLocal || symbol->isExtern) ? 0 : symbol->offset;
                                }
                            }
                        }

                        if (!symbolExist)
                        {
                            addError("Symbol is not in symbol table", currentLine);
                            hasError = true;
                            continue;
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    case RegexWrapper::LOAD_STORE_REG_IND_DISPL_VALUE:
                    {
                        string displacement = loadStore.param2;
                        regDescr += (loadStore.param1 == PSW ? 8 : loadStore.param1.at(1) - '0');
                        adrMode = 0x03;
                        int value = convertToDecimalValueFromLiteral(displacement);
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    case RegexWrapper::LOAD_STORE_MEM_DIR_SYMBOL:
                    {
                        regDescr += 0xF;
                        adrMode = 0x04;
                        int value;
                        bool symbolExist = false;
                        for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
                        {
                            if (symbol->name == operand)
                            {
                                symbolExist = true;

                                if (symbol->section == ABSOLUTE)
                                    value = symbol->offset;
                                else
                                {
                                    addRelocationValue(false, currentSection, R_H_16, (!symbol->isLocal || symbol->isExtern) ? symbol->name : symbol->section, locationCounter + 4, 0);
                                    value = (!symbol->isLocal || symbol->isExtern) ? 0 : symbol->offset;
                                }
                            }
                        }

                        if (!symbolExist)
                        {
                            addError("Symbol is not in symbol table", currentLine);
                            hasError = true;
                            continue;
                        }

                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    case RegexWrapper::LOAD_STORE_MEM_DIR_VALUE:
                    {
                        regDescr += 0xF;
                        adrMode = 0x04;
                        int value = convertToDecimalValueFromLiteral(operand);
                        insertJumpDataInCurrentSection(instrDescr, regDescr, adrMode, value, true);
                        locationCounter += 5;

                        break;
                    }

                    default:
                        addError("Addressing type is invalid", currentLine);
                        hasError = true;
                        continue;
                    }

                    break;
                }

                case RegexWrapper::TWO_OPERAND:
                {
                    string operation = instruction.param1;
                    string regD = instruction.param2;
                    string regS = instruction.param3;
                    int instrDescr, regDescr;

                    if (operation == XCHG)
                        instrDescr = 0x60;
                    else if (operation == ADD)
                        instrDescr = 0x70;
                    else if (operation == SUB)
                        instrDescr = 0x71;
                    else if (operation == MUL)
                        instrDescr = 0x72;
                    else if (operation == DIV)
                        instrDescr = 0x73;
                    else if (operation == CMP)
                        instrDescr = 0x74;
                    else if (operation == AND)
                        instrDescr = 0x81;
                    else if (operation == OR)
                        instrDescr = 0x82;
                    else if (operation == XOR)
                        instrDescr = 0x83;
                    else if (operation == TEST)
                        instrDescr = 0x84;
                    else if (operation == SHL)
                        instrDescr = 0x90;
                    else if (operation == SHR)
                        instrDescr = 0x91;

                    regDescr = (regD == PSW ? 0x8 : regD.at(1) - '0') << 4;
                    regDescr += regS == PSW ? 0x8 : regS.at(1) - '0';

                    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
                    {
                        if (section->sectionName == currentSection)
                        {
                            section->offsets.push_back(locationCounter);
                            section->data.push_back(instrDescr);
                            section->data.push_back(regDescr);
                        }
                    }

                    locationCounter += 2;

                    break;
                }

                default:
                    addError("Instruction does not exists", currentLine);
                    hasError = true;
                    continue;
                }

                break;
            }
            }
        }
    }

    return !hasError;
}

void Parser::createTxtFile()
{
    FileWriter *fw = new FileWriter(outputFilePath);

    fw->writeLine("Section table:");
    fw->writeLine("Id\tName\t\tSize");
    for (Section section : sectionTable)
    {
        fw->writeSection(section.sectionId, section.sectionName, section.sectionSize);
    }
    fw->changeToDec();
    fw->addNewLine();

    fw->writeLine("Symbol table:");
    fw->writeLine("Value\tType\tSection\t\tName\t\tId");
    for (Symbol symbol : symbolTable)
    {
        fw->writeSymbol(symbol.offset, symbol.isLocal, symbol.isDefined, symbol.isExtern, symbol.section, symbol.name, symbol.symbolId);
    }
    fw->changeToDec();
    fw->addNewLine();

    for (Section section : sectionTable)
    {
        fw->writeLine("Relocation data <" + section.sectionName + ">:");
        fw->writeLine("Offset\tType\t\tDat/Ins\tSymbol\tSection name");

        for (RelocationValue relocation : relocationTable)
        {
            if (relocation.sectionName == section.sectionName)
            {
                fw->writeRelocationValue(relocation.offset, relocation.type, relocation.isData, relocation.symbolName, relocation.sectionName);
            }
        }
        fw->changeToDec();

        fw->writeLine("Section data <" + section.sectionName + ">:");
        if (section.sectionSize == 0)
        {
            fw->changeToDec();
            fw->addNewLine();
            continue;
        }

        fw->writeSectionData(section.offsets, section.data);

        fw->changeToDec();
        fw->addNewLine();
    }

    delete fw;
}

int Parser::convertToDecimalValueFromLiteral(string stringLiteral)
{
    int number = -1;
    RegexWrapper::Literal literal = regexWrapper->searchLiteral(stringLiteral);
    switch (literal.type)
    {
    case RegexWrapper::HEXA_DECIMAL:
    {
        stringstream ss;
        ss << literal.param1.substr(2);
        ss >> hex >> number;
        break;
    }

    case RegexWrapper::DECIMAL:
        number = stoi(literal.param1);
        break;

    default:
        addError("Bad literal format!", currentLine);
        break;
    }

    return number;
}

void Parser::increaseSectionSizeAndCounter(int size, string name)
{
    locationCounter += size;
    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
    {
        if (section->sectionName == name)
        {
            section->sectionSize += size;
        }
    }
}

bool Parser::handleLabel(string symbolName)
{
    if (currentSection == "")
    {
        addError("Label has to be defined in section!", currentLine);
        return false;
    }

    bool hasSymbol = false;
    for (vector<Symbol>::iterator symbol = symbolTable.begin(); symbol != symbolTable.end(); symbol++)
    {
        if (symbol->name == symbolName)
        {
            hasSymbol = true;
            if (symbol->isDefined)
            {
                addError("Symbol is already defined in this module!", currentLine);
                return false;
            }
            if (symbol->isExtern)
            {
                addError("Symbol is already defined in another module!", currentLine);
                return false;
            }
            symbol->isDefined = true;
            symbol->offset = locationCounter;
            symbol->section = currentSection;
            break;
        }
    }

    if (!hasSymbol)
    {
        addSymbol(locationCounter, true, true, false, currentSection, symbolName);
    }
    return true;
}

void Parser::updateAbsoluteSection(int value)
{
    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
    {
        if (section->sectionName == ABSOLUTE)
        {
            section->offsets.push_back(section->sectionSize);
            section->data.push_back(0xff & value);
            section->data.push_back(0xff & (value >> 8));
            section->sectionSize += 2;
        }
    }
}

void Parser::insertWordDataInCurrentSection(int value)
{
    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
    {
        if (section->sectionName == currentSection)
        {
            // push upper 8 bits and then lower 8 bits, little endian
            section->offsets.push_back(locationCounter);
            section->data.push_back(value & 0xff);
            section->data.push_back((value >> 8) & 0xff);
        }
    }
}
void Parser::insertJumpDataInCurrentSection(int instrDescr, int regDescr, int adrMode, int value, bool includeValue)
{
    for (vector<Section>::iterator section = sectionTable.begin(); section != sectionTable.end(); section++)
    {
        if (section->sectionName == currentSection)
        {
            section->offsets.push_back(locationCounter);
            section->data.push_back(instrDescr);
            section->data.push_back(regDescr);
            section->data.push_back(adrMode);
            if (includeValue)
            {
                section->data.push_back(0xff & (value >> 8));
                section->data.push_back(0xff & value);
            }
        }
    }
}

void Parser::addError(string message, int lineNumber)
{
    AssemblerError error(message, lineNumber);
    errors.push_back(error);
}

void Parser::addSymbol(int o, bool local, bool defined, bool ext, string s, string n)
{
    Symbol newSymbol(symbolId++, o, local, defined, ext, s, n);
    symbolTable.push_back(newSymbol);
}

void Parser::addSection(int s, string n)
{
    int id = n == ABSOLUTE ? -1 : sectionId++;
    Section newSection(id, s, n);
    sectionTable.push_back(newSection);
}

void Parser::addRelocationValue(bool data, string section, string t, string symbol, int o, int a)
{
    RelocationValue relocationValue(data, section, t, symbol, o, a);
    relocationTable.push_back(relocationValue);
}

void Parser::printErrors()
{
    cout << "Assembler detects some errors:" << endl;
    for (vector<AssemblerError>::iterator it = errors.begin(); it != errors.end(); it++)
    {
        cout << "Line " << lineNumberBeforeProcessing[it->lineNumber] << ":" << it->message << endl;
    }
}
