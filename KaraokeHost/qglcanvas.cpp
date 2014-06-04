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

#include "qglcanvas.h"

#ifdef USE_GL
QGLCanvas::QGLCanvas(QWidget *parent) :
    QGLWidget(parent)
{
}
#else
QGLCanvas::QGLCanvas(QWidget *parent) :
    QWidget(parent)
{
}
#endif
void QGLCanvas::setImage(const QImage& image)
{
    img = image;
    update();
}

void QGLCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::HighQualityAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform, 1);
    p.drawImage(this->rect(), img);
    p.end();
}
