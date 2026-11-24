#include "FileBillRepository.h"

#include "BillRecordCodec.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using std::string;
using std::vector;

namespace {

string readWholeFile(const string& path, bool& exists)
{
    std::ifstream input(path.c_str());
    if (!input)
    {
        exists = false;
        return string();
    }
    exists = true;

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad())
    {
        throw std::runtime_error("error while reading file: " + path);
    }

    string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        content.erase(0, 3);
    }
    return content;
}

void atomicWriteAll(const string& filePath, const vector<string>& formattedLines)
{
    const string tempPath = filePath + ".tmp";
    std::ofstream output(tempPath.c_str(), std::ios::out | std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error("cannot open temp file for writing: " + tempPath);
    }

    for (const string& line : formattedLines)
    {
        output << line << "\n";
    }

    output.flush();
    if (!output)
    {
        output.close();
        std::remove(tempPath.c_str());
        throw std::runtime_error("error while writing temp file: " + tempPath);
    }
    output.close();

    std::remove(filePath.c_str());
    if (std::rename(tempPath.c_str(), filePath.c_str()) != 0)
    {
        std::remove(tempPath.c_str());
        throw std::runtime_error("failed to rename temp file to: " + filePath);
    }
}

} // namespace

// ============================================================================
// INCREMENT 2 IMPLEMENTATION
// ============================================================================

FileBillRepository::FileBillRepository(const string& filePath)
    : filePath_(filePath)
{
}

vector<BillLine> FileBillRepository::loadAll()
{
    bool exists = false;
    const string content = readWholeFile(filePath_, exists);
    if (!exists)
    {
        return vector<BillLine>();
    }

    vector<string> records;
    try
    {
        records = BillRecordCodec::splitRecords(content);
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
            lines.push_back(BillRecordCodec::parse(records[i]));
        }
        catch (const std::invalid_argument& error)
        {
            throw std::invalid_argument(filePath_ + ", record " + std::to_string(i + 1) +
                                        ": " + error.what());
        }
    }

    return lines;
}

void FileBillRepository::append(const vector<BillLine>& lines)
{
    if (lines.empty())
    {
        return;
    }

    bool needsNewline = false;
    {
        std::ifstream input(filePath_.c_str(), std::ios::binary | std::ios::ate);
        if (input && input.tellg() > 0)
        {
            input.seekg(-1, std::ios::end);
            char last = '\0';
            input.get(last);
            needsNewline = (last != '\n');
        }
    }

    std::ofstream output(filePath_.c_str(), std::ios::app);
    if (!output)
    {
        throw std::runtime_error("cannot open bill file for appending: " + filePath_);
    }

    if (needsNewline)
    {
        output << '\n';
    }

    for (const BillLine& line : lines)
    {
        output << BillRecordCodec::format(line) << "\n";
    }

    output.flush();
    if (!output)
    {
        throw std::runtime_error("error while writing bill file: " + filePath_);
    }
}

// ============================================================================
// INCREMENT 3 IMPLEMENTATION
// ============================================================================

namespace inc3 {

FileBillHeaderRepository::FileBillHeaderRepository(const string& filePath)
    : filePath_(filePath)
{
}

vector<BillHeader> FileBillHeaderRepository::loadAll()
{
    bool exists = false;
    const string content = readWholeFile(filePath_, exists);
    if (!exists)
    {
        return vector<BillHeader>();
    }

    vector<string> records;
    try
    {
        records = BillHeaderCodec::splitRecords(content);
    }
    catch (const std::invalid_argument& error)
    {
        throw std::invalid_argument(filePath_ + ": " + error.what());
    }

    vector<BillHeader> bills;
    std::unordered_set<string> seenIds;
    for (std::size_t i = 0; i < records.size(); ++i)
    {
        try
        {
            BillHeader bill = BillHeaderCodec::parse(records[i]);
            if (!seenIds.insert(bill.billId).second)
            {
                throw std::invalid_argument("duplicate bill id '" + bill.billId + "'");
            }
            bills.push_back(std::move(bill));
        }
        catch (const std::invalid_argument& error)
        {
            throw std::invalid_argument(filePath_ + ", record " + std::to_string(i + 1) +
                                        ": " + error.what());
        }
    }
    return bills;
}

void FileBillHeaderRepository::saveAll(const vector<BillHeader>& bills)
{
    std::unordered_set<string> seenIds;
    vector<string> formatted;
    formatted.reserve(bills.size());

    for (const BillHeader& bill : bills)
    {
        if (!seenIds.insert(bill.billId).second)
        {
            throw std::invalid_argument("duplicate bill id on save: '" + bill.billId + "'");
        }
        formatted.push_back(BillHeaderCodec::format(bill));
    }

    atomicWriteAll(filePath_, formatted);
}

void FileBillHeaderRepository::append(const BillHeader& bill)
{
    vector<BillHeader> existing = loadAll();
    for (const BillHeader& item : existing)
    {
        if (item.billId == bill.billId)
        {
            throw std::invalid_argument("duplicate bill id on append: '" + bill.billId + "'");
        }
    }
    existing.push_back(bill);
    saveAll(existing);
}

FileBillDetailRepository::FileBillDetailRepository(const string& filePath)
    : filePath_(filePath)
{
}

vector<BillDetailLine> FileBillDetailRepository::loadAll()
{
    bool exists = false;
    const string content = readWholeFile(filePath_, exists);
    if (!exists)
    {
        return vector<BillDetailLine>();
    }

    vector<string> records;
    try
    {
        records = BillDetailCodec::splitRecords(content);
    }
    catch (const std::invalid_argument& error)
    {
        throw std::invalid_argument(filePath_ + ": " + error.what());
    }

    vector<BillDetailLine> lines;
    for (std::size_t i = 0; i < records.size(); ++i)
    {
        try
        {
            lines.push_back(BillDetailCodec::parse(records[i]));
        }
        catch (const std::invalid_argument& error)
        {
            throw std::invalid_argument(filePath_ + ", record " + std::to_string(i + 1) +
                                        ": " + error.what());
        }
    }
    return lines;
}

void FileBillDetailRepository::saveAll(const vector<BillDetailLine>& lines)
{
    vector<string> formatted;
    formatted.reserve(lines.size());
    for (const BillDetailLine& line : lines)
    {
        formatted.push_back(BillDetailCodec::format(line));
    }
    atomicWriteAll(filePath_, formatted);
}

void FileBillDetailRepository::append(const vector<BillDetailLine>& lines)
{
    if (lines.empty())
    {
        return;
    }
    vector<BillDetailLine> existing = loadAll();
    existing.insert(existing.end(), lines.begin(), lines.end());
    saveAll(existing);
}

} // namespace inc3
