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
#include <crypto++/aes.h>
#include <crypto++/modes.h>
#include <crypto++/filters.h>
#include <crypto++/osrng.h>
#include <crypto++/hex.h>
#include <crypto++/sha3.h>

using namespace CryptoPP;


const int timeout = 600000; // in msecs
const int uploadReminderInterval = 5; // in minutes
const int logReminderInterval = 5; // in minutes

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
    return QByteArray::fromStdString(getMacAddress().toStdString());
}

static QString dateStringNow(){
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd_hh-mm-ss");
}

FileHandler::FileHandler(QString userName, QString passWord, QString dataPath, quint32 fileSizeMB, QObject *parent)
    : QObject(parent)
{
    lastUploadDateTime = QDateTime(QDate::currentDate(),QTime(0,0,0,0),Qt::TimeSpec::UTC);
    dailyUploadTime = QTime(11,11,11,111);
    if (dataPath != ""){
        mainDataFolderName = dataPath;
    }
    fileSize = fileSizeMB;
    QDir temp;
    QString fullPath = temp.homePath()+"/"+mainDataFolderName;
    QCryptographicHash hashFunction(QCryptographicHash::Sha3_256);
    hashedMacAddress = QString(hashFunction.hash(getMacAddressByteArray(), QCryptographicHash::Sha3_224).toHex());
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

    if (userName!=""||passWord!=""){
        username=userName;
        password=passWord;
        if(!saveLoginData(userName,passWord)){
            qDebug() << "could not save login data";
        }
    }else{
        if (!readLoginData()){
            qDebug() << "could not read login data from file";
        }
    }
    // set upload reminder
    QTimer *uploadReminder = new QTimer(this);
    uploadReminder->setInterval(60*1000*uploadReminderInterval); // every 5 minutes or so
    uploadReminder->setSingleShot(false);
    connect(uploadReminder, &QTimer::timeout, this, &FileHandler::onUploadRemind);
    uploadReminder->start();
    // set log reminder
    QTimer *logReminder = new QTimer(this);
    logReminder->setInterval(60*1000*logReminderInterval);
    uploadReminder->setSingleShot(false);
    connect(logReminder, &QTimer::timeout, this, &FileHandler::onLogRemind);
    logReminder->start();
    // open files that are currently written
    openFiles();
}

// SLOTS
void FileHandler::onReceivedLogParameter(LogParameter log){
    writeToLogFile(QString(log.name()+" "+log.value()+" "+dateStringNow()+"\n"));
    log.setUpdatedRecently(true);
    logData.insert(log.name(),log);
}

void FileHandler::onLogRemind(){

}

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
        uploadRecentDataFiles();
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
    if (!dataFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
        qDebug() << "file open failed in 'ReadWrite' mode at location " << currentWorkingFilePath;
        return false;
    }
    logFile = new QFile(currentWorkingLogPath);
    if (!logFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
        qDebug() << "file open failed in 'ReadWrite' mode at location " << currentWorkingLogPath;
        return false;
    }
    // write header
    if (writeHeader){
        QTextStream dataOut(dataFile);
        dataOut << "#rising               falling               accEst valid timebase utc\n";
        QTextStream logOut(logFile);
        logOut << "#temperature ... etc.\n";
    }
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
    out << data;
}

void FileHandler::writeToLogFile(QString log) {
    if (logFile == nullptr){
        return;
    }
    QTextStream out(logFile);
    out << log;
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
    QProcess lftpProcess(this);
    lftpProcess.setProgram("lftp");
    QStringList arguments;
    arguments << "-p" << "35221";
    arguments << "-u" << QString(username+","+password);
    arguments << "balu.physik.uni-giessen.de:/cosmicshower";
    arguments << "-e" << QString("mkdir "+hashedMacAddress+" ; cd "+hashedMacAddress+" && put "+fileName+" ; exit");
    lftpProcess.setArguments(arguments);
    qDebug() << lftpProcess.arguments();
    lftpProcess.start();
    if (!lftpProcess.waitForFinished(timeout)){
        qDebug() << lftpProcess.readAllStandardOutput();
        qDebug() << lftpProcess.readAllStandardError();
        qDebug() << "lftp not installed or timed out after "<< timeout/1000<< " s";
        return false;
    }
    if (lftpProcess.exitStatus()!=0){
        qDebug() << "lftp returned exit status other than 0";
        return false;
    }
    return true;
}

bool FileHandler::uploadRecentDataFiles(){
    readFileInformation();
    for (auto &fileName : notUploadedFilesNames){
        QString filePath = dataFolderPath+fileName;
        if (filePath!=currentWorkingFilePath&&filePath!=currentWorkingLogPath){
            qDebug() << "attempt to upload " << filePath;
            if (!uploadDataFile(filePath)){
                return false;
            }
            QFile::rename(filePath,QString(configPath+"uploadedFiles/"+fileName));
        }
    }
    return true;
}

// crypto related stuff

bool FileHandler::saveLoginData(QString username, QString password){
    QFile loginDataFile(loginDataFilePath);
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
    encrypted = loginDataFile.readAll().toStdString();
    //qDebug() << "encrypted = " << QByteArray::fromStdString(encrypted).toHex();

    //////////////////////////////////////////////////////////////////////////
    // Decrypt
    CFB_Mode<AES>::Decryption cfbDecryption;
    cfbDecryption.SetKeyWithIV(key, key.size(), iv, iv.size());
    //cfbDecryption.ProcessData(plainText, cypheredText, messageLen);

    StringSource decryptor(encrypted, true,
                           new StreamTransformationFilter(cfbDecryption,
                                                          new StringSink(recovered)));

    //qDebug() << "recovered = " << QString::fromStdString(recovered);
    QString recoverdQString = QString::fromStdString(recovered);
    QStringList loginData = recoverdQString.split(';',QString::SkipEmptyParts);
    username = loginData.at(0);
    password = loginData.at(1);
    return true;
}
