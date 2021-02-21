#ifndef DLGCUSTOMPATTERNS_H
#define DLGCUSTOMPATTERNS_H

#include <QDialog>
#include "tablemodelcustomnamingpatterns.h"

namespace Ui {
class DlgCustomPatterns;
}

class DlgCustomPatterns : public QDialog
{
    Q_OBJECT
    int selectedRow;
    Pattern selectedPattern;
    void evaluateRegEx();
public:
    explicit DlgCustomPatterns(QWidget *parent = 0);
    ~DlgCustomPatterns();

private slots:
    void on_btnClose_clicked();

    void on_tableViewPatterns_clicked(const QModelIndex &index);

    void on_lineEditDiscIdRegEx_textChanged(const QString &arg1);

    void on_lineEditArtistRegEx_textChanged(const QString &arg1);

    void on_lineEditTitleRegEx_textChanged(const QString &arg1);

    void on_lineEditFilenameExample_textChanged(const QString &arg1);

    void on_spinBoxDiscIdCaptureGrp_valueChanged(int arg1);

    void on_spinBoxArtistCaptureGrp_valueChanged(int arg1);

    void on_spinBoxTitleCaptureGrp_valueChanged(int arg1);

    void on_btnAdd_clicked();

    void on_btnDelete_clicked();

    void on_btnApplyChanges_clicked();

private:
    Ui::DlgCustomPatterns *ui;
    TableModelCustomNamingPatterns *patternsModel;
};

#endif // DLGCUSTOMPATTERNS_H
