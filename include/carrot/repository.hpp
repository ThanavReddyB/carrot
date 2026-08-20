#pragma once
#include <string>
#include <vector>  


class Repository
{
public:
    bool init();
    bool status();
    bool add(const std::string& filePath); 
    bool commit(const std::string& message);
    void log() const;
    void show(const std::string& objectId) const;
    bool checkout(const std::string& commitId);
    void branch(const std::string& branchName);
    bool switchBranch(const std::string& branchName);
    bool merge(const std::string& branchName);
    bool mergeContinue();
    void diff() const;
    void diffCached() const;

private:
    bool repositoryExists() const;
    void listUntrackedFiles() const;
    bool isFileStaged(const std::string& filePath) const;
    void listStagedFiles() const;
    bool isFileModified(const std::string& filePath) const;
    void listModifiedFiles() const;
    bool hasChangesToCommit() const;
    std::string createTreeFromIndex() const;
    bool hasUncommittedChanges() const;
    std::vector<std::string> getCommitParents(
    const std::string& commitId
    ) const;
    std::string getCommitTree(
        const std::string& commitId
    ) const;
    
};