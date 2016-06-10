#include "imagewidget.h"

ImageWidget::ImageWidget(QWidget *parent)
{
    Q_UNUSED(parent)
    this->setMinimumSize(1,1);
}

int ImageWidget::heightForWidth(int width) const
{
     return ((qreal)pix.height()*width)/pix.width();
}

QSize ImageWidget::sizeHint() const
{
    int w = this->width();
    return QSize( w, heightForWidth(w) );
}

void ImageWidget::setPixmap(const QPixmap &p)
{
    pix = p;
    QLabel::setPixmap(pix.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ImageWidget::resizeEvent(QResizeEvent *e)
{
    if(!pix.isNull())
        QLabel::setPixmap(pix.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
