#ifndef SOUNDFXBUTTON_H
#define SOUNDFXBUTTON_H

#include <QPushButton>
#include <QVariant>

class SoundFxButton : public QPushButton
{
private:
    QVariant data;
public:
    SoundFxButton();
    void setButtonData(QVariant data);
    QVariant buttonData() { return data; }

    // QWidget interface
protected:
    void mouseReleaseEvent(QMouseEvent *event);
};

#endif // SOUNDFXBUTTON_H
