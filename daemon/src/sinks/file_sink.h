#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include "logparameter.h"

#include <config.h>
#include <cstdint>
#include <muondetector_structs.h>
#include <string>

class FileSink {
  public:
    FileSink(const std::string& username, const std::string& password,
             std::uint32_t fileSizeMB = 500);
    std::string getCurrentDataFileName() const;
    std::string getCurrentLogFileName() const;
    QFileInfo dataFileInfo() const;
    QFileInfo logFileInfo() const;
    std::chrono::seconds currentLogAge();
    std::chrono::seconds logRotatePeriod() const { return m_logrotate_period; }
    LogInfoStruct getInfo();
    LogInfoStruct::status_t getStatus();

    // signals:
    //     void logIntervalSignal();
    //     void mqttConnect(std::string username, std::string password);
    //     void logRotateSignal();

    void start();
    void writeToDataFile(const std::string& data); //!< writes data to the file opened in "dataFile"
    void
    writeToLogFile(const std::string& log); //!< writes log data to the file opened in "logFile"
    void setLogRotatePeriod(std::chrono::seconds period) { m_logrotate_period = period; }
    void onUploadRemind();

  private:
    QFile* dataFile = nullptr; //!< pointer to the file the events are currently written to
    QFile* logFile = nullptr;  //!< pointer to the file the log information is written to
    std::string hashedMacAddress;
    std::string configFilePath;
    std::string loginDataFilePath;
    std::string configPath;
    std::string dataFolderPath;
    std::string currentWorkingFilePath;
    std::string currentWorkingLogPath;
    std::string m_username{};
    std::string m_password{};
    QFlags<QFileDevice::Permission> defaultPermissions =
        QFileDevice::WriteOwner | QFileDevice::WriteUser | QFileDevice::WriteGroup |
        QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ReadGroup |
        QFileDevice::ReadOther;
    std::stringList m_filename_list;
    bool saveLoginData(std::string username, std::string password);
    bool readLoginData();
    bool
    openFiles(bool writeHeader =
                  false); //!< reads the config file and opens the correct data file to write into
    bool readFileInformation();
    bool rotateFiles(); //!< closes the old files and opens a new data/log file pair, changing the
                        //!< log config to the new files
    bool removeOldFiles();
    bool writeConfigFile();
    void closeFiles();
    std::string createFileName(); //!< creates a fileName based on date and time
    std::uint32_t fileSize;       //!< max file size limit in MB
    QDateTime lastUploadDateTime;
    QTime dailyUploadTime;
    std::chrono::seconds m_logrotate_period{MuonPi::Settings::log.rotate_period};
};

#endif // FILEHANDLER_H
