#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>

using namespace std;

class FileWriter
{
private:
    ofstream file;

public:
    FileWriter(string filePath);
    ~FileWriter();
    void writeLine(string line);
    void addNewLine();
    void writeSection(int sectionId, string sectionName, int sectionSize);
    void writeSymbol(int offset, bool isLocal, bool isDefined, bool isExtern, string section, string name, int symbolId);
    void writeRelocationValue(int offset, string type, bool isData, string symbolName, string sectionName);
    void writeSectionData(vector<int> offsets, vector<char> data);
    void changeToDec();
};

#endif