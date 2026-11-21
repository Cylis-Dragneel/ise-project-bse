#include "FileBillRepository.h"

#include "BillRecordCodec.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

using std::string;
using std::vector;

FileBillRepository::FileBillRepository(const string& filePath)
    : filePath_(filePath)
{
}

vector<BillLine> FileBillRepository::loadAll()
{
    std::ifstream input(filePath_.c_str());
    if (!input)
    {
        // Bill.txt does not exist until the first bill is generated.
        return vector<BillLine>();
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad())
    {
        throw std::runtime_error("error while reading bill file: " + filePath_);
    }

    string content = buffer.str();
    if (content.compare(0, 3, "\xEF\xBB\xBF") == 0) // UTF-8 BOM added by some editors
    {
        content.erase(0, 3);
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

    // If the existing file does not end with a newline, start on a fresh line
    // so the previous record is not glued to the first new one.
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
