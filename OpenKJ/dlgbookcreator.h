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

    void on_fontCbxArtist_currentFontChanged(const QFont &f);
    void on_fontCbxHeader_currentFontChanged(const QFont &f);
    void on_fontCbxTitle_currentFontChanged(const QFont &f);
    
    void on_spinBoxSizeArtist_valueChanged(int arg1);
    void on_spinBoxSizeHeader_valueChanged(int arg1);
    void on_spinBoxSizeTitle_valueChanged(int arg1);
    
    void on_checkBoxBoldArtist_stateChanged(int arg1);
    void on_checkBoxBoldHeader_stateChanged(int arg1);
    void on_checkBoxBoldTitle_stateChanged(int arg1);
        
    void on_comboBoxSort_currentIndexChanged(int index);
    
    void on_btnGenerate_clicked();

    void on_cbxColumns_currentIndexChanged(int index);

    void on_cbxPageSize_currentIndexChanged(int index);

    void on_doubleSpinBoxLeft_valueChanged(double arg1);

    void on_doubleSpinBoxRight_valueChanged(double arg1);

    void on_doubleSpinBoxTop_valueChanged(double arg1);

    void on_doubleSpinBoxBottom_valueChanged(double arg1);

    void on_lineEditHeaderText_editingFinished();

private:
    Ui::DlgBookCreator *ui;
    Settings *settings;
    void saveFontSettings();
    bool setupdone;
    QString htmlOut;
    QTextDocument doc;
    void writePdf(QString filename, int nCols = 2);
    QStringList getArtists();
    QStringList getTitles(QString artist);
};

#endif // DLGBOOKCREATOR_H
