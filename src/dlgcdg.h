/*
 * Copyright (c) 2013-2021 Thomas Isaac Lightburn
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
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include "settings.h"
#include <QTimer>
#include "mediabackend.h"
#include "videodisplay.h"
#include <QShortcut>
#include <memory>


class TransparentWidget : public QWidget {
Q_OBJECT
public:
    std::unique_ptr<QLabel> m_label;
    explicit TransparentWidget(QWidget *parent = nullptr);
    ~TransparentWidget() override;
    void setString(const QString &string) const;
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint m_startPoint;
    Settings m_settings;

public slots:

    void setTextColor(const QColor &color) const;
    void setBackgroundColor(const QColor &color) const;
    void setTextFont(const QFont &font);
    void resetPosition();

protected:
    void moveEvent(QMoveEvent *event) override;
};


namespace Ui {
    class DlgCdg;
}

class DlgCdg : public QDialog {
Q_OBJECT

private:
    std::unique_ptr<Ui::DlgCdg> ui;
    bool m_fullScreen{false};
    int m_countdownPos{0};
    int m_curSlideshowPos{-1};
    QRect m_lastSize;
    QTimer m_timer1s;
    QTimer m_timerAlertCountdown;
    QTimer m_timerButtonShow;
    QTimer m_timerSlideShow;
    MediaBackend &m_kmb;
    MediaBackend &m_bmb;
    std::unique_ptr<TransparentWidget> m_tWidget;
    Settings m_settings;

public:
    explicit DlgCdg(MediaBackend &KaraokeBackend, MediaBackend &BreakBackend, QWidget *parent = nullptr,
                    Qt::WindowFlags f = QFlags<Qt::WindowType>());
    ~DlgCdg() override;
    void setTickerText(const QString &text);
    void stopTicker();
    VideoDisplay *getVideoDisplay();
    VideoDisplay *getVideoDisplayBm();
    void slideShowMoveNext();
    TransparentWidget* durationWidget() {return m_tWidget.get(); }
    QFileInfoList getSlideShowImages();

public slots:
    void showAlert(bool show);
    void setNextSinger(const QString &name);
    void setNextSong(const QString &song);
    void setCountdownSecs(int seconds);
    void applyBackgroundImageMode();
    void timerSlideShowTimeout();
    void alertFontChanged(const QFont &font);
    void mouseMove(QMouseEvent *event);
    void timer1sTimeout();
    void timerCountdownTimeout();
    void btnToggleFullscreenClicked();
    void cdgOffsetsChanged();
    void tickerFontChanged();
    void tickerSpeedChanged();
    void tickerTextColorChanged();
    void tickerBgColorChanged();
    void tickerEnableChanged();
    void alertBgColorChanged(const QColor &color);
    void alertTxtColorChanged(const QColor &color);
    void setSlideshowInterval(int secs);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

    signals:
    void visibilityChanged(bool visible);
};

#endif // CDGWINDOW_H
