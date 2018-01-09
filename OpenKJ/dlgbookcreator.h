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

    void on_fontCbxHeader_currentFontChanged(const QFont &f);
    
    void on_fontCbxItem_currentFontChanged(const QFont &f);
    
    void on_spinBoxSizeHeader_valueChanged(int arg1);
    
    void on_spinBoxSizeItem_valueChanged(int arg1);
    
    void on_checkBoxBoldHeader_stateChanged(int arg1);
    
    void on_checkBoxBoldItem_stateChanged(int arg1);    
        
    void on_comboBoxSort_currentIndexChanged(int index);
    
    void on_btnGenerate_clicked();

private:
    Ui::DlgBookCreator *ui;
    Settings *settings;
    void saveFontSettings();
    bool setupdone;
    QString htmlOut;
    QTextDocument doc;
    QString getTable();
    void writePdf(QString fileName);
};

#endif // DLGBOOKCREATOR_H
