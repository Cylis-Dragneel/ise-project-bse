#pragma once
#ifndef RECORD_SPLITTER_H
#define RECORD_SPLITTER_H

#include <string>
#include <vector>

namespace RecordSplitter
{
    std::vector<std::string> split(const std::string& fileContent);
    std::vector<std::string> tokens(const std::string& record);
    std::string endRecord(const std::string& body);
}

#endif