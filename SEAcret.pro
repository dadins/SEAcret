#-------------------------------------------------
#
# Project created by QtCreator 2012-05-19T20:58:04
#
#-------------------------------------------------
QT       += core gui
TARGET = SEAcret
TEMPLATE = app
SOURCES += main.cpp\
        AES.cpp\
        md5.cpp\
        mainwindow.cpp
HEADERS  += mainwindow.h\
        AES.h\
        md5.h
FORMS    += mainwindow.ui

INCLUDEPATH += C:\opencv\build\include

CONFIG(debug,debug|release)
{
LIBS += C:\opencv\build\x86\mingw\lib\libopencv_core245.dll.a \
    C:\opencv\build\x86\mingw\lib\libopencv_highgui245.dll.a \
    C:\opencv\build\x86\mingw\lib\libopencv_imgproc245.dll.a \
    C:\opencv\build\x86\mingw\lib\libopencv_photo245.dll.a \
}
