#ifndef REGULARIMPORTDIALOG_H
#define REGULARIMPORTDIALOG_H

#include <QDialog>
#include <QStringList>
#include "rotationmodel.h"

namespace Ui {
class DlgRegularImport;
}

class DlgRegularImport : public QDialog
{
    Q_OBJECT

public:
    explicit DlgRegularImport(RotationModel *rotationModel, QWidget *parent = 0);
    ~DlgRegularImport();

private slots:
    void on_pushButtonSelectFile_clicked();
    void on_pushButtonClose_clicked();
    void on_pushButtonImport_clicked();
    void on_pushButtonImportAll_clicked();

private:
    Ui::DlgRegularImport *ui;
    QString curImportFile;
    QStringList loadSingerList(QString fileName);
    void importSinger(QString name);
    RotationModel *rotModel;
};

#endif // REGULARIMPORTDIALOG_H
