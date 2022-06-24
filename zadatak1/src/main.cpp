#include <iostream>

#include "../inc/Parser.h"

using namespace std;

int main(int argc, const char *argv[])
{
    string argument = argv[1];

    if (argument != "-o")
    {
        cout << "Output file does not exists!" << endl;
        return -1;
    }

    Parser *parser = Parser::getInstance();
    parser->setFilesPath(argv[3], argv[2]);
    parser->compile();
}