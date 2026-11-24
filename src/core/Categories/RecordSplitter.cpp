#include "RecordSplitter.h"
#include <cctype>
#include <sstream>
#include <stdexcept>

using std::string;
using std::vector;

namespace
{
    bool isBlank(const string& s)
    {
        for (size_t i = 0; i < s.size(); i++)
            if (!std::isspace(static_cast<unsigned char>(s[i]))) return false;
        return true;
    }
}

namespace RecordSplitter
{
    vector<string> split(const string& content)
    {
        size_t lastMarker = content.rfind('#');
        string tail = (lastMarker == string::npos) ? content : content.substr(lastMarker + 1);
        if (!isBlank(tail))
            throw std::invalid_argument("the last record is missing the '#' end-of-line marker");

        vector<string> records;
        size_t start = 0;
        while (true)
        {
            size_t end = content.find('#', start);
            if (end == string::npos) break;
            string record = content.substr(start, end - start);
            if (!isBlank(record)) records.push_back(record);
            start = end + 1;
        }
        return records;
    }

    vector<string> tokens(const string& record)
    {
        vector<string> result;
        std::istringstream in(record);
        string token;
        while (in >> token) result.push_back(token);
        return result;
    }

    string endRecord(const string& body)
    {
        return body + " #";
    }
}