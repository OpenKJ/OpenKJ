#ifndef IDLEDETECT_H
#define IDLEDETECT_H

#include <QObject>
#include <QEvent>
#include <QTimer>

class IdleDetect : public QObject
{
    Q_OBJECT
public:
    explicit IdleDetect(QObject *parent = nullptr);
private:
    int idleMins;
    bool idle;
    QTimer *idleIncrement;
protected:
    bool eventFilter(QObject *obj, QEvent *ev);
private slots:
    void idleIncrementTimeout();
signals:
    void idleStateChanged(bool isIdle);
};

#endif // IDLEDETECT_H
