#ifndef DLGCDGVIDEO_H
#define DLGCDGVIDEO_H

#include <QDialog>
#include <QVideoFrame>

namespace Ui {
class DlgCdgVideo;
}

class DlgCdgVideo : public QDialog
{
    Q_OBJECT

public:
    explicit DlgCdgVideo(QWidget *parent = 0);
    ~DlgCdgVideo();

private:
    Ui::DlgCdgVideo *ui;

public slots:
    void present(QVideoFrame frame);
};

#endif // DLGCDGVIDEO_H
