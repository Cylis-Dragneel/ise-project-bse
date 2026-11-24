#include "FileProductRepository.h"
#include "ProductRecordCodec.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cstdio>

FileProductRepository::FileProductRepository(const std::string& path)
    : path_(path),
    tempPath_(path + ".tmp"),
    backupPath_(path + ".bak") {
    recoverIfNeeded();
}

const std::string& FileProductRepository::path() const {
    return path_;
}

bool FileProductRepository::fileExists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

bool FileProductRepository::renameFile(const std::string& from, const std::string& to) {
    return std::rename(from.c_str(), to.c_str()) == 0;
}

bool FileProductRepository::removeFile(const std::string& path) {
    return std::remove(path.c_str()) == 0;
}

void FileProductRepository::recoverIfNeeded() {
    if (fileExists(tempPath_) && !fileExists(path_) && fileExists(backupPath_)) {
        renameFile(backupPath_, path_);
    }
    if (fileExists(backupPath_)) {
        removeFile(backupPath_);
    }
    if (fileExists(tempPath_)) {
        removeFile(tempPath_);
    }
}

std::vector<Product> FileProductRepository::loadAll() {
    recoverIfNeeded();

    std::ifstream file(path_.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path_);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Strip UTF-8 BOM if present
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3);
    }

    std::vector<Product> products;
    std::stringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        // Strip Windows \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Skip empty lines
        if (line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        // M5 codec takes only the string
        products.push_back(ProductRecordCodec::parse(line));
    }

    return products;
}

void FileProductRepository::saveAll(const std::vector<Product>& products) {
    std::stringstream content;
    for (const Product& p : products) {
        // M5 codec formats one product at a time
        content << ProductRecordCodec::format(p) << "\n";
    }

    // 1. Write to temp file
    std::ofstream out(tempPath_.c_str());
    if (!out.is_open()) {
        throw std::runtime_error("Could not write temp file: " + tempPath_);
    }
    out << content.str();
    out.close();
    if (out.fail()) {
        removeFile(tempPath_);
        throw std::runtime_error("Failed writing to temp file: " + tempPath_);
    }

    // 2. Rename original to backup
    if (fileExists(path_)) {
        if (!renameFile(path_, backupPath_)) {
            removeFile(tempPath_);
            throw std::runtime_error("Failed to backup original file: " + path_);
        }
    }

    // 3. Rename temp to real
    if (!renameFile(tempPath_, path_)) {
        // Restore backup
        if (fileExists(backupPath_)) {
            renameFile(backupPath_, path_);
        }
        removeFile(tempPath_);
        throw std::runtime_error("Failed to save new file. Original restored.");
    }

    // 4. Success: delete backup
    if (fileExists(backupPath_)) {
        removeFile(backupPath_);
    }
}