#include <filehandler.h>
#include <QTextStream>
#include <QDebug>
#include <QProcess>
#include <QNetworkInterface>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QTimer>
#include <QDir>
#include <QCryptographicHash>
#include <QByteArray>
#include <QThread>
#include <unistd.h>
#include <sys/syscall.h>
#include <crypto++/aes.h>
#include <crypto++/modes.h>
#include <crypto++/filters.h>
#include <crypto++/osrng.h>
#include <crypto++/hex.h>
//#include <crypto++/sha3.h>
#include <crypto++/sha.h>
#include <config.h>
#include <QtGlobal>

using namespace CryptoPP;

const unsigned long int lftpUploadTimeout = MuonPi::Config::Upload::timeout/*MUONPI_UPLOAD_TIMEOUT_MS*/; // in msecs
const int uploadReminderInterval = MuonPi::Config::Upload::reminder/*MUONPI_UPLOAD_REMINDER_MINUTES*/; // in minutes
const int logReminderInterval = MuonPi::Config::Log::interval/*MUONPI_LOG_INTERVAL_MINUTES*/; // in minutes

static std::string SHA256HashString(std::string aString){
    std::string digest;
    CryptoPP::SHA256 hash;

    CryptoPP::StringSource foo(aString, true,
    new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(digest)));
    return digest;
}

static QString getMacAddress(){
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

static QByteArray getMacAddressByteArray(){
    //QString::fromLocal8Bit(temp.data()).toStdString();
    //return QByteArray(getMacAddress().toStdString().c_str());
//    return QByteArray::fromStdString(getMacAddress().toStdString());
    QString mac=getMacAddress();
    //qDebug()<<"MAC address: "<<mac;
    QByteArray byteArray;
    byteArray.append(mac);
    return byteArray;
    //return QByteArray::fromRawData(mac.toStdString().c_str(),mac.size());
}

static QString dateStringNow(){
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd_hh-mm-ss");
}

FileHandler::FileHandler(const QString& userName, const QString& passWord, quint32 fileSizeMB, QObject *parent)
    : QObject(parent)
{
    lastUploadDateTime = QDateTime(QDate::currentDate(),QTime(0,0,0,0),Qt::TimeSpec::UTC);
    dailyUploadTime = QTime(11,11,11,111);
    fileSize = fileSizeMB;
    QDir temp;
    QString fullPath = +"/var/muondetector/";
    hashedMacAddress = QString(QCryptographicHash::hash(getMacAddressByteArray(), QCryptographicHash::Sha224).toHex());
    //qDebug()<<"hashed MAC: "<<hashedMacAddress;
    fullPath += hashedMacAddress;
    configPath = fullPath+"/";
    configFilePath = fullPath + "/currentWorkingFileInformation.conf";
    loginDataFilePath = fullPath + "/loginData.save";
    fullPath += "/notUploadedFiles/";
    dataFolderPath = fullPath;
    if (!temp.exists(fullPath)){
        temp.mkpath(fullPath);
        if (!temp.exists(fullPath)){
            qDebug() << "could not create folder " << fullPath;
        }
    }
    QString uploadedFiles = configPath+"uploadedFiles/";
    if (!temp.exists(uploadedFiles)){
        temp.mkpath(uploadedFiles);
        if (!temp.exists(uploadedFiles)){
            qDebug() << "could not create folder " << uploadedFiles;
        }
    }
    bool rewrite_login = false;
    if (!readLoginData()){
        qDebug() << "could not read login data from file";
    }
    if (userName!=""||passWord!=""){
        username=userName;
        password=passWord;
        rewrite_login = true;
    }
    if (rewrite_login){
        if(!saveLoginData(userName,passWord)){
            qDebug() << "could not save login data";
        }
    }
}

QString FileHandler::getCurrentDataFileName() const {
    if (dataFile==nullptr) return "";
    QFileInfo fi( *dataFile );
    return fi.absoluteFilePath();
}

QString FileHandler::getCurrentLogFileName() const {
    if (logFile!=nullptr) return "";
    QFileInfo fi( *logFile );
    return fi.absoluteFilePath();
}

QFileInfo FileHandler::dataFileInfo() const {
    if (dataFile==nullptr) return QFileInfo();
    QFileInfo fi( *dataFile );
    return fi;
}

QFileInfo FileHandler::logFileInfo() const {
    if (logFile==nullptr) return QFileInfo();
    QFileInfo fi( *logFile );
    return fi;
}

// return current log file age in s
qint64 FileHandler::currentLogAge() {
    if (dataFile==nullptr) return -1;
    if (configFilePath=="") return -1;
    QDateTime now = QDateTime::currentDateTime();
    QFileInfo fi(configFilePath);
//    qDebug()<<"QT version is "<<QString::number(QT_VERSION,16);
#if QT_VERSION >= 0x050a00
//    qint64 difftime=-now.secsTo(dataFile->fileTime(QFileDevice::FileMetadataChangeTime));
    qint64 difftime=-now.secsTo(dataFile->fileTime(QFileDevice::FileBirthTime));
#else
    qint64 difftime=-now.secsTo(fi.created());
#endif

    return difftime;
}

void FileHandler::start(){
    // set upload reminder
    //qInfo() << this->thread()->objectName() << " thread id (pid): " << syscall(SYS_gettid);
    QTimer *uploadReminder = new QTimer(this);
    uploadReminder->setInterval(60*1000*uploadReminderInterval); // every 5 minutes or so
    uploadReminder->setSingleShot(false);
    connect(uploadReminder, &QTimer::timeout, this, &FileHandler::onUploadRemind);
    uploadReminder->start();
    // open files that are currently written
    openFiles();
    emit mqttConnect(username,password);
    qDebug() << "sent mqttConnect";
}

// SLOTS
void FileHandler::onUploadRemind(){
    if (dataFile==nullptr){
        return;
    }
    QDateTime todaysRegularUploadTime = QDateTime(QDate::currentDate(),dailyUploadTime,Qt::TimeSpec::UTC);
    if (dataFile->size()>(1024*1024*fileSize)){
        switchFiles();
    }
    if (lastUploadDateTime<todaysRegularUploadTime&&QDateTime::currentDateTimeUtc()>todaysRegularUploadTime){
        switchFiles();
        if (password.size() != 0 || username.size() != 0){
            //uploadRecentDataFiles(); // commented out because the upload server is not online
        }
        lastUploadDateTime = QDateTime::currentDateTimeUtc();
    }
}

// DATA SAVING
bool FileHandler::openFiles(bool writeHeader){
    if (dataFile != nullptr || logFile != nullptr){
        closeFiles();
    }
    if (currentWorkingFilePath==""||currentWorkingLogPath==""){
        readFileInformation();
    }
    if (currentWorkingFilePath==""||currentWorkingLogPath==""){
        writeHeader=true;
        QString fileNamePart = createFileName();
        currentWorkingFilePath = dataFolderPath+"data_"+fileNamePart;
        currentWorkingLogPath = dataFolderPath+"log_"+fileNamePart;
        while (notUploadedFilesNames.contains(QFileInfo(currentWorkingFilePath).fileName())){
            fileNamePart = createFileName();
            currentWorkingFilePath = dataFolderPath+"data_"+fileNamePart;
            currentWorkingLogPath = dataFolderPath+"log_"+fileNamePart;
        }
        writeConfigFile();
    }
    dataFile = new QFile(currentWorkingFilePath);
    dataFile->setPermissions(defaultPermissions);
    if (!dataFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
        qDebug() << "file open failed in 'ReadWrite' mode at location " << currentWorkingFilePath;
        // the following return statement induced wrong behavior:
    // in case the data file couldn't be opened, the log file QFile object would never be instantiated
    // this would prevent local logging
    //return false;
    }
    logFile = new QFile(currentWorkingLogPath);
    logFile->setPermissions(defaultPermissions);
    if (!logFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
        qDebug() << "file open failed in 'ReadWrite' mode at location " << currentWorkingLogPath;
        //return false;
    }
    if (!dataFile->isOpen() || !logFile->isOpen()) return false;
    // write header
    if (writeHeader){
        QTextStream dataOut(dataFile);
        dataOut << "#unix_timestamp_rising(s)  unix_timestamp_trailing(s)  time_accuracy(ns)  valid  timebase(0=gps,2=utc)  utc_available\n";
        QTextStream logOut(logFile);
        logOut << "#log parameters: time<YYYY-MM-DD_hh-mm-ss>  parname   value  unit\n";
        //onceLogFlag=true;
    }
    emit logRotateSignal();
    return true;
}

void FileHandler::closeFiles(){
    if (dataFile!=nullptr){
        if (dataFile->isOpen()){
            dataFile->close();
        }
        delete dataFile;
        dataFile = nullptr;
    }
    if (logFile != nullptr){
        if (logFile->isOpen()){
            logFile->close();
        }
        delete logFile;
        logFile = nullptr;
    }
}

bool FileHandler::writeConfigFile(){
    QFile configFile(configFilePath);
    if (!configFile.open(QIODevice::ReadWrite | QIODevice::Truncate)){
        qDebug() << "file open failed in 'ReadWrite' mode at location " << configFilePath;
        return false;
    }
    configFile.resize(0);
    QTextStream out(&configFile);
    out << currentWorkingFilePath << endl << currentWorkingLogPath << endl;
    return true;
}

bool FileHandler::switchFiles(QString fileName){
    closeFiles();
    currentWorkingFilePath = "data_"+fileName;
    currentWorkingLogPath = "log_"+fileName;
    if (fileName==""){
        QString fileNamePart = createFileName();
        currentWorkingFilePath = dataFolderPath+"data_"+fileNamePart;
        currentWorkingLogPath = dataFolderPath+"log_"+fileNamePart;
    }
    writeConfigFile();
    if (!openFiles(true)){
        closeFiles();
        return false;
    }
    return true;
}

bool FileHandler::readFileInformation(){
    QDir directory(dataFolderPath);
    notUploadedFilesNames = directory.entryList(QStringList() << "*.dat",QDir::Files);
    QFile configFile(configFilePath);
    if (!configFile.open(QIODevice::ReadWrite)){
        qDebug() << "file open failed in 'ReadWrite' mode at location " << configFilePath;
        return false;
    }
    QTextStream in(&configFile);
    if(configFile.size()==0){
        return true;
    }
    if (!in.atEnd()){
        currentWorkingFilePath = in.readLine();
    }
    if (!in.atEnd()){
        currentWorkingLogPath = in.readLine();
    }
    return true;
}

void FileHandler::writeToDataFile(const QString &data){
    if (dataFile == nullptr){
        return;
    }
    QTextStream out(dataFile);
    out << data << "\n";
}

void FileHandler::writeToLogFile(const QString& log) {
    if (logFile == nullptr){
        return;
    }
    QTextStream out(logFile);
    out << log << "\n";
}

QString FileHandler::createFileName(){
    // creates a fileName based on date time and mac address
    if (dataFolderPath==""){
        qDebug() << "could not open data folder";
        return "";
    }
    QString fileName = dateStringNow();
    fileName = fileName+".dat";
    return fileName;
}

// upload related stuff

bool FileHandler::uploadDataFile(QString fileName){
    char envName[] = "LFTP_PASSWORD";
    if (setenv(envName, password.toStdString().c_str(), 1)!=0){
        qDebug() << "setenv returned not 0";
    }
    QProcess lftpProcess(this);
    lftpProcess.setProgram("lftp");
    QStringList arguments;
    arguments << "--env-password";
    arguments << "-p" << QString::number(MuonPi::Config::Upload::port/*MUONPI_UPLOAD_PORT*/);
    arguments << "-u" << QString(username);
    arguments << MuonPi::Config::Upload::url/*MUONPI_UPLOAD_URL*/;
    arguments << "-e" << QString("mkdir "+hashedMacAddress+" ; cd "+hashedMacAddress+" && put "+fileName+" ; exit");
    lftpProcess.setArguments(arguments);
    //qDebug() << lftpProcess.arguments();
    lftpProcess.start();
    //qDebug() << "started upload of " << fileName << "user:" << username << ";pw:" << password <<";";
    if (!lftpProcess.waitForFinished(lftpUploadTimeout)){
        qDebug() << lftpProcess.readAllStandardOutput();
        qDebug() << lftpProcess.readAllStandardError();
        qDebug() << "lftp not installed or timed out after "<< lftpUploadTimeout/1000<< " s";
        system("unset LFTP_PASSWORD");
        return false;
    }
    //qDebug() << "standard output" << lftpProcess.readAllStandardOutput();
    //qDebug() << "standard error:" << lftpProcess.readAllStandardError();
    //qDebug() << "exit status:" << lftpProcess.exitStatus();
    if (lftpProcess.exitStatus()!=0){
        qDebug() << "lftp returned exit status other than 0";
        unsetenv(envName);
        return false;
    }
    unsetenv(envName);
    return true;
}

bool FileHandler::uploadRecentDataFiles(){
    readFileInformation();
    QDir lftpDir;
    lftpDir.setPath(lftpDir.homePath()+"/.lftp");
    if (!lftpDir.exists()){
        lftpDir.mkpath(".");
    }
    QFile lftp_rc_file(lftpDir.path()+"/rc");
    if (!lftp_rc_file.exists()||lftp_rc_file.size()<10){
        if (!lftp_rc_file.open(QIODevice::ReadWrite)){
            qDebug() << "could not open .lftp/rc file";
            return false;
        }
        lftp_rc_file.write("set ssl:verify-certificate no\n");
        lftp_rc_file.close();
    }
    for (auto &fileName : notUploadedFilesNames){
        QString filePath = dataFolderPath+fileName;
        //qDebug() << "checking for upload: " << filePath;
        if (filePath!=currentWorkingFilePath&&filePath!=currentWorkingLogPath){
            //qDebug() << "attempt to upload " << filePath;
            if (!uploadDataFile(filePath)){
                qDebug() << "failed to upload recent files";
                return false;
            }
            //qDebug() << "success uploading recent files";
            QFile::rename(filePath,QString(configPath+"uploadedFiles/"+fileName));
        }
    }
    return true;
}

// crypto related stuff

bool FileHandler::saveLoginData(QString username, QString password){
    QFile loginDataFile(loginDataFilePath);
    loginDataFile.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
    if(!loginDataFile.open(QIODevice::ReadWrite)){
        qDebug() << "could not open login data save file";
        return false;
    }
    loginDataFile.resize(0);

    AutoSeededRandomPool rnd;
    std::string plainText = QString(username+";"+password).toStdString();
    std::string keyText;
    std::string encrypted;

    keyText = SHA256HashString(getMacAddress().toStdString());
    SecByteBlock key((const byte*)keyText.data(),keyText.size());

    // Generate a random IV
    SecByteBlock iv(AES::BLOCKSIZE);
    rnd.GenerateBlock(iv, iv.size());

    //qDebug() << "key length = " << keyText.size();
    //qDebug() << "macAddressHashed = " << QByteArray::fromStdString(keyText).toHex();
    //qDebug() << "plainText = " << QString::fromStdString(plainText);

    //////////////////////////////////////////////////////////////////////////
    // Encrypt

    CFB_Mode<AES>::Encryption cfbEncryption;
    cfbEncryption.SetKeyWithIV(key, key.size(), iv, iv.size());
    //cfbEncryption.ProcessData(cypheredText, plainText, messageLen);

    StringSource encryptor(plainText, true,
                           new StreamTransformationFilter(cfbEncryption,
                                                          new StringSink(encrypted)));
    //qDebug() << "encrypted = " << QByteArray::fromStdString(encrypted).toHex();
    // write encrypted message and IV to file
    loginDataFile.write((const char*)iv.data(),iv.size());
    loginDataFile.write(encrypted.c_str());
    return true;
}

bool FileHandler::readLoginData(){
    QFile loginDataFile(loginDataFilePath);
    if(!loginDataFile.open(QIODevice::ReadWrite)){
        qDebug() << "could not open login data save file";
        return false;
    }

    std::string keyText;
    std::string encrypted;
    std::string recovered;

    keyText = SHA256HashString(getMacAddress().toStdString());
    SecByteBlock key((const byte*)keyText.data(),keyText.size());

    // read encrypted message and IV from file
    SecByteBlock iv(AES::BLOCKSIZE);
    char ivData[AES::BLOCKSIZE];
    if (int read = loginDataFile.read(ivData,AES::BLOCKSIZE)!=AES::BLOCKSIZE){
        qDebug() << "read " << read << " bytes but should read " << AES::BLOCKSIZE << " bytes";
        return false;
    }
    iv.Assign((byte*)ivData, AES::BLOCKSIZE);
      QByteArray data = loginDataFile.readAll();
      encrypted = std::string(data.constData(), data.length()); // <-- right :)
    //QString::fromLocal8Bit(temp.data()).toStdString() <-- wrong!!

    //////////////////////////////////////////////////////////////////////////
    // Decrypt
    CFB_Mode<AES>::Decryption cfbDecryption;
    cfbDecryption.SetKeyWithIV(key, key.size(), iv, iv.size());
    //cfbDecryption.ProcessData(plainText, cypheredText, messageLen);

    StringSource decryptor(encrypted, true,
                           new StreamTransformationFilter(cfbDecryption,
                                                          new StringSink(recovered)));
    QString recoverdQString = QString::fromStdString(recovered);
    QStringList loginData = recoverdQString.split(';',QString::SkipEmptyParts);
    if (loginData.size() < 2){
        return false;
    }
    username = loginData.at(0);
    password = loginData.at(1);
    return true;
}
