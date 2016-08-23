QT += core
QT -= gui

CONFIG += c++11

TARGET = Test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    CommunicationTest.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/x64/ -lCppUTest
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/x64/ -lCppUTestd

INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/include
