#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QQueue>
#include <QDateTime>
#include <QStandardPaths>
#include <logparameter.h>
#include <QMap>
#include <QVector>
#include <async_client.h>
#include <connect_options.h>

class callback : public virtual mqtt::callback{
public:
    void connection_lost(const std::string& cause) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override;
};

class action_listener : public QObject, public virtual mqtt::iaction_listener{
    Q_OBJECT
protected:
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
};

class delivery_action_listener : public action_listener{
    Q_OBJECT
signals:
    void done(bool status);
public:
    delivery_action_listener() : done_(false) {}
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
    bool is_done() const { return done_; }
private:
    std::atomic<bool> done_;
};

class FileHandler : public QObject
{
    Q_OBJECT

public:
    FileHandler(const QString& userName, const QString& passWord, quint32 fileSizeMB = 500, QObject *parent = nullptr);

    QString getCurrentDataFileName() const;
    QString getCurrentLogFileName() const;
    QFileInfo dataFileInfo() const;
    QFileInfo logFileInfo() const;
    qint64 currentLogAge();
    bool mqttConnectionStatus();

signals:
	void logIntervalSignal();
    void mqttConnectionStatusChanged();

public slots:
    void start();
    void writeToDataFile(const QString& data); // writes data to the file opened in "dataFile"
    void onReceivedLogParameter(const LogParameter& log);

private slots:
    void onUploadRemind();
    void onLogRemind();

private:
    void writeToLogFile(const QString& log); // writes log information to logFile
    // save and send data everyday
    QMap<QString, QVector<LogParameter> > logData;
    QFile *dataFile = nullptr; // the file date is currently written to. (timestamps)
    QFile *logFile = nullptr; // the file log information is written to.
    QString hashedMacAddress;
    QString configFilePath;
    QString loginDataFilePath;
    QString configPath;
    QString dataFolderPath;
    QString currentWorkingFilePath;
    QString currentWorkingLogPath;
    QString mqttAddress = "116.202.96.181:1883";
    QString username;
    QString password;
    QFlags<QFileDevice::Permission> defaultPermissions = QFileDevice::WriteOwner|QFileDevice::WriteUser|
            QFileDevice::WriteGroup|QFileDevice::ReadOwner|QFileDevice::ReadUser|
            QFileDevice::ReadGroup|QFileDevice::ReadOther;
    QStringList notUploadedFilesNames;
    bool saveLoginData(QString username, QString password);
    bool readLoginData();
    bool openFiles(bool writeHeader = false); // reads the config file and opens the correct data file to write to
    bool readFileInformation();
    bool uploadDataFile(QString fileName); // sends a data file with some filename via lftp script to the server
    bool uploadRecentDataFiles();
    bool switchFiles(QString fileName = ""); // closes the old file and opens a new one, changing "dataConfig.conf" to the new file
    bool writeConfigFile();
    bool mqttConnect();
    void mqttDisconnect();
    bool mqttSendMessage(QString message);
    mqtt::async_client *mqttClient = nullptr;
    void closeFiles();
    QString createFileName(); // creates a fileName based on date time and mac address
    quint32 fileSize; // in MB
    QDateTime lastUploadDateTime;
    QTime dailyUploadTime;
    bool onceLogFlag=true;
    float temperature = 0.0;
    quint8 pcaChannel = 0;
    bool _mqttConnectionStatus = false;
};

#endif // FILEHANDLER_H
