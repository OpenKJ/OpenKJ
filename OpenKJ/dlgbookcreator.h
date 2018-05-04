#ifndef DLGBOOKCREATOR_H
#define DLGBOOKCREATOR_H

#include <QAbstractButton>
#include <QDialog>
#include <QTextDocument>
#include "settings.h"

namespace Ui {
class DlgBookCreator;
}

class DlgBookCreator : public QDialog
{
    Q_OBJECT

public:
    explicit DlgBookCreator(QWidget *parent = 0);
    ~DlgBookCreator();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);        
    void on_comboBoxSort_currentIndexChanged(int index);
    void on_btnGenerate_clicked();
    void on_cbxColumns_currentIndexChanged(int index);
    void on_cbxPageSize_currentIndexChanged(int index);
    void saveFontSettings();


private:
    Ui::DlgBookCreator *ui;
    Settings *settings;
    bool setupdone;
    QString htmlOut;
    QTextDocument doc;
    void writePdf(QString filename, int nCols = 2);
    QStringList getArtists();
    QStringList getTitles(QString artist);
};

#endif // DLGBOOKCREATOR_H
