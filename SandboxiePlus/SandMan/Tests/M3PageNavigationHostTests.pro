TEMPLATE = app
TARGET = M3PageNavigationHostTests

QT += core gui network widgets testlib
CONFIG += testcase console c++17
CONFIG -= app_bundle

INCLUDEPATH += .. ../Windows ../../MiscHelpers

MISCHELPERS_LIB_DIR = $$(MISCHELPERS_LIB_DIR)
isEmpty(MISCHELPERS_LIB_DIR): MISCHELPERS_LIB_DIR = $$PWD/../../Bin/x64/Release
win32:LIBS += -L$$MISCHELPERS_LIB_DIR -lMiscHelpers

SOURCES += \
    M3PageNavigationHostTests.cpp \
    ../Windows/M3DialogHost.cpp \
    ../Windows/M3PageNavigationHost.cpp \
    ../Windows/M3Menu.cpp \
    ../Windows/M3SearchField.cpp \
    ../Windows/RegexBuilderDialog.cpp \
    ../Windows/M3RegexExecutionPolicy.cpp \
    ../../MiscHelpers/Common/MaterialTheme.cpp \
    ../../MiscHelpers/Common/M3Tokens.cpp

HEADERS += \
    ../Windows/M3DialogHost.h \
    ../Windows/M3PageNavigationHost.h \
    ../Windows/M3Menu.h \
    ../Windows/M3SearchField.h \
    ../Windows/RegexBuilderDialog.h \
    ../Windows/M3RegexExecutionPolicy.h
