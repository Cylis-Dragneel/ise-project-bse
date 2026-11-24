#pragma once
#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include "SafeFileStore.h"

namespace TestSupport
{
    inline void writeFile(const std::string& path, const std::string& text)
    {
        std::ofstream out(path.c_str(), std::ios::trunc);
        out << text;
    }

    inline std::string readFile(const std::string& path)
    {
        std::ifstream in(path.c_str());
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    inline bool fileExists(const std::string& path)
    {
        std::ifstream in(path.c_str());
        return in.good();
    }

    inline void removeAll(const std::string& path)
    {
        std::remove(path.c_str());
        std::remove((path + ".tmp").c_str());
        std::remove((path + ".bak").c_str());
    }

    class FailingStore : public SafeFileStore
    {
    private:
        std::set<int> failOn_;
        int calls_;

    public:
        FailingStore(const std::string& path, const std::set<int>& failOn)
            : SafeFileStore(path), failOn_(failOn), calls_(0) {
        }

    protected:
        bool renameFile(const std::string& from, const std::string& to) override
        {
            calls_++;
            if (failOn_.count(calls_)) return false;
            return SafeFileStore::renameFile(from, to);
        }
    };
}

#endif