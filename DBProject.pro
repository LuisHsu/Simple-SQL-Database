#-------------------------------------------------
#
# Project created by QtCreator 2016-05-19T04:20:36
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DBProject
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    createdbdialog.cpp \
    filesystem.cpp \
    createtabledialog.cpp \
    loadtabledialog.cpp

HEADERS  += mainwindow.h \
    createdbdialog.h \
    filesystem.h \
    createtabledialog.h \
    loadtabledialog.h \
    Qsci/qsciabstractapis.h \
    Qsci/qsciapis.h \
    Qsci/qscicommand.h \
    Qsci/qscicommandset.h \
    Qsci/qscidocument.h \
    Qsci/qsciglobal.h \
    Qsci/qscilexer.h \
    Qsci/qscilexeravs.h \
    Qsci/qscilexerbash.h \
    Qsci/qscilexerbatch.h \
    Qsci/qscilexercmake.h \
    Qsci/qscilexercoffeescript.h \
    Qsci/qscilexercpp.h \
    Qsci/qscilexercsharp.h \
    Qsci/qscilexercss.h \
    Qsci/qscilexercustom.h \
    Qsci/qscilexerd.h \
    Qsci/qscilexerdiff.h \
    Qsci/qscilexerfortran.h \
    Qsci/qscilexerfortran77.h \
    Qsci/qscilexerhtml.h \
    Qsci/qscilexeridl.h \
    Qsci/qscilexerjava.h \
    Qsci/qscilexerjavascript.h \
    Qsci/qscilexerlua.h \
    Qsci/qscilexermakefile.h \
    Qsci/qscilexermatlab.h \
    Qsci/qscilexeroctave.h \
    Qsci/qscilexerpascal.h \
    Qsci/qscilexerperl.h \
    Qsci/qscilexerpo.h \
    Qsci/qscilexerpostscript.h \
    Qsci/qscilexerpov.h \
    Qsci/qscilexerproperties.h \
    Qsci/qscilexerpython.h \
    Qsci/qscilexerruby.h \
    Qsci/qscilexerspice.h \
    Qsci/qscilexersql.h \
    Qsci/qscilexertcl.h \
    Qsci/qscilexertex.h \
    Qsci/qscilexerverilog.h \
    Qsci/qscilexervhdl.h \
    Qsci/qscilexerxml.h \
    Qsci/qscilexeryaml.h \
    Qsci/qscimacro.h \
    Qsci/qscintillaplugin.h \
    Qsci/qsciprinter.h \
    Qsci/qsciscintilla.h \
    Qsci/qsciscintillabase.h \
    Qsci/qscistyle.h \
    Qsci/qscistyledtext.h

FORMS    += mainwindow.ui \
    createdbdialog.ui \
    createtabledialog.ui \
    loadtabledialog.ui

unix:!macx: LIBS += -L$$PWD/./ -lqscintilla2

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

unix:!macx: PRE_TARGETDEPS += $$PWD/./libqscintilla2.a

unix:!macx: LIBS += -L$$PWD/./ -lqscintillaplugin

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

unix:!macx: PRE_TARGETDEPS += $$PWD/./libqscintillaplugin.a
