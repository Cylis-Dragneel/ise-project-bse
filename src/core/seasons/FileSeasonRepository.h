#ifndef FILE_SEASON_REPOSITORY_H
#define FILE_SEASON_REPOSITORY_H

#include <string>

#include "ISeasonRepository.h"

// Stores seasons in a text file (Seasons.txt). Only does file I/O; the format
// itself is handled by SeasonRecordCodec.
//
// Saving uses the same crash-safe rename sequence as FileProductRepository.
class FileSeasonRepository : public ISeasonRepository
{
private:
    std::string filePath_;

    std::string tempPath() const { return filePath_ + ".tmp"; }
    std::string backupPath() const { return filePath_ + ".bak"; }

    void recoverInterruptedSave();

protected:
    // Virtual so tests can simulate a failing disk.
    virtual bool renameFile(const std::string& from, const std::string& to);
    virtual bool removeFile(const std::string& path);

public:
    explicit FileSeasonRepository(const std::string& filePath);

    // A missing file means "no seasons yet" and returns an empty vector.
    std::vector<Season> loadAll() override;

    // Throws std::runtime_error on failure; the previous file is restored.
    void saveAll(const std::vector<Season>& seasons) override;
};

#endif // FILE_SEASON_REPOSITORY_H
