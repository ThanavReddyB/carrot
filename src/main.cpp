#include <iostream>
#include <string>

#include "init.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: carrot <command>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "init")
    {
        initRepository();
    }
    else
    {
        std::cout << "Unknown command.\n";
    }

    return 0;
}