#include "SafeFileStore.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

using std::string;

namespace
{
    bool fileExists(const string& path)
    {
        std::ifstream f(path.c_str());
        return f.good();
    }
}

SafeFileStore::SafeFileStore(const string& filePath) : filePath_(filePath) {}

bool SafeFileStore::renameFile(const string& from, const string& to)
{
    return std::rename(from.c_str(), to.c_str()) == 0;
}

bool SafeFileStore::removeFile(const string& path)
{
    return std::remove(path.c_str()) == 0;
}

void SafeFileStore::recoverInterruptedSave()
{
    if (!fileExists(filePath_) && fileExists(backupPath()))
    {
        if (!renameFile(backupPath(), filePath_))
            throw std::runtime_error("cannot restore " + filePath_ + " from backup " + backupPath());
    }
}

bool SafeFileStore::exists()
{
    recoverInterruptedSave();
    return fileExists(filePath_);
}

string SafeFileStore::read()
{
    recoverInterruptedSave();

    std::ifstream in(filePath_.c_str());
    if (!in)
        throw std::runtime_error("cannot open file: " + filePath_);

    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (in.bad())
        throw std::runtime_error("error while reading file: " + filePath_);

    string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0)
        content.erase(0, 3);
    return content;
}

void SafeFileStore::write(const string& content)
{
    recoverInterruptedSave();

    const string temp = tempPath();
    const string backup = backupPath();

    bool written = false;
    {
        std::ofstream out(temp.c_str(), std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot create temporary file: " + temp);
        out << content;
        out.close();
        written = !out.fail();
    }
    if (!written)
    {
        removeFile(temp);
        throw std::runtime_error("error while writing temporary file: " + temp +
            " (the file was not changed)");
    }

    const bool hadOriginal = fileExists(filePath_);
    if (hadOriginal)
    {
        removeFile(backup);
        if (!renameFile(filePath_, backup))
        {
            removeFile(temp);
            throw std::runtime_error("cannot create backup " + backup +
                " (the file was not changed)");
        }
    }

    if (!renameFile(temp, filePath_))
    {
        string message = "cannot put the new file in place: " + filePath_;
        if (hadOriginal)
        {
            if (renameFile(backup, filePath_))
                message += " (the previous file was restored)";
            else
                message += " (the previous data is kept in " + backup + ")";
        }
        removeFile(temp);
        throw std::runtime_error(message);
    }

    if (hadOriginal)
        removeFile(backup);
}