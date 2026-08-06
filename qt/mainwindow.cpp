#include "mainwindow.h"

#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

static const char *SOCKET_PATH = "/tmp/smart-gateway.sock";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      connectionLabel(new QLabel("未连接")),
      ds18b20Label(new QLabel("-- ℃")),
      sht30TemperatureLabel(new QLabel("-- ℃")),
      sht30HumidityLabel(new QLabel("-- %")),
      eventLabel(new QLabel("暂无事件")),
      ledOnButton(new QPushButton("打开 LED")),
      ledOffButton(new QPushButton("关闭 LED"))
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    auto *form = new QFormLayout;

    form->addRow("服务状态：", connectionLabel);
    form->addRow("DS18B20 温度：", ds18b20Label);
    form->addRow("SHT30 温度：", sht30TemperatureLabel);
    form->addRow("SHT30 湿度：", sht30HumidityLabel);
    form->addRow("最近事件：", eventLabel);
    layout->addLayout(form);
    layout->addWidget(ledOnButton);
    layout->addWidget(ledOffButton);
    setCentralWidget(central);
    setWindowTitle("i.MX6ULL 智能网关监控");
    resize(420, 260);

    connect(&socket, &QLocalSocket::readyRead,
            this, &MainWindow::readSocketData);
    connect(&socket, &QLocalSocket::connected, this, [this]() {
        connectionLabel->setText("已连接");
    });
    connect(&socket, &QLocalSocket::disconnected, this, [this]() {
        connectionLabel->setText("服务已断开");
    });
    connect(ledOnButton, &QPushButton::clicked,
            this, &MainWindow::setLedOn);
    connect(ledOffButton, &QPushButton::clicked,
            this, &MainWindow::setLedOff);
    connect(&timer, &QTimer::timeout, this, &MainWindow::requestStatus);
    timer.start(2000);
    requestStatus();
}

void MainWindow::sendCommand(const QByteArray &command)
{
	/* 后端采用“一次连接、一条命令、一次响应”的简单协议。 */
	socket.abort();
	socket.connectToServer(SOCKET_PATH);
	if (!socket.waitForConnected(200)) {
		connectionLabel->setText("等待 gateway-manager");
		return;
	}
	socket.write(command);
    socket.flush();
}

void MainWindow::requestStatus()
{
    sendCommand("status\n");
}

void MainWindow::setLedOn()
{
    sendCommand("led 1\n");
}

void MainWindow::setLedOff()
{
    sendCommand("led 0\n");
}

void MainWindow::readSocketData()
{
    const QByteArray data = socket.readAll();
    updateFromJson(data);
    /* 后端按“一次请求、一次响应”工作，读完后断开。 */
    socket.disconnectFromServer();
}

void MainWindow::updateFromJson(const QByteArray &data)
{
    const QJsonDocument document = QJsonDocument::fromJson(data);
    if (!document.isObject())
        return;
    const QJsonObject object = document.object();
    connectionLabel->setText("已连接");
    if (object.value("ds18b20_valid").toInt())
        ds18b20Label->setText(QString::number(
            object.value("ds18b20_temperature").toDouble(), 'f', 2) + " ℃");
    if (object.value("sht30_valid").toInt()) {
        sht30TemperatureLabel->setText(QString::number(
            object.value("sht30_temperature").toDouble(), 'f', 2) + " ℃");
        sht30HumidityLabel->setText(QString::number(
            object.value("sht30_humidity").toDouble(), 'f', 2) + " %");
    }
    eventLabel->setText(object.value("event").toString());
}
