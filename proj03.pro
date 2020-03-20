#-------------------------------------------------
#
# Project created by QtCreator 2019-03-03T17:22:16
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Object_Recognizer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
        /usr/local/include/opencv4 \
        /usr/local/include \
        ../include

LIBS += \
        -L/usr/local/lib \
            -lopencv_core \
            -lopencv_highgui \
            -lopencv_video \
            -lopencv_videoio \
            -lopencv_imgproc \
            -lopencv_imgcodecs \
            -lopencv_ml

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        putility.cpp \
    processor.cpp \
    dbmanager.cpp

HEADERS += \
        mainwindow.h \
        putility.h \
    processor.h \
    processor.h \
    processingvars.h \
    dbmanager.h

FORMS += \
        mainwindow.ui
