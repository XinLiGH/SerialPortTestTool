#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <stdint.h>

#define HEADER  ((uint64_t)0x123456789ABCDEF0)

#pragma pack(push)
#pragma pack(1)

typedef struct
{
    uint64_t year;
    uint64_t month;
    uint64_t day;

}Date;

typedef struct
{
    uint64_t hour;
    uint64_t minute;
    uint64_t second;
    uint64_t msec;

}Time;

typedef struct
{
    uint64_t header;
    uint64_t number;
    Date     date;
    Time     time;
    uint64_t check;

}TestFrame;

#pragma pack(pop)

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    void parameterInit();
    uint64_t checkSum(uint8_t *buff, uint64_t len);

private slots:
    void on_startEnd_clicked();
    void sendTestFrame();
    void showsStatus();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;
    QSerialPortInfo *serialPortInfo;
    QTimer *sendTimer;
    QTimer *showsTimer;
    QFile *writeTestFile;
    QFile *writeLogFile;

    TestFrame testFrame = {0};
    uint64_t sendNum = 0;
    uint64_t droppedFramesNum = 0;
    uint64_t wrongFramesNum = 0;

    bool firstSend = false;
};

#endif // WIDGET_H
