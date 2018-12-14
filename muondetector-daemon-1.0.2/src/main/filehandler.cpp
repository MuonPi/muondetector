#include <filehandler.h>
#include <QTextStream>
#include <QDebug>
#include <QProcess>

FileHandler::FileHandler(QString configFileName, QObject *parent)
{
    if (configFileName != ""){
        dataConfigFileName = configFileName;
    }
    openDataFile();
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
    QStringList arguments("-p 35221"
                          "-u <user>,<pass>"
                          "balu.physik.uni-giessen.de:/cosmicshower/<mac-address>"
						 );
	arguments.append("-e put "+fileName);
    lftpProcess.setArguments(arguments);
    lftpProcess.start();
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
    QFile *configFile = new QFile("dataFileInformation.conf");
    configFile->open(QIODevice::ReadWrite);
    QTextStream *configStream = new QTextStream(configFile);
}

QString FileHandler::createFileName(){
    // creates a fileName based on date time and mac address
}
