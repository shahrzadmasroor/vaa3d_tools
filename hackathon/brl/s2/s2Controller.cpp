#include "v3d_message.h"
#include <QDebug>
#include <QtGui>
#include <QtNetwork>
#include <QRegExp>
#include <stdlib.h>
#include "s2Controller.h"


//! [0]
S2Controller::S2Controller(QWidget *parent):   QWidget(parent), networkSession(0)
{

    hostLabel = new QLabel(tr("&Server name:"));
    portLabel = new QLabel(tr("S&erver port:"));
    cmdLabel = new QLabel(tr("Command:"));

    QString ipAddress;
    ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    int portnumber= 1236;
    hostLineEdit = new QLineEdit("10.128.50.123");
    portLineEdit = new QLineEdit("1236");
    portLineEdit->setValidator(new QIntValidator(1, 65535, this));
    cmdLineEdit = new QLineEdit;

    hostLabel->setBuddy(hostLineEdit);
    portLabel->setBuddy(portLineEdit);

    statusLabel = new QLabel(tr(" - - - "));

    sendCommandButton = new QPushButton(tr("Send Command"));
    sendCommandButton->setDefault(true);
    sendCommandButton->setEnabled(false);
    connectButton = new QPushButton(tr("connect to PrairieView"));
    connectButton->setEnabled(false);

    quitButton = new QPushButton(tr("Quit"));
    getReplyButton = new QPushButton(tr("get reply"));

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(sendCommandButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);
    buttonBox->addButton(connectButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(getReplyButton, QDialogButtonBox::ActionRole);




//! [1]
    tcpSocket = new QTcpSocket(this);
//! [1]

    connect(hostLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(enablesendCommandButton()));
    connect(portLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(enablesendCommandButton()));
    connect(sendCommandButton, SIGNAL(clicked()),
            this, SLOT(sendCommand()));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(initializeS2()));
//! [2] //! [3]
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(checkForMessage()));
    connect(this, SIGNAL(messageIsComplete()), this, SLOT(processMessage()));
    connect(this, SIGNAL(newMessage(QString)), this, SLOT(messageHandler(QString)));


    connect(quitButton, SIGNAL(clicked()), this, SLOT(sendX()));
//! [2] //! [4]
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
//! [3]
            this, SLOT(displayError(QAbstractSocket::SocketError)));
//! [4]

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(hostLabel, 0, 0);
    mainLayout->addWidget(hostLineEdit, 0, 1);
    mainLayout->addWidget(portLabel, 1, 0);
    mainLayout->addWidget(portLineEdit, 1, 1);
    mainLayout->addWidget(cmdLabel, 2,0);
    mainLayout->addWidget(cmdLineEdit,2,1);
    mainLayout->addWidget(statusLabel, 3, 0, 1, 3);
    mainLayout->addWidget(buttonBox, 4, 0, 1, 3);
    setLayout(mainLayout);

    setWindowTitle(tr("smartScope2 Controller"));
    portLineEdit->setFocus();

    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, SIGNAL(opened()), this, SLOT(sessionOpened()));

        sendCommandButton->setEnabled(false);
        statusLabel->setText(tr("Opening network session."));
        networkSession->open();
    }
//! [5]
}


void S2Controller::initializeS2(){
    initializeParameters();
    connectToS2();
}

void S2Controller::initConnection(){
}

void S2Controller::initializeParameters(){
    emit newMessage(QString("initialized"));
}


void S2Controller::connectToS2()
{
    tcpSocket->connectToHost(hostLineEdit->text(),
    portLineEdit->text().toInt());
    }

void S2Controller::sendCommand()
{
    sendCommandButton->setEnabled(false);
    sendAndReceive(cmdLineEdit->text());
}
void S2Controller::sendAndReceive(QString inputString){
    if (!okToSend){return;}
    okToSend = false;
    if (cleanAndSend(inputString)){
        qDebug()<<"sent "<<inputString;
    }

}

bool S2Controller::cleanAndSend(QString inputString)
{
    inputString.replace(' ', (char)1).append((char)13).append((char)10);
    tcpSocket->write(inputString.toLatin1());
    return true;
}

void S2Controller::sendX()
{
    const QString xQ = QString("-x");
    cmdLineEdit->setText(xQ);
    sendAndReceive(xQ);
    QTimer::singleShot(1000, this, SLOT(cleanUp()));
}

void S2Controller::cleanUp()
{
    tcpSocket->close();
    close();
}




void S2Controller::checkForMessage(){
    // this will get called by readyRead signal from tcpsocket
    // append any incoming data to a totalString attribute
    stringMessage.append(QString(tcpSocket->readAll()).toLatin1());
    if (stringMessage.contains("DONE\r\n")){
    // check for DONE.  if not, don't do anything!
    // [this will still be called when new data is available]

    // if DONE, emit a signal to processMessage
        emit messageIsComplete();
    qDebug()<<stringMessage;}
    // DO NOT unblock commands yet!

}

void S2Controller::processMessage(){
    // once a fullMessage is made, this method should only be called as a slot
    // because it unblocks the ability to send tcp commands
    // - parse the message here, extracting the returned message
    message = QString("");
    QStringList mList;
    mList = stringMessage.split("ACK\r\n");
    message = QString(mList.last().split("DONE\r\n").first());//  still has carriage return
    message.remove("\r\n");
    // and emitting the message, including updating text fields, etc.
    emit newMessage(message);

    // clear the fullMessage buffer  [this should be OK- only this method
    // and checkForMessages should ever access it]  note this is not scalable-
    // there's only one fullMessage at a time.

    stringMessage.clear();
    // unblock commands
    okToSend = true;
    sendCommandButton->setEnabled(true);

}

void S2Controller::messageHandler(QString messageH){
    // slot for handling messages.   first round will
    // just be updating text in this object and the calling UI
    myS2Data.messageString = messageH;
    emit newS2Data(myS2Data);
    statusLabel->setText(messageH);
}





void S2Controller::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("smartScope2 Controller"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("smartScope2 Controller"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure PrairieView is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("smartScope2 Controller"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }

    sendCommandButton->setEnabled(true);
}
//! [13]

void S2Controller::enablesendCommandButton()
{
    sendCommandButton->setEnabled((!networkSession || networkSession->isOpen()) &&
                                 !hostLineEdit->text().isEmpty() &&
                                 !portLineEdit->text().isEmpty());
    connectButton->setEnabled((!networkSession || networkSession->isOpen()) &&
                                 !hostLineEdit->text().isEmpty() &&
                                 !portLineEdit->text().isEmpty());
}

void S2Controller::sessionOpened()
{
    // Save the used configuration
    QNetworkConfiguration config = networkSession->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice)
        id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    else
        id = config.identifier();

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    statusLabel->setText(tr("Prototype Controller requires "
                            "PrairieView to run at the same time."));

    enablesendCommandButton();
}


void S2Controller::initROI(){//    set up the microscope with appropriate parameters for small 3D ROI.  This could be done with a single .xml file from a saved configuration or through setting parameters from Vaa3D.
}

void S2Controller::startROI(){ //    set a target file location and trigger the 3D ROI.
}

void S2Controller::getROIData(){ //    FILE VERSION: Wait for PV to signal ROI completion (?), wait for arbitrary delay or poll filesystem for available file
       //                        //SHARED MEMORY VERSION: during ROI initiation, Vaa3D will allocate a new 1d byte array and send the address and length to PV. It might be a bit tricky to know when this data is valid.
}

void S2Controller::processROIData(){ //Process image data and return 1 or more next locations.  Many alternative approaches could be used here, including: Run APP2 and locate ends of structure on boundary.  Identify foreground blobs in 1-D max or sum projections of ROI faces. Identify total intensity and variance in the entire ROI. Identify total tubularity in the ROI or near the edges, etc etc.  In any case, the resulting image coordinates will be transformed into coordinates that PV understands for (e.g.) "PanXY"  commands.
}
void S2Controller::startNextROI(){//   Move to the next ROI location and start the scan.  With the new 'PanXY' command, this should be trivial.
}


void S2Controller::getPosition(int axis, int subAxis){
    // axis 0 = X, 1 = Y, 2 = Z  (letter names are within microscope)
    // subaxis is system-dependent, default is zero
    axis =2;
    QString qaxis;
    QString qsubAxis;
    QString toSendString;

    if (axis == 2){
        qaxis = QString("ZAxis");
    }
    qsubAxis = QString(subAxis);
    toSendString = QString("-gts positionCurrent ").append(qaxis).append(' ').append(qsubAxis);
    qDebug()<<toSendString;
    sendAndReceive(toSendString);

}


