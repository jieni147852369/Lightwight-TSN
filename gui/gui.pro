QT += widgets network
CONFIG += c++17

TEMPLATE = app
TARGET = TSN_SoftWare
DESTDIR = $$PWD/../bin

SOURCES += \
  src/main.cpp \
  src/MainWindow.cpp \
  src/ConfigTab.cpp \
  src/ExperimentTab.cpp \
  src/SenderWidget.cpp \
  src/ReceiverWidget.cpp \
  src/SenderConfig.cpp \
  src/ReceiverConfig.cpp \
  src/ProcessLineBuffer.cpp

HEADERS += \
  src/MainWindow.h \
  src/ConfigTab.h \
  src/ExperimentTab.h \
  src/SenderWidget.h \
  src/ReceiverWidget.h \
  src/SenderConfig.h \
  src/ReceiverConfig.h \
  src/ProcessLineBuffer.h
