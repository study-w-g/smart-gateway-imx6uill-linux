#ifndef SMART_GATEWAY_MAINWINDOW_H
#define SMART_GATEWAY_MAINWINDOW_H

#include <QMainWindow>
#include <QLocalSocket>
#include <QTimer>

class QLabel;
class QPushButton;

/*
 * Qt 只负责界面和本地交互，不直接访问 /dev 或 I2C。
 * 它通过 QLocalSocket 请求 gateway-manager，再显示服务返回的 JSON。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void requestStatus();
    void readSocketData();
    void setLedOn();
    void setLedOff();

private:
    void sendCommand(const QByteArray &command);
    void updateFromJson(const QByteArray &data);

    QLocalSocket socket;
    QTimer timer;
    QLabel *connectionLabel;
    QLabel *ds18b20Label;
    QLabel *sht30TemperatureLabel;
    QLabel *sht30HumidityLabel;
    QLabel *eventLabel;
    QPushButton *ledOnButton;
    QPushButton *ledOffButton;
};

#endif
