#ifndef DLGCUSTOMPATTERNS_H
#define DLGCUSTOMPATTERNS_H

#include <QDialog>
#include <memory>
#include "src/models/tablemodelcustomnamingpatterns.h"

namespace Ui {
    class DlgCustomPatterns;
}

class DlgCustomPatterns : public QDialog {
Q_OBJECT
public:
    explicit DlgCustomPatterns(QWidget *parent = nullptr);
    ~DlgCustomPatterns() override;

private slots:
    void evaluateRegEx();
    void btnCloseClicked();
    void tableViewPatternsClicked(const QModelIndex &index);
    void btnAddClicked();
    void btnDeleteClicked();
    void btnApplyChangesClicked();

private:
    Pattern m_selectedPattern;
    std::unique_ptr<Ui::DlgCustomPatterns> ui;
    TableModelCustomNamingPatterns m_patternsModel;
};

#endif // DLGCUSTOMPATTERNS_H
