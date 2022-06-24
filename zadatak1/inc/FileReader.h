#ifndef FILE_READER_H
#define FILE_READER_H

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

class FileReader
{
private:
    ifstream file;
    bool endOfFile;

public:
    FileReader(string filePath);
    ~FileReader();
    string getNextLine();
    bool isFileOpened();
    bool isEndOfFile();
};

#endif