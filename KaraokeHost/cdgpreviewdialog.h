#ifndef CDGPREVIEWDIALOG_H
#define CDGPREVIEWDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QTemporaryDir>
#include "libCDG/include/libCDG.h"



namespace Ui {
class CdgPreviewDialog;
}

class CdgPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CdgPreviewDialog(QWidget *parent = 0);
    ~CdgPreviewDialog();
    void setZipFile(QString zipFile);

public slots:
    void preview();

private slots:
    void timerTimeout();

private:
    Ui::CdgPreviewDialog *ui;
    QString m_zipFile;
    QTimer *timer;
    QTemporaryDir *cdgTempDir;
    CDG *cdg;
    unsigned int cdgPosition;
};

#endif // CDGPREVIEWDIALOG_H
