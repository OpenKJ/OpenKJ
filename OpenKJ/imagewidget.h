#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QResizeEvent>

class ImageWidget : public QLabel
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = 0);
    virtual int heightForWidth( int width ) const;
    virtual QSize sizeHint() const;
signals:

public slots:
    void setPixmap (const QPixmap & p);

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *e);

private:
    QPixmap pix;
};

#endif // IMAGEWIDGET_H
