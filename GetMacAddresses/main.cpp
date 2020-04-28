#include <QCoreApplication>
#include <QNetworkInterface>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <iostream>

static void getMacAddress(){
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
    bool first = true;
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
            if (first){
                std::cout << "pick this: ";
                first = false;
            }
            std::cout << MAC.toStdString()<<std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    getMacAddress();
    return 0;
}
