#include <filehandler.h>
#include <QTextStream>
#include <QDebug>
#include <QProcess>
#include <QNetworkInterface>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QTimer>
#include <QDir>

FileHandler::FileHandler(QString dataFolder, QString configFileName, quint32 fileSizeMB, QObject *parent)
    : QObject(parent)
{
    if (configFileName != ""){
        dataConfigFileName = configFileName;
    }
    if (dataFolder != ""){
        configFolderName = dataFolder;
    }
    QDir temp;
    QString fullPath = temp.homePath()+"/"+configFolderName;
    muondetectorConfigPath = fullPath;
    fullPath+="data";
    if (!temp.exists(fullPath)){
        temp.mkpath(fullPath);
        if (!temp.exists(fullPath)){
            qDebug() << "could not create folder " << fullPath;
        }
    }
    QTimer *uploadReminder = new QTimer(this);
    uploadReminder->setInterval(60*1000*15); // every 15 minutes or so
    uploadReminder->setSingleShot(false);
    connect(uploadReminder, &QTimer::timeout, this, &FileHandler::onUploadRemind);
    fileSize = fileSizeMB;
    openDataFile();
    QString testFilePath = muondetectorConfigPath+"data/"+"testFile";
    switchToNewDataFile(testFilePath);
    writeToDataFile("this is a test file, please ignore this message\n");
    switchToNewDataFile();
    if (uploadDataFile(testFilePath)){
        qDebug() << "upload not crashing";
    }
    uploadReminder->start();
}

bool FileHandler::readFileInformation(){
    if (muondetectorConfigPath==""){
        qDebug() << "folder path not existent";
        return false;
    }
    QString configFilePath = muondetectorConfigPath+dataConfigFileName;
    QFile *configFile = new QFile(configFilePath);
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
        qDebug() << lftpProcess.readAll();
        qDebug() << "lftp timeout after " << timeout/1000 << "s";
        return false;
    }
    qDebug() << lftpProcess.readAll();
    if (lftpProcess.exitStatus()==-1){
        qDebug() << "lftp returned exit status -1";
        return false;
    }
    return true;
}

void FileHandler::closeDataFile(){
    dataFile->close();
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
    QString configFilePath = muondetectorConfigPath+dataConfigFileName;
    QFile *configFile = new QFile(configFilePath);
    configFile->open(QIODevice::ReadWrite | QIODevice::Truncate);
    configFile->resize(0);
    QTextStream *configStream = new QTextStream(configFile);
    for (auto file : files){
        *configStream << file << "\n";
    }
    configFile->close();
    delete configStream;
    configStream = nullptr;
    delete configFile;
    configFile = nullptr;
    return true;
}

QString FileHandler::getMacAddress(){
    QNetworkConfiguration nc;
    QNetworkConfigurationManager ncm;
    QList<QNetworkConfiguration> configsForEth,configsForWLAN,allConfigs;
    // getting all the configs we can
    foreach (nc,ncm.allConfigurations(QNetworkConfiguration::Active))
    {
        if(nc.type() == QNetworkConfiguration::InternetAccessPoint)
        {
            // selecting the bearer type here
            if(nc.bearerType() == QNetworkConfiguration::BearerWLAN)
            {
                configsForWLAN.append(nc);
            }
            if(nc.bearerType() == QNetworkConfiguration::BearerEthernet)
            {
                configsForEth.append(nc);
            }
        }
    }
    // further in the code WLAN's and Eth's were treated differently
    allConfigs.append(configsForWLAN);
    allConfigs.append(configsForEth);
    QString MAC;
    foreach(nc,allConfigs)
    {
        QNetworkSession networkSession(nc);
        QNetworkInterface netInterface = networkSession.interface();
        // these last two conditions are for omiting the virtual machines' MAC
        // works pretty good since no one changes their adapter name
        if(!(netInterface.flags() & QNetworkInterface::IsLoopBack)
                && !netInterface.humanReadableName().toLower().contains("vmware")
                && !netInterface.humanReadableName().toLower().contains("virtual"))
        {
            MAC = QString(netInterface.hardwareAddress());
            break;
        }
    }
    return MAC;
}

QString FileHandler::createFileName(){
    // creates a fileName based on date time and mac address
    if (muondetectorConfigPath==""){
        qDebug() << "could not open data folder";
        return "";
    }
    QString macAddress = "macaddress";
    QDateTime dateTime = QDateTime::currentDateTimeUtc();
    QString fileName = (macAddress+"_"+dateTime.toString("yyyy-MM-dd_hh:mm:ss"));
    fileName = muondetectorConfigPath+"data/"+fileName;
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
