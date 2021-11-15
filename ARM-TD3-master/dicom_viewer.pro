#-------------------------------------------------
#
# Project created by QtCreator 2020-09-04T09:30:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dicom_viewer
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



SOURCES += \
        main.cpp \
        dicom_viewer.cpp \
        image_label.cpp \
        double_slider.cpp \
        volumic_data.cpp \
        glwidget.cpp \
        int_slider.cpp \
        checkbox.cpp

HEADERS += \
        dicom_viewer.h \
        image_label.h \
        double_slider.h \
        volumic_data.h \
        glwidget.h \
        int_slider.h \
        checkbox.h

LIBS += \
        -ldcmdata \
        -ldcmimage \
        -ldcmimgle \
        -lofstd \
        -ldcmjpeg
