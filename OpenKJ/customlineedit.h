#ifndef CUSTOMLINEEDIT_H
#define CUSTOMLINEEDIT_H
#include <QLineEdit>

class CustomLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomLineEdit(QWidget *parent);

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event);
signals:
    void escapePressed();
};

#endif // CUSTOMLINEEDIT_H
