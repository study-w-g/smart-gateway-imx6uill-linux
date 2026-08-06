QT += core gui widgets network
CONFIG += c++11
TEMPLATE = app
TARGET = gateway-monitor

SOURCES += main.cpp mainwindow.cpp
HEADERS += mainwindow.h

unix:!android {
	QMAKE_CXXFLAGS += -Wall -Wextra
}
