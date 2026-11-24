#include "CategoryRecordCodec.h"
#include "RecordSplitter.h"
#include <stdexcept>
#include <vector>

using std::string;
using std::vector;

ProductCategory CategoryRecordCodec::parse(const string& record)
{
    const vector<string> t = RecordSplitter::tokens(record);
    if (t.size() < 3)
        throw std::invalid_argument("expected at least 3 fields but found " +
            std::to_string(t.size()));

    string name;
    for (size_t i = 1; i + 1 < t.size(); i++)
    {
        if (!name.empty()) name += ' ';
        name += t[i];
    }
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == '_') name[i] = ' ';

    return ProductCategory(t[0], name, t.back());
}

string CategoryRecordCodec::format(const ProductCategory& category)
{
    string name = category.name();
    for (size_t i = 0; i < name.size(); i++)
        if (name[i] == ' ') name[i] = '_';

    return RecordSplitter::endRecord(category.id() + " " + name + " " + category.acronym());
}