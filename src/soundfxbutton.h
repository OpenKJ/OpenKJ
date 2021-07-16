#ifndef SOUNDFXBUTTON_H
#define SOUNDFXBUTTON_H

#include <QPushButton>
#include <QVariant>

class SoundFxButton : public QPushButton
{
private:
    QVariant data;
public:
    SoundFxButton() = default;
    SoundFxButton(const QVariant &data, const QString &label);
    void setButtonData(const QVariant &data);
    QVariant buttonData() { return data; }

    // QWidget interface
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // SOUNDFXBUTTON_H
