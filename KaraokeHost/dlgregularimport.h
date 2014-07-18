#ifndef REGULARIMPORTDIALOG_H
#define REGULARIMPORTDIALOG_H

#include <QDialog>

namespace Ui {
class DlgRegularImport;
}

class DlgRegularImport : public QDialog
{
    Q_OBJECT

public:
    explicit DlgRegularImport(QWidget *parent = 0);
    ~DlgRegularImport();

private slots:
    void on_pushButtonSelectFile_clicked();

    void on_pushButtonClose_clicked();

    void on_pushButtonImport_clicked();

    void on_pushButtonImportAll_clicked();

private:
    Ui::DlgRegularImport *ui;
    QString curImportFile;
    //void importSinger(QString name);
    //KhSong *findExactSongMatch(KhRegImportSong importSong);
};

#endif // REGULARIMPORTDIALOG_H
