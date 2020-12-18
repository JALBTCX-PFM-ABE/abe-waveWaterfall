contains(QT_CONFIG, opengl): QT += opengl
QT += 
RESOURCES = icons.qrc
INCLUDEPATH += /c/PFM_ABEv7.0.0_Win64/include
LIBS += -L /c/PFM_ABEv7.0.0_Win64/lib -lCZMIL -lCHARTS -lnvutility -lpfm -lgdal -lxml2 -lpoppler -liconv
DEFINES += WIN32 NVWIN3X
CONFIG += console

#
# The following line is included so that the contents of acknowledgments.hpp will be available for translation
#

HEADERS += /c/PFM_ABEv7.0.0_Win64/include/acknowledgments.hpp

QMAKE_LFLAGS += 
######################################################################
# Automatically generated by qmake (2.01a) Wed Jan 22 14:56:21 2020
######################################################################

TEMPLATE = app
TARGET = waveWaterfall
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += version.hpp waveWaterfall.hpp waveWaterfallHelp.hpp
SOURCES += main.cpp waveWaterfall.cpp
RESOURCES += icons.qrc
TRANSLATIONS += waveWaterfall_xx.ts
