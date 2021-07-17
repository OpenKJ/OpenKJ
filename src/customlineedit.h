#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H
#include <QLineEdit>

class CustomLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit CustomLineEdit(QWidget *parent = nullptr);

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event) override;
signals:
    void escapePressed();
};

#endif // CUSTOMLINEEDIT_H
