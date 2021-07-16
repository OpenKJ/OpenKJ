#ifndef CDGDISPLAY_H
#define CDGDISPLAY_H

#include <QWidget>
#include <QBoxLayout>
#include <QResizeEvent>

class VideoDisplay : public QWidget
{
    Q_OBJECT
private:
    QPixmap m_currentBg;
    bool m_useDefaultBg{true};
    bool m_hasActiveVideo { false };
    bool m_fillOnPaint { false };
    bool m_repaintBackgroundOnce { false };

public:
    explicit VideoDisplay(QWidget *parent = nullptr);
    [[nodiscard]] bool hasActiveVideo() const { return m_hasActiveVideo; }

signals:
    void mouseMoveEvent(QMouseEvent *event) override;

public slots:
    void setBackground(const QPixmap &pixmap);
    void useDefaultBackground();
    void setHasActiveVideo(const bool &value);

    /**
     * @brief Fill with black on paint event when video is playing.
     * Video in HW mode seems to only paint black borders when the control is resized.
     * This causes some glitches in the monitor window when changing between tabs.
     * Set this property to start each paint event with a black fill.
     */
    void setFillOnPaint(const bool &value) { m_fillOnPaint = value; }
protected:
    void paintEvent(QPaintEvent *event) override;
};


#endif // CDGDISPLAY_H
