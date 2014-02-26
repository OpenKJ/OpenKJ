/*
 * Copyright (c) 2013-2014 Thomas Isaac Lightburn
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

#ifndef QGLCANVAS_H
#define QGLCANVAS_H

#include <QGLWidget>
#include <QPainter>

class QGLCanvas : public QGLWidget
{
    Q_OBJECT
public:
    explicit QGLCanvas(QWidget *parent = 0);
    void setImage(const QImage& image);
protected:
    void paintEvent(QPaintEvent*);
private:
    QImage img;

    // QGLWidget interface
protected:
    //void paintGL();
};

#endif // QGLCANVAS_H
