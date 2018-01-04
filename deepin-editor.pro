######################################################################
# Automatically generated by qmake (3.0) ?? 2? 4 17:49:37 2017
######################################################################

TEMPLATE = app
TARGET = deepin-editor

CONFIG += link_pkgconfig
CONFIG += c++11 
PKGCONFIG += xcb xcb-util dtkwidget 
RESOURCES = deepin-editor.qrc

# Input
HEADERS += src/mainwindow.h \
		   src/highlighter.h \		   
		   src/editor.h \		   
		   src/utils.h		   
SOURCES += src/mainwindow.cpp \
		   src/highlighter.cpp \		   
		   src/editor.cpp \		   
		   src/utils.cpp \		   
		   src/main.cpp		   
		   
QT += core
QT += widgets
QT += gui
QT += network
QT += x11extras
QT += dbus
QT += svg

QMAKE_CXXFLAGS += -g
LIBS += -lX11 -lXext -lXtst
