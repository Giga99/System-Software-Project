#include "../inc/FileWriter.h"

using namespace std;

FileWriter::FileWriter(string filePath) : file(filePath)
{
}

FileWriter::~FileWriter()
{
    file.close();
}

void FileWriter::writeLine(string line)
{
    file << line << endl;
}

void FileWriter::addNewLine()
{
    file << endl;
}

void FileWriter::writeSection(int sectionId, string sectionName, int sectionSize)
{
    file << sectionId << "\t" << sectionName << "\t" << hex << setfill('0') << setw(4) << (0xffff & sectionSize) << endl;
}

void FileWriter::changeToDec()
{
    file << dec << endl;
}
void FileWriter::writeSymbol(int offset, bool isLocal, bool isDefined, bool isExtern, string section, string name, int symbolId)
{
    file << hex << setfill('0') << setw(4) << (0xffff & offset) << "\t";
    if (isLocal)
    {
        file << "l\t";
    }
    else if (isDefined)
    {
        file << "g\t";
    }
    else if (isExtern)
    {
        file << "e\t";
    }
    else
    {
        file << "u\t";
    }
    file << section << "\t" << name << "\t" << hex << setfill('0') << setw(4) << (0xffff & symbolId) << endl;
}

void FileWriter::writeRelocationValue(int offset, string type, bool isData, string symbolName, string sectionName)
{
    file << hex << setfill('0') << setw(4) << (0xffff & offset) << "\t" << type << "\t" << (isData ? 'd' : 'i') << "\t" << symbolName << "\t" << sectionName << endl;
}

void FileWriter::writeSectionData(vector<int> offsets, vector<char> data)
{
    for (int i = 0; i < offsets.size() - 1; i++)
    {
        int currentOffset = offsets[i];
        int nextOffset = offsets[i + 1];
        file << hex << setfill('0') << setw(4) << (0xffff & currentOffset) << ": ";
        for (int j = currentOffset; j < nextOffset; j++)
        {
            char c = data[j];
            file << hex << setfill('0') << setw(2) << (0xff & c) << " ";
        }
        file << endl;
    }

    int currentOffset = offsets[offsets.size() - 1];
    int nextOffset = data.size();
    file << hex << setfill('0') << setw(4) << (0xffff & currentOffset) << ": ";
    for (int j = currentOffset; j < nextOffset; j++)
    {
        char c = data[j];
        file << hex << setfill('0') << setw(2) << (0xff & c) << " ";
    }
}
