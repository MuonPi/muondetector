#ifndef FILEHANDLER_H
#define FILEHANDLER_H
#include <QFile>
#include <QObject>
#include <QQueue>


class FileHandler : public QObject
{
    Q_OBJECT

public:
    FileHandler(QString configFileName = "", QObject *parent = nullptr);
    bool writeToDataFile(QString data); // writes data to the file opened in "dataFile"

private:
    // save and send data everyday
    QFile *dataFile = nullptr; // the file currently written to.
    QString dataConfigFileName = "dataFileInformation.conf";
    QQueue<QString> files; // first file name is always the current working file name
    bool openDataFile(); // reads the config file and opens the correct data file to write to
    bool readFileInformation();
    bool uploadDataFile(QString fileName); // sends a data file with some filename via lftp script to the server
    bool switchToNewDataFile(QString fileName = ""); // closes the old file and opens a new one, changing "dataConfig.conf" to the new file
    void closeDataFile();
    QString createFileName(); // creates a fileName based on date time and mac address
};

#endif // FILEHANDLER_H
