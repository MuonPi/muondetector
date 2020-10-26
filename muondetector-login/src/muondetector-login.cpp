#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <QNetworkInterface>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QCryptographicHash>
#include <crypto++/aes.h>
#include <crypto++/modes.h>
#include <crypto++/filters.h>
#include <crypto++/osrng.h>
#include <crypto++/hex.h>
//#include <crypto++/sha3.h>
#include <crypto++/sha.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <mqtthandler.h>

using namespace CryptoPP;

// crypto related stuff

[[nodiscard]] auto getch() -> int;
[[nodiscard]] auto getpass(const char *prompt, const bool show_asterisk) -> std::string;
[[nodiscard]] auto SHA256HashString(const std::string& aString) -> std::string;
[[nodiscard]] auto getMacAddress() -> QString;
[[nodiscard]] auto getMacAddressByteArray() -> QByteArray;
[[nodiscard]] auto saveLoginData(const QString& loginFilePath, const QString& username, const QString& password) -> bool;


auto getch() -> int {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

auto getpass(const char *prompt, const bool show_asterisk) -> std::string
{
  const char BACKSPACE=127;
  const char RETURN=10;

  std::string password;
  unsigned char ch=0;
  ch=getch();
  std::cout <<prompt<<std::endl;
  while((ch=getch())!=RETURN)
    {
       if(ch==BACKSPACE)
         {
            if(password.length()!=0)
              {
                 if(show_asterisk)
                 std::cout <<"\b \b";
                 password.resize(password.length()-1);
              }
         }
       else
         {
             password+=ch;
             if(show_asterisk)
                 std::cout <<'*';
         }
    }
  std::cout << std::endl;
  return password;
}

auto SHA256HashString(const std::string& aString) -> std::string
{
    std::string digest;
    CryptoPP::SHA256 hash;

    CryptoPP::StringSource foo(aString, true,
    new CryptoPP::HashFilter(hash, new CryptoPP::StringSink(digest)));
    return digest;
}

auto getMacAddress() -> QString
{
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

auto getMacAddressByteArray() -> QByteArray
{
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

auto saveLoginData(const QString &loginFilePath, const QString &username, const QString &password) -> bool
{
    QFile loginDataFile(loginFilePath);
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

int main(int argc, char *argv[])
{
    //QCoreApplication a(argc, argv);
    std::cout << "To set the login for the mqtt-server, please enter user name:"<<std::endl;
    std::string username;
    std::cin >> username;
    std::string password = getpass("please enter password:",true);
    QString hashedMacAddress = QString(QCryptographicHash::hash(getMacAddressByteArray(), QCryptographicHash::Sha224).toHex());
    saveLoginData(QString("/var/muondetector/"+hashedMacAddress+"/loginData.save"),QString::fromStdString(username),QString::fromStdString(password));
    MqttHandler mqttHandler("");
    std::unique_ptr<QObject> context{new QObject};
    QObject* pcontext = context.get();
    QObject::connect(&mqttHandler, &MqttHandler::mqttConnectionStatus, [context = std::move(context)](bool connected) mutable{
        if (connected){
            std::cout << "login data is correct!" << std::endl;
        }{
            //std::cout << "invalid login data or server not reachable!" << std::endl;
        }
        context.release();
    });
    mqttHandler.start(QString::fromStdString(username),QString::fromStdString(password));
    mqttHandler.mqttDisconnect();
    return 0;
}
