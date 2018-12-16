#include <filehandler.h>
#include <QTextStream>
#include <QDebug>
#include <QProcess>
#include <QNetworkInterface>
#include <QTimer>
#include <QDir>

FileHandler::FileHandler(QString dataFolder, QString configFileName, quint32 fileSizeMB, QObject *parent)
    : QObject(parent)
{
    if (configFileName != ""){
        dataConfigFileName = configFileName;
    }
    if (dataFolder != ""){
        dataFolderPath = dataFolder;
    }
    QDir temp;
    if (!temp.exists(dataFolderPath)){
        temp.mkpath(dataFolderPath);
    }
    QDir pathDir(dataFolderPath);
    if (!pathDir.exists()){
        qDebug() << "could not create folder " << dataFolderPath;
    }
    QTimer *uploadReminder = new QTimer(this);
    uploadReminder->setInterval(60*1000*15); // every 15 minutes or so
    uploadReminder->setSingleShot(false);
    connect(uploadReminder, &QTimer::timeout, this, &FileHandler::onUploadRemind);
    fileSize = fileSizeMB;
    openDataFile();
    qDebug() << "initialisation of filehandler ok";
    switchToNewDataFile("testFile");
    qDebug() << "switch file does not crash";
    writeToDataFile("this is a test file, please ignore this message");
    qDebug() << "writing to data file seems to work";
    switchToNewDataFile();
    qDebug() << "miep";
    if (uploadDataFile("testFile")){

    }
    qDebug() << "upload not crashing";
    uploadReminder->start();
}

bool FileHandler::readFileInformation(){
    QFile *configFile = new QFile(dataConfigFileName);
    if (!configFile->open(QIODevice::ReadWrite)){
        qDebug() << "open "<<dataConfigFileName<<" failed";
        return false;
    }
    QTextStream *configStream = new QTextStream(configFile);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // here comes all information how to read the configuration and pick the currently active file
    // for now it's all the filenames that are not sent, then a line full of '+' then all the files already sent
    // first file in the queue is the current working file
    while (!(configStream->atEnd())){
        QString fileName;
        *configStream >> fileName;
        files.push_back(fileName);
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    delete configStream;
    configStream = nullptr;
    configFile->close();
    delete configFile;
    configFile = nullptr;
    return true;
}

// DATA SAVING
bool FileHandler::openDataFile(){
    if (dataFile != nullptr){
        return false;
    }
    if (files.empty()){
        readFileInformation();
    }
    if (files.empty()){
        files.push_back(createFileName());
    }
    QString fileName = files.first(); // current working file name is first in the queue
    dataFile = new QFile(fileName);
    if (!dataFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
        qDebug() << "file open failed in 'ReadWrite' mode at location " << fileName;
        return false;
    }
    return true;
}

bool FileHandler::writeToDataFile(QString data){
    if (dataFile == nullptr){
        return false;
    }
    QTextStream out(dataFile);
    out << data;
    return true;
}

bool FileHandler::uploadDataFile(QString fileName){
    QProcess lftpProcess(this);
    lftpProcess.setProgram("lftp");
    QStringList arguments({"-p 35221",
                          "-u <user>,<pass>",
                          "balu.physik.uni-giessen.de:/cosmicshower/<mac-address>",
                          QString("-e put "+fileName)});
    lftpProcess.setArguments(arguments);
    lftpProcess.start();
    const int timeout = 300000;
    if (!lftpProcess.waitForFinished(timeout)){
        qDebug() << "lftp timeout after " << timeout/1000 << "s";
        return false;
    }
    if (lftpProcess.exitStatus()==-1){
        qDebug() << "lftp returned exit status -1";
        return false;
    }
    return true;
}

void FileHandler::closeDataFile(){
    dataFile-> close();
    delete dataFile;
    dataFile = nullptr;
}

bool FileHandler::switchToNewDataFile(QString fileName){
    if (dataFile == nullptr){
        return false;
    }
    if (files.empty()){
        readFileInformation();
    }
    closeDataFile();
    if(fileName == ""){
        fileName = createFileName();
    }
    files.push_front(fileName);
    if (!openDataFile()){
        return false;
    }
    QFile *configFile = new QFile(dataConfigFileName);
    configFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
    QTextStream *configStream = new QTextStream(configFile);
    for (auto file : files){
        *configStream << file;
    }
    delete configStream;
    configStream = nullptr;
    delete configFile;
    configFile = nullptr;
    return true;
}

QString FileHandler::createFileName(){
    // creates a fileName based on date time and mac address
    QNetworkInterface interface;
    QString macAddress = interface.hardwareAddress();
    QDateTime dateTime = QDateTime::currentDateTimeUtc();
    QString fileName = (macAddress+"-"+dateTime.toString("yyyyMMdd-hh:mm:ss"));
    fileName = dataFolderPath+"/"+fileName;
    return fileName;
}

void FileHandler::onUploadRemind(){
    if (dataFile==nullptr){
        return;
    }
    QDateTime todaysRegularUploadTime = QDateTime(QDate::currentDate(),dailyUploadTime,Qt::TimeSpec::UTC);
    if (dataFile->size()>(1024*1024*fileSize)||lastUploadDateTime<todaysRegularUploadTime){
        QString oldFileName = dataFile->fileName();
        switchToNewDataFile();
        uploadDataFile(oldFileName);
    }
}
