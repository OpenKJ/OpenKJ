/*
 * Copyright (c) 2013-2017 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


//#include <QtGui/QApplication>
#include <QApplication>
#include "mainwindow.h"
#include <QStyleFactory>
#include <QSplashScreen>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include "settings.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Settings okjSettings;
    if (okjSettings.theme() == 1)
    {
        //a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
        QPalette palette;
        a.setStyle(QStyleFactory::create("Fusion"));
        palette.setColor(QPalette::Window,QColor(53,53,53));
        palette.setColor(QPalette::WindowText,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
        palette.setColor(QPalette::Base,QColor(42,42,42));
        palette.setColor(QPalette::AlternateBase,QColor(66,66,66));
        palette.setColor(QPalette::ToolTipBase,Qt::white);
        palette.setColor(QPalette::ToolTipText,QColor(53,53,53));
        palette.setColor(QPalette::Text,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
        palette.setColor(QPalette::Dark,QColor(35,35,35));
        palette.setColor(QPalette::Shadow,QColor(20,20,20));
        palette.setColor(QPalette::Button,QColor(53,53,53));
        palette.setColor(QPalette::ButtonText,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
        palette.setColor(QPalette::BrightText,Qt::red);
        palette.setColor(QPalette::Link,QColor(42,130,218));
        palette.setColor(QPalette::Highlight,QColor(42,130,218));
        palette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
        palette.setColor(QPalette::HighlightedText,Qt::white);
        palette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));
        a.setPalette(palette);
        a.setFont(okjSettings.applicationFont(), "QWidget");

    }
    else if (okjSettings.theme() == 2)
    {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        a.setFont(okjSettings.applicationFont(), "QWidget");
    }
//    QFile file(":/QTDark.css");
//    QString stylesheet;
//    QString line;
//    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
//        QTextStream stream(&file);
//        stylesheet = stream.readAll();
//    }
//    file.close();
//    stylesheet = "* { background: #191919; color: #DDDDDD; border: 1px solid #5A5A5A;}";
//    a.setStyleSheet(stylesheet);
    MainWindow w;
    if (!w.isSingleInstance())
    {
        QMessageBox msgBox;
        msgBox.setText("OpenKJ is already running!");
        msgBox.setInformativeText("In order to protect the database, you can only run one instance of OpenKJ at a time.\nExiting now.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return 1;
    }
    w.show();

    return a.exec();
}
