#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QFile>
#include <QObject>
#include <QQueue>
#include <QDateTime>
#include <QStandardPaths>

class FileHandler : public QObject
{
    Q_OBJECT

public:
    FileHandler(QString userName, QString passWord, QString dataPath = "", quint32 fileSizeMB = 500, QObject *parent = nullptr);

public slots:
    void writeToDataFile(QString data); // writes data to the file opened in "dataFile"

private:
    // save and send data everyday
    QFile *dataFile = nullptr; // the file currently written to.
    QString mainDataFolderName = ".muondetector-daemon/";
    QString hashedMacAddress;
    QString configFilePath;
    QString loginDataFilePath;
    QString configPath;
    QString dataFolderPath;
    QString currentWorkingFilePath;
    QString username;
    QString password;
    QStringList notUploadedFilesNames;
    bool saveLoginData(QString username, QString password);
    bool readLoginData();
    bool openDataFile(); // reads the config file and opens the correct data file to write to
    bool readFileInformation();
    bool uploadDataFile(QString fileName); // sends a data file with some filename via lftp script to the server
    bool uploadRecentDataFiles();
    bool switchToNewDataFile(QString fileName = ""); // closes the old file and opens a new one, changing "dataConfig.conf" to the new file
    void closeDataFile();
    QString getMacAddress();
    QByteArray getMacAddressByteArray();
    QString createFileName(); // creates a fileName based on date time and mac address
    quint32 fileSize; // in MB
    QDateTime lastUploadDateTime;
    QTime dailyUploadTime;

private slots:
    void onUploadRemind();
};

#endif // FILEHANDLER_H
