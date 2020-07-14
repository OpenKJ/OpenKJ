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
#include <QTimer>
#include "audiobackendgstreamer.h"

namespace Ui {
class DlgCdg;
}

class DlgCdg : public QDialog
{
    Q_OBJECT

private:
    Ui::DlgCdg *ui;
    bool m_showBgImage{false};
    bool m_fullScreen{false};
    int m_countdownPos{0};
    QRect m_lastSize;
    QTimer m_timer1s;
    QTimer m_timerAlertCountdown;
    QTimer m_timerButtonShow;
    QTimer m_timerFullScreen;
    QTimer m_timerSlideShow;
    MediaBackend *m_kmb;
    MediaBackend *m_bmb;


public:
    explicit DlgCdg(MediaBackend *KaraokeBackend, MediaBackend *BreakBackend, QWidget *parent = nullptr, Qt::WindowFlags f = nullptr);
    ~DlgCdg();
    void setTickerText(QString text);
    void stopTicker();
    WId getCdgWinId();

protected:
    void mouseDoubleClickEvent(QMouseEvent *e);

private slots:
    void timerSlideShowTimeout();
    void alertFontChanged(QFont font);
    void mouseMove(QMouseEvent *event);
    void timer1sTimeout();
    void timerCountdownTimeout();
    void on_btnToggleFullscreen_clicked();
    void cdgOffsetsChanged();
    void cdgRemainFontChanged(QFont font);
    void cdgRemainTextColorChanged(QColor color);
    void tickerFontChanged();
    void tickerSpeedChanged();
    void tickerHeightChanged(const int &height);
    void tickerTextColorChanged();
    void tickerBgColorChanged();
    void tickerEnableChanged();
    void cdgRemainBgColorChanged(QColor color);
    QFileInfoList getSlideShowImages();
    void alertBgColorChanged(QColor color);
    void alertTxtColorChanged(QColor color);
    void cdgRemainEnabledChanged(bool enabled);

public slots:
    void setFullScreen(bool fullscreen);
    void setFullScreenMonitor(int monitor);
    void setShowBgImage(bool show);
    void showAlert(bool show);
    void setNextSinger(QString name);
    void setNextSong(QString song);
    void setCountdownSecs(int seconds);
    void triggerBg();

protected:
    void closeEvent(QCloseEvent *event);
};

#endif // CDGWINDOW_H
