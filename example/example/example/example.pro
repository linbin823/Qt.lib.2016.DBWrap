QT += core
QT -= gui

CONFIG += c++11

TARGET = example
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

include( ../../../src/DBProcess.pri)
