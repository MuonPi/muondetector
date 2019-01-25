#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QFile>
#include <QObject>
#include <QQueue>
#include <QDateTime>
#include <QStandardPaths>
#include <logparameter.h>
#include <QMap>

class FileHandler : public QObject
{
    Q_OBJECT

public:
    FileHandler(QString userName, QString passWord, QString dataPath = "", quint32 fileSizeMB = 500, QObject *parent = nullptr);
    float temperature = 0.0;
    quint8 pcaChannel = 0;

public slots:
    void writeToDataFile(const QString& data); // writes data to the file opened in "dataFile"
    void onReceivedLogParameter(LogParameter log);

private:
    void writeToLogFile(QString log); // writes log information to logFile
    // save and send data everyday
    QMap<QString, LogParameter> logData;
    QFile *dataFile = nullptr; // the file date is currently written to. (timestamps)
    QFile *logFile = nullptr; // the file log information is written to.
    QString mainDataFolderName = ".muondetector-daemon/";
    QString hashedMacAddress;
    QString configFilePath;
    QString loginDataFilePath;
    QString configPath;
    QString dataFolderPath;
    QString currentWorkingFilePath;
    QString currentWorkingLogPath;
    QString username;
    QString password;
    QStringList notUploadedFilesNames;
    bool saveLoginData(QString username, QString password);
    bool readLoginData();
    bool openFiles(bool writeHeader = false); // reads the config file and opens the correct data file to write to
    bool readFileInformation();
    bool uploadDataFile(QString fileName); // sends a data file with some filename via lftp script to the server
    bool uploadRecentDataFiles();
    bool switchFiles(QString fileName = ""); // closes the old file and opens a new one, changing "dataConfig.conf" to the new file
    bool writeConfigFile();
    void closeFiles();
    QString createFileName(); // creates a fileName based on date time and mac address
    quint32 fileSize; // in MB
    QDateTime lastUploadDateTime;
    QTime dailyUploadTime;

private slots:
    void onUploadRemind();
    void onLogRemind();
};

#endif // FILEHANDLER_H
