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

FileBillRepository::FileBillRepository(const string& filePath)
    : filePath_(filePath)
{
}

vector<Bill> FileBillRepository::loadAll()
{
    bool exists = false;
    const string content = readWholeFile(filePath_, exists);
    if (!exists)
    {
        return vector<Bill>();
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

    vector<Bill> bills;
    std::unordered_set<string> seenIds;
    for (std::size_t i = 0; i < records.size(); ++i)
    {
        try
        {
            Bill bill = BillRecordCodec::parse(records[i]);
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

void FileBillRepository::saveAll(const vector<Bill>& bills)
{
    std::unordered_set<string> seenIds;
    vector<string> formatted;
    formatted.reserve(bills.size());

    for (const Bill& bill : bills)
    {
        if (!seenIds.insert(bill.billId).second)
        {
            throw std::invalid_argument("duplicate bill id on save: '" + bill.billId + "'");
        }
        formatted.push_back(BillRecordCodec::format(bill));
    }

    atomicWriteAll(filePath_, formatted);
}

void FileBillRepository::append(const Bill& bill)
{
    vector<Bill> existing = loadAll();
    for (const Bill& item : existing)
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

vector<BillLine> FileBillDetailRepository::loadAll()
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
    vector<string> formatted;
    formatted.reserve(lines.size());
    for (const BillLine& line : lines)
    {
        formatted.push_back(BillDetailRecordCodec::format(line));
    }
    atomicWriteAll(filePath_, formatted);
}

void FileBillDetailRepository::append(const vector<BillLine>& lines)
{
    if (lines.empty())
    {
        return;
    }
    vector<BillLine> existing = loadAll();
    existing.insert(existing.end(), lines.begin(), lines.end());
    saveAll(existing);
}