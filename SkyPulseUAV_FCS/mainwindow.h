#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThreadPool>
#include <QThread>
#include <QCloseEvent>

#include "i2c_device.h"
#include "tcp.h"
#include "barometer_bmp180.h"
#include "magnetometer_gy271.h"
#include "gyroacelemeter_gy521.h"

#define HMC5883l_DEVICE_ADDR 0x0D


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void prepareForQuit();


private slots:
    void on_pushButton_BMP_clicked();
    void on_pushButton_HMC_clicked();

private:
    Ui::MainWindow *ui;
    /* Thread */
    QThreadPool threadPool;
    QThread *BMP_Thread;
    /* Network Protocol */
    TCP *TCPServer;
    /* Sensors */
    Magnetometer_GY271 *MagnetoMeter;
    Barometer_BMP180 *BaroMeter;

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void sig_readPressure();
    void sig_readTemperature();

};
#endif // MAINWINDOW_H