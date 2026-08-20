#include "carrot/repository.hpp"
#include "carrot/hash.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    Repository repo;

    if (argc < 2)
    {
        std::cout << "Usage: carrot <command>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "init")
    {
        repo.init();
    }
    else if(command == "status")
    {
        repo.status();
    }

    else if (command == "add")
    {
        if (argc < 3)
        {
            std::cout << "Usage: carrot add <file>\n";
            return 1;
        }

        repo.add(argv[2]);
    }
    else if (command == "commit")
    {
        if (argc < 3)
        {
            std::cout << "Usage: carrot commit <message>\n";
            return 1;
        }

        repo.commit(argv[2]);
    }
    else if (command == "log")
    {
        repo.log();
    }
    else if (command == "show")
    {
        if (argc < 3)
        {
            std::cout << "Usage: carrot show <object-id>\n";
            return 1;
        }

        repo.show(argv[2]);
    }
    else if (command == "checkout")
    {
        if (argc < 3)
        {
            std::cout << "Usage: carrot checkout <commit-id>\n";
            return 1;
        }

        repo.checkout(argv[2]);
    }
    else if (command == "branch")
    {
        if (argc == 2)
        {
            repo.branch("");
        }
        else
        {
            repo.branch(argv[2]);
        }
    }
    else if (command == "switch")
    {
        if (argc < 3)
        {
            std::cout << "Usage: carrot switch <branch-name>\n";
            return 1;
        }

        repo.switchBranch(argv[2]);
    }
    else if (command == "merge")
    {
        if (argc < 3)
        {
            std::cout << "Usage: carrot merge <branch-name>\n";
            return 1;
        }

        repo.merge(argv[2]);
    }
    else
    {
        std::cout << "Unknown command.\n";
    }

    return 0;
}