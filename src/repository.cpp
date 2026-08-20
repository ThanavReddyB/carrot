#include "carrot/repository.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <map>
#include <sstream>
#include "carrot/hash.hpp"

namespace fs = std::filesystem;

bool Repository::repositoryExists() const
{
    return fs::exists(".carrot");
}

bool Repository::init()
{
    if (repositoryExists())
    {
        std::cout << "Repository already initialized.\n";
        return false;
    }

    const fs::path carrotPath = ".carrot";

    fs::create_directory(carrotPath);

    const std::vector<std::string> directories = {
        "objects",
        "refs",
        "logs"
    };

    for (const auto& dir : directories)
    {
        fs::create_directory(carrotPath / dir);
    }

    std::ofstream headFile(carrotPath / "HEAD");
    headFile << "master";
    std::ofstream(carrotPath / "index");
    std::ofstream(carrotPath / "config");

    std::cout << "Initialized empty Carrot repository.\n";

    return true;
}

bool Repository::isFileStaged(const std::string& filePath) const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return false;
    }

    std::string indexLine;
    std::string indexHash;

    while (std::getline(indexFile, indexLine))
    {
        if (indexLine.empty())
        {
            continue;
        }

        std::size_t space = indexLine.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string name = indexLine.substr(0, space);

        if (name == filePath)
        {
            indexHash = indexLine.substr(space + 1);
            break;
        }
    }

    if (indexHash.empty())
    {
        return false;
    }

    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        return true;
    }

    std::string branch;
    std::getline(headFile, branch);

    std::ifstream branchFile(
        fs::path(".carrot") / "refs" / "heads" / branch
    );

    if (!branchFile)
    {
        return true;
    }

    std::string commitId;
    std::getline(branchFile, commitId);

    if (commitId.empty())
    {
        return true;
    }

    std::string treeHash = getCommitTree(commitId);

    if (treeHash.empty())
    {
        return true;
    }

    std::ifstream treeFile(
        fs::path(".carrot") / "objects" / treeHash
    );

    if (!treeFile)
    {
        return true;
    }

    std::string line;
    std::getline(treeFile, line); // "tree"

    while (std::getline(treeFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t space = line.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string name = line.substr(0, space);
        std::string treeHash = line.substr(space + 1);

        if (name == filePath)
        {
            return treeHash != indexHash;
        }
    }

    // File exists in index but not in HEAD's tree.
    return true;
}

bool Repository::isFileModified(const std::string& filePath) const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return false;
    }

    std::string line;

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t space = line.find(' ');

        std::string indexedFile = line.substr(0, space);
        std::string indexedHash = line.substr(space + 1);

        if (indexedFile == filePath)
        {
            std::ifstream file(filePath);

            if (!file)
            {
                return false;
            }

            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );

        std::string objectContent = "blob\n" + content;

        std::string currentHash = Hash::sha256(objectContent);

        return currentHash != indexedHash;
        }
    }

    return false;
}

void Repository::listUntrackedFiles() const
{
    std::cout << "Untracked files:\n";

    std::ifstream indexFile(".carrot/index");

    std::vector<std::string> trackedFiles;

    if (indexFile)
    {
        std::string line;

        while (std::getline(indexFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            std::size_t space = line.find(' ');

            if (space == std::string::npos)
            {
                continue;
            }

            trackedFiles.push_back(line.substr(0, space));
        }
    }

    for (const auto& entry : fs::directory_iterator("."))
    {
        auto filename = entry.path().filename();

        if (filename == ".carrot")
        {
            continue;
        }

        std::string fileName = filename.string();

        bool tracked = false;

        for (const auto& trackedFile : trackedFiles)
        {
            if (trackedFile == fileName)
            {
                tracked = true;
                break;
            }
        }

        if (!tracked)
        {
            std::cout << "    " << fileName << '\n';
        }
    }
}

void Repository::listStagedFiles() const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return;
    }

    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        return;
    }

    std::string branch;
    std::getline(headFile, branch);

    std::ifstream branchFile(
        fs::path(".carrot") / "refs" / "heads" / branch
    );

    if (!branchFile)
    {
        return;
    }

    std::string currentCommit;
    std::getline(branchFile, currentCommit);

    if (currentCommit.empty())
    {
        return;
    }

    std::ifstream commitFile(
        fs::path(".carrot") / "objects" / currentCommit
    );

    if (!commitFile)
    {
        return;
    }

    std::string line;
    std::string treeHash;

    while (std::getline(commitFile, line))
    {
        if (line == "tree")
        {
            std::getline(commitFile, treeHash);
            break;
        }
    }

    if (treeHash.empty())
    {
        return;
    }

    std::ifstream treeFile(
        fs::path(".carrot") / "objects" / treeHash
    );

    if (!treeFile)
    {
        return;
    }

    std::string treeType;
    std::getline(treeFile, treeType);

    if (treeType != "tree")
    {
        return;
    }

    std::vector<std::string> treeEntries;

    while (std::getline(treeFile, line))
    {
        if (!line.empty())
        {
            treeEntries.push_back(line);
        }
    }

    std::cout << "Changes to be committed:\n";

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        bool found = false;

        for (const auto& entry : treeEntries)
        {
            if (entry == line)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::size_t space = line.find(' ');
            std::string fileName = line.substr(0, space);

            std::cout << "    " << fileName << '\n';
        }
    }
}

void Repository::listModifiedFiles() const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return;
    }

    std::string line;

    std::cout << "Changes not staged for commit:\n";

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t space = line.find(' ');

        std::string fileName = line.substr(0, space);

        if (isFileModified(fileName))
        {
            std::cout << "    " << fileName << '\n';
        }
    }
}

bool Repository::status()
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return false;
    }

    std::string branch;
    std::getline(headFile, branch);

    std::cout << "On branch " << branch << "\n\n";

    fs::path branchPath =
        fs::path(".carrot") / "refs" / "heads" / branch;

    std::ifstream branchFile(branchPath);

    std::string currentCommit;

    if (branchFile)
    {
        std::getline(branchFile, currentCommit);
    }

    if (currentCommit.empty())
    {
        std::cout << "No commits yet.\n\n";
    }

    listStagedFiles();

    std::cout << '\n';

    listModifiedFiles();

    std::cout << '\n';

    listUntrackedFiles();

    return true;
}

bool Repository::add(const std::string& filePath)
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    fs::path path(filePath);

    if (!fs::exists(path))
    {
        std::cout << "File '" << filePath << "' does not exist.\n";
        return false;
    }

    std::ifstream file(path);

    if (!file)
    {
        std::cout << "Could not open file.\n";
        return false;
    }
    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    std::string objectContent = "blob\n" + content;

    std::string hash = Hash::sha256(objectContent);

    fs::path objectPath = fs::path(".carrot") / "objects" / hash;

    if (!fs::exists(objectPath))
    {
        std::ofstream objectFile(objectPath);

        if (!objectFile)
        {
            std::cout << "Could not create object.\n";
            return false;
        }

        objectFile << objectContent;
    }

    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not open index.\n";
        return false;
    }

    std::vector<std::string> entries;
    std::string line;

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::string indexedFile = line.substr(0, line.find(' '));

        if (indexedFile != filePath)
        {
            entries.push_back(line);
        }
    }

    indexFile.close();

    entries.push_back(filePath + " " + hash);

    std::ofstream outputIndex(".carrot/index");

    if (!outputIndex)
    {
        std::cout << "Could not write index.\n";
        return false;
    }

    for (const auto& entry : entries)
    {
        outputIndex << entry << '\n';
    }

    std::cout << "SHA-256: " << hash << '\n';

    return true;
}

bool Repository::hasChangesToCommit() const
{
    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        return true;
    }

    std::string currentBranch;
    std::getline(headFile, currentBranch);

    if (currentBranch.empty())
    {
        return true;
    }

    std::ifstream branchFile(
        fs::path(".carrot") / "refs" / "heads" / currentBranch
    );

    if (!branchFile)
    {
        return true;
    }

    std::string currentCommit;
    std::getline(branchFile, currentCommit);

    if (currentCommit.empty())
    {
        return true;
    }

    std::ifstream commitFile(
        fs::path(".carrot") / "objects" / currentCommit
    );

    if (!commitFile)
    {
        return true;
    }

    std::string line;
    std::string treeHash;

    while (std::getline(commitFile, line))
    {
        if (line == "tree")
        {
            std::getline(commitFile, treeHash);
            break;
        }
    }

    if (treeHash.empty())
    {
        return true;
    }

    std::ifstream treeFile(
        fs::path(".carrot") / "objects" / treeHash
    );

    if (!treeFile)
    {
        return true;
    }

    std::string treeType;
    std::getline(treeFile, treeType);

    if (treeType != "tree")
    {
        return true;
    }

    std::vector<std::string> treeEntries;

    while (std::getline(treeFile, line))
    {
        if (!line.empty())
        {
            treeEntries.push_back(line);
        }
    }

    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return false;
    }

    std::vector<std::string> indexEntries;

    while (std::getline(indexFile, line))
    {
        if (!line.empty())
        {
            indexEntries.push_back(line);
        }
    }

    std::sort(treeEntries.begin(), treeEntries.end());
    std::sort(indexEntries.begin(), indexEntries.end());

    return treeEntries != indexEntries;
}

bool Repository::hasUncommittedChanges() const
{
    // Check staged changes.
    if (hasChangesToCommit())
    {
        return true;
    }

    // Get current index.
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return false;
    }

    std::string line;

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t space = line.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string fileName = line.substr(0, space);
        std::string stagedHash = line.substr(space + 1);

        std::ifstream file(fileName);

        if (!file)
        {
            return true;
        }

       std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
        );

        std::string objectContent = "blob\n" + content;

        std::string currentHash = Hash::sha256(objectContent);

        if (currentHash != stagedHash)
        {
            return true;
        }
    }

    return false;
}

bool Repository::commit(const std::string& message)
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not open index.\n";
        return false;
    }

    std::string indexContent;
    std::string line;

    while (std::getline(indexFile, line))
    {
        if (!line.empty())
        {
            indexContent += line + '\n';
        }
    }

    if (indexContent.empty())
    {
        std::cout << "Nothing to commit.\n";
        return false;
    }

    if (!hasChangesToCommit())
    {
        std::cout << "Nothing to commit. Working tree clean.\n";
        return false;
    }

    // Find the branch currently pointed to by HEAD.
    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return false;
    }

    std::string currentBranch;
    std::getline(headFile, currentBranch);

    if (currentBranch.empty())
    {
        std::cout << "HEAD is not pointing to a branch.\n";
        return false;
    }

    // Get the current commit of that branch.
    std::string previousCommit;

    std::ifstream branchFile(
        fs::path(".carrot") / "refs" / "heads" / currentBranch
    );

    if (branchFile)
    {
        std::getline(branchFile, previousCommit);
    }

    // Create the commit object.
    std::string commitContent;

    commitContent += "commit\n";

    std::string treeHash = createTreeFromIndex();

    if (treeHash.empty())
    {
        std::cout << "Could not create tree.\n";
        return false;
    }

    commitContent += "tree\n";
    commitContent += treeHash;
    commitContent += "\n";

    commitContent += "parent\n";
    commitContent += previousCommit;
    commitContent += "\n";

    commitContent += "message\n";
    commitContent += message;
    commitContent += "\n";

    std::string commitHash = Hash::sha256(commitContent);

    fs::path commitPath =
        fs::path(".carrot") / "objects" / commitHash;

    std::ofstream commitFile(commitPath);

    if (!commitFile)
    {
        std::cout << "Could not create commit object.\n";
        return false;
    }

    commitFile << commitContent;

    // Update the branch currently pointed to by HEAD.
    fs::path branchPath =
        fs::path(".carrot") / "refs" / "heads" / currentBranch;

    fs::create_directories(branchPath.parent_path());

    std::ofstream branchOutput(branchPath);

    if (!branchOutput)
    {
        std::cout << "Could not update branch.\n";
        return false;
    }

    branchOutput << commitHash;

    // Update index to match the committed tree.
    std::ifstream treeFile(
        fs::path(".carrot") / "objects" / treeHash
    );

    if (!treeFile)
    {
        std::cout << "Could not read committed tree.\n";
        return false;
    }

    std::string treeLine;
    std::getline(treeFile, treeLine);

    std::ofstream newIndex(".carrot/index");

    if (!newIndex)
    {
        std::cout << "Could not update index.\n";
        return false;
    }

    while (std::getline(treeFile, treeLine))
    {
        if (!treeLine.empty())
        {
            newIndex << treeLine << '\n';
        }
    }

    std::cout << "Committed: " << commitHash << '\n';

    return true;
}

std::vector<std::string> Repository::getCommitParents(
    const std::string& commitId
) const
{
    std::vector<std::string> parents;

    fs::path commitPath =
        fs::path(".carrot") / "objects" / commitId;

    std::ifstream commitFile(commitPath);

    if (!commitFile)
    {
        return parents;
    }

    std::string line;

    while (std::getline(commitFile, line))
    {
        if (line == "parent")
        {
            std::string parent;

            if (std::getline(commitFile, parent) && !parent.empty())
            {
                parents.push_back(parent);
            }
        }
    }

    return parents;
}

std::string Repository::getCommitTree(
    const std::string& commitId
) const
{
    fs::path commitPath =
        fs::path(".carrot") / "objects" / commitId;

    std::ifstream commitFile(commitPath);

    if (!commitFile)
    {
        return "";
    }

    std::string line;

    while (std::getline(commitFile, line))
    {
        if (line == "tree")
        {
            std::string treeHash;

            if (std::getline(commitFile, treeHash))
            {
                return treeHash;
            }

            return "";
        }
    }

    return "";
}

void Repository::log() const
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return;
    }

    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return;
    }

    std::string branch;
    std::getline(headFile, branch);

    if (branch.empty())
    {
        std::cout << "HEAD is not pointing to a branch.\n";
        return;
    }

    std::ifstream branchFile(
        fs::path(".carrot") / "refs" / "heads" / branch
    );

    if (!branchFile)
    {
        std::cout << "No commits yet.\n";
        return;
    }

    std::string currentCommit;
    std::getline(branchFile, currentCommit);

    if (currentCommit.empty())
    {
        std::cout << "No commits yet.\n";
        return;
    }

    std::vector<std::string> stack;
    std::vector<std::string> visited;

    stack.push_back(currentCommit);

    while (!stack.empty())
    {
        std::string commitId = stack.back();
        stack.pop_back();

        if (commitId.empty())
        {
            continue;
        }

        if (std::find(
                visited.begin(),
                visited.end(),
                commitId
            ) != visited.end())
        {
            continue;
        }

        visited.push_back(commitId);

        fs::path commitPath =
            fs::path(".carrot") / "objects" / commitId;

        std::ifstream commitFile(commitPath);

        if (!commitFile)
        {
            continue;
        }

        std::string line;
        std::string message;

        while (std::getline(commitFile, line))
        {
            if (line == "message")
            {
                std::getline(commitFile, message);
                break;
            }
        }

        std::cout << "commit " << commitId << '\n';
        std::cout << "    " << message << "\n\n";

        std::vector<std::string> parents =
            getCommitParents(commitId);

        for (const auto& parent : parents)
        {
            stack.push_back(parent);
        }
    }
}

std::string Repository::createTreeFromIndex() const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        return "";
    }

    std::string treeContent;
    std::string line;

    while (std::getline(indexFile, line))
    {
        if (!line.empty())
        {
            treeContent += line + '\n';
        }
    }

    if (treeContent.empty())
    {
        return "";
    }

    std::string objectContent = "tree\n" + treeContent;

    std::string treeHash = Hash::sha256(objectContent);

    fs::path treePath =
        fs::path(".carrot") / "objects" / treeHash;

    if (!fs::exists(treePath))
    {
        std::ofstream treeFile(treePath);

        if (!treeFile)
        {
            return "";
        }

        treeFile << objectContent;
    }

    return treeHash;
}

void Repository::show(const std::string& objectId) const
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return;
    }

    fs::path objectPath =
        fs::path(".carrot") / "objects" / objectId;

    if (!fs::exists(objectPath))
    {
        std::cout << "Object not found: " << objectId << '\n';
        return;
    }

    std::ifstream objectFile(objectPath);

    if (!objectFile)
    {
        std::cout << "Could not read object.\n";
        return;
    }

    std::string type;

    std::getline(objectFile, type);

    if (type == "blob")
    {
        std::cout << "Type: blob\n\n";

        std::string line;

        while (std::getline(objectFile, line))
        {
            std::cout << line << '\n';
        }
    }
    else if (type == "tree")
    {
        std::cout << "Type: tree\n\n";

        std::string line;

        while (std::getline(objectFile, line))
        {
            std::cout << line << '\n';
        }
    }
    else if (type == "commit")
    {
        std::cout << "Type: commit\n\n";

        std::string line;

        while (std::getline(objectFile, line))
        {
            std::cout << line << '\n';
        }
    }
    else
    {
        std::cout << "Unknown object type.\n";
    }
}

bool Repository::checkout(const std::string& commitId)
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    if (hasUncommittedChanges())
    {
        std::cout << "Cannot checkout: you have uncommitted changes.\n";
        return false;
    }

    fs::path commitPath =
        fs::path(".carrot") / "objects" / commitId;

    std::ifstream commitFile(commitPath);

    if (!commitFile)
    {
        std::cout << "Commit not found: " << commitId << '\n';
        return false;
    }

    std::string line;
    std::string type;

    std::getline(commitFile, type);

    if (type != "commit")
    {
        std::cout << "Object is not a commit.\n";
        return false;
    }

    std::string treeHash;

    while (std::getline(commitFile, line))
    {
        if (line == "tree")
        {
            std::getline(commitFile, treeHash);
            break;
        }
    }

    if (treeHash.empty())
    {
        std::cout << "Commit does not contain a tree.\n";
        return false;
    }

    fs::path treePath =
        fs::path(".carrot") / "objects" / treeHash;

    std::ifstream treeFile(treePath);

    if (!treeFile)
    {
        std::cout << "Tree not found.\n";
        return false;
    }

    std::string treeType;
    std::getline(treeFile, treeType);

    if (treeType != "tree")
    {
        std::cout << "Object is not a tree.\n";
        return false;
    }

    // Store the tree entries so we can update the index
    // after restoring the working files.
    std::vector<std::string> treeEntries;

    while (std::getline(treeFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        treeEntries.push_back(line);

        std::size_t space = line.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string fileName = line.substr(0, space);
        std::string blobHash = line.substr(space + 1);

        fs::path blobPath =
            fs::path(".carrot") / "objects" / blobHash;

        std::ifstream blobFile(blobPath);

        if (!blobFile)
        {
            std::cout << "Blob not found: " << blobHash << '\n';
            return false;
        }

        std::string blobType;
        std::getline(blobFile, blobType);

        if (blobType != "blob")
        {
            std::cout << "Object is not a blob.\n";
            return false;
        }

        std::string content(
            (std::istreambuf_iterator<char>(blobFile)),
            std::istreambuf_iterator<char>()
        );

        std::ofstream outputFile(fileName);

        if (!outputFile)
        {
            std::cout << "Could not write file: "
                      << fileName << '\n';
            return false;
        }

        outputFile << content;
    }

    // Make the index represent the checked-out commit.
    std::ofstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not update index.\n";
        return false;
    }

    for (const auto& entry : treeEntries)
    {
        indexFile << entry << '\n';
    }

    std::cout << "Checked out commit " << commitId << '\n';

    return true;
}

void Repository::branch(const std::string& branchName)
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return;
    }

    if (branchName.empty())
    {
        std::ifstream headFile(".carrot/HEAD");

        if (!headFile)
        {
            std::cout << "Could not open HEAD.\n";
            return;
        }

        std::string currentBranch;
        std::getline(headFile, currentBranch);

        fs::path headsPath = fs::path(".carrot") / "refs" / "heads";

        if (!fs::exists(headsPath))
        {
            return;
        }

        for (const auto& entry : fs::directory_iterator(headsPath))
        {
            std::string name = entry.path().filename().string();

            if (name == currentBranch)
            {
                std::cout << "* " << name << '\n';
            }
            else
            {
                std::cout << "  " << name << '\n';
            }
        }

        return;
    }

    // Check whether the branch already exists.
    fs::path branchPath =
        fs::path(".carrot") / "refs" / "heads" / branchName;

    if (fs::exists(branchPath))
    {
        std::cout << "Branch already exists.\n";
        return;
    }

    // Find the current branch.
    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return;
    }

    std::string currentBranch;
    std::getline(headFile, currentBranch);

    // Find the commit currently pointed to by that branch.
    std::ifstream currentBranchFile(
        fs::path(".carrot") / "refs" / "heads" / currentBranch
    );

    std::string currentCommit;

    if (currentBranchFile)
    {
        std::getline(currentBranchFile, currentCommit);
    }

    // Create the new branch.
    fs::create_directories(branchPath.parent_path());

    std::ofstream newBranch(branchPath);

    if (!newBranch)
    {
        std::cout << "Could not create branch.\n";
        return;
    }

    newBranch << currentCommit;

    std::cout << "Created branch " << branchName << '\n';
}


bool Repository::switchBranch(const std::string& branchName)
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    fs::path branchPath =
        fs::path(".carrot") / "refs" / "heads" / branchName;

    if (!fs::exists(branchPath))
    {
        std::cout << "Branch does not exist: "
                  << branchName << '\n';
        return false;
    }

    std::ifstream branchFile(branchPath);

    if (!branchFile)
    {
        std::cout << "Could not read branch.\n";
        return false;
    }

    std::string commitId;
    std::getline(branchFile, commitId);

    if (commitId.empty())
    {
        std::cout << "Branch has no commits yet.\n";
        return false;
    }

    if (hasUncommittedChanges())
    {
        std::cout << "Cannot switch branches: "
                     "you have uncommitted changes.\n";
        return false;
    }

    if (!checkout(commitId))
    {
        return false;
    }

    std::ofstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not update HEAD.\n";
        return false;
    }

    headFile << branchName;

    std::cout << "Switched to branch " << branchName << '\n';

    return true;
}

bool Repository::merge(const std::string& branchName)
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    // Get current branch from HEAD.
    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return false;
    }

    std::string currentBranch;
    std::getline(headFile, currentBranch);

    if (currentBranch.empty())
    {
        std::cout << "HEAD is not pointing to a branch.\n";
        return false;
    }

    if (currentBranch == branchName)
    {
        std::cout << "Cannot merge a branch into itself.\n";
        return false;
    }

    // Find target branch.
    fs::path targetBranchPath =
        fs::path(".carrot") / "refs" / "heads" / branchName;

    if (!fs::exists(targetBranchPath))
    {
        std::cout << "Branch does not exist: "
                  << branchName << '\n';
        return false;
    }

    std::ifstream targetBranchFile(targetBranchPath);

    if (!targetBranchFile)
    {
        std::cout << "Could not read target branch.\n";
        return false;
    }

    std::string targetCommit;
    std::getline(targetBranchFile, targetCommit);

    if (targetCommit.empty())
    {
        std::cout << "Target branch has no commits.\n";
        return false;
    }

    // Find current commit.
    fs::path currentBranchPath =
        fs::path(".carrot") / "refs" / "heads" / currentBranch;

    std::ifstream currentBranchFile(currentBranchPath);

    if (!currentBranchFile)
    {
        std::cout << "Current branch has no commits.\n";
        return false;
    }

    std::string currentCommit;
    std::getline(currentBranchFile, currentCommit);

    if (currentCommit.empty())
    {
        std::cout << "Current branch has no commits.\n";
        return false;
    }

    // Never overwrite local work during a merge.
    if (hasUncommittedChanges())
    {
        std::cout << "Cannot merge: you have uncommitted changes.\n";
        return false;
    }

    // Already merged / same commit.
    if (currentCommit == targetCommit)
    {
        std::cout << "Already up to date.\n";
        return true;
    }

    // Helper: determine whether ancestor is in the history of descendant.
    auto isAncestor =
        [&](const std::string& ancestor,
            const std::string& descendant) -> bool
    {
        std::vector<std::string> stack;
        std::vector<std::string> visited;

        stack.push_back(descendant);

        while (!stack.empty())
        {
            std::string commit = stack.back();
            stack.pop_back();

            if (commit.empty())
            {
                continue;
            }

            if (commit == ancestor)
            {
                return true;
            }

            bool alreadyVisited = false;

            for (const auto& id : visited)
            {
                if (id == commit)
                {
                    alreadyVisited = true;
                    break;
                }
            }

            if (alreadyVisited)
            {
                continue;
            }

            visited.push_back(commit);

            std::vector<std::string> parents =
                getCommitParents(commit);

            for (const auto& parent : parents)
            {
                stack.push_back(parent);
            }
        }

        return false;
    };

    // Fast-forward:
    //
    // current → A
    // target  → B
    //
    // A is an ancestor of B.
    if (isAncestor(currentCommit, targetCommit))
    {
        if (!checkout(targetCommit))
        {
            return false;
        }

        std::ofstream branchOutput(currentBranchPath);

        if (!branchOutput)
        {
            std::cout << "Could not update current branch.\n";
            return false;
        }

        branchOutput << targetCommit;

        // Update index to match the target tree.
        std::string targetTree = getCommitTree(targetCommit);

        if (targetTree.empty())
        {
            std::cout << "Could not find target tree.\n";
            return false;
        }

        std::ifstream treeFile(
            fs::path(".carrot") / "objects" / targetTree
        );

        if (!treeFile)
        {
            std::cout << "Could not read target tree.\n";
            return false;
        }

        std::string treeLine;
        std::getline(treeFile, treeLine); // "tree"

        std::ofstream indexFile(".carrot/index");

        if (!indexFile)
        {
            std::cout << "Could not update index.\n";
            return false;
        }

        while (std::getline(treeFile, treeLine))
        {
            if (!treeLine.empty())
            {
                indexFile << treeLine << '\n';
            }
        }

        std::cout << "Fast-forwarded " << currentBranch
                  << " to " << targetCommit << '\n';

        return true;
    }

    // If target is already an ancestor of current, nothing needs to happen.
    if (isAncestor(targetCommit, currentCommit))
    {
        std::cout << "Already up to date.\n";
        return true;
    }

    // Find a common ancestor.
    std::vector<std::string> currentHistory;
    std::vector<std::string> stack;
    stack.push_back(currentCommit);

    while (!stack.empty())
    {
        std::string commit = stack.back();
        stack.pop_back();

        if (commit.empty())
        {
            continue;
        }

        bool visited = false;

        for (const auto& id : currentHistory)
        {
            if (id == commit)
            {
                visited = true;
                break;
            }
        }

        if (visited)
        {
            continue;
        }

        currentHistory.push_back(commit);

        std::vector<std::string> parents =
            getCommitParents(commit);

        for (const auto& parent : parents)
        {
            stack.push_back(parent);
        }
    }

    std::string baseCommit;

    stack.clear();
    stack.push_back(targetCommit);

    while (!stack.empty() && baseCommit.empty())
    {
        std::string commit = stack.back();
        stack.pop_back();

        if (commit.empty())
        {
            continue;
        }

        for (const auto& ancestor : currentHistory)
        {
            if (ancestor == commit)
            {
                baseCommit = commit;
                break;
            }
        }

        if (!baseCommit.empty())
        {
            break;
        }

        std::vector<std::string> parents =
            getCommitParents(commit);

        for (const auto& parent : parents)
        {
            stack.push_back(parent);
        }
    }

    if (baseCommit.empty())
    {
        std::cout << "Could not find a common ancestor.\n";
        return false;
    }

    std::string baseTree = getCommitTree(baseCommit);
    std::string currentTree = getCommitTree(currentCommit);
    std::string targetTree = getCommitTree(targetCommit);

    if (baseTree.empty() ||
        currentTree.empty() ||
        targetTree.empty())
    {
        std::cout << "Could not read merge trees.\n";
        return false;
    }

    // Read a tree into filename → blob SHA.
    auto readTree =
        [&](const std::string& treeHash)
        -> std::map<std::string, std::string>
    {
        std::map<std::string, std::string> files;

        std::ifstream treeFile(
            fs::path(".carrot") / "objects" / treeHash
        );

        if (!treeFile)
        {
            return files;
        }

        std::string line;
        std::getline(treeFile, line); // "tree"

        while (std::getline(treeFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            std::size_t space = line.find(' ');

            if (space == std::string::npos)
            {
                continue;
            }

            std::string fileName = line.substr(0, space);
            std::string blobHash = line.substr(space + 1);

            files[fileName] = blobHash;
        }

        return files;
    };

    auto baseFiles = readTree(baseTree);
    auto currentFiles = readTree(currentTree);
    auto targetFiles = readTree(targetTree);

    // Read blob contents.
    auto readBlob =
        [&](const std::string& blobHash)
        -> std::string
    {
        if (blobHash.empty())
        {
            return "";
        }

        std::ifstream blobFile(
            fs::path(".carrot") / "objects" / blobHash
        );

        if (!blobFile)
        {
            return "";
        }

        std::string type;
        std::getline(blobFile, type);

        if (type != "blob")
        {
            return "";
        }

        std::string content;
        char c;

        while (blobFile.get(c))
        {
            content += c;
        }

        return content;
    };

    std::map<std::string, std::string> mergedFiles;

    std::vector<std::string> allFiles;

    for (const auto& pair : baseFiles)
        allFiles.push_back(pair.first);

    for (const auto& pair : currentFiles)
        allFiles.push_back(pair.first);

    for (const auto& pair : targetFiles)
        allFiles.push_back(pair.first);

    std::sort(allFiles.begin(), allFiles.end());

    allFiles.erase(
        std::unique(allFiles.begin(), allFiles.end()),
        allFiles.end()
    );

    bool conflict = false;

    for (const auto& fileName : allFiles)
    {
        std::string baseHash =
            baseFiles.count(fileName)
                ? baseFiles[fileName]
                : "";

        std::string currentHash =
            currentFiles.count(fileName)
                ? currentFiles[fileName]
                : "";

        std::string targetHash =
            targetFiles.count(fileName)
                ? targetFiles[fileName]
                : "";

        // Both sides are identical.
        if (currentHash == targetHash)
        {
            if (!currentHash.empty())
            {
                mergedFiles[fileName] = currentHash;
            }

            continue;
        }

        // Current branch did not change it.
        if (currentHash == baseHash)
        {
            if (!targetHash.empty())
            {
                mergedFiles[fileName] = targetHash;
            }

            if (targetHash.empty())
            {
                fs::remove(fileName);
            }

            continue;
        }

        // Target branch did not change it.
        if (targetHash == baseHash)
        {
            if (!currentHash.empty())
            {
                mergedFiles[fileName] = currentHash;
            }

            if (currentHash.empty())
            {
                fs::remove(fileName);
            }

            continue;
        }

        // Both sides changed it differently.
        conflict = true;

        std::string currentContent = readBlob(currentHash);
        std::string targetContent = readBlob(targetHash);

        std::ofstream conflictFile(fileName);

        if (!conflictFile)
        {
            std::cout << "Could not write conflict file: "
                      << fileName << '\n';
            return false;
        }

        conflictFile << "<<<<<<< HEAD\n";
        conflictFile << currentContent;

        if (!currentContent.empty() &&
            currentContent.back() != '\n')
        {
            conflictFile << '\n';
        }

        conflictFile << "=======\n";
        conflictFile << targetContent;

        if (!targetContent.empty() &&
            targetContent.back() != '\n')
        {
            conflictFile << '\n';
        }

        conflictFile << ">>>>>>> "
                     << branchName << '\n';

        std::cout << "CONFLICT: " << fileName << '\n';
    }

    if (conflict)
    {
        std::ofstream mergeHead(".carrot/MERGE_HEAD");

        if (!mergeHead)
        {
            std::cout << "Could not create MERGE_HEAD.\n";
            return false;
        }

        mergeHead << targetCommit;

        std::cout << "Merge failed due to conflicts.\n";
        std::cout << "Resolve the conflicts and run "
                << "carrot merge --continue\n";

        return false;
    }

    // Build the merged tree.
    std::string mergedTreeContent;

    for (const auto& pair : mergedFiles)
    {
        mergedTreeContent +=
            pair.first + " " + pair.second + '\n';
    }

    std::string mergedTreeObject =
        "tree\n" + mergedTreeContent;

    std::string mergedTreeHash =
        Hash::sha256(mergedTreeObject);

    fs::path mergedTreePath =
        fs::path(".carrot") / "objects" / mergedTreeHash;

    if (!fs::exists(mergedTreePath))
    {
        std::ofstream treeFile(mergedTreePath);

        if (!treeFile)
        {
            std::cout << "Could not create merge tree.\n";
            return false;
        }

        treeFile << mergedTreeObject;
    }

    // Create a two-parent merge commit.
    std::string commitContent;

    commitContent += "commit\n";

    commitContent += "tree\n";
    commitContent += mergedTreeHash;
    commitContent += "\n";

    commitContent += "parent\n";
    commitContent += currentCommit;
    commitContent += "\n";

    commitContent += "parent\n";
    commitContent += targetCommit;
    commitContent += "\n";

    commitContent += "message\n";
    commitContent += "Merge branch '" + branchName + "'";
    commitContent += "\n";

    std::string mergeCommitHash =
        Hash::sha256(commitContent);

    fs::path mergeCommitPath =
        fs::path(".carrot") / "objects" / mergeCommitHash;

    if (!fs::exists(mergeCommitPath))
    {
        std::ofstream mergeCommitFile(mergeCommitPath);

        if (!mergeCommitFile)
        {
            std::cout << "Could not create merge commit.\n";
            return false;
        }

        mergeCommitFile << commitContent;
    }

    // Update the current branch only.
    std::ofstream branchOutput(currentBranchPath);

    if (!branchOutput)
    {
        std::cout << "Could not update current branch.\n";
        return false;
    }

    branchOutput << mergeCommitHash;

    // Update working tree and index.
    for (const auto& fileName : allFiles)
    {
        auto it = mergedFiles.find(fileName);

        if (it == mergedFiles.end())
        {
            fs::remove(fileName);
            continue;
        }

        std::string content =
            readBlob(it->second);

        std::ofstream outputFile(fileName);

        if (!outputFile)
        {
            std::cout << "Could not write file: "
                      << fileName << '\n';
            return false;
        }

        outputFile << content;
    }

    std::ofstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not update index.\n";
        return false;
    }

    for (const auto& pair : mergedFiles)
    {
        indexFile << pair.first
                  << " "
                  << pair.second
                  << '\n';
    }

    std::cout << "Merged branch "
              << branchName
              << " into "
              << currentBranch
              << ".\n";

    std::cout << "Merge commit: "
              << mergeCommitHash
              << '\n';

    return true;
}

bool Repository::mergeContinue()
{
    if (!repositoryExists())
    {
        std::cout << "Not a Carrot repository. Initialize a repository first.\n";
        return false;
    }

    fs::path mergeHeadPath =
        fs::path(".carrot") / "MERGE_HEAD";

    std::ifstream mergeHeadFile(mergeHeadPath);

    if (!mergeHeadFile)
    {
        std::cout << "No merge in progress.\n";
        return false;
    }

    std::string targetCommit;
    std::getline(mergeHeadFile, targetCommit);

    if (targetCommit.empty())
    {
        std::cout << "Invalid MERGE_HEAD.\n";
        return false;
    }

    // Make sure there are no unresolved conflict markers.
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not open index.\n";
        return false;
    }

    std::string line;

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t space = line.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string fileName = line.substr(0, space);

        std::ifstream file(fileName);

        if (!file)
        {
            continue;
        }

        std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        if (content.find("<<<<<<< ") != std::string::npos ||
            content.find("=======") != std::string::npos ||
            content.find(">>>>>>> ") != std::string::npos)
        {
            std::cout << "Cannot continue merge: unresolved conflicts remain.\n";
            return false;
        }
    }

    // The current branch's commit becomes parent 1.
    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return false;
    }

    std::string currentBranch;
    std::getline(headFile, currentBranch);

    fs::path branchPath =
        fs::path(".carrot") / "refs" / "heads" / currentBranch;

    std::ifstream branchFile(branchPath);

    if (!branchFile)
    {
        std::cout << "Could not read current branch.\n";
        return false;
    }

    std::string currentCommit;
    std::getline(branchFile, currentCommit);

    if (currentCommit.empty())
    {
        std::cout << "Current branch has no commit.\n";
        return false;
    }

    // The index now represents the resolved merge result.
    std::string treeHash = createTreeFromIndex();

    if (treeHash.empty())
    {
        std::cout << "Could not create merge tree.\n";
        return false;
    }

    std::string commitContent;

    commitContent += "commit\n";

    commitContent += "tree\n";
    commitContent += treeHash;
    commitContent += "\n";

    commitContent += "parent\n";
    commitContent += currentCommit;
    commitContent += "\n";

    commitContent += "parent\n";
    commitContent += targetCommit;
    commitContent += "\n";

    commitContent += "message\n";
    commitContent += "Merge branch";
    commitContent += "\n";

    std::string mergeCommitHash =
        Hash::sha256(commitContent);

    fs::path commitPath =
        fs::path(".carrot") / "objects" / mergeCommitHash;

    if (!fs::exists(commitPath))
    {
        std::ofstream commitFile(commitPath);

        if (!commitFile)
        {
            std::cout << "Could not create merge commit.\n";
            return false;
        }

        commitFile << commitContent;
    }

    std::ofstream branchOutput(branchPath);

    if (!branchOutput)
    {
        std::cout << "Could not update current branch.\n";
        return false;
    }

    branchOutput << mergeCommitHash;

    mergeHeadFile.close();
    fs::remove(mergeHeadPath);

    std::cout << "Merge completed: "
              << mergeCommitHash << '\n';

    return true;
}

void Repository::diff() const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not open index.\n";
        return;
    }

    std::string line;

    while (std::getline(indexFile, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::size_t space = line.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string fileName = line.substr(0, space);
        std::string indexedHash = line.substr(space + 1);

        std::ifstream file(fileName);

        if (!file)
        {
            std::cout << "deleted: " << fileName << '\n';
            continue;
        }

        std::string currentContent(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        std::string currentHash =
            Hash::sha256("blob\n" + currentContent);

        if (currentHash == indexedHash)
        {
            continue;
        }

        // Read the indexed blob.
        fs::path blobPath =
            fs::path(".carrot") / "objects" / indexedHash;

        std::ifstream blobFile(blobPath);

        if (!blobFile)
        {
            std::cout << "Could not read indexed object: "
                      << fileName << '\n';
            continue;
        }

        std::string blobType;
        std::getline(blobFile, blobType);

        std::string oldContent(
            (std::istreambuf_iterator<char>(blobFile)),
            std::istreambuf_iterator<char>()
        );

        std::cout << "--- " << fileName << '\n';
        std::cout << "+++ " << fileName << '\n';

        std::istringstream oldStream(oldContent);
        std::istringstream newStream(currentContent);

        std::vector<std::string> oldLines;
        std::vector<std::string> newLines;

        std::string oldLine;
        std::string newLine;

        while (std::getline(oldStream, oldLine))
        {
            oldLines.push_back(oldLine);
        }

        while (std::getline(newStream, newLine))
        {
            newLines.push_back(newLine);
        }

        std::size_t maxLines =
            std::max(oldLines.size(), newLines.size());

        for (std::size_t i = 0; i < maxLines; ++i)
        {
            if (i >= oldLines.size())
            {
                std::cout << "+ " << newLines[i] << '\n';
            }
            else if (i >= newLines.size())
            {
                std::cout << "- " << oldLines[i] << '\n';
            }
            else if (oldLines[i] != newLines[i])
            {
                std::cout << "- " << oldLines[i] << '\n';
                std::cout << "+ " << newLines[i] << '\n';
            }
        }
    }
}


void Repository::diffCached() const
{
    std::ifstream indexFile(".carrot/index");

    if (!indexFile)
    {
        std::cout << "Could not open index.\n";
        return;
    }

    std::ifstream headFile(".carrot/HEAD");

    if (!headFile)
    {
        std::cout << "Could not open HEAD.\n";
        return;
    }

    std::string branch;
    std::getline(headFile, branch);

    std::ifstream branchFile(
        fs::path(".carrot") / "refs" / "heads" / branch
    );

    if (!branchFile)
    {
        std::cout << "No commits yet.\n";
        return;
    }

    std::string commitId;
    std::getline(branchFile, commitId);

    if (commitId.empty())
    {
        std::cout << "No commits yet.\n";
        return;
    }

    std::string treeHash = getCommitTree(commitId);

    if (treeHash.empty())
    {
        std::cout << "Could not find commit tree.\n";
        return;
    }

    std::ifstream treeFile(
        fs::path(".carrot") / "objects" / treeHash
    );

    if (!treeFile)
    {
        std::cout << "Could not open tree.\n";
        return;
    }

    std::vector<std::string> headFiles;

    std::string line;
    std::getline(treeFile, line); // "tree"

    while (std::getline(treeFile, line))
    {
        if (!line.empty())
        {
            headFiles.push_back(line);
        }
    }

    std::vector<std::string> indexFiles;

    while (std::getline(indexFile, line))
    {
        if (!line.empty())
        {
            indexFiles.push_back(line);
        }
    }

    for (const auto& indexEntry : indexFiles)
    {
        std::size_t space = indexEntry.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string fileName = indexEntry.substr(0, space);
        std::string indexHash = indexEntry.substr(space + 1);

        bool found = false;

        for (const auto& headEntry : headFiles)
        {
            std::size_t headSpace = headEntry.find(' ');

            if (headSpace == std::string::npos)
            {
                continue;
            }

            std::string headFileName =
                headEntry.substr(0, headSpace);

            std::string headHash =
                headEntry.substr(headSpace + 1);

            if (headFileName == fileName)
            {
                found = true;

                if (headHash != indexHash)
                {
                    std::cout << "modified: "
                              << fileName << '\n';
                }

                break;
            }
        }

        if (!found)
        {
            std::cout << "new file: "
                      << fileName << '\n';
        }
    }

    // Detect files removed from the index.
    for (const auto& headEntry : headFiles)
    {
        std::size_t space = headEntry.find(' ');

        if (space == std::string::npos)
        {
            continue;
        }

        std::string fileName = headEntry.substr(0, space);

        bool found = false;

        for (const auto& indexEntry : indexFiles)
        {
            std::size_t indexSpace = indexEntry.find(' ');

            if (indexSpace == std::string::npos)
            {
                continue;
            }

            if (indexEntry.substr(0, indexSpace) == fileName)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::cout << "deleted: "
                      << fileName << '\n';
        }
    }
}