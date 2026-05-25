#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "data/events/file_log_event.h"
#include "sinks/sink.h"

#include <chrono>
#include <config.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <list>
#include <muondetector_structs.h>
#include <mutex>
#include <string>

class FileSink : public Sink {
  public:
    struct FileInfo {
        std::filesystem::path path;
        std::uintmax_t size;
        std::filesystem::file_time_type modified;
    };
    FileSink(std::uint32_t fileSizeMB = 500);
    ~FileSink();
    std::string getCurrentDataFileName() const;
    std::string getCurrentLogFileName() const;

    auto dataFileInfo() const -> FileInfo;
    auto logFileInfo() const -> FileInfo;

    std::chrono::seconds currentLogAge();
    std::chrono::seconds logRotatePeriod() const { return m_logrotate_period; }
    LogInfoStruct getInfo();
    LogInfoStruct::status_t getStatus();

    void onRotationRemind();
    void handle(const UbxTimeMarkStruct& tm);
    void handle(const FileLogEvent& event);

  private:
    static auto generateNextDailyTime(std::chrono::system_clock::duration offset)
        -> std::chrono::system_clock::time_point;
    std::ofstream dataFile;
    std::ofstream logFile;

    std::filesystem::path configPath;
    std::filesystem::path configFilePath;
    std::filesystem::path dataFolderPath;
    std::filesystem::path currentWorkingFilePath;
    std::filesystem::path currentWorkingLogPath;

    std::filesystem::perms defaultPermissions =
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read | std::filesystem::perms::group_write |
        std::filesystem::perms::others_read;
    std::list<std::string> m_filename_list;

    auto makeBasePaths() -> std::pair<std::filesystem::path, std::filesystem::path>;
    auto resolveUniquePaths(std::filesystem::path baseData, std::filesystem::path baseLog)
        -> std::pair<std::filesystem::path, std::filesystem::path>;
    auto rotateFiles()
        -> bool; //!< closes the old files and opens a new data/log file pair, changing the
    //                     //!< log config to the new files
    auto openFilesAtPaths(bool writeHeader) -> bool;
    auto openFiles(bool writeHeader = false)
        -> bool; //!< reads the config file and opens the correct data file to write into
    bool removeOldFiles();
    auto readConfigFile() -> bool;
    auto writeConfigFile() -> bool;
    void closeFiles();
    void start();
    void writeToDataFile(const std::string& data); //!< writes data to the file opened in "dataFile"
    void
    writeToLogFile(const std::string& log); //!< writes log data to the file opened in "logFile"
    void setLogRotatePeriod(std::chrono::seconds period) { m_logrotate_period = period; }
    mutable std::mutex m_mutex;
    std::string createFileName(); //!< creates a fileName based on date and time
    std::uint32_t m_fileSizeMB;   //!< max file size limit in MB
    std::chrono::seconds dailyUploadTime;
    std::chrono::system_clock::time_point lastRotationDateTime;
    std::chrono::system_clock::time_point nextRotationTime;
    std::chrono::seconds m_logrotate_period{MuonPi::Settings::log.rotate_period};
};

#endif // FILEHANDLER_H
