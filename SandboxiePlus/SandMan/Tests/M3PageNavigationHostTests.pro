TEMPLATE = app
TARGET = M3PageNavigationHostTests

QT += core gui network widgets testlib
CONFIG += testcase console c++17
CONFIG -= app_bundle

INCLUDEPATH += .. ../Windows ../../MiscHelpers

SOURCES += \
    M3PageNavigationHostTests.cpp \
    ../Windows/M3PageNavigationHost.cpp \
    ../Windows/M3SearchField.cpp \
    ../Windows/RegexBuilderDialog.cpp

HEADERS += \
    ../Windows/M3PageNavigationHost.h \
    ../Windows/M3SearchField.h \
    ../Windows/RegexBuilderDialog.h
