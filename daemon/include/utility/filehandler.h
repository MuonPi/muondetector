#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QStandardPaths>
#include <QVector>
#include <config.h>
#include <muondetector_structs.h>
#include "logparameter.h"

class FileHandler : public QObject {
    Q_OBJECT

public:
    FileHandler(const QString& username, const QString& password, quint32 fileSizeMB = 500, QObject* parent = nullptr);
    QString getCurrentDataFileName() const;
    QString getCurrentLogFileName() const;
    QFileInfo dataFileInfo() const;
    QFileInfo logFileInfo() const;
    std::chrono::seconds currentLogAge();
    std::chrono::seconds logRotatePeriod() const { return m_logrotate_period; }
    LogInfoStruct getInfo();
    LogInfoStruct::status_t getStatus();

signals:
    void logIntervalSignal();
    void mqttConnect(QString username, QString password);
    void logRotateSignal();

public slots:
    void start();
    void writeToDataFile(const QString& data); //!< writes data to the file opened in "dataFile"
    void writeToLogFile(const QString& log); //!< writes log data to the file opened in "logFile"
    void setLogRotatePeriod(std::chrono::seconds period) { m_logrotate_period = period; }

private slots:
    void onUploadRemind();

private:
    QFile* dataFile = nullptr; //!< pointer to the file the events are currently written to
    QFile* logFile = nullptr; //!< pointer to the file the log information is written to
    QString hashedMacAddress;
    QString configFilePath;
    QString loginDataFilePath;
    QString configPath;
    QString dataFolderPath;
    QString currentWorkingFilePath;
    QString currentWorkingLogPath;
    QString m_username {};
    QString m_password {};
    QFlags<QFileDevice::Permission> defaultPermissions = QFileDevice::WriteOwner | QFileDevice::WriteUser | QFileDevice::WriteGroup | QFileDevice::ReadOwner | QFileDevice::ReadUser | QFileDevice::ReadGroup | QFileDevice::ReadOther;
    QStringList m_filename_list;
    bool saveLoginData(QString username, QString password);
    bool readLoginData();
    bool openFiles(bool writeHeader = false); //!< reads the config file and opens the correct data file to write into
    bool readFileInformation();
    bool rotateFiles(); //!< closes the old files and opens a new data/log file pair, changing the log config to the new files
    bool removeOldFiles();
    bool writeConfigFile();
    void closeFiles();
    QString createFileName(); //!< creates a fileName based on date and time
    quint32 fileSize; //!< max file size limit in MB
    QDateTime lastUploadDateTime;
    QTime dailyUploadTime;
    std::chrono::seconds m_logrotate_period { MuonPi::Settings::log.rotate_period };
};

#endif // FILEHANDLER_H
