#include "FileBillDetailRepository.h"

#include "BillDetailRecordCodec.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

using std::string;
using std::vector;

namespace
{
    bool fileExists(const string& path)
    {
        std::ifstream f(path.c_str());
        return f.good();
    }
}

FileBillDetailRepository::FileBillDetailRepository(const string& filePath)
    : filePath_(filePath)
{
}

bool FileBillDetailRepository::renameFile(const string& from, const string& to)
{
    return std::rename(from.c_str(), to.c_str()) == 0;
}

bool FileBillDetailRepository::removeFile(const string& path)
{
    return std::remove(path.c_str()) == 0;
}

void FileBillDetailRepository::recoverInterruptedSave()
{
    if (!fileExists(filePath_) && fileExists(backupPath()))
    {
        if (!renameFile(backupPath(), filePath_))
            throw std::runtime_error("cannot restore " + filePath_ + " from backup " + backupPath());

        if (fileExists(tempPath()))
            removeFile(tempPath());
    }
}

vector<BillLine> FileBillDetailRepository::loadAll()
{
    recoverInterruptedSave();

    std::ifstream input(filePath_.c_str());
    if (!input)
    {
        return vector<BillLine>();
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad())
    {
        throw std::runtime_error("error while reading bill detail file: " + filePath_);
    }

    string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        content.erase(0, 3);
    }

    vector<string> records;
    try
    {
        records = BillDetailRecordCodec::splitRecords(content);
    }
    catch (const std::invalid_argument& error)
    {
        throw std::invalid_argument(filePath_ + ": " + error.what());
    }

    vector<BillLine> lines;
    for (std::size_t i = 0; i < records.size(); ++i)
    {
        try
        {
            lines.push_back(BillDetailRecordCodec::parse(records[i]));
        }
        catch (const std::invalid_argument& error)
        {
            throw std::invalid_argument(filePath_ + ", record " + std::to_string(i + 1) +
                                        ": " + error.what());
        }
    }

    return lines;
}

void FileBillDetailRepository::saveAll(const vector<BillLine>& lines)
{
    recoverInterruptedSave();

    const string temp = tempPath();
    const string backup = backupPath();

    bool written = false;
    {
        std::ofstream output(temp.c_str(), std::ios::trunc);
        if (!output)
            throw std::runtime_error("cannot create temporary file: " + temp);

        for (const BillLine& line : lines)
            output << BillDetailRecordCodec::format(line) << "\n";

        output.close();
        written = !output.fail();
    }
    if (!written)
    {
        removeFile(temp);
        throw std::runtime_error("error while writing temporary file: " + temp +
            " (the bill detail file was not changed)");
    }

    const bool hadOriginal = fileExists(filePath_);
    if (hadOriginal)
    {
        removeFile(backup);
        if (!renameFile(filePath_, backup))
        {
            removeFile(temp);
            throw std::runtime_error("cannot create backup " + backup +
                " (the bill detail file was not changed)");
        }
    }

    if (!renameFile(temp, filePath_))
    {
        string message = "cannot put the new bill detail file in place: " + filePath_;
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
