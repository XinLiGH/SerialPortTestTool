#include "widget.h"
#include "ui_widget.h"
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    serialPort(new QSerialPort),
    serialPortInfo(new QSerialPortInfo),
    sendTimer(new QTimer),
    showsTimer(new QTimer),
    writeTestFile(new QFile),
    writeLogFile(new QFile)
{
    ui->setupUi(this);

    foreach(*serialPortInfo, QSerialPortInfo::availablePorts()) /* Scans the serial port list */
    {
        ui->comPort->addItem(serialPortInfo->portName());       /* Add a project */
    }

    connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendTestFrame()));
    connect(showsTimer, SIGNAL(timeout()), this, SLOT(showsStatus()));
}

Widget::~Widget()
{
    delete ui;
    delete serialPort;
    delete serialPortInfo;
    delete sendTimer;
    delete showsTimer;
    delete writeTestFile;
    delete writeLogFile;
}

void Widget::parameterInit()
{
    memset(&testFrame, 0, sizeof(testFrame));
    sendNum = 0;
    droppedFramesNum = 0;
    wrongFramesNum = 0;
    firstSend = false;
}

uint64_t Widget::checkSum(uint8_t *buff, uint64_t len)
{
    uint64_t check = 0;

    for(uint64_t i = 8; i < (len - 8); i++)
    {
        check += buff[i];
    }

    return check;
}

void Widget::on_startEnd_clicked()
{
    static bool checked = false;

    if(checked == false)
    {
        serialPort->setPortName(ui->comPort->currentText());            /* Set port name */
        serialPort->setBaudRate(ui->baudRate->currentText().toInt());   /* Set baud rate */
        serialPort->setDataBits(QSerialPort::Data8);                    /* Set data bits */
        serialPort->setStopBits(QSerialPort::OneStop);                  /* Set stop bits */
        serialPort->setParity(QSerialPort::NoParity);                   /* Set parity */
        serialPort->setFlowControl(QSerialPort::NoFlowControl);         /* Set flow control */

        if(serialPort->open(QSerialPort::ReadWrite) == true)            /* Open port */
        {
            checked = true;

            ui->comPort->setDisabled(true);
            ui->baudRate->setDisabled(true);
            ui->startEnd->setText(u8"结束");

            parameterInit();
            showsStatus();

            QDir fileDir;
            char filePath[100] = {0};
            QDateTime fileTime = QDateTime::currentDateTime();

            sprintf(filePath, "TestFile/%u-%.2u-%.2u %.2u.%.2u.%.2u", fileTime.date().year(), fileTime.date().month(), fileTime.date().day(), fileTime.time().hour(), fileTime.time().minute(), fileTime.time().second());
            fileDir.mkdir("TestFile");
            fileDir.mkdir(filePath);

            writeTestFile->setFileName(filePath + QString("/data.txt"));
            writeLogFile->setFileName(filePath + QString("/log.txt"));

            writeTestFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
            writeLogFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);

            uint32_t sendTime = sizeof(TestFrame) * 50000 / ui->baudRate->currentText().toInt();

            if(sendTime > 100)
            {
                sendTimer->start(sendTime);
            }
            else
            {
                sendTimer->start(100);
            }

            showsTimer->start(40);
        }
        else
        {
            QMessageBox::warning(this, u8"串口打开失败", u8"请检查串口是否已被占用！", u8"关闭");
        }
    }
    else
    {
        checked = false;

        serialPort->close();                            /* Close port */
        ui->startEnd->setText(u8"开始");
        ui->comPort->setEnabled(true);
        ui->baudRate->setEnabled(true);

        writeLogFile->close();
        writeLogFile->close();

        sendTimer->stop();
        showsTimer->stop();
    }
}

void Widget::sendTestFrame()
{
    if(firstSend == true)
    {
        TestFrame data = {0};
        QByteArray buff = serialPort->readAll();    /* Read port all data */

        if(buff.count() < sizeof(data))
        {
            droppedFramesNum++;

            char testFrameBuff[128] = {0};
            int testFrameLen = 0;

            testFrameLen = sprintf(testFrameBuff, "%llu\t%llu-%.2llu-%.2llu\t%.2llu:%.2llu:%.2llu:%.3llu\tDroppedFrames\n", testFrame.number, testFrame.date.year, testFrame.date.month, testFrame.date.day, testFrame.time.hour, testFrame.time.minute, testFrame.time.second, testFrame.time.msec);

            writeLogFile->write(testFrameBuff, testFrameLen);
            writeLogFile->flush();
        }
        else
        {
            memcpy(&data, buff.data(), sizeof(data));

            if((data.header == HEADER) && (data.check == checkSum((uint8_t*)(&data), sizeof(data))) && (data.number == testFrame.number))
            {

            }
            else
            {
                wrongFramesNum++;

                char testFrameBuff[128] = {0};
                int testFrameLen = 0;

                testFrameLen = sprintf(testFrameBuff, "%llu\t%llu-%.2llu-%.2llu\t%.2llu:%.2llu:%.2llu:%.3llu\tWrongFrames\n", testFrame.number, testFrame.date.year, testFrame.date.month, testFrame.date.day, testFrame.time.hour, testFrame.time.minute, testFrame.time.second, testFrame.time.msec);

                writeLogFile->write(testFrameBuff, testFrameLen);
                writeLogFile->flush();
            }
        }
    }
    else
    {
        firstSend = true;
    }

    QDateTime time = QDateTime::currentDateTime();

    testFrame.header      = HEADER;
    testFrame.number      = testFrame.number + 1;
    testFrame.date.year   = time.date().year();
    testFrame.date.month  = time.date().month();
    testFrame.date.day    = time.date().day();
    testFrame.time.hour   = time.time().hour();
    testFrame.time.minute = time.time().minute();
    testFrame.time.second = time.time().second();
    testFrame.time.msec   = time.time().msec();
    testFrame.check       = checkSum((uint8_t*)(&testFrame), sizeof(testFrame));

    serialPort->write((char*)(&testFrame), sizeof(testFrame));

    char testFrameBuff[128] = {0};
    int testFrameLen = 0;

    testFrameLen = sprintf(testFrameBuff, "%llu\t%llu-%.2llu-%.2llu\t%.2llu:%.2llu:%.2llu:%.3llu\n", testFrame.number, testFrame.date.year, testFrame.date.month, testFrame.date.day, testFrame.time.hour, testFrame.time.minute, testFrame.time.second, testFrame.time.msec);

    writeTestFile->write(testFrameBuff, testFrameLen);
    writeTestFile->flush();

    sendNum++;
}

void Widget::showsStatus()
{
    ui->sendNum->setText(QString::number(sendNum));
    ui->droppedFramesNum->setText(QString::number(droppedFramesNum));
    ui->wrongFrameNum->setText(QString::number(wrongFramesNum));
}
