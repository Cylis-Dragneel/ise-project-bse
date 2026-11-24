#pragma once
#ifndef SAFE_FILE_STORE_H
#define SAFE_FILE_STORE_H

#include <string>

class SafeFileStore
{
private:
    std::string filePath_;

    std::string tempPath() const { return filePath_ + ".tmp"; }
    std::string backupPath() const { return filePath_ + ".bak"; }
    void recoverInterruptedSave();

protected:
    virtual bool renameFile(const std::string& from, const std::string& to);
    virtual bool removeFile(const std::string& path);

public:
    explicit SafeFileStore(const std::string& filePath);
    virtual ~SafeFileStore() = default;

    const std::string& path() const { return filePath_; }
    bool exists();
    std::string read();
    void write(const std::string& content);
};

#endif