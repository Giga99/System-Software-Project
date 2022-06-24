#include <string>

#include "../inc/FileReader.h"

using namespace std;

FileReader::FileReader(string filePath)
{
    file.open(filePath);
    endOfFile = false;
}

FileReader::~FileReader()
{
    file.close();
}

string FileReader::getNextLine()
{
    string line;
    endOfFile = !getline(file, line);
    return line;
}

bool FileReader::isFileOpened()
{
    return file.is_open();
}

bool FileReader::isEndOfFile()
{
    return endOfFile;
}
