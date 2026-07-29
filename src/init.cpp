#include "carrot/init.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

void initRepository()
{
    if (fs::exists(".carrot"))
    {
        std::cout << "Repository already initialized.\n";
        return;
    }

    fs::create_directory(".carrot");

    fs::create_directory(".carrot/objects");
    fs::create_directory(".carrot/refs");
    fs::create_directory(".carrot/logs");

    std::ofstream(".carrot/HEAD");
    std::ofstream(".carrot/index");
    std::ofstream(".carrot/config");

    std::cout << "Initialized empty Carrot repository.\n";
}