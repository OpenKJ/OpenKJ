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

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    QApplication::setStyle(QStyleFactory::create("Fusion"));
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
