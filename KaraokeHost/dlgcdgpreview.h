#ifndef CDGPREVIEWDIALOG_H
#define CDGPREVIEWDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QTemporaryDir>
#include "libCDG/include/libCDG.h"



namespace Ui {
class DlgCdgPreview;
}

class DlgCdgPreview : public QDialog
{
    Q_OBJECT

public:
    explicit DlgCdgPreview(QWidget *parent = 0);
    ~DlgCdgPreview();
    void setSourceFile(QString srcFile);

public slots:
    void preview();

private slots:
    void timerTimeout();

    void on_pushButtonClose_clicked();

private:
    Ui::DlgCdgPreview *ui;
    QString m_srcFile;
    QTimer *timer;
    QTemporaryDir *cdgTempDir;
    CDG *cdg;
    unsigned int cdgPosition;
};

#endif // CDGPREVIEWDIALOG_H
