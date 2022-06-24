#include <iostream>

#include "../inc/RegexWrapper.h"

using namespace std;

RegexWrapper::RegexWrapper()
{
}

RegexWrapper::~RegexWrapper()
{
}

RegexWrapper::Directive RegexWrapper::searchLine(string line)
{
    smatch lineSmatch;

    if (regex_search(line, lineSmatch, regexLabel))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), LABEL);
    }
    else if (regex_search(line, lineSmatch, regexLabelWithInstruction))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), LABEL_WITH_INSTRUCTION);
    }
    else if (regex_search(line, lineSmatch, regexSectionDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), SECTION);
    }
    else if (regex_search(line, lineSmatch, regexEquDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), EQU);
    }
    else if (regex_search(line, lineSmatch, regexSkipDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), SKIP);
    }
    else if (regex_search(line, lineSmatch, regexEndDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), END);
    }
    else if (regex_search(line, lineSmatch, regexGlobalDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), GLOBAL);
    }
    else if (regex_search(line, lineSmatch, regexExternalDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), EXTERNAL);
    }
    else if (regex_search(line, lineSmatch, regexWordDirective))
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), WORD);
    }
    else
    {
        return Directive(lineSmatch.str(1), lineSmatch.str(2), INSTRUCTION);
    }
}

RegexWrapper::Instruction RegexWrapper::searchInstruction(string line)
{
    smatch instructionSmatch;
    if (regex_search(line, instructionSmatch, regexNoOperandInstruction))
    {
        return Instruction(instructionSmatch.str(1), instructionSmatch.str(2), instructionSmatch.str(3), NO_OPERAND);
    }
    else if (regex_search(line, instructionSmatch, regexOneOperandRegisterInstruction))
    {
        return Instruction(instructionSmatch.str(1), instructionSmatch.str(2), instructionSmatch.str(3), ONE_OPERAND);
    }
    else if (regex_search(line, instructionSmatch, regexOneOperandJump))
    {
        return Instruction(instructionSmatch.str(1), instructionSmatch.str(2), instructionSmatch.str(3), ONE_OPERAND_JUMP);
    }
    else if (regex_search(line, instructionSmatch, regexTwoOperandLoadStore))
    {
        return Instruction(instructionSmatch.str(1), instructionSmatch.str(2), instructionSmatch.str(3), TWO_OPERAND_LOAD_STORE);
    }
    else if (regex_search(line, instructionSmatch, regexTwoOperandRegisterInstruction))
    {
        return Instruction(instructionSmatch.str(1), instructionSmatch.str(2), instructionSmatch.str(3), TWO_OPERAND);
    }
    else
    {
        return Instruction(instructionSmatch.str(1), instructionSmatch.str(2), instructionSmatch.str(3), BAD_INSTRUCTION);
    }
}

RegexWrapper::Jump RegexWrapper::searchJump(string operand)
{
    smatch operandSmatch;

    if (regex_search(operand, operandSmatch, regexJumpAbsolute))
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), JUMP_ABS);
    }
    else if (regex_search(operand, operandSmatch, regexJumPCRelative))
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), JUMP_PC_RELATIVE);
    }
    else if (regex_search(operand, operandSmatch, regexJumpRegDir))
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), JUMP_REG_DIR);
    }
    else if (regex_search(operand, operandSmatch, regexJumpRegInd))
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), JUMP_REG_IND);
    }
    else if (regex_search(operand, operandSmatch, regexJumpRegIndWithDisplacement))
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), JUMP_REG_IND_DISPL);
    }
    else if (regex_search(operand, operandSmatch, regexJumpMemDir))
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), JUMP_MEM_DIR);
    }
    else
    {
        return Jump(operandSmatch.str(1), operandSmatch.str(2), BAD_JUMP);
    }
}

RegexWrapper::LoadStore RegexWrapper::searchLoadStore(string operand)
{
    smatch operandSmatch;

    if (regex_search(operand, operandSmatch, regexLoadStoreAbsolute))
    {
        string op = operandSmatch.str(1);
        if (regex_match(op, regexSymbol))
        {
            return LoadStore(op, operandSmatch.str(2), LOAD_STORE_ABS_SYMBOL);
        }
        else
        {
            return LoadStore(op, operandSmatch.str(2), LOAD_STORE_ABS_VALUE);
        }
    }
    else if (regex_search(operand, operandSmatch, regexLoadStorePCRelative))
    {
        return LoadStore(operandSmatch.str(1), operandSmatch.str(2), LOAD_STORE_PC_RELATIVE);
    }
    else if (regex_search(operand, operandSmatch, regexLoadStoreRegDir))
    {
        return LoadStore(operandSmatch.str(1), operandSmatch.str(2), LOAD_STORE_REG_DIR);
    }
    else if (regex_search(operand, operandSmatch, regexLoadStoreRegInd))
    {
        return LoadStore(operandSmatch.str(1), operandSmatch.str(2), LOAD_STORE_REG_IND);
    }
    else if (regex_search(operand, operandSmatch, regexLoadStoreRegIndWithDisplacement))
    {
        string displacement = operandSmatch.str(2);

        if (regex_match(displacement, regexSymbol))
        {
            return LoadStore(operandSmatch.str(1), displacement, LOAD_STORE_REG_IND_DISPL_SYMBOL);
        }
        else
        {
            return LoadStore(operandSmatch.str(1), displacement, LOAD_STORE_REG_IND_DISPL_VALUE);
        }
    }
    else if (regex_search(operand, operandSmatch, regexLoadStoreMemDir))
    {
        if (regex_match(operand, regexSymbol))
        {
            return LoadStore(operand, operandSmatch.str(2), LOAD_STORE_MEM_DIR_SYMBOL);
        }
        else
        {
            return LoadStore(operand, operandSmatch.str(2), LOAD_STORE_MEM_DIR_VALUE);
        }
    }
    else
    {
        return LoadStore(operandSmatch.str(1), operandSmatch.str(2), BAD_LOAD_STORE);
    }
}

RegexWrapper::Literal RegexWrapper::searchLiteral(string literal)
{
    smatch numberSmatch;
    if (regex_search(literal, numberSmatch, regexHexaDecimal))
    {
        return Literal(numberSmatch.str(1), HEXA_DECIMAL);
    }
    else if (regex_search(literal, numberSmatch, regexDecimal))
    {
        return Literal(numberSmatch.str(1), DECIMAL);
    }
    else
    {
        return Literal(numberSmatch.str(1), ERROR);
    }
}

bool RegexWrapper::isSymbol(string operand)
{
    return regex_match(operand, regexSymbol);
}

string RegexWrapper::removeBlankLinesComments(string line)
{
    string temp = regex_replace(line, regexComment, "$1", regex_constants::format_first_only);
    temp = regex_replace(temp, regexSpaces, " ");
    temp = regex_replace(temp, regexTabs, " ");
    temp = regex_replace(temp, regexBoundarySpaces, "$2");
    temp = regex_replace(temp, regexCommaSpaces, ",");
    temp = regex_replace(temp, regexColumnsSpaces, ":");

    return temp;
}
