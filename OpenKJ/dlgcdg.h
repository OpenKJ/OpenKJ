/*
 * Copyright (c) 2013-2019 Thomas Isaac Lightburn
 *
 *
 * This file is part of OpenKJ.
 *
 * OpenKJ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CDGWINDOW_H
#define CDGWINDOW_H

#include <QDialog>
#include <QFileInfoList>
#include <QMouseEvent>
#include "settings.h"
#include "scrolltext.h"
#include "cdgvideowidget.h"
#include <QTimer>
#include "abstractaudiobackend.h"

namespace Ui {
class DlgCdg;
}

class DlgCdg : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgCdg *ui;
    bool m_fullScreen;
    QRect m_lastSize;
    int vOffset;
    int hOffset;
    int vSizeAdjustment;
    int hSizeAdjustment;
    QTimer *fullScreenTimer;
    QTimer *slideShowTimer;
    bool showBgImage;
    QTimer *alertCountdownTimer;
    QTimer *oneSecTimer;
    int countdownPos;
    QTimer *buttonShowTimer;
    AbstractAudioBackend *kAudioBackend;
    AbstractAudioBackend *bAudioBackend;


public:
    explicit DlgCdg(AbstractAudioBackend *KaraokeBackend, AbstractAudioBackend *BreakBackend, QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~DlgCdg();
    void updateCDG(QImage image, bool overrideVisibleCheck = false);
    void makeFullscreen();
    void makeWindowed();
    void setTickerText(QString text);
    int getVOffset() { return vOffset; }
    int getHOffset() { return hOffset; }
    int getVAdjustment() { return vSizeAdjustment; }
    int getHAdjustment() { return hSizeAdjustment; }
    void setKAudioBackend(AbstractAudioBackend *value);
    void setBAudioBackend(AbstractAudioBackend *value);

protected:
    void mouseDoubleClickEvent(QMouseEvent *e);

private slots:
    void fullScreenTimerTimeout();
    void slideShowTimerTimeout();
    void alertFontChanged(QFont font);
    void mouseMove(QMouseEvent *event);
    void buttonShowTimerTimeout();
    void oneSecTimerTimeout();
    void on_btnToggleFullscreen_clicked();

public slots:
    void setFullScreen(bool fullscreen);
    void setFullScreenMonitor(int monitor);
    void tickerFontChanged();
    void tickerHeightChanged();
    void tickerSpeedChanged();
    void tickerTextColorChanged();
    void tickerBgColorChanged();
    void tickerEnableChanged();
    void cdgRemainFontChanged(QFont font);
    void cdgRemainTextColorChanged(QColor color);
    void cdgRemainBgColorChanged(QColor color);
    void setVOffset(int pixels);
    void setHOffset(int pixels);
    void setVSizeAdjustment(int pixels);
    void setHSizeAdjustment(int pixels);
    void setShowBgImage(bool show);
    void cdgSurfaceResized(QSize size);
    QFileInfoList getSlideShowImages();
    void setAlert(QString text);
    void showAlert(bool show);
    void setNextSinger(QString name);
    void setNextSong(QString song);
    void setCountdownSecs(int seconds);
    void countdownTimerTimeout();
    void alertBgColorChanged(QColor color);
    void alertTxtColorChanged(QColor color);
    void triggerBg();
    void cdgRemainEnabledChanged(bool enabled);
    void remainOffsetsChanged(int r, int b);



    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event);
};

#endif // CDGWINDOW_H
