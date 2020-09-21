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
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include "settings.h"
#include <QTimer>
#include "mediabackend.h"


class TransparentWidget : public QWidget
{
  Q_OBJECT
public:
    QLabel *label;
  TransparentWidget(QWidget *parent = 0)
    : QWidget(parent)
  {
    setWindowFlags(Qt::FramelessWindowHint);
    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    setContentsMargins(0,0,0,0);
    label = new QLabel(this);
    layout->addWidget(label);
    label->setMargin(0);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    label->setText("00:00");
    label->setAutoFillBackground(true);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

  }
  void setString(QString string) {
      label->setText(string);
  }
  void paintEvent(QPaintEvent *) override
  {
    QPainter painter(this);
    painter.fillRect (this->rect(), QColor(0, 0, 0, 0x20)); /* set transparent color*/
  }

  void mousePressEvent(QMouseEvent *event) override
  {
    if (event->button() == Qt::LeftButton) {
      m_startPoint = frameGeometry().topLeft() - event->globalPos();
    }
  }

  void mouseMoveEvent(QMouseEvent *event) override
  {
    this->move(event->globalPos() + m_startPoint);
  }

private:
  QPoint m_startPoint;

  // QWidget interface
protected:
  void moveEvent(QMoveEvent *event) override;
};



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
    QRect m_lastPos;
    QTimer m_timer1s;
    QTimer m_timerAlertCountdown;
    QTimer m_timerButtonShow;
    QTimer m_timerSlideShow;
    MediaBackend *m_kmb;
    MediaBackend *m_bmb;
    TransparentWidget *tWidget;


public:
    explicit DlgCdg(MediaBackend *KaraokeBackend, MediaBackend *BreakBackend, QWidget *parent = nullptr, Qt::WindowFlags f = nullptr);
    ~DlgCdg();
    void setTickerText(QString text);
    void stopTicker();
    WId getCdgWinId();

protected:
    void mouseDoubleClickEvent(QMouseEvent *e) override;

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
    void setShowBgImage(bool show);
    void showAlert(bool show);
    void setNextSinger(QString name);
    void setNextSong(QString song);
    void setCountdownSecs(int seconds);
    void triggerBg();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

    // QWidget interface
protected:
    void hideEvent(QHideEvent *event) override;
};

#endif // CDGWINDOW_H
