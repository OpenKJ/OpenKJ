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

#include <qglobal.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDesktopWidget>
#include <QMenu>
#include <QInputDialog>
#include <QFileDialog>
#include <QImageReader>
#include <QDesktopServices>
#include "mzarchive.h"
#include "tagreader.h"
#include "dlgeditsong.h"
#include "soundfxbutton.h"
#include "src/models/tableviewtooltipfilter.h"
#include <tickernew.h>
#include "dbupdater.h"
#include "okjutil.h"
#include <algorithm>
#include "dlgaddsong.h"

#ifdef _MSC_VER
#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>
#endif

extern Settings settings;
OKJSongbookAPI *songbookApi;

// for some reason clang-tidy is choking on this function
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"

void MainWindow::addSfxButton(const QString &filename, const QString &label, const bool reset) {
    static int numButtons = 0;
    if (reset)
        numButtons = 0;
    logger->info("{} sfxButtonGrid contains {} children", m_loggingPrefix, numButtons);
    int col{0};
    if (numButtons % 2)
        col = 1;
    int row = numButtons / 2;
    auto button = new SoundFxButton(filename, label);
    ui->sfxButtonGrid->addWidget(button, row, col);
    connect(button, &SoundFxButton::clicked, this, &MainWindow::sfxButtonPressed);
    connect(button, &SoundFxButton::customContextMenuRequested, this,
            &MainWindow::sfxButton_customContextMenuRequested);
    numButtons++;
}

#pragma clang diagnostic pop

void MainWindow::refreshSfxButtons() {
    QLayoutItem *item;
    while ((item = ui->sfxButtonGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    SfxEntryList list = settings.getSfxEntries();
    bool first = true;
    for (const auto &entry : list) {
        if (first) {
            first = false;
            addSfxButton(entry.path, entry.name, true);
            continue;
        }
        addSfxButton(entry.path, entry.name);
    }
}

void MainWindow::updateIcons() {
    QString thm = (settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
    ui->buttonClearRotation->setIcon(QIcon(thm + "actions/22/edit-clear-all.svg"));
    ui->buttonAddSinger->setIcon(QIcon(thm + "actions/22/list-add-user.svg"));
    ui->buttonRegulars->setIcon(QIcon(thm + "actions/22/user-others.svg"));
    ui->btnRotTop->setIcon(QIcon(thm + "actions/22/go-top.svg"));
    ui->btnRotUp->setIcon(QIcon(thm + "actions/22/go-up.svg"));
    ui->btnRotBottom->setIcon(QIcon(thm + "actions/22/go-bottom.svg"));
    ui->btnRotDown->setIcon(QIcon(thm + "actions/22/go-down.svg"));
    ui->pushButton->setIcon(QIcon(thm + "actions/22/edit-find.svg"));
    ui->buttonClearQueue->setIcon(QIcon(thm + "actions/22/edit-clear-all.svg"));
    ui->btnQTop->setIcon(QIcon(thm + "actions/22/go-top.svg"));
    ui->btnQUp->setIcon(QIcon(thm + "actions/22/go-up.svg"));
    ui->btnQBottom->setIcon(QIcon(thm + "actions/22/go-bottom.svg"));
    ui->btnQDown->setIcon(QIcon(thm + "actions/22/go-down.svg"));
    ui->buttonPause->setIcon(QIcon(thm + "actions/22/media-playback-pause.svg"));
    ui->buttonStop->setIcon(QIcon(thm + "actions/22/media-playback-stop.svg"));
    ui->labelVolume->setPixmap(QPixmap(thm + "actions/16/player-volume.svg"));
    ui->pushButtonTempoDn->setIcon(QIcon(thm + "actions/22/downindicator.svg"));
    ui->pushButtonTempoUp->setIcon(QIcon(thm + "actions/22/upindicator.svg"));
    ui->pushButtonKeyDn->setIcon(QIcon(thm + "actions/22/downindicator.svg"));
    ui->pushButtonKeyUp->setIcon(QIcon(thm + "actions/22/upindicator.svg"));
    ui->btnSfxStop->setIcon(QIcon(thm + "actions/22/media-playback-stop.svg"));

    ui->buttonBmPause->setIcon(QIcon(thm + "actions/22/media-playback-pause.svg"));
    ui->buttonBmStop->setIcon(QIcon(thm + "actions/22/media-playback-stop.svg"));
    ui->btnPlTop->setIcon(QIcon(thm + "actions/22/go-top.svg"));
    ui->btnPlUp->setIcon(QIcon(thm + "actions/22/go-up.svg"));
    ui->btnPlBottom->setIcon(QIcon(thm + "actions/22/go-bottom.svg"));
    ui->btnPlDown->setIcon(QIcon(thm + "actions/22/go-down.svg"));
    ui->labelVolumeBm->setPixmap(QPixmap(thm + "actions/16/player-volume.svg"));
    ui->buttonBmSearch->setIcon(QIcon(thm + "actions/22/edit-find.svg"));

    requestsDialog->updateIcons();
    connect(&settings, &Settings::treatAllSingersAsRegsChanged, this, &MainWindow::treatAllSingersAsRegsChanged);
    treatAllSingersAsRegsChanged(settings.treatAllSingersAsRegs());
}

void MainWindow::setupShortcuts() {

    scutKSelectNextSinger = new QShortcut(settings.loadShortcutKeySequence("kSelectNextSinger"), this, nullptr, nullptr,
                                          Qt::ApplicationShortcut);

    connect(scutKSelectNextSinger, &QShortcut::activated, [&]() {
        int nextSinger{-1};
        QString nextSongPath;
        bool empty{false};
        int curSingerId{rotModel.currentSinger()};
        int curPos{rotModel.getSingerPosition(curSingerId)};
        if (curSingerId == -1)
            curPos = rotModel.rowCount() - 1;
        int loops = 0;
        while ((nextSongPath == "") && (!empty)) {
            if (loops > rotModel.rowCount()) {
                empty = true;
            } else {
                if (++curPos >= rotModel.rowCount()) {
                    curPos = 0;
                }
                nextSinger = rotModel.singerIdAtPosition(curPos);
                nextSongPath = rotModel.nextSongPath(nextSinger);
                loops++;
            }
        }
        if (empty) {
            QMessageBox::information(this, "Unable to select next",
                                     "Sorry, no unsung karaoke songs are currently in any singer's queue");
            return;
        }
        ui->tableViewRotation->clearSelection();
        ui->tableViewRotation->selectRow(rotModel.getSingerPosition(nextSinger));
    });

    scutKPlayNextUnsung = new QShortcut(settings.loadShortcutKeySequence("kPlayNextUnsung"), this, nullptr, nullptr,
                                        Qt::ApplicationShortcut);

    connect(scutKPlayNextUnsung, &QShortcut::activated, [&]() {
        if (auto state = kMediaBackend.state(); state == MediaBackend::PlayingState ||
                                                state == MediaBackend::PausedState) {
            if (settings.showSongInterruptionWarning()) {
                QMessageBox msgBox(this);
                auto *cb = new QCheckBox("Show this warning in the future");
                cb->setChecked(settings.showSongInterruptionWarning());
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Interrupt currenly playing karaoke song?");
                msgBox.setInformativeText(
                        "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
                QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.setCheckBox(cb);
                connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSongInterruptionWarning);
                msgBox.exec();
                if (msgBox.clickedButton() != yesButton) {
                    return;
                }
            }
            kMediaBackend.stop();
        }
        int nextSinger{-1};
        QString nextSongPath;
        bool empty{false};
        int curSingerId{rotModel.currentSinger()};
        int curPos{rotModel.getSingerPosition(curSingerId)};
        if (curSingerId == -1)
            curPos = rotModel.rowCount() - 1;
        int loops = 0;
        while ((nextSongPath == "") && (!empty)) {
            if (loops > rotModel.rowCount()) {
                empty = true;
            } else {
                if (++curPos >= rotModel.rowCount()) {
                    curPos = 0;
                }
                nextSinger = rotModel.singerIdAtPosition(curPos);
                nextSongPath = rotModel.nextSongPath(nextSinger);
                loops++;
            }
        }
        if (empty) {
            QMessageBox::information(this, "Unable to play next",
                                     "Sorry, no unsung karaoke songs are currently in any singer's queue");
            return;
        }
        curSinger = rotModel.getSingerName(nextSinger);
        curArtist = rotModel.nextSongArtist(nextSinger);
        curTitle = rotModel.nextSongTitle(nextSinger);

        if (settings.treatAllSingersAsRegs() || rotModel.singerIsRegular(nextSinger)) {
            historySongsModel.saveSong(
                    curSinger,
                    nextSongPath,
                    curArtist,
                    curTitle,
                    rotModel.nextSongSongId(nextSinger),
                    rotModel.nextSongKeyChg(nextSinger)
            );
        }
        karaokeSongsModel.updateSongHistory(karaokeSongsModel.getIdForPath(nextSongPath));
        play(nextSongPath);
        kMediaBackend.setPitchShift(rotModel.nextSongKeyChg(nextSinger));
        qModel.setPlayed(rotModel.nextSongQueueId(nextSinger));
        rotModel.setCurrentSinger(nextSinger);
        rotDelegate.setCurrentSinger(nextSinger);
        if (settings.rotationAltSortOrder()) {
            auto curSingerPos = rotModel.getSingerPosition(nextSinger);
            m_curSingerOriginalPosition = curSingerPos;
            if (curSingerPos != 0)
                rotModel.singerMove(curSingerPos, 0);
        }
        ui->labelArtist->setText(curArtist);
        ui->labelTitle->setText(curTitle);
        ui->labelSinger->setText(curSinger);
        ui->tableViewRotation->clearSelection();
        ui->tableViewRotation->selectRow(rotModel.getSingerPosition(rotModel.currentSinger()));
    });

    scutAddSinger = new QShortcut(settings.loadShortcutKeySequence("addSinger"), this, nullptr, nullptr,
                                  Qt::ApplicationShortcut);
    connect(scutAddSinger, &QShortcut::activated, this, &MainWindow::on_buttonAddSinger_clicked);

    scutBFfwd = new QShortcut(settings.loadShortcutKeySequence("bFfwd"), this, nullptr, nullptr,
                              Qt::ApplicationShortcut);
    connect(scutBFfwd, &QShortcut::activated, [&]() {
        auto mediaState = bmMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            auto curPos = bmMediaBackend.position();
            auto duration = bmMediaBackend.duration();
            if (curPos + 5000 < duration)
                bmMediaBackend.setPosition(curPos + 5000);
        }
    });

    scutBPause = new QShortcut(settings.loadShortcutKeySequence("bPause"), this, nullptr, nullptr,
                               Qt::ApplicationShortcut);
    connect(scutBPause, &QShortcut::activated, [&]() {
        auto mediaState = bmMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState) {
            bmMediaBackend.pause();
            ui->buttonBmPause->setChecked(true);
        } else if (mediaState == MediaBackend::PausedState) {
            bmMediaBackend.play();
            ui->buttonBmPause->setChecked(false);
        }
    });

    scutBRestartSong = new QShortcut(settings.loadShortcutKeySequence("bRestartSong"), this, nullptr, nullptr,
                                     Qt::ApplicationShortcut);
    connect(scutBRestartSong, &QShortcut::activated, [&]() {
        auto mediaState = bmMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            bmMediaBackend.setPosition(0);
        }
    });

    scutBRwnd = new QShortcut(settings.loadShortcutKeySequence("bRwnd"), this, nullptr, nullptr,
                              Qt::ApplicationShortcut);
    connect(scutBRwnd, &QShortcut::activated, [&]() {
        auto mediaState = bmMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            auto curPos = bmMediaBackend.position();
            if (curPos - 5000 > 0)
                bmMediaBackend.setPosition(curPos - 5000);
            else
                bmMediaBackend.setPosition(0);
        }
    });

    scutBStop = new QShortcut(settings.loadShortcutKeySequence("bStop"), this, nullptr, nullptr,
                              Qt::ApplicationShortcut);
    connect(scutBStop, &QShortcut::activated, [&]() {
        bmMediaBackend.stop();
    });

    scutBVolDn = new QShortcut(settings.loadShortcutKeySequence("bVolDn"), this, nullptr, nullptr,
                               Qt::ApplicationShortcut);
    connect(scutBVolDn, &QShortcut::activated, [&]() {
        int curVol = bmMediaBackend.getVolume();
        if (curVol > 0)
            bmMediaBackend.setVolume(curVol - 1);
    });

    scutBVolMute = new QShortcut(settings.loadShortcutKeySequence("bVolMute"), this, nullptr, nullptr,
                                 Qt::ApplicationShortcut);
    connect(scutBVolMute, &QShortcut::activated, [&]() {
        bmMediaBackend.setMuted(!bmMediaBackend.isMuted());
    });

    scutBVolUp = new QShortcut(settings.loadShortcutKeySequence("bVolUp"), this, nullptr, nullptr,
                               Qt::ApplicationShortcut);
    connect(scutBVolUp, &QShortcut::activated, [&]() {
        int curVol = bmMediaBackend.getVolume();
        if (curVol < 100)
            bmMediaBackend.setVolume(curVol + 1);
    });

    scutJumpToSearch = new QShortcut(settings.loadShortcutKeySequence("jumpToSearch"), this, nullptr, nullptr,
                                     Qt::ApplicationShortcut);
    connect(scutJumpToSearch, &QShortcut::activated, [&]() {
        activateWindow();
        if (!hasFocus() && !ui->lineEdit->hasFocus())
            setFocus();
        if (ui->lineEdit->hasFocus())
            ui->lineEdit->clear();
        else
            ui->lineEdit->setFocus();
    });

    scutKFfwd = new QShortcut(settings.loadShortcutKeySequence("kFfwd"), this, nullptr, nullptr,
                              Qt::ApplicationShortcut);
    connect(scutKFfwd, &QShortcut::activated, [&]() {
        auto mediaState = kMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            auto curPos = kMediaBackend.position();
            auto duration = kMediaBackend.duration();
            if (curPos + 5000 < duration)
                kMediaBackend.setPosition(curPos + 5000);
        }
    });

    scutKPause = new QShortcut(settings.loadShortcutKeySequence("kPause"), this, nullptr, nullptr,
                               Qt::ApplicationShortcut);
    connect(scutKPause, &QShortcut::activated, this, &MainWindow::on_buttonPause_clicked);

    scutKRestartSong = new QShortcut(settings.loadShortcutKeySequence("kRestartSong"), this, nullptr, nullptr,
                                     Qt::ApplicationShortcut);
    connect(scutKRestartSong, &QShortcut::activated, [&]() {
        auto mediaState = kMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            kMediaBackend.setPosition(0);
        }
    });


    scutKRwnd = new QShortcut(settings.loadShortcutKeySequence("kRwnd"), this, nullptr, nullptr,
                              Qt::ApplicationShortcut);
    connect(scutKRwnd, &QShortcut::activated, [&]() {
        auto mediaState = kMediaBackend.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            auto curPos = kMediaBackend.position();
            if (curPos - 5000 > 0)
                kMediaBackend.setPosition(curPos - 5000);
            else
                kMediaBackend.setPosition(0);
        }
    });

    scutKStop = new QShortcut(settings.loadShortcutKeySequence("kStop"), this, nullptr, nullptr,
                              Qt::ApplicationShortcut);
    connect(scutKStop, &QShortcut::activated, [&]() {
        kMediaBackend.stop();
    });

    scutKVolDn = new QShortcut(settings.loadShortcutKeySequence("kVolDn"), this, nullptr, nullptr,
                               Qt::ApplicationShortcut);
    connect(scutKVolDn, &QShortcut::activated, [&]() {
        int curVol = kMediaBackend.getVolume();
        if (curVol > 0)
            kMediaBackend.setVolume(curVol - 1);
    });

    scutKVolMute = new QShortcut(settings.loadShortcutKeySequence("kVolMute"), this, nullptr, nullptr,
                                 Qt::ApplicationShortcut);
    connect(scutKVolMute, &QShortcut::activated, [&]() {
        kMediaBackend.setMuted(!kMediaBackend.isMuted());
    });

    scutKVolUp = new QShortcut(settings.loadShortcutKeySequence("kVolUp"), this, nullptr, nullptr,
                               Qt::ApplicationShortcut);
    connect(scutKVolUp, &QShortcut::activated, [&]() {
        int curVol = kMediaBackend.getVolume();
        if (curVol < 100)
            kMediaBackend.setVolume(curVol + 1);
    });

    scutLoadRegularSinger = new QShortcut(settings.loadShortcutKeySequence("loadRegularSinger"), this, nullptr, nullptr,
                                          Qt::ApplicationShortcut);
    connect(scutLoadRegularSinger, &QShortcut::activated, this, &MainWindow::on_buttonRegulars_clicked);

    scutToggleSingerWindow = new QShortcut(settings.loadShortcutKeySequence("toggleSingerWindow"), this, nullptr,
                                           nullptr, Qt::ApplicationShortcut);
    connect(scutToggleSingerWindow, &QShortcut::activated, [&]() {
        if (cdgWindow->isVisible())
            cdgWindow->hide();
        else
            cdgWindow->show();
    });

    scutRequests = new QShortcut(settings.loadShortcutKeySequence("showIncomingRequests"), this, nullptr, nullptr,
                                 Qt::ApplicationShortcut);
    connect(scutRequests, &QShortcut::activated, this, &MainWindow::on_pushButtonIncomingRequests_clicked);

    scutDeleteSinger = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableViewRotation, nullptr, nullptr,
                                     Qt::WidgetShortcut);
    connect(scutDeleteSinger, &QShortcut::activated, [&]() {
        auto indexes = ui->tableViewRotation->selectionModel()->selectedRows(0);
        std::vector<int> singerIds;
        std::for_each(indexes.begin(), indexes.end(), [&](auto index) {
            singerIds.emplace_back(index.data(Qt::UserRole).toInt());
        });
        if (singerIds.empty())
            return;
        if (settings.showSingerRemovalWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings.showSingerRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            if (singerIds.size() == 1) {
                msgBox.setText("Are you sure you want to remove this singer?");
                msgBox.setInformativeText(
                        "Unless this singer is a tracked regular, you will be unable retrieve any queue data for this singer once they are deleted.");
            } else {
                msgBox.setText("Are you sure you want to remove these singers?");
                msgBox.setInformativeText(
                        "Unless these singers are tracked regulars, you will be unable retrieve any queue data for them once they are deleted.");
            }
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSingerRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }

        qModel.loadSinger(-1);
        std::for_each(singerIds.begin(), singerIds.end(), [&](auto singerId) {
            if (rotModel.currentSinger() == singerId) {
                rotModel.setCurrentSinger(-1);
                rotDelegate.setCurrentSinger(-1);
            }
            rotModel.singerDelete(singerId);
        });
        ui->tableViewRotation->clearSelection();
        ui->tableViewQueue->clearSelection();
    });

    scutDeleteSong = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableViewQueue, nullptr, nullptr,
                                   Qt::WidgetShortcut);
    connect(scutDeleteSong, &QShortcut::activated, [&]() {
        auto indexes = ui->tableViewQueue->selectionModel()->selectedRows(0);
        bool containsUnplayed{false};
        std::vector<int> songIds;
        std::for_each(indexes.begin(), indexes.end(), [&](auto index) {
            songIds.emplace_back(index.data().toInt());
            if (!qModel.getPlayed(index.data().toInt()))
                containsUnplayed = true;
        });
        if ((settings.showQueueRemovalWarning()) && containsUnplayed) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings.showQueueRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Removing un-played song from queue");
            msgBox.setInformativeText("This song has not been played yet, are you sure you want to remove it?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowQueueRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
                return;
        }
        std::for_each(songIds.begin(), songIds.end(), [&](auto songId) {
            qModel.remove(songId);
        });
    });

    scutDeletePlSong = new QShortcut(QKeySequence(QKeySequence::Delete), ui->tableViewBmPlaylist, nullptr, nullptr,
                                     Qt::WidgetShortcut);
    connect(scutDeletePlSong, &QShortcut::activated, [&]() {
        auto rows = ui->tableViewBmPlaylist->selectionModel()->selectedRows(0);
        std::vector<int> positions;
        bool curPlayingSelected{false};
        std::for_each(rows.begin(), rows.end(), [&](auto index) {
            positions.emplace_back(index.row());
            if (playlistSongsModel.isCurrentlyPlayingSong(index.data(Qt::UserRole).toInt()))
                curPlayingSelected = true;
        });
        auto state = bmMediaBackend.state();
        if (curPlayingSelected && (state == MediaBackend::PlayingState || state == MediaBackend::PausedState)) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Unable to remove");
            msgBox.setText("The playlist song you are trying to remove is currently playing and can not be removed.");
            msgBox.exec();
            return;
        }
        std::sort(positions.begin(), positions.end());
        std::reverse(positions.begin(), positions.end());
        std::for_each(positions.begin(), positions.end(), [&](auto position) {
            playlistSongsModel.deleteSong(position);
        });
        if (curPlayingSelected) {
            playlistSongsModel.setCurrentPosition(-1);
        }
        playlistSongsModel.savePlaylistChanges();
        if (state != MediaBackend::PlayingState && state != MediaBackend::PausedState)
            return;
        if (ui->checkBoxBmBreak->isChecked())
            ui->labelBmNext->setText("None - Breaking after current song");
        else {
            auto nextSong = playlistSongsModel.getNextPlSong();
            if (nextSong.has_value())
                ui->labelBmNext->setText(nextSong->get().artist + " - " + nextSong->get().title);
            else
                ui->labelBmNext->setText("None - Breaking after current song");
        }
    });

    connect(&settings, &Settings::shortcutsChanged, this, &MainWindow::shortcutsUpdated);
}

void MainWindow::shortcutsUpdated() {
    scutKSelectNextSinger->setKey(settings.loadShortcutKeySequence("kSelectNextSinger"));
    scutKPlayNextUnsung->setKey(settings.loadShortcutKeySequence("kPlayNextUnsung"));
    scutAddSinger->setKey(settings.loadShortcutKeySequence("addSinger"));
    scutBFfwd->setKey(settings.loadShortcutKeySequence("bFfwd"));
    scutBPause->setKey(settings.loadShortcutKeySequence("bPause"));
    scutBRestartSong->setKey(settings.loadShortcutKeySequence("bRestartSong"));
    scutBRwnd->setKey(settings.loadShortcutKeySequence("bRwnd"));
    scutBStop->setKey(settings.loadShortcutKeySequence("bStop"));
    scutBVolDn->setKey(settings.loadShortcutKeySequence("bVolDn"));
    scutBVolMute->setKey(settings.loadShortcutKeySequence("bVolMute"));
    scutBVolUp->setKey(settings.loadShortcutKeySequence("bVolUp"));
    scutJumpToSearch->setKey(settings.loadShortcutKeySequence("jumpToSearch"));
    scutKFfwd->setKey(settings.loadShortcutKeySequence("kFfwd"));
    scutKPause->setKey(settings.loadShortcutKeySequence("kPause"));
    scutKRestartSong->setKey(settings.loadShortcutKeySequence("kRestartSong"));
    scutKRwnd->setKey(settings.loadShortcutKeySequence("kRwnd"));
    scutKStop->setKey(settings.loadShortcutKeySequence("kStop"));
    scutKVolDn->setKey(settings.loadShortcutKeySequence("kVolDn"));
    scutKVolMute->setKey(settings.loadShortcutKeySequence("kVolMute"));
    scutKVolUp->setKey(settings.loadShortcutKeySequence("kVolUp"));
    scutLoadRegularSinger->setKey(settings.loadShortcutKeySequence("loadRegularSinger"));
    scutRequests->setKey(settings.loadShortcutKeySequence("showIncomingRequests"));
    scutToggleSingerWindow->setKey(settings.loadShortcutKeySequence("toggleSingerWindow"));
}

void MainWindow::treatAllSingersAsRegsChanged(bool enabled) {
    if (enabled) {
        ui->tableViewRotation->hideColumn(TableModelRotation::COL_REGULAR);
        if (ui->tabWidgetQueue->count() == 1)
            ui->tabWidgetQueue->addTab(historyTabWidget, "History");
    } else {
        ui->tableViewRotation->showColumn(TableModelRotation::COL_REGULAR);
        int curSelSingerId{-1};
        if (ui->tableViewRotation->selectionModel()->selectedRows().count() > 1) {
            curSelSingerId = ui->tableViewRotation->selectionModel()->selectedRows(0).at(
                    TableModelRotation::COL_ID).data(Qt::UserRole).toInt();
        }
        if (!rotModel.singerIsRegular(curSelSingerId) && ui->tabWidgetQueue->count() == 2)
            ui->tabWidgetQueue->removeTab(1);
    }
    resizeRotation();
}

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow) {
    logger = spdlog::get("logger");
#ifdef _MSC_VER
    timeBeginPeriod(1);
#endif
    shop = new SongShop(this);
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
    ui->setupUi(this);
    setMouseTracking(true);
    ui->tableViewBmPlaylist->setMouseTracking(true);
    historyTabWidget = ui->tabWidgetQueue->widget(1);
    ui->actionShow_Debug_Log->setChecked(settings.logShow());
#ifdef Q_OS_WIN
    ui->sliderBmPosition->setMaximumHeight(12);
    ui->sliderBmVolume->setMaximumWidth(12);
    ui->sliderProgress->setMaximumHeight(12);
#endif
    QDir okjDataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!okjDataDir.exists()) {
        okjDataDir.mkpath(okjDataDir.absolutePath());
    }
    if (settings.theme() != 0) {
        ui->pushButtonIncomingRequests->setStyleSheet("");
        update();
    }
    ui->videoPreviewBm->hide();
    ui->pushButtonKeyDn->setEnabled(false);
    ui->pushButtonKeyUp->setEnabled(false);
    ui->pushButtonTempoDn->setEnabled(false);
    ui->pushButtonTempoUp->setEnabled(false);
    songbookApi = new OKJSongbookAPI(this);
    int initialKVol = settings.audioVolume();
    int initialBMVol = settings.bmVolume();
    logger->debug("{} Initial volumes - K: {} - BM {}",m_loggingPrefix, initialKVol, initialBMVol);
    QTimer::singleShot(250, [&]() {
        settings.restoreWindowState(this);
    });
    dbInit(okjDataDir);
    setupShortcuts();
    karaokeSongsModel.loadData();
    rotModel.loadData();
    ui->comboBoxHistoryDblClick->addItems(QStringList{"Adds to queue", "Plays song"});
    ui->comboBoxHistoryDblClick->setCurrentIndex(settings.historyDblClickAction());
    ui->tabWidgetQueue->setCurrentIndex(0);
    connect(ui->comboBoxHistoryDblClick, QOverload<int>::of(&QComboBox::currentIndexChanged), &settings,
            &Settings::setHistoryDblClickAction);
    ui->tableViewHistory->setModel(&historySongsModel);
    ui->tableViewHistory->hideColumn(0);
    ui->tableViewHistory->hideColumn(1);
    ui->tableViewHistory->hideColumn(2);
    ui->tableViewHistory->sortByColumn(3, Qt::AscendingOrder);
    ui->comboBoxSearchType->addItems({QString("All"), QString("Artist"), QString("Title")});
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewRotation->setModel(&rotModel);
    ui->tableViewRotation->setItemDelegate(&rotDelegate);
    ui->tableViewRotation->hideColumn(TableModelRotation::COL_ADDTS);
    ui->tableViewRotation->hideColumn(TableModelRotation::COL_POSITION);
    if (settings.treatAllSingersAsRegs())
        ui->tableViewRotation->hideColumn(TableModelRotation::COL_REGULAR);
    if (settings.rotationShowNextSong()) {
        ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_NAME,
                                                                        QHeaderView::Interactive);
        ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_NEXT_SONG,
                                                                        QHeaderView::Stretch);
    } else {
        ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_NAME,
                                                                        QHeaderView::Stretch);
        ui->tableViewRotation->hideColumn(TableModelRotation::COL_NEXT_SONG);
    }
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_ID,
                                                                    QHeaderView::ResizeToContents);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_DELETE,
                                                                    QHeaderView::ResizeToContents);
    ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_REGULAR,
                                                                    QHeaderView::ResizeToContents);
    if (settings.rotationShowNextSong())
        settings.restoreColumnWidths(ui->tableViewRotation);
    ui->tableViewQueue->setModel(&qModel);
    ui->tableViewQueue->setItemDelegate(&qDelegate);
    ui->tableViewQueue->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewQueue));
    ui->labelNoSinger->setVisible(true);
    autosizeQueue();
    ui->tabWidgetQueue->setVisible(false);
    khTmpDir = new QTemporaryDir();
    dbDialog = new DlgDatabase(karaokeSongsModel, this);
    dlgKeyChange = new DlgKeyChange(&qModel, this);
    requestsDialog = new DlgRequests(&rotModel);
    requestsDialog->setModal(false);
    dlgBookCreator = new DlgBookCreator(this);
    dlgEq = new DlgEq(this);
    dlgAddSinger = new DlgAddSinger(rotModel, this);
    connect(dlgAddSinger, &DlgAddSinger::newSingerAdded, [&](auto pos) {
        ui->tableViewRotation->selectRow(pos);
        ui->lineEdit->setFocus();
    });
    dlgSongShop = new DlgSongShop(shop);
    dlgSongShop->setModal(false);
    ui->tableViewDB->setModel(&karaokeSongsModel);
    ui->tableViewDB->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewDB));
    kMediaBackend.setUseFader(settings.audioUseFader());
    if (!MediaBackend::canPitchShift()) {
        ui->spinBoxKey->hide();
        ui->lblKey->hide();
        ui->tableViewQueue->hideColumn(7);
    }
    if (!MediaBackend::canChangeTempo()) {
        ui->spinBoxTempo->hide();
        ui->lblTempo->hide();
    }
    ui->videoPreview->setFillOnPaint(true);
    cdgWindow = new DlgCdg(kMediaBackend, bmMediaBackend, nullptr, Qt::Window);
    settings.restoreWindowState(cdgWindow);

    connect(&rotModel, &TableModelRotation::songDroppedOnSinger, this, &MainWindow::songDroppedOnSinger);
    connect(dbDialog, &DlgDatabase::databaseUpdateComplete, this, &MainWindow::databaseUpdated);
    connect(dbDialog, &DlgDatabase::databaseSongAdded, &karaokeSongsModel, &TableModelKaraokeSongs::loadData);
    connect(dbDialog, &DlgDatabase::databaseSongAdded, requestsDialog, &DlgRequests::databaseSongAdded);
    connect(dbDialog, &DlgDatabase::databaseCleared, this, &MainWindow::databaseCleared);
    connect(&kMediaBackend, &MediaBackend::volumeChanged, ui->sliderVolume, &QSlider::setValue);
    connect(&kMediaBackend, &MediaBackend::positionChanged, this, &MainWindow::karaokeMediaBackend_positionChanged);
    connect(&kMediaBackend, &MediaBackend::durationChanged, this, &MainWindow::karaokeMediaBackend_durationChanged);
    connect(&kMediaBackend, &MediaBackend::stateChanged, this, &MainWindow::karaokeMediaBackend_stateChanged);
    connect(&kMediaBackend, &MediaBackend::hasActiveVideoChanged, [=](const bool &isActive) {
        m_kHasActiveVideo = isActive;
        hasActiveVideoChanged();
    });
    connect(&kMediaBackend, &MediaBackend::pitchChanged, ui->spinBoxKey, &QSpinBox::setValue);
    connect(&kMediaBackend, &MediaBackend::audioError, this, &MainWindow::audioError);
    connect(&kMediaBackend, &MediaBackend::silenceDetected, this, &MainWindow::silenceDetectedKar);
    connect(&bmMediaBackend, &MediaBackend::audioError, this, &MainWindow::audioError);
    connect(&bmMediaBackend, &MediaBackend::silenceDetected, this, &MainWindow::silenceDetectedBm);
    connect(&bmMediaBackend, &MediaBackend::hasActiveVideoChanged, [=](const bool &isActive) {
        m_bmHasActiveVideo = isActive;
        hasActiveVideoChanged();
    });
    connect(&sfxMediaBackend, &MediaBackend::positionChanged, this, &MainWindow::sfxAudioBackend_positionChanged);
    connect(&sfxMediaBackend, &MediaBackend::durationChanged, this, &MainWindow::sfxAudioBackend_durationChanged);
    connect(&sfxMediaBackend, &MediaBackend::stateChanged, this, &MainWindow::sfxAudioBackend_stateChanged);
    connect(&rotModel, &TableModelRotation::rotationModified, this, &MainWindow::rotationDataChanged);
    connect(&settings, &Settings::tickerOutputModeChanged, this, &MainWindow::rotationDataChanged);
    connect(shop, &SongShop::karaokeSongDownloaded, dbDialog, &DlgDatabase::singleSongAdd);
    kMediaBackend.setUseFader(settings.audioUseFader());

    kMediaBackend.setUseSilenceDetection(settings.audioDetectSilence());
    connect(ui->pushButtonTempoDn, &QPushButton::clicked, ui->spinBoxTempo, &QSpinBox::stepDown);
    connect(ui->pushButtonTempoUp, &QPushButton::clicked, ui->spinBoxTempo, &QSpinBox::stepUp);
    connect(ui->pushButtonKeyDn, &QPushButton::clicked, ui->spinBoxKey, &QSpinBox::stepDown);
    connect(ui->pushButtonKeyUp, &QPushButton::clicked, ui->spinBoxKey, &QSpinBox::stepUp);
    bmMediaBackend.setUseFader(settings.audioUseFaderBm());
    bmMediaBackend.setUseSilenceDetection(settings.audioDetectSilenceBm());

    kMediaBackend.setDownmix(settings.audioDownmix());
    bmMediaBackend.setDownmix(settings.audioDownmixBm());
    connect(requestsDialog, &DlgRequests::addRequestSong, &qModel, &TableModelQueueSongs::songAddSlot);
    connect(&settings, &Settings::tickerCustomStringChanged, this, &MainWindow::rotationDataChanged);

    settings.restoreWindowState(requestsDialog);
    settings.restoreWindowState(dlgSongShop);
    settings.restoreWindowState(dbDialog);
    settings.restoreSplitterState(ui->splitter);
    settings.restoreSplitterState(ui->splitter_2);
    settings.restoreSplitterState(ui->splitterBm);
    settings.restoreWindowState(dlgSongShop);
    rotationDataChanged();
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewQueue->hideColumn(TableModelQueueSongs::COL_ID);
    ui->tableViewQueue->hideColumn(TableModelQueueSongs::COL_DBSONGID);
    if (!MediaBackend::canPitchShift()) {
        ui->tableViewQueue->hideColumn(TableModelQueueSongs::COL_KEY);
    }
    rotModel.setHeaderData(0, Qt::Horizontal, "");
    rotModel.setHeaderData(1, Qt::Horizontal, "Singer");
    rotModel.setHeaderData(3, Qt::Horizontal, "");
    rotModel.setHeaderData(4, Qt::Horizontal, "");
    //ui->tableViewRotation->hideColumn(2);
    //ui->tableViewRotation->hideColumn(5);
    logger->info("{} Adding singer count to status bar", m_loggingPrefix);
    ui->statusBar->addWidget(&labelSingerCount);
    ui->statusBar->addWidget(&labelRotationDuration);


    bmPlaylistsModel = new QSqlTableModel(this, database);
    bmPlaylistsModel->setTable("bmplaylists");
    bmPlaylistsModel->sort(2, Qt::AscendingOrder);
    bmDbDialog = new BmDbDialog(this);
    bmCurrentPlaylist = settings.bmPlaylistIndex();
    ui->comboBoxBmPlaylists->setModel(bmPlaylistsModel);
    ui->comboBoxBmPlaylists->setModelColumn(1);
    ui->comboBoxBmPlaylists->setCurrentIndex(settings.bmPlaylistIndex());
    if (bmPlaylistsModel->rowCount() == 0) {
        bmAddPlaylist("Default");
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
    }
    ui->tableViewBmDb->setModel(&bmDbModel);
    bmDbModel.loadDatabase();
    ui->tableViewBmDb->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewBmDb));
    ui->tableViewBmPlaylist->setModel(&playlistSongsModel);
    ui->tableViewBmPlaylist->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewBmPlaylist));
    ui->tableViewBmPlaylist->setItemDelegate(&bmPlDelegate);
    ui->actionDisplay_Filenames->setChecked(settings.bmShowFilenames());
    ui->actionDisplay_Metadata->setChecked(settings.bmShowMetadata());
    settings.restoreSplitterState(ui->splitterBm);
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_ID, true);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_POSITION, true);
    settings.restoreSplitterState(ui->splitter_3);
    on_actionDisplay_Filenames_toggled(settings.bmShowFilenames());
    on_actionDisplay_Metadata_toggled(settings.bmShowMetadata());


    connect(&bmMediaBackend, &MediaBackend::stateChanged, this, &MainWindow::bmMediaStateChanged);
    connect(&bmMediaBackend, &MediaBackend::positionChanged, this, &MainWindow::bmMediaPositionChanged);
    connect(&bmMediaBackend, &MediaBackend::durationChanged, this, &MainWindow::bmMediaDurationChanged);
    connect(&bmMediaBackend, &MediaBackend::volumeChanged, ui->sliderBmVolume, &QSlider::setValue);
    connect(bmDbDialog, &BmDbDialog::bmDbUpdated, this, &MainWindow::bmDbUpdated);
    connect(bmDbDialog, &BmDbDialog::bmDbCleared, this, &MainWindow::bmDbCleared);
    connect(bmDbDialog, &BmDbDialog::bmDbAboutToUpdate, this, &MainWindow::bmDatabaseAboutToUpdate);
    ui->sliderBmVolume->setValue(initialBMVol);
    bmMediaBackend.setVolume(initialBMVol);
    ui->sliderVolume->setValue(initialKVol);
    kMediaBackend.setVolume(initialKVol);
    if (settings.mplxMode() == Multiplex_Normal)
        ui->pushButtonMplxBoth->setChecked(true);
    else if (settings.mplxMode() == Multiplex_LeftChannel)
        ui->pushButtonMplxLeft->setChecked(true);
    else if (settings.mplxMode() == Multiplex_RightChannel)
        ui->pushButtonMplxRight->setChecked(true);
    connect(&m_timerKaraokeAA, &QTimer::timeout, this, &MainWindow::karaokeAATimerTimeout);
    ui->actionAutoplay_mode->setChecked(settings.karaokeAutoAdvance());
    connect(ui->actionAutoplay_mode, &QAction::toggled, &settings, &Settings::setKaraokeAutoAdvance);
    connect(&settings, &Settings::karaokeAutoAdvanceChanged, ui->actionAutoplay_mode, &QAction::setChecked);


    // todo - athom: what's this?
    // isaac: this is the AV offset between audio and video to compensate for any downstream signal processing
    // delay on either the video or audio side (hdmi decoder lag on TVs or audio effects hardware and such).
    // It's just badly named because it existed when OpenK only supported cdg files and not mp4s and such.
    // Not sure why it's just there alone, though, as it returns an offset in milliseconds lol
    settings.cdgDisplayOffset();

    connect(&settings, &Settings::eqKBypassChanged, &kMediaBackend, &MediaBackend::setEqBypass);
    connect(&settings, &Settings::eqKLevelChanged, &kMediaBackend, &MediaBackend::setEqLevel);

    connect(&settings, &Settings::eqBBypassChanged, &bmMediaBackend, &MediaBackend::setEqBypass);
    connect(&settings, &Settings::eqBLevelChanged, &bmMediaBackend, &MediaBackend::setEqLevel);

    connect(&settings, &Settings::enforceAspectRatioChanged, &kMediaBackend, &MediaBackend::setEnforceAspectRatio);
    connect(&settings, &Settings::enforceAspectRatioChanged, &bmMediaBackend, &MediaBackend::setEnforceAspectRatio);
    connect(&settings, &Settings::mplxModeChanged, &kMediaBackend, &MediaBackend::setMplxMode);
    connect(&settings, &Settings::videoOffsetChanged, [&](auto offsetMs) {
        kMediaBackend.setVideoOffset(offsetMs);
        bmMediaBackend.setVideoOffset(offsetMs);
    });

    kMediaBackend.setEnforceAspectRatio(settings.enforceAspectRatio());
    bmMediaBackend.setEnforceAspectRatio(settings.enforceAspectRatio());

    kMediaBackend.setEqBypass(settings.eqKBypass());
    bmMediaBackend.setEqBypass(settings.eqBBypass());
    for (int band = 0; band < 10; band++) {
        kMediaBackend.setEqLevel(band, settings.getEqKLevel(band));
        bmMediaBackend.setEqLevel(band, settings.getEqBLevel(band));
    }

    connect(ui->lineEdit, &CustomLineEdit::escapePressed, ui->lineEdit, &CustomLineEdit::clear);
    connect(ui->lineEditBmSearch, &CustomLineEdit::escapePressed, ui->lineEditBmSearch, &CustomLineEdit::clear);
    connect(&qModel, &TableModelQueueSongs::songDroppedWithoutSinger, this, &MainWindow::songDropNoSingerSel);
    connect(ui->splitter_3, &QSplitter::splitterMoved, [&]() { autosizeViews(); });
    connect(ui->splitterBm, &QSplitter::splitterMoved, [&]() { autosizeBmViews(); });
    checker = new UpdateChecker(this);
    connect(checker, &UpdateChecker::newVersionAvailable, this, &MainWindow::newVersionAvailable);
    checker->checkForUpdates();
    connect(&m_timerButtonFlash, &QTimer::timeout, this, &MainWindow::timerButtonFlashTimeout);
    m_timerButtonFlash.start(1000);
    ui->pushButtonIncomingRequests->setVisible(settings.requestServerEnabled());
    connect(&settings, &Settings::requestServerEnabledChanged, ui->pushButtonIncomingRequests,
            &QPushButton::setVisible);
    connect(ui->actionSong_Shop, &QAction::triggered, [&]() { show(); });
    logger->info("{} Initial UI stup complete", m_loggingPrefix);
    connect(&qModel, &TableModelQueueSongs::filesDroppedOnSinger, this, &MainWindow::filesDroppedOnQueue);
    connect(&settings, &Settings::applicationFontChanged, this, &MainWindow::appFontChanged);
    QApplication::processEvents();
    QApplication::processEvents();
    appFontChanged(settings.applicationFont());
    QTimer::singleShot(500, [&]() {
        autosizeViews();
        autosizeBmViews();
    });
    connect(&playlistSongsModel, &TableModelPlaylistSongs::bmSongMoved, this, &MainWindow::bmSongMoved);
    connect(songbookApi, &OKJSongbookAPI::alertRecieved, this, &MainWindow::showAlert);
    connect(&settings, &Settings::cdgShowCdgWindowChanged, this, &MainWindow::cdgVisibilityChanged);
    connect(&settings, &Settings::rotationShowNextSongChanged, [&]() { resizeRotation(); });
    connect(&m_dlgRegularSingers.historySingersModel(), &TableModelHistorySingers::historySingersModified, [&]() {
        historySongsModel.refresh();
    });
    connect(&qModel, &TableModelQueueSongs::queueModified, &m_dlgRegularSingers, &DlgRegularSingers::regularsChanged);
    m_dlgRegularSingers.regularsChanged();
    m_dlgRegularSingers.setModal(false);
    SfxEntryList list = settings.getSfxEntries();
    logger->info("{} SfxEntryList size: ", m_loggingPrefix, list.size());
            foreach (SfxEntry entry, list) {
            addSfxButton(entry.path, entry.name);
        }
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &MainWindow::tableViewRotationCurrentChanged);
    rotModel.setCurrentSinger(settings.currentRotationPosition());
    rotDelegate.setCurrentSinger(settings.currentRotationPosition());
    updateRotationDuration();
    connect(&m_timerSlowUiUpdate, &QTimer::timeout, this, &MainWindow::updateRotationDuration);
    m_timerSlowUiUpdate.start(10000);
    connect(&qModel, &TableModelQueueSongs::queueModified, [&]() {
        updateRotationDuration();
        rotModel.layoutChanged();
    });
    connect(&settings, &Settings::rotationDurationSettingsModified, this, &MainWindow::updateRotationDuration);
    lazyDurationUpdater = new LazyDurationUpdateController(this);
    connect(lazyDurationUpdater, &LazyDurationUpdateController::gotDuration, &karaokeSongsModel,
            &TableModelKaraokeSongs::setSongDuration);
    if (settings.dbLazyLoadDurations())
        lazyDurationUpdater->getDurations();
    ui->btnToggleCdgWindow->setChecked(settings.showCdgWindow());
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::rotationSelectionChanged);
    ui->groupBoxNowPlaying->setVisible(settings.showMainWindowNowPlaying());
    ui->groupBoxSoundClips->setVisible(settings.showMainWindowSoundClips());
    if (settings.showMainWindowSoundClips()) {
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Ignored);
        ui->groupBoxSoundClips->setVisible(true);
    } else {
        ui->groupBoxSoundClips->setVisible(false);
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Expanding);
    }
    ui->videoPreview->setVisible(settings.showMainWindowVideo());
    ui->actionNow_Playing->setChecked(settings.showMainWindowNowPlaying());
    ui->actionSound_Clips->setChecked(settings.showMainWindowSoundClips());
    ui->actionVideo_Output_2->setChecked(settings.showMainWindowVideo());
    ui->actionMultiplex_Controls->setChecked(settings.showMplxControls());
    ui->widgetMplxControls->setVisible(settings.showMplxControls());
    switch (settings.mainWindowVideoSize()) {
        case Settings::Small:
            on_actionVideoSmall_triggered();
            break;
        case Settings::Medium:
            on_actionVideoMedium_triggered();
            break;
        case Settings::Large:
            on_actionVideoLarge_triggered();
            break;
    }
    ui->labelVolume->setPixmap(QIcon::fromTheme("player-volume").pixmap(QSize(22, 22)));
    ui->labelVolumeBm->setPixmap(QIcon::fromTheme("player-volume").pixmap(QSize(22, 22)));
    updateIcons();
    ui->menuTesting->menuAction()->setVisible(settings.testingEnabled());

    connect(ui->tableViewQueue->selectionModel(), &QItemSelectionModel::selectionChanged, [&]() {
        if (ui->tableViewQueue->selectionModel()->selectedRows().empty()) {
            ui->btnQBottom->setEnabled(false);
            ui->btnQDown->setEnabled(false);
            ui->btnQTop->setEnabled(false);
            ui->btnQUp->setEnabled(false);
        } else if (ui->tableViewQueue->selectionModel()->selectedRows().size() == 1) {
            ui->btnQBottom->setEnabled(true);
            ui->btnQDown->setEnabled(true);
            ui->btnQTop->setEnabled(true);
            ui->btnQUp->setEnabled(true);
        } else {
            ui->btnQBottom->setEnabled(true);
            ui->btnQDown->setEnabled(false);
            ui->btnQTop->setEnabled(true);
            ui->btnQUp->setEnabled(false);
        }
    });

    connect(ui->tableViewBmPlaylist->selectionModel(), &QItemSelectionModel::selectionChanged, [&]() {
        if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().empty()) {
            ui->btnPlBottom->setEnabled(false);
            ui->btnPlDown->setEnabled(false);
            ui->btnPlTop->setEnabled(false);
            ui->btnPlUp->setEnabled(false);
        } else if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().size() == 1) {
            ui->btnPlBottom->setEnabled(true);
            ui->btnPlDown->setEnabled(true);
            ui->btnPlTop->setEnabled(true);
            ui->btnPlUp->setEnabled(true);
        } else {
            ui->btnPlBottom->setEnabled(true);
            ui->btnPlDown->setEnabled(false);
            ui->btnPlTop->setEnabled(true);
            ui->btnPlUp->setEnabled(false);
        }
    });

    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::selectionChanged, [&]() {
        if (ui->tableViewRotation->selectionModel()->selectedRows().empty()) {
            ui->btnRotBottom->setEnabled(false);
            ui->btnRotDown->setEnabled(false);
            ui->btnRotTop->setEnabled(false);
            ui->btnRotUp->setEnabled(false);
            ui->tabWidgetQueue->hide();
            ui->labelNoSinger->show();
        } else if (ui->tableViewRotation->selectionModel()->selectedRows().size() == 1) {
            ui->btnRotBottom->setEnabled(true);
            ui->btnRotDown->setEnabled(true);
            ui->btnRotTop->setEnabled(true);
            ui->btnRotUp->setEnabled(true);
        } else {
            ui->btnRotBottom->setEnabled(true);
            ui->btnRotDown->setEnabled(false);
            ui->btnRotTop->setEnabled(true);
            ui->btnRotUp->setEnabled(false);
        }
    });

    connect(&playlistSongsModel, &TableModelPlaylistSongs::bmPlSongsMoved,
            [&](auto startRow, auto startCol, auto endRow, auto endCol) {
                auto topLeft = ui->tableViewBmPlaylist->model()->index(startRow, startCol);
                auto bottomRight = ui->tableViewBmPlaylist->model()->index(endRow, endCol);
                ui->tableViewBmPlaylist->clearSelection();
                ui->tableViewBmPlaylist->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                                  QItemSelectionModel::Select);
                auto nextPlSong = playlistSongsModel.getNextPlSong();
                if (nextPlSong.has_value() && !ui->checkBoxBmBreak->isChecked()) {
                    ui->labelBmNext->setText(nextPlSong->get().artist + " - " + nextPlSong->get().title);
                } else {
                    ui->labelBmNext->setText("Breaking after current song");
                }

            });
    connect(&playlistSongsModel, &TableModelPlaylistSongs::playingPlSongIdChanged, &bmPlDelegate,
            &ItemDelegatePlaylistSongs::setPlayingPlSongId);
    connect(&qModel, &TableModelQueueSongs::qSongsMoved, [&](auto startRow, auto startCol, auto endRow, auto endCol) {
        auto topLeft = ui->tableViewQueue->model()->index(startRow, startCol);
        auto bottomRight = ui->tableViewQueue->model()->index(endRow, endCol);
        ui->tableViewQueue->clearSelection();
        ui->tableViewQueue->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    });
    connect(&rotModel, &TableModelRotation::singersMoved, [&](auto startRow, auto startCol, auto endRow, auto endCol) {
        if (startRow == endRow) {
            ui->tableViewRotation->clearSelection();
            ui->tableViewRotation->selectRow(startRow);
            return;
        }
        auto topLeft = ui->tableViewRotation->model()->index(startRow, startCol);
        auto bottomRight = ui->tableViewRotation->model()->index(endRow, endCol);
        ui->tableViewRotation->clearSelection();
        ui->tableViewRotation->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                        QItemSelectionModel::Select);
    });
    std::vector<QWidget *> videoWidgets{cdgWindow->getVideoDisplay(), ui->videoPreview};
    bmMediaBackend.setVideoOutputWidgets({cdgWindow->getVideoDisplayBm(), ui->videoPreviewBm});
    kMediaBackend.setVideoOutputWidgets(videoWidgets);
    settings.setStartupOk(true);
    m_initialUiSetupDone = true;
    bmMediaBackend.stop(true);
    if ((settings.bmAutoStart()) && (playlistSongsModel.rowCount() > 0)) {
        QTimer::singleShot(1000, [&]() {
            playlistSongsModel.setCurrentPosition(0);
            auto plSong = playlistSongsModel.getCurrentSong();
            if (plSong.has_value()) {
                if (QFile::exists(plSong->get().path)) {
                    bmMediaBackend.setMedia(plSong->get().path);
                    bmMediaBackend.play();
                    bmMediaBackend.setVolume(ui->sliderBmVolume->value());
                } else {
                    QMessageBox::warning(this, tr("Break music autostart failure"),
                                         tr("Break music is set to autostart but the first song in the current playlist was not found.\n\nAborting playback."),
                                         QMessageBox::Ok);
                }
            }
        });
    }

}

void MainWindow::dbInit(const QDir &okjDataDir) {
    database = QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    database.setDatabaseName(okjDataDir.absolutePath() + QDir::separator() + "openkj.sqlite");
    database.open();
    QSqlQuery query(
            "CREATE TABLE IF NOT EXISTS dbSongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, DiscId COLLATE NOCASE, 'Duration' INTEGER, path VARCHAR(700) NOT NULL UNIQUE, filename COLLATE NOCASE, searchstring TEXT)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS rotationSingers ( singerid INTEGER PRIMARY KEY AUTOINCREMENT, name COLLATE NOCASE UNIQUE, 'position' INTEGER NOT NULL, 'regular' LOGICAL DEFAULT(0), 'regularid' INTEGER)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS queueSongs ( qsongid INTEGER PRIMARY KEY AUTOINCREMENT, singer INT, song INTEGER NOT NULL, artist INT, title INT, discid INT, path INT, keychg INT, played LOGICAL DEFAULT(0), 'position' INT)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS regularSingers ( regsingerid INTEGER PRIMARY KEY AUTOINCREMENT, Name COLLATE NOCASE UNIQUE, ph1 INT, ph2 INT, ph3 INT)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS regularSongs ( regsongid INTEGER PRIMARY KEY AUTOINCREMENT, regsingerid INTEGER NOT NULL, songid INTEGER NOT NULL, 'keychg' INTEGER, 'position' INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS sourceDirs ( path VARCHAR(255) UNIQUE, pattern INTEGER)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS bmsongs ( songid INTEGER PRIMARY KEY AUTOINCREMENT, Artist COLLATE NOCASE, Title COLLATE NOCASE, path VARCHAR(700) NOT NULL UNIQUE, Filename COLLATE NOCASE, Duration TEXT, searchstring TEXT)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS bmplaylists ( playlistid INTEGER PRIMARY KEY AUTOINCREMENT, title COLLATE NOCASE NOT NULL UNIQUE)");
    query.exec(
            "CREATE TABLE IF NOT EXISTS bmplsongs ( plsongid INTEGER PRIMARY KEY AUTOINCREMENT, playlist INT, position INT, Artist INT, Title INT, Filename INT, Duration INT, path INT)");
    query.exec("CREATE TABLE IF NOT EXISTS bmsrcdirs ( path NOT NULL)");
    query.exec("PRAGMA synchronous=OFF");
    query.exec("PRAGMA cache_size=300000");
    query.exec("PRAGMA temp_store=2");

    int schemaVersion = 0;
    query.exec("PRAGMA user_version");
    if (query.first())
        schemaVersion = query.value(0).toInt();
    logger->info("{} Database schema version: {}", m_loggingPrefix, schemaVersion);

    if (schemaVersion < 100) {
        logger->info("{} Updating database schema to version 101", m_loggingPrefix);
        query.exec("ALTER TABLE sourceDirs ADD COLUMN custompattern INTEGER");
        query.exec("PRAGMA user_version = 100");
        logger->info("{} DB Schema update to v100 completed", m_loggingPrefix);
    }
    if (schemaVersion < 101) {
        logger->info("{} Updating database schema to version 101", m_loggingPrefix);
        query.exec(
                "CREATE TABLE custompatterns ( patternid INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, artistregex TEXT, artistcapturegrp INT, titleregex TEXT, titlecapturegrp INT, discidregex TEXT, discidcapturegrp INT)");
        query.exec("PRAGMA user_version = 101");
        logger->info("{} DB Schema update to v101 completed", m_loggingPrefix);
    }
    if (schemaVersion < 102) {
        logger->info("{} Updating database schema to version 102", m_loggingPrefix);
        query.exec("CREATE UNIQUE INDEX idx_path ON dbsongs(path)");
        query.exec("PRAGMA user_version = 102");
        logger->info("{} DB Schema update to v102 completed", m_loggingPrefix);
    }
    if (schemaVersion < 103) {
        logger->info("{} Updating database schema to version 103", m_loggingPrefix);
        query.exec("ALTER TABLE dbsongs ADD COLUMN searchstring TEXT");
        query.exec("UPDATE dbsongs SET searchstring = filename || ' ' || artist || ' ' || title || ' ' || discid");
        query.exec("PRAGMA user_version = 103");
        logger->info("{} DB Schema update to v103 completed", m_loggingPrefix);

    }
    if (schemaVersion < 105) {
        logger->info("{} Updating database schema to version 105", m_loggingPrefix);
        query.exec("ALTER TABLE rotationSingers ADD COLUMN addts TIMESTAMP");
        query.exec("PRAGMA user_version = 105");
        logger->info("{} DB Schema update to v105 completed", m_loggingPrefix);
    }
    if (schemaVersion < 106) {
        logger->info("{} Updating database schema to version 106", m_loggingPrefix);
        query.exec(
                "CREATE TABLE dbSongHistory ( id INTEGER PRIMARY KEY AUTOINCREMENT, filepath TEXT, artist TEXT, title TEXT, songid TEXT, timestamp TIMESTAMP)");
        query.exec("CREATE INDEX idx_filepath ON dbSongHistory(filepath)");
        query.exec("ALTER TABLE dbsongs ADD COLUMN plays INT DEFAULT(0)");
        query.exec("ALTER TABLE dbsongs ADD COLUMN lastplay TIMESTAMP");
        query.exec("CREATE TABLE historySingers(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE)");
        query.exec(
                "CREATE TABLE historySongs(id INTEGER PRIMARY KEY AUTOINCREMENT, historySinger INT NOT NULL, filepath TEXT NOT NULL, artist TEXT, title TEXT, songid TEXT, keychange INT DEFAULT(0), plays INT DEFAULT(0), lastplay TIMESTAMP)");
        query.exec("CREATE INDEX idx_historySinger on historySongs(historySinger)");
        query.exec("PRAGMA user_version = 106");
        logger->info("{} DB Schema update to v106 completed", m_loggingPrefix);
        logger->info("{} Importing old regular singers data into singer history", m_loggingPrefix);
        QSqlQuery songImportQuery;
        songImportQuery.prepare(
                "INSERT INTO historySongs (historySinger, filepath, artist, title, songid, keychange) values(:historySinger, :filepath, :artist, :title, :songid, :keychange)");
        QSqlQuery singersQuery;
        singersQuery.exec("SELECT regsingerid,name FROM regularSingers");
        while (singersQuery.next()) {
            logger->info("{} Running import for singer: {}", m_loggingPrefix, singersQuery.value("name").toString().toStdString());
            QSqlQuery songsQuery;
            songsQuery.exec(
                    "SELECT dbsongs.artist,dbsongs.title,dbsongs.discid,regularsongs.keychg,dbsongs.path FROM regularsongs,dbsongs WHERE dbsongs.songid == regularsongs.songid AND regularsongs.regsingerid == " +
                    singersQuery.value("regsingerid").toString() + " ORDER BY regularsongs.position");
            while (songsQuery.next()) {
                logger->info("{} Importing song: {}", m_loggingPrefix, songsQuery.value(4).toString().toStdString());
                historySongsModel.saveSong(
                        singersQuery.value("name").toString(),
                        songsQuery.value(4).toString(),
                        songsQuery.value(0).toString(),
                        songsQuery.value(1).toString(),
                        songsQuery.value(2).toString(),
                        songsQuery.value(3).toInt()
                );
            }
            logger->info("{} Import complete for singer: {}", m_loggingPrefix, singersQuery.value("name").toString().toStdString());
        }
    }

}

void MainWindow::play(const QString &karaokeFilePath, const bool &k2k) {
    khTmpDir->remove();
    delete khTmpDir;
    khTmpDir = new QTemporaryDir();
    if (kMediaBackend.state() != MediaBackend::PausedState) {
        logger->info("{} Playing file: {}", m_loggingPrefix, karaokeFilePath.toStdString());
        if (kMediaBackend.state() == MediaBackend::PlayingState) {
            if (settings.karaokeAutoAdvance()) {
                kAASkip = true;
                cdgWindow->showAlert(false);
            }
            kMediaBackend.stop();
            if (k2kTransition && settings.rotationAltSortOrder())
                rotModel.singerMove(0, rotModel.rowCount() - 1);
            ui->spinBoxTempo->setValue(100);
        }
        if (karaokeFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
            MzArchive archive(karaokeFilePath);
            if ((archive.checkCDG()) && (archive.checkAudio())) {
                if (archive.checkAudio()) {
                    if (!archive.extractAudio(khTmpDir->path(), "tmp" + archive.audioExtension())) {
                        m_timerTest.stop();
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract audio file."),
                                             QMessageBox::Ok);
                        return;
                    }
                    if (!archive.extractCdg(khTmpDir->path(), "tmp.cdg")) {
                        m_timerTest.stop();
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract CDG file."),
                                             QMessageBox::Ok);
                        return;
                    }
                    QString audioFile = khTmpDir->path() + QDir::separator() + "tmp" + archive.audioExtension();
                    QString cdgFile = khTmpDir->path() + QDir::separator() + "tmp.cdg";
                    logger->info("{} Extracted audio file size: {}", m_loggingPrefix, QFileInfo(audioFile).size());
                    logger->info("{} Setting karaoke backend source file to: {}", m_loggingPrefix, audioFile.toStdString());
                    kMediaBackend.setMediaCdg(cdgFile, audioFile);
                    if (!k2k)
                        bmMediaBackend.fadeOut(!settings.bmKCrossFade());
                    logger->info("{} Beginning playback of file: {}", m_loggingPrefix, audioFile.toStdString());
                    QApplication::setOverrideCursor(Qt::WaitCursor);
                    kMediaBackend.play();
                    QApplication::restoreOverrideCursor();
                    kMediaBackend.fadeInImmediate();
                }
            } else {
                QMessageBox::warning(this, tr("Bad karaoke file"),
                                     tr("Zip file does not contain a valid karaoke track.  CDG or audio file missing or corrupt."),
                                     QMessageBox::Ok);
                return;
            }
        } else if (karaokeFilePath.endsWith(".cdg", Qt::CaseInsensitive)) {
            QString cdgTmpFile = "tmp.cdg";
            QString audTmpFile = "tmp.mp3";
            QFile cdgFile(karaokeFilePath);
            if (!cdgFile.exists()) {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file missing."), QMessageBox::Ok);
                return;
            } else if (cdgFile.size() == 0) {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("CDG file contains no data"), QMessageBox::Ok);
                return;
            }
            QString audiofn = findMatchingAudioFile(karaokeFilePath);
            if (audiofn == "") {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file missing."), QMessageBox::Ok);
                return;
            }
            QFile audioFile(audiofn);
            if (audioFile.size() == 0) {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file contains no data"), QMessageBox::Ok);
                return;
            }
            cdgFile.copy(khTmpDir->path() + QDir::separator() + cdgTmpFile);
            QFile::copy(audiofn, khTmpDir->path() + QDir::separator() + audTmpFile);
            kMediaBackend.setMediaCdg(khTmpDir->path() + QDir::separator() + cdgTmpFile,
                                      khTmpDir->path() + QDir::separator() + audTmpFile);
            if (!k2k)
                bmMediaBackend.fadeOut(!settings.bmKCrossFade());
            QApplication::setOverrideCursor(Qt::WaitCursor);
            kMediaBackend.play();
            QApplication::restoreOverrideCursor();
            kMediaBackend.fadeInImmediate();
        } else {
            // Close CDG if open to avoid double video playback
            logger->info("{} Playing non-CDG video file: {}", m_loggingPrefix, karaokeFilePath.toStdString());
            QString tmpFileName = khTmpDir->path() + QDir::separator() + "tmpvid." + karaokeFilePath.right(4);
            QFile::copy(karaokeFilePath, tmpFileName);
            logger->info("{} Playing temporary copy to avoid bad filename stuff w/ gstreamer: {}", m_loggingPrefix, tmpFileName.toStdString());
            kMediaBackend.setMedia(tmpFileName);
            if (!k2k)
                bmMediaBackend.fadeOut();
            kMediaBackend.play();
            kMediaBackend.fadeInImmediate();
        }
        kMediaBackend.setTempo(ui->spinBoxTempo->value());
        if (settings.recordingEnabled()) {
            logger->info("{} Starting recording", m_loggingPrefix);
            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");
            audioRecorder.record(curSinger + " - " + curArtist + " - " + curTitle + " - " + timeStamp);
        }


    } else if (kMediaBackend.state() == MediaBackend::PausedState) {
        if (settings.recordingEnabled())
            audioRecorder.unpause();
        kMediaBackend.play();
        kMediaBackend.fadeIn(false);
    }
    k2kTransition = false;
    if (settings.karaokeAutoAdvance())
        kAASkip = false;
}

MainWindow::~MainWindow() {
    m_shuttingDown = true;
    cdgWindow->stopTicker();
#ifdef _MSC_VER
    timeEndPeriod(1);
#endif
    lazyDurationUpdater->stopWork();
    settings.bmSetVolume(ui->sliderBmVolume->value());
    settings.setAudioVolume(ui->sliderVolume->value());
    logger->info("{} Saving volumes - K: {} BM {}", m_loggingPrefix, settings.audioVolume(), settings.bmVolume());
    logger->info("{} Saving window and widget sizing and positioning info", m_loggingPrefix);
    settings.saveSplitterState(ui->splitter);
    settings.saveSplitterState(ui->splitter_2);
    settings.saveSplitterState(ui->splitter_3);
    settings.saveColumnWidths(ui->tableViewDB);
    settings.saveColumnWidths(ui->tableViewRotation);
    settings.saveColumnWidths(ui->tableViewQueue);
    settings.saveWindowState(requestsDialog);
    settings.saveWindowState(dlgSongShop);
    settings.saveWindowState(dlgSongShop);
    settings.saveWindowState(dbDialog);
    settings.saveWindowState(this);
    settings.saveSplitterState(ui->splitterBm);
    settings.saveColumnWidths(ui->tableViewBmDb);
    settings.saveColumnWidths(ui->tableViewBmPlaylist);
    settings.bmSetPlaylistIndex(ui->comboBoxBmPlaylists->currentIndex());
    settings.sync();
    logger->info("{} Deleting non-owned objects", m_loggingPrefix);
    delete ui;
    delete khTmpDir;
    delete dlgSongShop;
    delete requestsDialog;
    logger->info("{} OpenKJ mainwindow destructor complete", m_loggingPrefix);
}

void MainWindow::search() {
    ui->tableViewDB->scrollToTop();
    karaokeSongsModel.search(ui->lineEdit->text());
}

void MainWindow::databaseUpdated() {
    karaokeSongsModel.loadData();
    search();
    settings.restoreColumnWidths(ui->tableViewDB);
    requestsDialog->databaseUpdateComplete();
    autosizeViews();
    lazyDurationUpdater->stopWork();
    lazyDurationUpdater->deleteLater();
    lazyDurationUpdater = new LazyDurationUpdateController(this);
    connect(lazyDurationUpdater, &LazyDurationUpdateController::gotDuration, &karaokeSongsModel,
            &TableModelKaraokeSongs::setSongDuration);
    lazyDurationUpdater->getDurations();
}

void MainWindow::databaseCleared() {
    lazyDurationUpdater->stopWork();
    karaokeSongsModel.loadData();
    rotModel.loadData();
    qModel.loadSinger(-1);
    ui->tableViewQueue->reset();
    autosizeViews();
    rotationDataChanged();


}

void MainWindow::on_buttonStop_clicked() {
    if (kMediaBackend.state() == MediaBackend::PlayingState) {
        if (settings.showSongPauseStopWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show warning on pause/stop in the future");
            cb->setChecked(settings.showSongPauseStopWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Stop currenly playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSongPauseStopWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
    }
    kAASkip = true;
    cdgWindow->showAlert(false);
    audioRecorder.stop();
    if (settings.bmKCrossFade()) {
        bmMediaBackend.fadeIn(false);
        kMediaBackend.stop();
    } else {
        kMediaBackend.stop();
        bmMediaBackend.fadeIn();
    }
//    ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
}

void MainWindow::on_buttonPause_clicked() {
    if (kMediaBackend.state() == MediaBackend::PausedState) {
        kMediaBackend.play();
    } else if (kMediaBackend.state() == MediaBackend::PlayingState) {
        if (settings.showSongPauseStopWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show warning on pause/stop in the future");
            cb->setChecked(settings.showSongPauseStopWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Pause currenly playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be paused.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSongPauseStopWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        kMediaBackend.pause();
    }
}

void MainWindow::on_lineEdit_returnPressed() {
    search();
}

void MainWindow::on_tableViewDB_doubleClicked(const QModelIndex &index) {
    if (settings.dbDoubleClickAddsSong()) {
        auto addSongDlg = new DlgAddSong(rotModel, qModel, index.sibling(index.row(), 0).data().toInt(), this);
        connect(addSongDlg, &DlgAddSong::newSingerAdded, [&](auto pos) {
            ui->tableViewRotation->selectRow(pos);
            ui->lineEdit->setFocus();
        });
        addSongDlg->setModal(true);
        addSongDlg->show();
        return;
    }
    if (qModel.getSingerId() >= 0) {
        qModel.add(index.sibling(index.row(), 0).data().toInt());
        updateRotationDuration();
    } else {
        QMessageBox msgBox;
        msgBox.setText("No singer selected.  You must select a singer before you can double-click to add to a queue.");
        msgBox.exec();
    }
}

void MainWindow::on_buttonAddSinger_clicked() {
    dlgAddSinger->show();
}

void MainWindow::on_tableViewRotation_doubleClicked(const QModelIndex &index) {
    if (index.column() <= 3) {
        k2kTransition = false;
        int singerId = index.data(Qt::UserRole).toInt();
        QString nextSongPath = rotModel.nextSongPath(singerId);
        if (nextSongPath != "") {
            if ((kMediaBackend.state() == MediaBackend::PlayingState) && (settings.showSongInterruptionWarning())) {
                QMessageBox msgBox(this);
                auto *cb = new QCheckBox("Show this warning in the future");
                cb->setChecked(settings.showSongInterruptionWarning());
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Interrupt currenly playing karaoke song?");
                msgBox.setInformativeText(
                        "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
                QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.setCheckBox(cb);
                connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSongInterruptionWarning);
                msgBox.exec();
                if (msgBox.clickedButton() != yesButton) {
                    return;
                }
                k2kTransition = true;
            }
            if (kMediaBackend.state() == MediaBackend::PausedState) {
                if (settings.karaokeAutoAdvance()) {
                    kAASkip = true;
                    cdgWindow->showAlert(false);
                }
                audioRecorder.stop();
                kMediaBackend.stop(true);
            }
            //           play(nextSongPath);
            //           kAudioBackend.setPitchShift(rotModel.nextSongKeyChg(singerId));

            curSinger = rotModel.getSingerName(singerId);
            curArtist = rotModel.nextSongArtist(singerId);
            curTitle = rotModel.nextSongTitle(singerId);
            QString curSongId = rotModel.nextSongSongId(singerId);
            int curKeyChange = rotModel.nextSongKeyChg(singerId);

            karaokeSongsModel.updateSongHistory(karaokeSongsModel.getIdForPath(nextSongPath));
            play(nextSongPath, k2kTransition);
            ui->labelArtist->setText(curArtist);
            ui->labelTitle->setText(curTitle);
            ui->labelSinger->setText(curSinger);
            if (settings.treatAllSingersAsRegs() || rotModel.singerIsRegular(singerId))
                historySongsModel.saveSong(curSinger, nextSongPath, curArtist, curTitle, curSongId, curKeyChange);
            kMediaBackend.setPitchShift(curKeyChange);
            qModel.setPlayed(rotModel.nextSongQueueId(singerId));
            rotDelegate.setCurrentSinger(singerId);
            rotModel.setCurrentSinger(singerId);
            if (settings.rotationAltSortOrder()) {
                auto curSingerPos = rotModel.getSingerPosition(singerId);
                m_curSingerOriginalPosition = curSingerPos;
                if (curSingerPos != 0) {
                    rotModel.singerMove(curSingerPos, 0);
                    ui->tableViewRotation->clearSelection();
                    ui->tableViewRotation->selectRow(0);
                }
            }
        }
    }
}

void MainWindow::on_tableViewRotation_clicked(const QModelIndex &index) {
    if (index.column() == TableModelRotation::COL_DELETE) {
        if (settings.showSingerRemovalWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings.showSingerRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Are you sure you want to remove this singer?");
            msgBox.setInformativeText(
                    "Unless this singer is a tracked regular, you will be unable retrieve any queue data for this singer once they are deleted.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSingerRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        int singerId = index.data(Qt::UserRole).toInt();
        logger->info("{} Singer id selected: {}", m_loggingPrefix, singerId);
        qModel.loadSinger(-1);
        if (rotModel.currentSinger() == singerId) {
            rotModel.setCurrentSinger(-1);
            rotDelegate.setCurrentSinger(-1);
        }
        rotModel.singerDelete(singerId);
        ui->tableViewRotation->clearSelection();
        ui->tableViewQueue->clearSelection();
        return;

    }
    if (index.column() == TableModelRotation::COL_REGULAR) {
        if (!rotModel.singerIsRegular(index.data(Qt::UserRole).toInt())) {
            QString name = index.sibling(index.row(), TableModelRotation::COL_NAME).data().toString();
            if (rotModel.historySingerExists(name)) {
                auto answer = QMessageBox::question(this,
                                                    "A regular singer with this name already exists!",
                                                    "There is already a regular singer saved with this name. Would you like to load "
                                                    "the matching regular singer's history for this singer?",
                                                    QMessageBox::StandardButtons(
                                                            QMessageBox::Yes | QMessageBox::Cancel),
                                                    QMessageBox::Cancel
                );
                if (answer == QMessageBox::Yes)
                    rotModel.singerMakeRegular(rotModel.getSingerId(name));
            } else
                rotModel.singerMakeRegular(index.data(Qt::UserRole).toInt());
        } else {
            QMessageBox msgBox(this);
            msgBox.setText("Are you sure you want to disable regular tracking for this singer?");
            msgBox.setInformativeText(
                    "Doing so will not remove the regular singer entry, but it will prevent any changes made to the singer's queue from being saved to the regular singer until the regular singer is either reloaded or the rotation singer is re-merged with the regular singer.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton) {
                rotModel.singerDisableRegularTracking(index.data(Qt::UserRole).toInt());
            }
        }
    }
}

void MainWindow::on_tableViewQueue_doubleClicked(const QModelIndex &index) {
    k2kTransition = false;
    if (kMediaBackend.state() == MediaBackend::PlayingState) {
        if (settings.showSongInterruptionWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings.showSongInterruptionWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Interrupt currenly playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSongInterruptionWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        k2kTransition = true;
    }
    if (kMediaBackend.state() == MediaBackend::PausedState) {
        if (settings.karaokeAutoAdvance()) {
            kAASkip = true;
            cdgWindow->showAlert(false);
        }
        audioRecorder.stop();
        kMediaBackend.stop(true);
    }
    int curSingerId = qModel.getSingerId();
    curSinger = rotModel.getSingerName(curSingerId);
    curArtist = index.sibling(index.row(), TableModelQueueSongs::COL_ARTIST).data().toString();
    curTitle = index.sibling(index.row(), TableModelQueueSongs::COL_TITLE).data().toString();
    QString curSongId = index.sibling(index.row(), TableModelQueueSongs::COL_SONGID).data().toString();
    QString filePath = index.sibling(index.row(), TableModelQueueSongs::COL_PATH).data().toString();
    int curKeyChange = index.sibling(index.row(), TableModelQueueSongs::COL_KEY).data().toInt();
    ui->labelSinger->setText(curSinger);
    ui->labelArtist->setText(curArtist);
    ui->labelTitle->setText(curTitle);
    karaokeSongsModel.updateSongHistory(karaokeSongsModel.getIdForPath(filePath));
    play(filePath, k2kTransition);
    if (settings.treatAllSingersAsRegs() || rotModel.singerIsRegular(curSingerId))
        historySongsModel.saveSong(curSinger, filePath, curArtist, curTitle, curSongId, curKeyChange);
    kMediaBackend.setPitchShift(curKeyChange);
    qModel.setPlayed(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt());

    rotModel.setCurrentSinger(curSingerId);
    rotDelegate.setCurrentSinger(curSingerId);
    if (settings.rotationAltSortOrder()) {
        auto curSingerPos = rotModel.getSingerPosition(curSingerId);
        m_curSingerOriginalPosition = curSingerPos;
        if (curSingerPos != 0) {
            rotModel.singerMove(curSingerPos, 0);
            ui->tableViewRotation->clearSelection();
            ui->tableViewRotation->selectRow(0);
        }
    }
}

void MainWindow::on_actionManage_DB_triggered() {
    dbDialog->showNormal();
}

void MainWindow::on_actionExport_Regulars_triggered() {
    auto exportdlg = new DlgRegularExport(karaokeSongsModel, this);
    exportdlg->setModal(true);
    exportdlg->show();
}

void MainWindow::on_actionImport_Regulars_triggered() {
    auto iDialog = new DlgRegularImport(karaokeSongsModel, this);
    iDialog->setModal(true);
    iDialog->show();
}

void MainWindow::on_actionSettings_triggered() {
    auto settingsDialog = new DlgSettings(&kMediaBackend, &bmMediaBackend, this);
    settingsDialog->setModal(true);
    connect(settingsDialog, &DlgSettings::audioUseFaderChanged, &kMediaBackend, &MediaBackend::setUseFader);
    connect(settingsDialog, &DlgSettings::audioSilenceDetectChanged, &kMediaBackend,
            &MediaBackend::setUseSilenceDetection);
    connect(settingsDialog, &DlgSettings::audioUseFaderChangedBm, &bmMediaBackend, &MediaBackend::setUseFader);
    connect(settingsDialog, &DlgSettings::audioSilenceDetectChangedBm, &bmMediaBackend,
            &MediaBackend::setUseSilenceDetection);
    connect(settingsDialog, &DlgSettings::audioDownmixChanged, &kMediaBackend, &MediaBackend::setDownmix);
    connect(settingsDialog, &DlgSettings::audioDownmixChangedBm, &bmMediaBackend, &MediaBackend::setDownmix);
    settingsDialog->show();
}

void MainWindow::on_actionRegulars_triggered() {
    on_buttonRegulars_clicked();
}

void MainWindow::on_actionIncoming_Requests_triggered() {
    requestsDialog->show();
}

void MainWindow::songDroppedOnSinger(const int &singerId, const int &songId, const int &dropRow) {
    qModel.loadSinger(singerId);
    qModel.add(songId);
    ui->tableViewRotation->clearSelection();
    QItemSelectionModel *selmodel = ui->tableViewRotation->selectionModel();
    QModelIndex topLeft;
    QModelIndex bottomRight;
    topLeft = rotModel.index(dropRow, 0, QModelIndex());
    bottomRight = rotModel.index(dropRow, 4, QModelIndex());
    QItemSelection selection(topLeft, bottomRight);
    selmodel->select(selection, QItemSelectionModel::Select);
}

void MainWindow::on_pushButton_clicked() {
    search();
}

void MainWindow::on_tableViewQueue_clicked(const QModelIndex &index) {
    if (index.column() == TableModelQueueSongs::COL_PATH) {
        if ((settings.showQueueRemovalWarning()) &&
            (!qModel.getPlayed(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt()))) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings.showQueueRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Removing un-played song from queue");
            msgBox.setInformativeText("This song has not been played yet, are you sure you want to remove it?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowQueueRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton) {
                qModel.remove(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt());
            }
        } else {
            qModel.remove(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt());
        }
    }
}

void MainWindow::on_buttonClearRotation_clicked() {
    if (m_testMode) {
        settings.setCurrentRotationPosition(-1);
        rotModel.clearRotation();
        rotDelegate.setCurrentSinger(-1);
        qModel.loadSinger(-1);
        return;
    }
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText(
            "This action will clear all rotation singers and queues. This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        settings.setCurrentRotationPosition(-1);
        rotModel.clearRotation();
        rotDelegate.setCurrentSinger(-1);
        qModel.loadSinger(-1);
    }
}

void MainWindow::on_buttonClearQueue_clicked() {
    if (m_testMode) {
        qModel.removeAll();
        return;
    }
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText(
            "This action will clear all queued songs for the selected singer.  If the singer is a regular singer, it will delete their saved regular songs as well! This operation can not be undone.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        qModel.removeAll();
    }
}

void MainWindow::on_spinBoxKey_valueChanged(const int &arg1) {
    kMediaBackend.setPitchShift(arg1);
    if (arg1 > 0)
        ui->spinBoxKey->setPrefix("+");
    else
        ui->spinBoxKey->setPrefix("");
    QTimer::singleShot(20, [&]() {
        ui->spinBoxKey->findChild<QLineEdit *>()->deselect();
    });
}

void MainWindow::karaokeMediaBackend_positionChanged(const qint64 &position) {
    if (kMediaBackend.state() == MediaBackend::PlayingState) {
        if (!sliderPositionPressed) {
            ui->sliderProgress->setMaximum(kMediaBackend.duration());
            ui->sliderProgress->setValue(position);
        }
        ui->labelElapsedTime->setText(MediaBackend::msToMMSS(position));
        ui->labelRemainTime->setText(MediaBackend::msToMMSS(kMediaBackend.duration() - position));
        rotModel.setCurRemainSecs((kMediaBackend.duration() - position) / 1000);
    }
}

void MainWindow::karaokeMediaBackend_durationChanged(const qint64 &duration) {
    ui->labelTotalTime->setText(MediaBackend::msToMMSS(duration));
}

void MainWindow::karaokeMediaBackend_stateChanged(const MediaBackend::State &state) {
    if (m_shuttingDown)
        return;
    if (state == MediaBackend::StoppedState) {
        logger->info("{} MainWindow - audio backend state is now STOPPED", m_loggingPrefix);
        if (ui->labelTotalTime->text() == "0:00") {
            return;
        }
        logger->info("{} KAudio entered StoppedState", m_loggingPrefix);
        audioRecorder.stop();
        if (k2kTransition)
            return;
        ui->labelArtist->setText("None");
        ui->labelTitle->setText("None");
        ui->labelSinger->setText("None");
        ui->labelElapsedTime->setText("0:00");
        ui->labelRemainTime->setText("0:00");
        ui->labelTotalTime->setText("0:00");
        ui->sliderProgress->setValue(0);
        ui->spinBoxTempo->setValue(100);
        ui->spinBoxKey->setValue(0);
        ui->pushButtonKeyDn->setEnabled(false);
        ui->pushButtonKeyUp->setEnabled(false);
        ui->pushButtonTempoDn->setEnabled(false);
        ui->pushButtonTempoUp->setEnabled(false);
        if (state == m_lastAudioState)
            return;
        m_lastAudioState = state;
        bmMediaBackend.fadeIn(false);
        if (settings.karaokeAutoAdvance()) {
            logger->info("{}  - Karaoke Autoplay is enabled", m_loggingPrefix);
            if (kAASkip) {
                kAASkip = false;
                logger->info("{}  - Karaoke Autoplay set to skip, bailing out", m_loggingPrefix);
            } else {
                int nextSinger = -1;
                QString nextSongPath;
                bool empty = false;

                int curSingerId = rotModel.currentSinger();

                int curPos = rotModel.getSingerPosition(curSingerId);
                if (settings.rotationAltSortOrder())
                    curPos = m_curSingerOriginalPosition;
                if (curSingerId == -1)
                    curPos = rotModel.rowCount() - 1;
                int loops = 0;
                while ((nextSongPath == "") && (!empty)) {
                    if (loops > rotModel.rowCount()) {
                        empty = true;
                    } else {
                        if (++curPos >= rotModel.rowCount()) {
                            curPos = 0;
                        }
                        nextSinger = rotModel.singerIdAtPosition(curPos);
                        nextSongPath = rotModel.nextSongPath(nextSinger);
                        loops++;
                    }
                }
                if (empty)
                    logger->info("{} KaraokeAA - No more songs to play, giving up", m_loggingPrefix);
                else {
                    kAANextSinger = nextSinger;
                    kAANextSongPath = nextSongPath;
                    logger->info("{} KaraokeAA - Will play: {} - {}", m_loggingPrefix, rotModel.getSingerName(nextSinger).toStdString(), nextSongPath.toStdString());
                    logger->info("{} KaraokeAA - Starting {} second timer", m_loggingPrefix, settings.karaokeAATimeout());
                    m_timerKaraokeAA.start(settings.karaokeAATimeout() * 1000);
                    cdgWindow->setNextSinger(rotModel.getSingerName(nextSinger));
                    cdgWindow->setNextSong(
                            rotModel.nextSongArtist(nextSinger) + " - " + rotModel.nextSongTitle(nextSinger));
                    cdgWindow->setCountdownSecs(settings.karaokeAATimeout());
                    cdgWindow->showAlert(true);
                }
            }
        }
        if (settings.rotationAltSortOrder()) {
            rotModel.singerMove(0, rotModel.rowCount() - 1);
            rotModel.setCurrentSinger(-1);
            rotDelegate.setCurrentSinger(-1);
            ui->tableViewRotation->clearSelection();
            ui->tableViewRotation->selectRow(0);
            rotModel.setCurRemainSecs(0);
        }
    }
    if (state == MediaBackend::EndOfMediaState) {
        logger->info("{} KAudio entered EndOfMediaState", m_loggingPrefix);
        audioRecorder.stop();
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        //bmMediaBackend.setVideoEnabled(true);
        kMediaBackend.stop(true);
        bmMediaBackend.fadeIn(false);
    }
    if (state == MediaBackend::PausedState) {
        logger->info("{} KAudio entered PausedState", m_loggingPrefix);
        audioRecorder.pause();
    }
    if (state == MediaBackend::PlayingState) {
        logger->info("{} KAudio entered PlayingState", m_loggingPrefix);
        m_lastAudioState = state;
        //bmMediaBackend.setVideoEnabled(false);
        ui->pushButtonKeyUp->setEnabled(true);
        ui->pushButtonKeyDn->setEnabled(true);
        ui->pushButtonTempoDn->setEnabled(true);
        ui->pushButtonTempoUp->setEnabled(true);
    }
    if (state == MediaBackend::UnknownState) {
        logger->info("{} KAudio entered UnknownState", m_loggingPrefix);
    }
    rotationDataChanged();
}

void MainWindow::sfxAudioBackend_positionChanged(const qint64 &position) {
    ui->sliderSfxPos->setValue(position);
}

void MainWindow::sfxAudioBackend_durationChanged(const qint64 &duration) {
    ui->sliderSfxPos->setMaximum(duration);
}

void MainWindow::sfxAudioBackend_stateChanged(const MediaBackend::State &state) {
    if (state == MediaBackend::EndOfMediaState) {
        ui->sliderSfxPos->setValue(0);
        sfxMediaBackend.stop();
    }
    if (state == MediaBackend::StoppedState || state == MediaBackend::UnknownState)
        ui->sliderSfxPos->setValue(0);
}

void MainWindow::hasActiveVideoChanged() {
    ui->videoPreview->setHasActiveVideo(m_kHasActiveVideo);
    ui->videoPreviewBm->setHasActiveVideo(m_bmHasActiveVideo);
    cdgWindow->getVideoDisplay()->setHasActiveVideo(m_kHasActiveVideo);
    cdgWindow->getVideoDisplayBm()->setHasActiveVideo(m_bmHasActiveVideo);
    if (m_timerKaraokeAA.isActive() && settings.karaokeAAAlertEnabled())
        return;
    if (m_bmHasActiveVideo && !m_kHasActiveVideo) {
        cdgWindow->getVideoDisplay()->hide();
        cdgWindow->getVideoDisplayBm()->show();
        ui->videoPreview->hide();
        ui->videoPreviewBm->show();
    } else {
        cdgWindow->getVideoDisplay()->show();
        cdgWindow->getVideoDisplayBm()->hide();
        ui->videoPreview->show();
        ui->videoPreviewBm->hide();
    }
}

void MainWindow::on_buttonRegulars_clicked() {
    m_dlgRegularSingers.setVisible(!m_dlgRegularSingers.isVisible());
}

void MainWindow::rotationDataChanged() {
    if (m_shuttingDown)
        return;
    if (settings.rotationShowNextSong())
        resizeRotation();
    updateRotationDuration();
    QString sep = "•";
    requestsDialog->rotationChanged();
    QString statusBarText = "Singers: ";
    statusBarText += QString::number(rotModel.rowCount());
    labelSingerCount.setText(statusBarText);
    QString tickerText;
    if (settings.tickerCustomString() != "") {
        tickerText += settings.tickerCustomString() + " " + sep + " ";
        QString cs = rotModel.getSingerName(rotModel.currentSinger());
        int nsPos;
        if (cs == "") {
            cs = rotModel.getSingerName(rotModel.singerIdAtPosition(0));
            if (cs == "")
                cs = "[nobody]";
            nsPos = 0;
        } else
            nsPos = rotModel.getSingerPosition(rotModel.currentSinger());
        QString ns = "[nobody]";
        if (rotModel.rowCount() > 0) {
            if (nsPos + 1 < rotModel.rowCount())
                nsPos++;
            else
                nsPos = 0;
            ns = rotModel.getSingerName(rotModel.singerIdAtPosition(nsPos));
        }
        tickerText.replace("%cs", cs);
        tickerText.replace("%ns", ns);
        tickerText.replace("%rc", QString::number(rotModel.rowCount()));
        if (ui->labelArtist->text() == "None" && ui->labelTitle->text() == "None")
            tickerText.replace("%curSong", "None");
        else
            tickerText.replace("%curSong", ui->labelArtist->text() + " - " + ui->labelTitle->text());
        tickerText.replace("%curArtist", ui->labelArtist->text());
        tickerText.replace("%curTitle", ui->labelTitle->text());
        tickerText.replace("%curSinger", cs);
        tickerText.replace("%nextSinger", ns);

    }
    if (settings.tickerShowRotationInfo()) {
        tickerText += "Singers: ";
        tickerText += QString::number(rotModel.rowCount());
        tickerText += " " + sep + " Current: ";
        int displayPos;
        QString curSingerName = rotModel.getSingerName(rotModel.currentSinger());
        if (curSingerName != "") {
            tickerText += curSingerName;
            displayPos = rotModel.getSingerPosition(rotModel.currentSinger());
        } else {
            tickerText += "None ";
            displayPos = -1;
        }
        int listSize;
        if (settings.tickerFullRotation() || (rotModel.rowCount() < settings.tickerShowNumSingers())) {
            if (curSingerName == "")
                listSize = rotModel.rowCount();
            else
                listSize = rotModel.rowCount() - 1;
            if (listSize > 0)
                tickerText += " " + sep + " Upcoming: ";
        } else {
            listSize = settings.tickerShowNumSingers();
            tickerText += " " + sep + " Next ";
            tickerText += QString::number(settings.tickerShowNumSingers());
            tickerText += " Singers: ";
        }
        for (int i = 0; i < listSize; i++) {
            if (displayPos + 1 < rotModel.rowCount())
                displayPos++;
            else
                displayPos = 0;
            tickerText += QString::number(i + 1);
            tickerText += ") ";
            tickerText += rotModel.getSingerName(rotModel.singerIdAtPosition(displayPos));
            if (i < listSize - 1)
                tickerText += " ";
        }
    }
    cdgWindow->setTickerText(tickerText);
}

void MainWindow::silenceDetectedKar() {
    logger->info("{} Karaoke music silence detected", m_loggingPrefix);
    kMediaBackend.rawStop();
    if (settings.karaokeAutoAdvance())
        kAASkip = false;
    bmMediaBackend.fadeIn();
}

void MainWindow::silenceDetectedBm() {
    if (bmMediaBackend.position() > 10000 && bmMediaBackend.position() < (bmMediaBackend.duration() - 3)) {
        logger->info("{} Break music silence detected, reporting EndOfMediaState to trigger playlist advance", m_loggingPrefix);
        bmMediaStateChanged(MediaBackend::EndOfMediaState);
    }
}

void MainWindow::on_tableViewDB_customContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->tableViewDB->indexAt(pos);
    if (index.isValid()) {
        dbRtClickFile = karaokeSongsModel.getPath(
                index.sibling(index.row(), TableModelKaraokeSongs::COL_ID).data().toInt());
        QMenu contextMenu(this);
        contextMenu.addAction("Preview", this, &MainWindow::previewCdg);
        contextMenu.addSeparator();
        contextMenu.addAction("Edit", this, &MainWindow::editSong);
        contextMenu.addAction("Mark bad", this, &MainWindow::markSongBad);
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_tableViewRotation_customContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->tableViewRotation->indexAt(pos);
    if (index.isValid()) {
        m_rtClickRotationSingerId = index.data(Qt::UserRole).toInt();
        QMenu contextMenu(this);
        if (ui->tableViewRotation->selectionModel()->selectedRows().size() > 1) {
            contextMenu.addAction("Delete", scutDeleteSinger, &QShortcut::activated);
        } else {
            contextMenu.addAction("Rename", this, &MainWindow::renameSinger);
            contextMenu.addAction("Set as top of rotation", [&] () {
                rotModel.setRotationTopSingerId(m_rtClickRotationSingerId);
            });
        }
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::sfxButton_customContextMenuRequested([[maybe_unused]]const QPoint &pos) {
    auto *btn = (SoundFxButton *) sender();
    lastRtClickedSfxBtn.path = btn->buttonData().toString();
    lastRtClickedSfxBtn.name = btn->text();
    QMenu contextMenu(this);
    contextMenu.addAction("Remove", this, &MainWindow::removeSfxButton);
    contextMenu.exec(QCursor::pos());
}

void MainWindow::renameSinger() {
    bool ok;
    QString currentName = rotModel.getSingerName(m_rtClickRotationSingerId);
    QString name = QInputDialog::getText(this, "Rename singer", "New name:", QLineEdit::Normal, currentName, &ok);
    if (ok && !name.isEmpty()) {
        if ((name.toLower() == currentName.toLower() && name != currentName) || !rotModel.singerExists(name)) {
            rotModel.singerSetName(m_rtClickRotationSingerId, name);
        } else if (rotModel.singerExists(name)) {
            QMessageBox::warning(this, "Singer exists!", "A singer named " + name +
                                                         " already exists. Please choose a unique name and try again. The operation has been cancelled.",
                                 QMessageBox::Ok);
        }

    }
}

void MainWindow::on_tableViewBmPlaylist_customContextMenuRequested([[maybe_unused]]const QPoint &pos) {
    QMenu contextMenu(this);
    contextMenu.addAction("Delete", scutDeletePlSong, &QShortcut::activated);
    contextMenu.exec(QCursor::pos());
}

void MainWindow::on_tableViewQueue_customContextMenuRequested(const QPoint &pos) {
    int selCount = ui->tableViewQueue->selectionModel()->selectedRows().size();
    if (selCount == 1) {
        QModelIndex index = ui->tableViewQueue->indexAt(pos);
        if (index.isValid()) {
            dbRtClickFile = index.sibling(index.row(), TableModelQueueSongs::COL_PATH).data().toString();
            m_rtClickQueueSongId = index.sibling(index.row(), 0).data().toInt();
            dlgKeyChange->setActiveSong(m_rtClickQueueSongId);
            QMenu contextMenu(this);
            contextMenu.addAction("Preview", this, &MainWindow::previewCdg);
            contextMenu.addSeparator();
            contextMenu.addAction("Set Key Change", this, &MainWindow::setKeyChange);
            contextMenu.addAction("Toggle played", this, &MainWindow::toggleQueuePlayed);
            contextMenu.addSeparator();
            contextMenu.addAction("Delete", scutDeleteSong, &QShortcut::activated);
            contextMenu.exec(QCursor::pos());
        }
    } else if (selCount > 1) {
        QMenu contextMenu(this);
        contextMenu.addAction("Set Played", this, &MainWindow::setMultiPlayed);
        contextMenu.addAction("Set Unplayed", this, &MainWindow::setMultiUnplayed);
        contextMenu.addSeparator();
        contextMenu.addAction("Delete", scutDeleteSong, &QShortcut::activated);

        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_sliderProgress_sliderPressed() {
    sliderPositionPressed = true;
}

void MainWindow::on_sliderProgress_sliderReleased() {
    kMediaBackend.setPosition(ui->sliderProgress->value());
    sliderPositionPressed = false;
}

void MainWindow::setKeyChange() {
    dlgKeyChange->show();
}

void MainWindow::toggleQueuePlayed() {
    qModel.setPlayed(m_rtClickQueueSongId, !qModel.getPlayed(m_rtClickQueueSongId));
    updateRotationDuration();
}

void MainWindow::previewCdg() {
    if (!QFile::exists(dbRtClickFile)) {
        QMessageBox::warning(this, tr("Missing File!"),
                             "Specified karaoke file missing, preview aborted!\n\n" + dbRtClickFile, QMessageBox::Ok);
        return;
    }
    auto *videoPreview = new DlgVideoPreview(dbRtClickFile, this);
    videoPreview->setAttribute(Qt::WA_DeleteOnClose);
    videoPreview->show();
}

void MainWindow::editSong() {
    bool isCdg = false;
    if (QFileInfo(dbRtClickFile).suffix().toLower() == "cdg")
        isCdg = true;
    QString mediaFile;
    if (isCdg)
        mediaFile = DbUpdater::findMatchingAudioFile(dbRtClickFile);
    TableModelKaraokeSourceDirs model;
    SourceDir srcDir = model.getDirByPath(dbRtClickFile);
    int rowId;
    QString artist;
    QString title;
    QString songId;
    QSqlQuery query;
    query.prepare("SELECT songid,artist,title,discid FROM dbsongs WHERE path = :path LIMIT 1");
    query.bindValue(":path", dbRtClickFile);
    query.exec();
    if (!query.next())
        logger->warn("{} Unable to find song in db!", m_loggingPrefix);
    artist = query.value("artist").toString();
    title = query.value("title").toString();
    songId = query.value("discid").toString();
    rowId = query.value("songid").toInt();
    logger->info("{} db song match: {} : {} - {} - {}", m_loggingPrefix, rowId, artist.toStdString(), title.toStdString(), songId.toStdString());
    bool allowRename = true;
    bool showSongId = true;
    if (srcDir.getPattern() == SourceDir::AT || srcDir.getPattern() == SourceDir::TA)
        showSongId = false;
    if (srcDir.getPattern() == SourceDir::CUSTOM || srcDir.getPattern() == SourceDir::METADATA)
        allowRename = false;
    if (srcDir.getIndex() == -1) {
        allowRename = false;
        QMessageBox msgBoxErr;
        msgBoxErr.setText("Unable to find a configured source path containing the file.");
        msgBoxErr.setInformativeText(
                "You won't be able to rename the file.  To fix this, ensure that a source directory is configured in the database settings which contains this file.");
        msgBoxErr.setStandardButtons(QMessageBox::Ok);
        msgBoxErr.exec();
    }
    DlgEditSong dlg(artist, title, songId, showSongId, allowRename, this);
    int result = dlg.exec();
    if (result != QDialog::Accepted)
        return;
    if (artist == dlg.artist() && title == dlg.title() && songId == dlg.songId())
        return;
    if (dlg.renameFile()) {
        if (!QFileInfo(dbRtClickFile).isWritable()) {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Unable to rename file");
            msgBoxErr.setInformativeText(
                    "Unable to rename file, your user does not have write permissions. Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
        if (isCdg) {
            if (!QFileInfo(mediaFile).isWritable()) {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Unable to rename file");
                msgBoxErr.setInformativeText(
                        "Unable to rename file, your user does not have write permissions. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
        }
        QString newFn;
        QString newMediaFn;
        bool unsupported = false;
        switch (srcDir.getPattern()) {
            case SourceDir::SAT:
                newFn = dlg.songId() + " - " + dlg.artist() + " - " + dlg.title() + "." +
                        QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.songId() + " - " + dlg.artist() + " - " + dlg.title() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::STA:
                newFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." +
                        QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::ATS:
                newFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." +
                        QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::TAS:
                newFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." +
                        QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::S_T_A:
                newFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." +
                        QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::AT:
                newFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::TA:
                newFn = dlg.title() + " - " + dlg.artist() + "." + QFileInfo(dbRtClickFile).suffix();
                if (isCdg)
                    newMediaFn = dlg.title() + " - " + dlg.artist() + "." + QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::CUSTOM:
            case SourceDir::METADATA:
                unsupported = true;
                break;
        }
        if (unsupported) {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Unable to rename file");
            msgBoxErr.setInformativeText(
                    "Unable to rename file, renaming custom and metadata based source files is not supported. Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
        if (QFile::exists(newFn)) {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Unable to rename file");
            msgBoxErr.setInformativeText(
                    "Unable to rename file, a file by that name already exists in the same directory. Operation cancelled.");
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
        if (isCdg) {
            if (QFile::exists(newMediaFn)) {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Unable to rename file");
                msgBoxErr.setInformativeText(
                        "Unable to rename file, a file by that name already exists in the same directory. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
        }
        QString newFilePath = QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn;
        if (newFilePath != dbRtClickFile) {
            if (!QFile::rename(dbRtClickFile, QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn)) {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Error while renaming file!");
                msgBoxErr.setInformativeText("An unknown error occurred while renaming the file. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
            if (isCdg) {
                if (!QFile::rename(mediaFile,
                                   QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newMediaFn)) {
                    QMessageBox msgBoxErr;
                    msgBoxErr.setText("Error while renaming file!");
                    msgBoxErr.setInformativeText(
                            "An unknown error occurred while renaming the file. Operation cancelled.");
                    msgBoxErr.setStandardButtons(QMessageBox::Ok);
                    msgBoxErr.exec();
                    return;
                }
            }
        }
        logger->info("{} New filename: {}", m_loggingPrefix, newFn.toStdString());
        query.prepare(
                "UPDATE dbsongs SET artist = :artist, title = :title, discid = :songid, path = :path, filename = :filename, searchstring = :searchstring WHERE songid = :rowid");
        QString newArtist = dlg.artist();
        QString newTitle = dlg.title();
        QString newSongId = dlg.songId();
        QString newPath = QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn;
        QString newSearchString =
                QFileInfo(newPath).completeBaseName() + " " + newArtist + " " + newTitle + " " + newSongId;
        query.bindValue(":artist", newArtist);
        query.bindValue(":title", newTitle);
        query.bindValue(":songid", newSongId);
        query.bindValue(":path", newPath);
        query.bindValue(":filename", newFn);
        query.bindValue(":searchstring", newSearchString);
        query.bindValue(":rowid", rowId);
        query.exec();
        qInfo() << query.lastError();
        if (!query.lastError().isValid()) {
            query.prepare(
                    "UPDATE mem.dbsongs SET artist = :artist, title = :title, discid = :songid, path = :path, filename = :filename, searchstring = :searchstring WHERE songid = :rowid");
            query.bindValue(":artist", newArtist);
            query.bindValue(":title", newTitle);
            query.bindValue(":songid", newSongId);
            query.bindValue(":path", newPath);
            query.bindValue(":filename", newFn);
            query.bindValue(":searchstring", newSearchString);
            query.bindValue(":rowid", rowId);
            query.exec();
            QMessageBox msgBoxInfo;
            msgBoxInfo.setText("Edit successful");
            msgBoxInfo.setInformativeText("The file has been renamed and the database has been updated successfully.");
            msgBoxInfo.setStandardButtons(QMessageBox::Ok);
            msgBoxInfo.exec();
            karaokeSongsModel.loadData();
            return;
        } else {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error while updating the database!");
            msgBoxErr.setInformativeText(query.lastError().text());
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
            //QFile::rename(QFileInfo(dbRtClickFile).absoluteDir().absolutePath() + "/" + newFn, dbRtClickFile);
        }
    } else {
        query.prepare(
                "UPDATE dbsongs SET artist = :artist, title = :title, discid = :songid, searchstring = :searchstring WHERE songid = :rowid");
        QString newArtist = dlg.artist();
        QString newTitle = dlg.title();
        QString newSongId = dlg.songId();
        QString newSearchString =
                QFileInfo(dbRtClickFile).completeBaseName() + " " + newArtist + " " + newTitle + " " + newSongId;
        query.bindValue(":artist", newArtist);
        query.bindValue(":title", newTitle);
        query.bindValue(":songid", newSongId);
        query.bindValue(":searchstring", newSearchString);
        query.bindValue(":rowid", rowId);
        query.exec();
        qInfo() << query.lastError();
        if (!query.lastError().isValid()) {
            query.prepare(
                    "UPDATE mem.dbsongs SET artist = :artist, title = :title, discid = :songid, searchstring = :searchstring WHERE songid = :rowid");
            query.bindValue(":artist", newArtist);
            query.bindValue(":title", newTitle);
            query.bindValue(":songid", newSongId);
            query.bindValue(":searchstring", newSearchString);
            query.bindValue(":rowid", rowId);
            query.exec();
            QMessageBox msgBoxInfo;
            msgBoxInfo.setText("Edit successful");
            msgBoxInfo.setInformativeText("The database has been updated successfully.");
            msgBoxInfo.setStandardButtons(QMessageBox::Ok);
            msgBoxInfo.exec();
            karaokeSongsModel.loadData();
            return;
        } else {
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error while updating the database!");
            msgBoxErr.setInformativeText(query.lastError().text());
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        }
    }

}

void MainWindow::markSongBad() {
    QMessageBox msgBox;
    QMessageBox msgBoxResult;
    msgBox.setText("Marking song as bad");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText("Would you like mark the file as bad in the DB, or remove it from disk permanently?");
    auto markBadButton = msgBox.addButton(tr("Mark Bad"), QMessageBox::ActionRole);
    auto removeFileButton = msgBox.addButton(tr("Remove File"), QMessageBox::ActionRole);
    auto cancelButton = msgBox.addButton(QMessageBox::Cancel);
    msgBox.exec();
    if (msgBox.clickedButton() == markBadButton) {
        karaokeSongsModel.markSongBad(dbRtClickFile);
        msgBoxResult.setText("File marked as bad and will no longer show up in searches.");
        msgBoxResult.setIcon(QMessageBox::Information);
        msgBoxResult.exec();
    } else if (msgBox.clickedButton() == removeFileButton) {
        bool isCdg = false;
        if (QFileInfo(dbRtClickFile).suffix().toLower() == "cdg")
            isCdg = true;
        QString mediaFile;
        if (isCdg)
            mediaFile = DbUpdater::findMatchingAudioFile(dbRtClickFile);
        QFile file(dbRtClickFile);
        auto ret = karaokeSongsModel.removeBadSong(dbRtClickFile);
        switch (ret) {
            case TableModelKaraokeSongs::DELETE_OK:
                msgBoxResult.setText("File removed successfully");
                msgBoxResult.setIcon(QMessageBox::Information);
                msgBoxResult.exec();
                break;
            case TableModelKaraokeSongs::DELETE_FAIL:
                msgBoxResult.setText("Error while removing file");
                msgBoxResult.setIcon(QMessageBox::Warning);
                msgBoxResult.setInformativeText(
                        "Unable to remove the file.  Please check file permissions.\nOperation cancelled.");
                msgBoxResult.exec();
                break;
            case TableModelKaraokeSongs::DELETE_CDG_AUDIO_FAIL:
                msgBoxResult.setText("Error while removing file");
                msgBoxResult.setIcon(QMessageBox::Warning);
                msgBoxResult.setInformativeText(
                        "The cdg file was deleted, but there was an error while deleting the matching media file.  You will need to manually remove the file.");
                msgBoxResult.exec();
                break;
        }
    }
}

void MainWindow::karaokeAATimerTimeout() {
    logger->info("{} KaraokeAA - timer timeout", m_loggingPrefix);
    m_timerKaraokeAA.stop();
    cdgWindow->showAlert(false);
    if (kAASkip) {
        logger->info("{} KaraokeAA - Aborted via stop button", m_loggingPrefix);
        kAASkip = false;
    } else {
        curSinger = rotModel.getSingerName(kAANextSinger);
        curArtist = rotModel.nextSongArtist(kAANextSinger);
        curTitle = rotModel.nextSongTitle(kAANextSinger);
        ui->labelArtist->setText(curArtist);
        ui->labelTitle->setText(curTitle);
        ui->labelSinger->setText(curSinger);
        if (settings.treatAllSingersAsRegs() || rotModel.singerIsRegular(kAANextSinger)) {
            historySongsModel.saveSong(
                    curSinger,
                    kAANextSongPath,
                    curArtist,
                    curTitle,
                    rotModel.nextSongSongId(kAANextSinger),
                    rotModel.nextSongKeyChg(kAANextSinger)
            );
        }
        karaokeSongsModel.updateSongHistory(karaokeSongsModel.getIdForPath(kAANextSongPath));
        play(kAANextSongPath);
        kMediaBackend.setPitchShift(rotModel.nextSongKeyChg(kAANextSinger));
        qModel.setPlayed(rotModel.nextSongQueueId(kAANextSinger));
        rotModel.setCurrentSinger(kAANextSinger);
        rotDelegate.setCurrentSinger(kAANextSinger);
        if (settings.rotationAltSortOrder()) {
            auto curSingerPos = rotModel.getSingerPosition(kAANextSinger);
            m_curSingerOriginalPosition = curSingerPos;
            if (curSingerPos != 0)
                rotModel.singerMove(curSingerPos, 0);
        }
    }
}

void MainWindow::timerButtonFlashTimeout() {
    static QString normalSS = " \
        QPushButton { \
            border: 2px solid #8f8f91; \
            border-radius: 6px; \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f7fa, stop: 1 #dadbde); \
            min-width: 80px; \
            padding-left: 5px; \
            padding-right: 5px; \
            padding-top: 3px; \
            padding-bottom: 3px; \
        } \
        QPushButton:pressed { \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); \
        } \
        QPushButton:flat { \
            border: none; /* no border for a flat push button */ \
        } \
        QPushButton:default { \
            border-color: navy; /* make the default button prominent */ \
        }";

    static QString blinkSS = " \
        QPushButton { \
            border: 2px solid #8f8f91; \
            border-radius: 6px; \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #f6f700, stop: 1 #dadb00); \
            min-width: 80px; \
            padding-left: 5px; \
            padding-right: 5px; \
            padding-top: 3px; \
            padding-bottom: 3px; \
        } \
        QPushButton:pressed { \
            background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); \
        } \
        QPushButton:flat { \
            border: none; /* no border for a flat push button */ \
        } \
        QPushButton:default { \
            border-color: navy; /* make the default button prominent */ \
        } \
    ";

    if (requestsDialog->numRequests() > 0) {
        static bool flashed = false;
        if (settings.theme() != 0) {
            auto normal = this->palette().button().color();
            auto blink = QColor("yellow");
            auto blinkTxt = QColor("black");
            auto normalTxt = this->palette().buttonText().color();
            auto palette = QPalette(ui->pushButtonIncomingRequests->palette());
            palette.setColor(QPalette::Button, (flashed) ? normal : blink);
            palette.setColor(QPalette::ButtonText, (flashed) ? normalTxt : blinkTxt);
            ui->pushButtonIncomingRequests->setPalette(palette);
            ui->pushButtonIncomingRequests->setText(
                    " Requests (" + QString::number(requestsDialog->numRequests()) + ") ");
            flashed = !flashed;
        } else {
            ui->pushButtonIncomingRequests->setText(
                    " Requests (" + QString::number(requestsDialog->numRequests()) + ") ");
            ui->pushButtonIncomingRequests->setStyleSheet((flashed) ? normalSS : blinkSS);
            flashed = !flashed;
        }
        update();
    } else if (ui->pushButtonIncomingRequests->text() != "Requests") {
        if (settings.theme() != 0) {
            ui->pushButtonIncomingRequests->setPalette(this->palette());
            ui->pushButtonIncomingRequests->setText("Requests");
        } else {
            ui->pushButtonIncomingRequests->setStyleSheet(normalSS);
            ui->pushButtonIncomingRequests->setText(" Requests ");
        }
        update();
    }
}

void MainWindow::bmAddPlaylist(const QString &title) {
    if (bmPlaylistsModel->insertRow(bmPlaylistsModel->rowCount())) {
        QModelIndex index = bmPlaylistsModel->index(bmPlaylistsModel->rowCount() - 1, 1);
        bmPlaylistsModel->setData(index, title);
        bmPlaylistsModel->submitAll();
        bmPlaylistsModel->select();
        ui->comboBoxBmPlaylists->setCurrentIndex(index.row());
    }
}

void MainWindow::bmDbUpdated() {
    bmDbModel.loadDatabase();
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
}

void MainWindow::bmDbCleared() {
    logger->info("{} bmDbCleared fired", m_loggingPrefix);
    bmDbModel.loadDatabase();
    bmAddPlaylist("Default");
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
}

void MainWindow::on_actionManage_Break_DB_triggered() {
    bmDbDialog->show();
}

void MainWindow::bmMediaStateChanged(const MediaBackend::State &newState) {
    static MediaBackend::State lastState = MediaBackend::StoppedState;
    if (newState == lastState)
        return;
    lastState = newState;
    switch (newState) {
        case MediaBackend::StoppedState:
            resetBmLabels();
            break;
        case MediaBackend::EndOfMediaState: {
            if (ui->checkBoxBmBreak->isChecked()) {
                ui->checkBoxBmBreak->setChecked(false);
                bmMediaBackend.stop(true);
                resetBmLabels();
                return;
            }
            auto plSong = playlistSongsModel.getNextPlSong();
            if (plSong.has_value()) {
                bmMediaBackend.setMedia(plSong->get().path);
                playlistSongsModel.setCurrentPosition(plSong->get().position);
                logger->info("{} Break music auto-advancing to song: {}", m_loggingPrefix, plSong->get().path.toStdString());
                bmMediaBackend.stop(true);
                bmMediaBackend.play();
                if (kMediaBackend.state() == MediaBackend::PlayingState)
                    bmMediaBackend.fadeOutImmediate();
            } else {
                bmMediaBackend.stop(true);
                resetBmLabels();
            }
            break;
        }
        case MediaBackend::PlayingState: {
            auto plSong = playlistSongsModel.getCurrentSong();
            if (plSong.has_value())
                ui->labelBmPlaying->setText(plSong->get().artist + " - " + plSong->get().title);
            auto plNextSong = playlistSongsModel.getNextPlSong();
            if (!ui->checkBoxBmBreak->isChecked() && plNextSong.has_value())
                ui->labelBmNext->setText(plNextSong->get().artist + " - " + plNextSong->get().title);
            else
                ui->labelBmNext->setText("None - Breaking after current song");
            break;
        }
        case MediaBackend::PausedState:
            break;
        case MediaBackend::UnknownState:
            resetBmLabels();
            break;
    }
}

void MainWindow::bmMediaPositionChanged(const qint64 &position) {
    if (!sliderBmPositionPressed) {
        ui->sliderBmPosition->setValue(position);
    }
    ui->labelBmPosition->setText(QTime(0, 0, 0, 0).addMSecs(position).toString("m:ss"));
    ui->labelBmRemaining->setText(QTime(0, 0, 0, 0).addMSecs(bmMediaBackend.duration() - position).toString("m:ss"));
}

void MainWindow::bmMediaDurationChanged(const qint64 &duration) {
    ui->sliderBmPosition->setMaximum(duration);
    ui->labelBmDuration->setText(QTime(0, 0, 0, 0).addMSecs(duration).toString("m:ss"));
}

void MainWindow::on_tableViewBmPlaylist_clicked(const QModelIndex &index) {
    logger->trace("{} DNDDEBUG - acceptDrops(): {}", m_loggingPrefix, ui->tableViewBmPlaylist->acceptDrops());
    logger->trace("{} DNDDEBUG - testAttribute(Qt::WA_AcceptDrops): {}", m_loggingPrefix, ui->tableViewBmPlaylist->testAttribute(Qt::WA_AcceptDrops));
    logger->trace("{} DNDDEBUG - hasMouseTracking(): {}", m_loggingPrefix, ui->tableViewBmPlaylist->hasMouseTracking());
    logger->trace("{} DNDDEBUG - mainwindow hasMouseTracking(): ", m_loggingPrefix, hasMouseTracking());
    logger->trace("{} DNDDEBUG - supportedDropActions(): ", m_loggingPrefix, playlistSongsModel.supportedDropActions());

    if (index.column() == TableModelPlaylistSongs::COL_PATH) {
        if (playlistSongsModel.isCurrentlyPlayingSong(index.data(Qt::UserRole).toInt())) {
            if (bmMediaBackend.state() == MediaBackend::PlayingState ||
                bmMediaBackend.state() == MediaBackend::PausedState) {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Unable to remove");
                msgBox.setText(
                        "The playlist song you are trying to remove is currently playing and can not be removed.");
                msgBox.exec();
                return;
            }
            playlistSongsModel.setCurrentPosition(-1);
            resetBmLabels();
        }
        playlistSongsModel.deleteSong(index.row());
        playlistSongsModel.savePlaylistChanges();
        if (playlistSongsModel.currentPosition() > index.row()) {
            playlistSongsModel.setCurrentPosition(playlistSongsModel.currentPosition() - 1);
        }
        if (ui->checkBoxBmBreak->isChecked()) {
            ui->labelBmNext->setText("None - Breaking after current song");
            return;
        }
        auto nextSong = playlistSongsModel.getNextPlSong();
        if (nextSong.has_value())
            ui->labelBmNext->setText(nextSong->get().artist + " - " + nextSong->get().title);
        else
            ui->labelBmNext->setText("None - Breaking after current song");
    }
}

void MainWindow::on_comboBoxBmPlaylists_currentIndexChanged(const int &index) {
    bmCurrentPlaylist = bmPlaylistsModel->index(index, 0).data().toInt();
    playlistSongsModel.setCurrentPlaylist(bmCurrentPlaylist);
    auto nextPlSong = playlistSongsModel.getNextPlSong();
    if (nextPlSong.has_value())
        ui->labelBmNext->setText(nextPlSong->get().artist + " - " + nextPlSong->get().title);
    else
        ui->labelBmNext->setText("None - Breaking after current song");
    ui->tableViewBmPlaylist->clearSelection();
}

void MainWindow::on_checkBoxBmBreak_toggled(const bool &checked) {
    if (!checked) {
        auto nextSong = playlistSongsModel.getNextPlSong();
        if (nextSong.has_value())
            ui->labelBmNext->setText(nextSong->get().artist + " - " + nextSong->get().title);
        return;
    }
    ui->labelBmNext->setText("None - Stopping after current song");
}

void MainWindow::on_tableViewBmDb_doubleClicked(const QModelIndex &index) {
    int songId = index.sibling(index.row(), 0).data().toInt();
    playlistSongsModel.addSong(songId);
    playlistSongsModel.savePlaylistChanges();
}

void MainWindow::on_buttonBmStop_clicked() {
    bmMediaBackend.stop(false);
}

void MainWindow::on_lineEditBmSearch_returnPressed() {
    bmDbModel.search(ui->lineEditBmSearch->text());
}

void MainWindow::on_tableViewBmPlaylist_doubleClicked(const QModelIndex &index) {
    if (bmMediaBackend.state() == MediaBackend::PlayingState || bmMediaBackend.state() == MediaBackend::PausedState)
        bmMediaBackend.stop(false);
    playlistSongsModel.setCurrentPosition(index.row());
    auto plSong = playlistSongsModel.getCurrentSong();
    if (!plSong.has_value())
        return;
    bmMediaBackend.setMedia(plSong->get().path);
    bmMediaBackend.play();
    if (kMediaBackend.state() != MediaBackend::PlayingState)
        bmMediaBackend.fadeInImmediate();
}

void MainWindow::on_buttonBmPause_clicked(const bool &checked) {
    if (checked)
        bmMediaBackend.pause();
    else
        bmMediaBackend.play();
}

bool MainWindow::bmPlaylistExists(const QString &name) {
    for (int i = 0; i < bmPlaylistsModel->rowCount(); i++) {
        if (bmPlaylistsModel->index(i, 1).data().toString().toLower() == name.toLower())
            return true;
    }
    return false;
}

void MainWindow::on_actionDisplay_Metadata_toggled(const bool &arg1) {
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_ARTIST, !arg1);
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_TITLE, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_ARTIST, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_TITLE, !arg1);
    settings.bmSetShowMetadata(arg1);
    autosizeBmViews();
}

void MainWindow::on_actionDisplay_Filenames_toggled(const bool &arg1) {
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_FILENAME, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_FILENAME, !arg1);
    settings.bmSetShowFilenames(arg1);
    autosizeBmViews();
}

void MainWindow::on_actionManage_Karaoke_DB_triggered() {
    dbDialog->showNormal();
}

void MainWindow::on_actionPlaylistNew_triggered() {
    bool ok;
    QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal,
                                          tr("New Playlist"), &ok);
    if (ok && !title.isEmpty()) {
        bmAddPlaylist(title);
    }
}

void MainWindow::on_actionPlaylistImport_triggered() {
    QString importFile = QFileDialog::getOpenFileName(
            this,
            tr("Select playlist to import"),
            QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0),
            tr("m3u playlist(*.m3u)"),
            nullptr,
            QFileDialog::DontUseNativeDialog
            );
    if (importFile != "") {
        QFileInfo fi(importFile);
        QString importPath = fi.absoluteDir().path();
        QStringList files;
        QFile textFile;
        textFile.setFileName(importFile);
        textFile.open(QFile::ReadOnly);
        QTextStream textStream(&textFile);
        while (true) {
            QString line = textStream.readLine();
            if (line.isNull())
                break;
            else {
                if (!line.startsWith("#"))
                    files.append(line.replace("\\", "/"));
            }
        }
        bool ok;
        QString plTitle = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal,
                                                tr("New Playlist"), &ok);

        if (ok && !plTitle.isEmpty()) {
            if (!bmPlaylistExists(plTitle)) {
                if (bmPlaylistsModel->insertRow(bmPlaylistsModel->rowCount())) {
                    QModelIndex index = bmPlaylistsModel->index(bmPlaylistsModel->rowCount() - 1, 1);
                    bmPlaylistsModel->setData(index, plTitle);
                    bmPlaylistsModel->submitAll();
                    bmPlaylistsModel->select();
                    ui->comboBoxBmPlaylists->setCurrentIndex(index.row());
                    playlistSongsModel.setCurrentPlaylist(bmPlaylistsModel->index(index.row(), 0).data().toInt());

                }
            }
        }
        QSqlQuery query;
        query.exec("BEGIN TRANSACTION");
        TagReader reader;
        for (int i = 0; i < files.size(); i++) {
            if (QFile(files.at(i)).exists()) {
                reader.setMedia(files.at(i).toLocal8Bit());
                QString duration = QString::number(reader.getDuration() / 1000);
                QString artist = reader.getArtist();
                QString title = reader.getTitle();
                QString filename = QFileInfo(files.at(i)).fileName();
                QString queryString =
                        "INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(\"" +
                        artist + "\",\"" + title + "\",\"" + files.at(i) + "\",\"" + filename + "\",\"" + duration +
                        "\",\"" + artist + title + filename + "\")";
                query.exec(queryString);
            } else if (QFile(importPath + "/" + files.at(i)).exists()) {
                reader.setMedia(importPath + "/" + files.at(i).toLocal8Bit());
                QString duration = QString::number(reader.getDuration() / 1000);
                QString artist = reader.getArtist();
                QString title = reader.getTitle();
                QString filename = QFileInfo(files.at(i)).fileName();
                QString path = importPath + "/" + files.at(i);
                QString searchstring = artist + " " + title + " " + filename;
                QString queryString = "INSERT OR IGNORE INTO bmsongs (artist,title,path,filename,duration,searchstring) VALUES(:artist,:title,:path,:filename,:duration,:searchstring)";
                query.prepare(queryString);
                query.bindValue(":artist", artist);
                query.bindValue(":title", title);
                query.bindValue(":path", path);
                query.bindValue(":filename", filename);
                query.bindValue(":duration", duration);
                query.bindValue(":searchstring", searchstring);
                query.exec();
            }
        }
        query.exec("COMMIT TRANSACTION");
        bmDbModel.loadDatabase();
        QApplication::processEvents();
        QList<int> songIds;
        for (int i = 0; i < files.size(); i++) {
            int songId = bmDbModel.getSongId(files.at(i));
            if (songId >= 0) {
                songIds.push_back(songId);
            } else {
                songId = bmDbModel.getSongId(importPath + "/" + files.at(i));
                if (songId >= 0) {
                    songIds.push_back(songId);
                }
            }
        }
        qInfo() << songIds;
        playlistSongsModel.setCurrentPlaylist(bmCurrentPlaylist);
        std::for_each(songIds.begin(), songIds.end(), [&](int songId) {
            playlistSongsModel.addSong(songId);
        });
        playlistSongsModel.savePlaylistChanges();
    }
}

void MainWindow::on_actionPlaylistExport_triggered() {
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() +
                              ui->comboBoxBmPlaylists->currentText() + ".m3u";
    logger->debug("{} Default save location: {}", m_loggingPrefix, defaultFilePath.toStdString());
    QString saveFilePath = QFileDialog::getSaveFileName(
            this,
            tr("Select filename to save playlist as"),
            defaultFilePath,
            tr("m3u playlist(*.m3u)"),
            nullptr,
            QFileDialog::DontUseNativeDialog
            );
    if (saveFilePath != "") {
        QFile file(saveFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Error saving file"),
                                 tr("Unable to open selected file for writing.  Please verify that you have the proper permissions to write to that location."),
                                 QMessageBox::Close);
            return;
        }
        QTextStream out(&file);
        for (int i = 0; i < playlistSongsModel.rowCount(); i++) {
            out << playlistSongsModel.index(i, TableModelPlaylistSongs::COL_PATH).data().toString() << "\n";
        }
    }
}

void MainWindow::on_actionPlaylistDelete_triggered() {
    QMessageBox msgBox;
    msgBox.setText("Are you sure?");
    msgBox.setInformativeText(
            "Are you sure you want to delete the current playlist?  If you have not exported it, you will not be able to undo this action!");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.addButton(QMessageBox::Cancel);
    QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        QSqlQuery query;
        query.exec("DELETE FROM bmplsongs WHERE playlist == " + QString::number(playlistSongsModel.currentPlaylist()));
        query.exec(
                "DELETE FROM bmplaylists WHERE playlistid == " + QString::number(playlistSongsModel.currentPlaylist()));
        bmPlaylistsModel->select();
        if (bmPlaylistsModel->rowCount() == 0) {
            bmAddPlaylist("Default");
        }
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
        playlistSongsModel.setCurrentPlaylist(bmPlaylistsModel->index(0, 0).data().toInt());
    }
}

void MainWindow::on_buttonBmSearch_clicked() {
    bmDbModel.search(ui->lineEditBmSearch->text());
}

void MainWindow::on_actionAbout_triggered() {
    QString title;
    QString text;
    QString date = QString::fromLocal8Bit(__DATE__) + " " + QString(__TIME__);
    title = "About OpenKJ";
    text = "OpenKJ\n\nVersion: " + QString(OKJ_VERSION_STRING) + " " + QString(OKJ_VERSION_BRANCH) + "\nBuilt: " +
           date + "\nLicense: GPL v3+";
    QMessageBox::about(this, title, text);
}

void MainWindow::on_pushButtonMplxLeft_toggled(const bool &checked) {
    if (checked)
        settings.setMplxMode(Multiplex_LeftChannel);
}

void MainWindow::on_pushButtonMplxBoth_toggled(const bool &checked) {
    if (checked)
        settings.setMplxMode(Multiplex_Normal);
}

void MainWindow::on_pushButtonMplxRight_toggled(const bool &checked) {
    if (checked)
        settings.setMplxMode(Multiplex_RightChannel);
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1) {
    if (!settings.progressiveSearchEnabled())
        return;
    ui->tableViewDB->scrollToTop();
    static QString lastVal;
    if (arg1.trimmed() != lastVal) {
        karaokeSongsModel.search(arg1);
        lastVal = arg1.trimmed();
    }
}

void MainWindow::setMultiPlayed() {
    QModelIndexList indexes = ui->tableViewQueue->selectionModel()->selectedIndexes();
    QModelIndex index;

            foreach(index, indexes) {
            if (index.column() == 0) {
                int queueId = index.sibling(index.row(), 0).data().toInt();
                logger->info("{} Selected row: {} - queueId: {}", m_loggingPrefix, index.row(), queueId);
                qModel.setPlayed(queueId);
            }
        }
}

void MainWindow::setMultiUnplayed() {
    QModelIndexList indexes = ui->tableViewQueue->selectionModel()->selectedIndexes();
    QModelIndex index;

            foreach(index, indexes) {
            if (index.column() == 0) {
                int queueId = index.sibling(index.row(), 0).data().toInt();
                logger->info("{} Selected row: {} - queueId: {}", m_loggingPrefix, index.row(), queueId);
                qModel.setPlayed(queueId, false);
            }
        }
}

void MainWindow::on_spinBoxTempo_valueChanged(const int &arg1) {
    kMediaBackend.setTempo(arg1);
    QTimer::singleShot(20, [&]() {
        ui->spinBoxTempo->findChild<QLineEdit *>()->deselect();
    });
}

void MainWindow::on_actionSongbook_Generator_triggered() {
    dlgBookCreator->show();
}

void MainWindow::on_actionEqualizer_triggered() {
    dlgEq->show();
}

void MainWindow::audioError(const QString &msg) {
    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("Audio playback error! - " + msg);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (kMediaBackend.state() == MediaBackend::PlayingState) {
        QMessageBox msgBox(this);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Are you sure you want to exit?");
        msgBox.setInformativeText(
                "There is currently a karaoke song playing.  If you continue, the current song will be stopped. Are you sure?");
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();
        if (msgBox.clickedButton() != yesButton) {
            event->ignore();
            return;
        }
    }
    if (!settings.cdgWindowFullscreen())
        settings.saveWindowState(cdgWindow);
    settings.setShowCdgWindow(cdgWindow->isVisible());
    cdgWindow->setVisible(false);
    dlgSongShop->setVisible(false);
    requestsDialog->setVisible(false);
    event->accept();
}

void MainWindow::on_sliderVolume_valueChanged(int value) {
    kMediaBackend.setVolume(value);
    kMediaBackend.fadeInImmediate();
}

void MainWindow::on_sliderBmVolume_valueChanged(int value) {
    bmMediaBackend.setVolume(value);
    if (kMediaBackend.state() != MediaBackend::PlayingState)
        bmMediaBackend.fadeInImmediate();
}

void MainWindow::songDropNoSingerSel() {
    QMessageBox msgBox;
    msgBox.setText("No singer selected.  You must select a singer before you can add songs to a queue.");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}

void MainWindow::newVersionAvailable(const QString &version) {
    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("New version of OpenKJ is available: " + version);
    msgBox.setIcon(QMessageBox::Information);
    if (checker->getOS() == "Linux") {
        msgBox.setInformativeText(
                "To install the update, please use your distribution's package manager or download and build the current source.");
    }
    if (checker->getOS() == "Win32" || checker->getOS() == "Win64") {
        msgBox.setInformativeText(
                "You can download the new version at <a href=https://openkj.org/software>https://openkj.org/software</a>");
    }
    if (checker->getOS() == "MacOS") {
        msgBox.setInformativeText(
                "You can download the new version at <a href=https://openkj.org/software>https://openkj.org/software</a>");
    }
    msgBox.exec();
}

void MainWindow::on_pushButtonIncomingRequests_clicked() {
    requestsDialog->show();
}

void MainWindow::on_pushButtonShop_clicked() {
    dlgSongShop->show();
    dlgSongShop->setModal(false);
}

void MainWindow::filesDroppedOnQueue(const QList<QUrl> &urls, const int &singerId, const int &position) {
            foreach (QUrl url, urls) {
            QString file = url.toLocalFile();
            if (QFile(file).exists()) {
                if (file.endsWith(".zip", Qt::CaseInsensitive)) {
                    MzArchive archive(file);
                    if (!archive.isValidKaraokeFile()) {
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("Invalid karaoke file!");
                        msgBox.setText("Invalid karaoke file dropped on queue");
                        msgBox.setIcon(QMessageBox::Warning);
                        msgBox.setInformativeText(file);
                        msgBox.exec();
                        continue;
                    }
                } else if (file.endsWith(".cdg", Qt::CaseInsensitive)) {
                    if (TableModelKaraokeSongs::findCdgAudioFile(file) == QString()) {
                        QMessageBox msgBox;
                        msgBox.setWindowTitle("Invalid karaoke file!");
                        msgBox.setIcon(QMessageBox::Warning);
                        msgBox.setText("CDG file dropped on queue has no matching audio file");
                        msgBox.setInformativeText(file);
                        msgBox.exec();
                        continue;
                    }
                } else if (!file.endsWith(".mp4", Qt::CaseInsensitive) && !file.endsWith(".mkv", Qt::CaseInsensitive) &&
                           !file.endsWith(".avi", Qt::CaseInsensitive) && !file.endsWith(".m4v", Qt::CaseInsensitive)) {
                    QMessageBox msgBox;
                    msgBox.setWindowTitle("Invalid karaoke file!");
                    msgBox.setText(
                            "Unsupported file type dropped on queue.  Supported file types: mp3+g zip, cdg, mp4, mkv, avi");
                    msgBox.setIcon(QMessageBox::Warning);
                    msgBox.setInformativeText(file);
                    msgBox.exec();
                    continue;
                }
                logger->info("{} Karaoke file dropped. Singer: {} Pos: {} Path: {}", m_loggingPrefix, singerId, position, file.toStdString());
                QFileInfo dFileInfo(file);

                KaraokeSong droppedSong{
                        -1,
                        "--Dropped Song--",
                        "--dropped song--",
                        dFileInfo.completeBaseName(),
                        dFileInfo.completeBaseName().toLower(),
                        "!!DROPPED!!",
                        "!!dropped!!",
                        0,
                        dFileInfo.fileName(),
                        file,
                        "",
                        0,
                        QDateTime()
                };
                int songId = karaokeSongsModel.addSong(droppedSong);
                logger->info("{} addSong returned songid: {}", m_loggingPrefix, songId);
                if (songId == -1)
                    continue;
                qModel.insert(songId, position);
            }
        }
}

void MainWindow::appFontChanged(const QFont &font) {
    auto smallerFont = font;
    smallerFont.setPointSize(font.pointSize() - 2);
    auto smallerFontBold = smallerFont;
    smallerFontBold.setBold(true);
    ui->labelTotal->setFont(smallerFont);
    ui->labelTotalTime->setFont(smallerFont);
    ui->labelElapsed->setFont(smallerFont);
    ui->labelElapsedTime->setFont(smallerFont);
    ui->labelRemain->setFont(smallerFont);
    ui->labelRemainTime->setFont(smallerFont);
    ui->labelArtistHeader->setFont(smallerFontBold);
    ui->labelArtist->setFont(smallerFont);
    ui->labelTitleHeader->setFont(smallerFontBold);
    ui->labelTitle->setFont(smallerFont);
    ui->labelSingerHeader->setFont(smallerFontBold);
    ui->labelSinger->setFont(smallerFont);

    QApplication::setFont(font, "QWidget");
    setFont(font);
    QFontMetrics fm(settings.applicationFont());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int cvwWidth = std::max(300, fm.horizontalAdvance("Total: 00:00  Current:00:00  Remain: 00:00"));
#else
    int cvwWidth = std::max(300, fm.width("Total: 00:00  Current:00:00  Remain: 00:00"));
#endif
    logger->info("{} Resizing videoPreview to width: {}", m_loggingPrefix, cvwWidth);
    QSize mcbSize(fm.height(), fm.height());
    if (mcbSize.width() < 32) {
        mcbSize.setWidth(32);
        mcbSize.setHeight(32);
    }
    ui->buttonBmStop->resize(mcbSize);
    ui->buttonBmPause->resize(mcbSize);
    ui->buttonBmStop->setIconSize(mcbSize);
    ui->buttonBmPause->setIconSize(mcbSize);
    ui->buttonBmStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->buttonBmPause->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    autosizeViews();
}

void MainWindow::resizeRotation() {
    if (settings.rotationShowNextSong()) {
        ui->tableViewRotation->showColumn(TableModelRotation::COL_NEXT_SONG);
        ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_NAME,
                                                                        QHeaderView::Interactive);
        ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_NEXT_SONG,
                                                                        QHeaderView::Stretch);
    } else {
        ui->tableViewRotation->horizontalHeader()->setSectionResizeMode(TableModelRotation::COL_NAME,
                                                                        QHeaderView::Stretch);
        ui->tableViewRotation->hideColumn(TableModelRotation::COL_NEXT_SONG);
    }
}

void MainWindow::autosizeViews() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int durationColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance(" Duration ");
    int songidColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance(" AA0000000-0000 ");
    int lastPlayColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("_00/00/00 00:00 MM_");
#else
    int durationColSize = QFontMetrics(settings.applicationFont()).width(" Duration ");
    int songidColSize = QFontMetrics(settings.applicationFont()).width(" AA0000000-0000 ");
    int lastPlayColSize = QFontMetrics(settings.applicationFont()).width("_00/00/00 00:00 MM_");
#endif
    int remainingSpace = ui->tableViewDB->width() - durationColSize - songidColSize;
    int artistColSize = (remainingSpace / 2) - 120;
    int titleColSize = (remainingSpace / 2) + 100;
    ui->tableViewDB->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_ARTIST, artistColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_TITLE, titleColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_DURATION, durationColSize);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_DURATION, QHeaderView::Fixed);
    ui->tableViewDB->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_SONGID, songidColSize);
    ui->tableViewDB->horizontalHeader()->resizeSection(TableModelKaraokeSongs::COL_LASTPLAY, lastPlayColSize);
    resizeRotation();
    autosizeQueue();
}

void MainWindow::autosizeQueue() {
    auto curTab = ui->tabWidgetQueue->currentIndex();

    ui->tabWidgetQueue->setCurrentIndex(0);
    QApplication::processEvents();
    int fH = QFontMetrics(settings.applicationFont()).height();
    int iconWidth = fH + fH;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int keyColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("Key") + iconWidth;
    int durationColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("Duration") + iconWidth;
    int songidColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance(" AA0000000-0000 ");
    int lastPlayedColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("00-00-0000") + iconWidth;
    int playsColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("Plays") + iconWidth;
#else
    int lastPlayedColSize = QFontMetrics(settings.applicationFont()).width("00-00-0000") + iconWidth;
    int playsColSize = QFontMetrics(settings.applicationFont()).width("Plays") + iconWidth;
    int durationColSize = QFontMetrics(settings.applicationFont()).width("Duration") + iconWidth;
    int keyColSize = QFontMetrics(settings.applicationFont()).width("Key") + iconWidth;
    int songidColSize = QFontMetrics(settings.applicationFont()).width(" AA0000000-0000 ");
#endif
    int remainingSpace = ui->tableViewQueue->width() - iconWidth - keyColSize - durationColSize - songidColSize - 16;
    int artistColSize = (remainingSpace / 2);
    int titleColSize = (remainingSpace / 2);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_ARTIST, artistColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_TITLE, titleColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_SONGID, songidColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_DURATION, durationColSize);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_PATH, iconWidth);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_PATH, QHeaderView::Fixed);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_KEY, keyColSize);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_KEY, QHeaderView::Fixed);

    ui->tabWidgetQueue->setCurrentIndex(1);
    QApplication::processEvents();
    remainingSpace = ui->tableViewHistory->width() - keyColSize - songidColSize - lastPlayedColSize - playsColSize - 16;
    artistColSize = (remainingSpace / 2);
    titleColSize = (remainingSpace / 2);
    ui->tableViewHistory->horizontalHeader()->resizeSection(3, artistColSize);
    ui->tableViewHistory->horizontalHeader()->resizeSection(4, titleColSize);
    ui->tableViewHistory->horizontalHeader()->resizeSection(5, songidColSize);
    ui->tableViewHistory->horizontalHeader()->resizeSection(6, keyColSize);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    ui->tableViewHistory->horizontalHeader()->resizeSection(7, playsColSize);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Fixed);
    QApplication::processEvents();
    ui->tabWidgetQueue->setCurrentIndex(curTab);
}

void MainWindow::autosizeBmViews() {

    int fH = QFontMetrics(settings.applicationFont()).height();
    int iconWidth = fH + fH;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int durationColSize = QFontMetrics(settings.applicationFont()).horizontalAdvance("Duration") + fH;
#else
    int durationColSize = QFontMetrics(settings.applicationFont()).width("Duration") + fH;
#endif
    // 4 = filename 1 = metadata artist 2 = medatada title

    int artistColSize = 0;
    int titleColSize = 0;
    int fnameColSize = 0;
    int remainingSpace = ui->tableViewBmDb->width() - durationColSize - 15;
    if (settings.bmShowMetadata() && settings.bmShowFilenames()) {
        artistColSize = (float) remainingSpace * .25;
        titleColSize = (float) remainingSpace * .25;
        fnameColSize = (float) remainingSpace * .5;
    } else if (settings.bmShowMetadata()) {
        artistColSize = remainingSpace * .5;
        titleColSize = remainingSpace * .5;
    } else if (settings.bmShowFilenames()) {
        fnameColSize = remainingSpace;
    }
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_ARTIST, artistColSize);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_TITLE, titleColSize);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_FILENAME, fnameColSize);
    ui->tableViewBmDb->horizontalHeader()->setSectionResizeMode(TableModelBreakSongs::COL_DURATION, QHeaderView::Fixed);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_DURATION, durationColSize);


    remainingSpace = ui->tableViewBmPlaylist->width() - durationColSize - (iconWidth * 2) - 15;
    if (settings.bmShowMetadata() && settings.bmShowFilenames()) {
        artistColSize = (float) remainingSpace * .25;
        titleColSize = (float) remainingSpace * .25;
        fnameColSize = (float) remainingSpace * .5;
    } else if (settings.bmShowMetadata()) {
        artistColSize = remainingSpace * .5;
        titleColSize = remainingSpace * .5;
    } else if (settings.bmShowFilenames()) {
        fnameColSize = remainingSpace;
    }
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_ARTIST, artistColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_TITLE, titleColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_FILENAME, fnameColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_DURATION, durationColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->setSectionResizeMode(TableModelPlaylistSongs::COL_ID,
                                                                      QHeaderView::Fixed);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_ID, iconWidth);
    ui->tableViewBmPlaylist->horizontalHeader()->setSectionResizeMode(TableModelPlaylistSongs::COL_PATH,
                                                                      QHeaderView::Fixed);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_PATH, iconWidth);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    if (!m_initialUiSetupDone)
        return;
    QMainWindow::resizeEvent(event);
    autosizeViews();
    autosizeBmViews();
    if (ui->tabWidget->currentIndex() == 0) {
        autosizeViews();
        bNeedAutoSize = true;
        kNeedAutoSize = false;
    }
    if (ui->tabWidget->currentIndex() == 1) {
        autosizeBmViews();
        bNeedAutoSize = false;
        kNeedAutoSize = true;
    }
    settings.saveWindowState(this);
}

void MainWindow::on_tabWidget_currentChanged(const int &index) {
    if (bNeedAutoSize && index == 1) {
        autosizeBmViews();
        bNeedAutoSize = false;
    }
    if (kNeedAutoSize && index == 0) {
        autosizeViews();
        kNeedAutoSize = false;
    }
}

void MainWindow::bmDatabaseAboutToUpdate() {
    bmPlaylistsModel->revertAll();
    bmPlaylistsModel->setTable("");
}

void MainWindow::bmSongMoved(const int &oldPos, const int &newPos) {
    int curPlPos = playlistSongsModel.currentPosition();
    if (oldPos < curPlPos && newPos >= curPlPos)
        curPlPos--;
    else if (oldPos > curPlPos && newPos <= curPlPos)
        curPlPos++;
    else if (oldPos == curPlPos)
        curPlPos = newPos;
    playlistSongsModel.setCurrentPosition(curPlPos);
    auto nextPlSong = playlistSongsModel.getNextPlSong();
    if (!ui->checkBoxBmBreak->isChecked() && nextPlSong.has_value()) {
        ui->labelBmNext->setText(nextPlSong->get().artist + " - " + nextPlSong->get().title);
    } else
        ui->labelBmNext->setText("None - Breaking after current song");
}

void MainWindow::on_sliderBmPosition_sliderPressed() {
    logger->trace("{} BM slider down", m_loggingPrefix);
    sliderBmPositionPressed = true;
}

void MainWindow::on_sliderBmPosition_sliderReleased() {
    bmMediaBackend.setPosition(ui->sliderBmPosition->value());
    sliderBmPositionPressed = false;
    logger->trace("{} BM slider up.  Position: {}", m_loggingPrefix, ui->sliderBmPosition->value());
}

void MainWindow::sfxButtonPressed() {
    auto *btn = (SoundFxButton *) sender();
    sfxMediaBackend.setMedia(btn->buttonData().toString());
    sfxMediaBackend.setVolume(ui->sliderVolume->value());
    sfxMediaBackend.play();
}

void MainWindow::on_btnAddSfx_clicked() {
    QString path = QFileDialog::getOpenFileName(
            this,
            "Select audio file",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            "Audio (*.mp3 *.ogg *.wav *.wma)",
            nullptr,
            QFileDialog::DontUseNativeDialog
            );
    if (path != "") {
        bool ok;
        QString name = QInputDialog::getText(this, tr("Button Text"), tr("Enter button text:"), QLineEdit::Normal,
                                             QString(), &ok);
        if (!ok || name.isEmpty())
            return;
        SfxEntry entry;
        entry.name = name;
        entry.path = path;
        settings.addSfxEntry(entry);
        addSfxButton(path, name);
    }

}

void MainWindow::on_btnSfxStop_clicked() {
    sfxMediaBackend.stop(true);
}

void MainWindow::removeSfxButton() {
    SfxEntryList entries = settings.getSfxEntries();
    SfxEntryList newEntries;
            foreach (SfxEntry entry, entries) {
            if (entry.name == lastRtClickedSfxBtn.name && entry.path == lastRtClickedSfxBtn.path)
                continue;
            newEntries.push_back(entry);
        }
    settings.setSfxEntries(newEntries);
    refreshSfxButtons();
}

void MainWindow::showAlert(const QString &title, const QString &message) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.exec();
}

void MainWindow::tableViewRotationCurrentChanged(const QModelIndex &cur, const QModelIndex &prev) {
    Q_UNUSED(prev)
    qModel.loadSinger(cur.data(Qt::UserRole).toInt());
    historySongsModel.loadSinger(rotModel.getSingerName(cur.data(Qt::UserRole).toInt()));
    if (!settings.treatAllSingersAsRegs() && !cur.sibling(cur.row(), TableModelRotation::COL_REGULAR).data().toBool())
        ui->tabWidgetQueue->removeTab(1);
    else {
        if (ui->tabWidgetQueue->count() == 1)
            ui->tabWidgetQueue->addTab(historyTabWidget, "History");
    }
    ui->gbxQueue->setTitle(
            QString("Song Queue - " + cur.sibling(cur.row(), TableModelRotation::COL_NAME).data().toString()));
    if (!ui->tabWidgetQueue->isVisible()) {
        ui->tabWidgetQueue->setVisible(true);
        ui->labelNoSinger->setVisible(false);
        QApplication::processEvents();
        autosizeQueue();
    }
}

void MainWindow::updateRotationDuration() {
    QString text;
    int secs = rotModel.rotationDuration();
    if (secs > 0) {
        int hours = 0;
        int minutes = secs / 60;
        int seconds = secs % 60;
        if (seconds > 0)
            minutes++;
        if (minutes > 60) {
            hours = minutes / 60;
            minutes = minutes % 60;
            if (hours > 1)
                text = " Rotation Duration: " + QString::number(hours) + " hours " + QString::number(minutes) + " min";
            else
                text = " Rotation Duration: " + QString::number(hours) + " hour " + QString::number(minutes) + " min";
        } else
            text = " Rotation Duration: " + QString::number(minutes) + " min";
    } else
        text = " Rotation Duration: 0 min";
    labelRotationDuration.setText(text);
}

void MainWindow::cdgVisibilityChanged() {
    ui->btnToggleCdgWindow->setChecked(cdgWindow->isVisible());
}

void MainWindow::rotationSelectionChanged(const QItemSelection &sel, const QItemSelection &desel) {
    if (sel.empty()) {
        logger->trace("{} Rotation Selection Cleared!", m_loggingPrefix);
        qModel.loadSinger(-1);
        ui->tableViewRotation->reset();
        ui->gbxQueue->setTitle("Song Queue");
        ui->tabWidgetQueue->setVisible(false);
        ui->labelNoSinger->setVisible(true);
    }

    logger->trace("{} Rotation Selection Changed", m_loggingPrefix);

}

void MainWindow::on_lineEditBmSearch_textChanged(const QString &arg1) {
    if (!settings.progressiveSearchEnabled())
        return;
    static QString lastVal;
    if (arg1.trimmed() != lastVal) {
        bmDbModel.search(arg1);
        lastVal = arg1.trimmed();
    }
}

void MainWindow::on_btnRotTop_clicked() {
    auto indexes = ui->tableViewRotation->selectionModel()->selectedRows();
    std::vector<int> singerIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        singerIds.emplace_back(index.data(Qt::UserRole).toInt());
    });
    std::for_each(singerIds.rbegin(), singerIds.rend(), [&](auto singerId) {
        rotModel.singerMove(rotModel.getSingerPosition(singerId), 0);
    });
    auto topLeft = ui->tableViewRotation->model()->index(0, 0);
    auto bottomRight = ui->tableViewRotation->model()->index(singerIds.size() - 1, rotModel.columnCount() - 1);
    ui->tableViewRotation->clearSelection();
    ui->tableViewRotation->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::on_btnRotUp_clicked() {
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    rotModel.singerMove(curPos, curPos - 1);
    ui->tableViewRotation->selectRow(curPos - 1);
    rotationDataChanged();
}

void MainWindow::on_btnRotDown_clicked() {
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == rotModel.rowCount() - 1)
        return;
    rotModel.singerMove(curPos, curPos + 1);
    ui->tableViewRotation->selectRow(curPos + 1);
    rotationDataChanged();
}

void MainWindow::on_btnRotBottom_clicked() {
    auto indexes = ui->tableViewRotation->selectionModel()->selectedRows();
    std::vector<int> singerIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        singerIds.emplace_back(index.data(Qt::UserRole).toInt());
    });
    std::for_each(singerIds.begin(), singerIds.end(), [&](auto songId) {
        rotModel.singerMove(rotModel.getSingerPosition(songId), rotModel.rowCount() - 1);
    });
    auto topLeft = ui->tableViewRotation->model()->index(rotModel.rowCount() - singerIds.size(), 0);
    auto bottomRight = ui->tableViewRotation->model()->index(rotModel.rowCount() - 1, rotModel.columnCount() - 1);
    ui->tableViewRotation->clearSelection();
    ui->tableViewRotation->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::on_btnQTop_clicked() {
    auto indexes = ui->tableViewQueue->selectionModel()->selectedRows();
    std::vector<int> songIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        songIds.emplace_back(index.data().toInt());
    });
    std::for_each(songIds.rbegin(), songIds.rend(), [&](auto songId) {
        qModel.moveSongId(songId, 0);
    });
    auto topLeft = ui->tableViewQueue->model()->index(0, 0);
    auto bottomRight = ui->tableViewQueue->model()->index(songIds.size() - 1, qModel.columnCount() - 1);
    ui->tableViewQueue->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::on_btnQUp_clicked() {
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    qModel.move(curPos, curPos - 1);
    ui->tableViewQueue->selectRow(curPos - 1);
    rotationDataChanged();
}

void MainWindow::on_btnQDown_clicked() {
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == ui->tableViewQueue->model()->rowCount() - 1)
        return;
    qModel.move(curPos, curPos + 1);
    ui->tableViewQueue->selectRow(curPos + 1);
    rotationDataChanged();
}

void MainWindow::on_btnQBottom_clicked() {
    auto indexes = ui->tableViewQueue->selectionModel()->selectedRows();
    std::vector<int> songIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        songIds.emplace_back(index.data().toInt());
    });
    std::for_each(songIds.begin(), songIds.end(), [&](auto songId) {
        qModel.moveSongId(songId, qModel.rowCount() - 1);
    });
    auto topLeft = ui->tableViewQueue->model()->index(qModel.rowCount() - songIds.size(), 0);
    auto bottomRight = ui->tableViewQueue->model()->index(qModel.rowCount() - 1, qModel.columnCount() - 1);
    ui->tableViewQueue->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::on_btnBmPlRandomize_clicked() {
    if (playlistSongsModel.rowCount() < 2)
        return;
    playlistSongsModel.randomizePlaylist();
}

void MainWindow::on_btnPlTop_clicked() {
    auto indexes = ui->tableViewBmPlaylist->selectionModel()->selectedRows();
    std::vector<int> plSongIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        plSongIds.emplace_back(index.data().toInt());
    });
    std::for_each(plSongIds.rbegin(), plSongIds.rend(), [&](auto plSongId) {
        playlistSongsModel.moveSong(playlistSongsModel.getSongPositionById(plSongId), 0);
    });
    playlistSongsModel.savePlaylistChanges();
    auto topLeft = ui->tableViewBmPlaylist->model()->index(0, 0);
    auto bottomRight = ui->tableViewBmPlaylist->model()->index(plSongIds.size() - 1, 7);
    ui->tableViewBmPlaylist->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                      QItemSelectionModel::Select);
}

void MainWindow::on_btnPlUp_clicked() {
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    playlistSongsModel.moveSong(curPos, curPos - 1);
    playlistSongsModel.savePlaylistChanges();
    ui->tableViewBmPlaylist->selectRow(curPos - 1);
}

void MainWindow::on_btnPlDown_clicked() {
    int maxpos = ui->tableViewBmPlaylist->model()->rowCount() - 1;
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == maxpos)
        return;
    playlistSongsModel.moveSong(curPos, curPos + 1);
    playlistSongsModel.savePlaylistChanges();
    ui->tableViewBmPlaylist->selectRow(curPos + 1);
}

void MainWindow::on_btnPlBottom_clicked() {
    auto indexes = ui->tableViewBmPlaylist->selectionModel()->selectedRows();
    std::vector<int> plSongIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        plSongIds.emplace_back(index.data().toInt());
    });
    std::for_each(plSongIds.begin(), plSongIds.end(), [&](auto plSongId) {
        playlistSongsModel.moveSong(playlistSongsModel.getSongPositionById(plSongId),
                                    playlistSongsModel.rowCount() - 1);
    });
    playlistSongsModel.savePlaylistChanges();
    auto topLeft = ui->tableViewBmPlaylist->model()->index(playlistSongsModel.rowCount() - plSongIds.size(), 0);
    auto bottomRight = ui->tableViewBmPlaylist->model()->index(playlistSongsModel.rowCount() - 1, 7);
    ui->tableViewBmPlaylist->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                      QItemSelectionModel::Select);
}

void MainWindow::on_actionSound_Clips_triggered(const bool &checked) {
    if (checked) {
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Ignored);
    } else {
        ui->groupBoxSoundClips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
        ui->verticalSpacerRtPanel->changeSize(0, 20, QSizePolicy::Ignored, QSizePolicy::Expanding);
    }
    ui->groupBoxSoundClips->setVisible(checked);
    settings.setShowMainWindowSoundClips(checked);
}

void MainWindow::on_actionNow_Playing_triggered(const bool &checked) {
    ui->groupBoxNowPlaying->setVisible(checked);
    settings.setShowMainWindowNowPlaying(checked);
}

void MainWindow::on_actionVideoSmall_triggered() {
    ui->videoPreview->setMinimumSize(QSize(256, 144));
    ui->videoPreview->setMaximumSize(QSize(256, 144));
    ui->videoPreviewBm->setMinimumSize(QSize(256, 144));
    ui->videoPreviewBm->setMaximumSize(QSize(256, 144));
    ui->mediaFrame->setMaximumWidth(300);
    ui->mediaFrame->setMinimumWidth(300);
    settings.setMainWindowVideoSize(Settings::Small);
    QTimer::singleShot(15, [&]() { autosizeViews(); });
}

void MainWindow::on_actionVideoMedium_triggered() {
    ui->videoPreview->setMinimumSize(QSize(384, 216));
    ui->videoPreview->setMaximumSize(QSize(384, 216));
    ui->videoPreviewBm->setMinimumSize(QSize(384, 216));
    ui->videoPreviewBm->setMaximumSize(QSize(384, 216));
    ui->mediaFrame->setMaximumWidth(430);
    ui->mediaFrame->setMinimumWidth(430);
    settings.setMainWindowVideoSize(Settings::Medium);
    QTimer::singleShot(15, [&]() { autosizeViews(); });
}

void MainWindow::on_actionVideoLarge_triggered() {
    ui->videoPreview->setMinimumSize(QSize(512, 288));
    ui->videoPreview->setMaximumSize(QSize(512, 288));
    ui->videoPreviewBm->setMinimumSize(QSize(512, 288));
    ui->videoPreviewBm->setMaximumSize(QSize(512, 288));
    ui->mediaFrame->setMaximumWidth(560);
    ui->mediaFrame->setMinimumWidth(560);
    settings.setMainWindowVideoSize(Settings::Large);
    QTimer::singleShot(15, [&]() { autosizeViews(); });
}

void MainWindow::on_actionVideo_Output_2_triggered(const bool &checked) {
    ui->videoPreview->setVisible(checked);
    settings.setShowMainWindowVideo(checked);
}

void MainWindow::on_actionKaraoke_torture_triggered() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static int runs = 0;
        logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        logger->info("{} num songs in db: {}", m_loggingPrefix, karaokeSongsModel.rowCount(QModelIndex()));
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        int randno = QRandomGenerator::global()->bounded(0, karaokeSongsModel.rowCount(QModelIndex()) - 1);
        randno = 1;
        logger->info("{} randno: {}", m_loggingPrefix, randno);
        ui->tableViewDB->selectRow(randno);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        play(karaokeSongsModel.getPath(
                ui->tableViewDB->selectionModel()->selectedRows(TableModelKaraokeSongs::COL_ID).at(0).data().toInt()),
             true);
        ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
    });
    m_timerTest.start(4000);
#endif
}

void MainWindow::on_actionK_B_torture_triggered() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        if (playing) {
            on_buttonStop_clicked();
            playing = false;
            ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
            return;
        }
        logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        logger->info("{} num songs in db: {}", m_loggingPrefix, karaokeSongsModel.rowCount(QModelIndex()));
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        int randno = QRandomGenerator::global()->bounded(0, karaokeSongsModel.rowCount(QModelIndex()) - 1);
        logger->info("{} randno: {}", m_loggingPrefix, randno);
        ui->tableViewDB->selectRow(randno);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        play(ui->tableViewDB->selectionModel()->selectedRows(5).at(0).data().toString(), false);
        ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
        playing = true;
    });
    m_timerTest.start(2000);
#endif
}

void MainWindow::on_actionBurn_in_triggered() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    m_testMode = true;
    emit ui->buttonClearRotation->clicked();
    for (auto i = 0; i < 21; i++) {
        auto singerName = "Test Singer " + QString::number(i);
        rotModel.singerAdd(singerName);
    }
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        if (playing) {
            on_buttonStop_clicked();
            playing = false;
            ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
            return;
        }

        rotModel.singerMove(QRandomGenerator::global()->bounded(0, 19), QRandomGenerator::global()->bounded(0, 19));
        ui->tableViewRotation->selectRow(QRandomGenerator::global()->bounded(0, 19));
        int randno{0};
        if (karaokeSongsModel.rowCount(QModelIndex()) > 1)
            randno = QRandomGenerator::global()->bounded(0, karaokeSongsModel.rowCount(QModelIndex()) - 1);
        logger->info("{} randno: {}", m_loggingPrefix, randno);
        ui->tableViewDB->selectRow(randno);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        emit ui->tableViewDB->doubleClicked(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        ui->tableViewQueue->selectRow(qModel.rowCount() - 1);
        ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        if (qModel.rowCount() > 2) {
            auto newPos = QRandomGenerator::global()->bounded(0, qModel.rowCount() - 1);
            qModel.move(qModel.rowCount() - 1, newPos);
            ui->tableViewQueue->selectRow(newPos);
            ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        }
        auto idx = ui->tableViewQueue->selectionModel()->selectedRows().at(0);
        emit ui->tableViewQueue->doubleClicked(idx);
        ui->tableViewQueue->selectRow(idx.row());
        playing = true;
        logger->info("{} Burn in test cycle: {}", m_loggingPrefix, ++runs);
    });
    m_timerTest.start(3000);
#endif
}

void MainWindow::on_actionMultiplex_Controls_triggered(bool checked) {
    ui->widgetMplxControls->setVisible(checked);
    settings.setShowMplxControls(checked);
}

void MainWindow::on_actionCDG_Decode_Torture_triggered() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static int runs = 0;
        logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        logger->info("{} num songs in db: {}", m_loggingPrefix, karaokeSongsModel.rowCount(QModelIndex()));
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        ui->tableViewDB->scrollToBottom();
        int randno = QRandomGenerator::global()->bounded(0, karaokeSongsModel.rowCount(QModelIndex()) - 1);
        logger->info("{} randno: {}", m_loggingPrefix, randno);
        ui->tableViewDB->selectRow(randno);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        QString karaokeFilePath = ui->tableViewDB->selectionModel()->selectedRows(5).at(0).data().toString();
        if (!karaokeFilePath.endsWith("zip", Qt::CaseInsensitive) &&
            !karaokeFilePath.endsWith("cdg", Qt::CaseInsensitive)) {
            return;
        }
        if (karaokeFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
            MzArchive archive(karaokeFilePath);
            if ((archive.checkCDG()) && (archive.checkAudio())) {
                if (archive.checkAudio()) {
                    if (!archive.extractAudio(khTmpDir->path(), "tmp" + archive.audioExtension())) {
                        return;
                    }
                    if (!archive.extractCdg(khTmpDir->path(), "tmp.cdg")) {
                        return;
                    }
                    QString audioFile = khTmpDir->path() + QDir::separator() + "tmp" + archive.audioExtension();
                    QString cdgFile = khTmpDir->path() + QDir::separator() + "tmp.cdg";
                    logger->info("{} Extracted audio file size: {}", m_loggingPrefix, QFileInfo(audioFile).size());
                    logger->info("{} Setting karaoke backend source file to: {}", m_loggingPrefix, audioFile.toStdString());
                    kMediaBackend.setMediaCdg(cdgFile, audioFile);
                    //kMediaBackend.testCdgDecode(); // todo: andth
                }
            } else {
                return;
            }
        } else if (karaokeFilePath.endsWith(".cdg", Qt::CaseInsensitive)) {
            QString cdgTmpFile = "tmp.cdg";
            QString audTmpFile = "tmp.mp3";
            QFile cdgFile(karaokeFilePath);
            if (!cdgFile.exists() || cdgFile.size() == 0) {
                return;
            }
            QString audiofn = findMatchingAudioFile(karaokeFilePath);
            if (audiofn == "") {
                return;
            }
            QFile audioFile(audiofn);
            if (audioFile.size() == 0) {
                return;
            }
            cdgFile.copy(khTmpDir->path() + QDir::separator() + cdgTmpFile);
            QFile::copy(audiofn, khTmpDir->path() + QDir::separator() + audTmpFile);
            kMediaBackend.setMediaCdg(khTmpDir->path() + QDir::separator() + cdgTmpFile,
                                      khTmpDir->path() + QDir::separator() + audTmpFile);
            // kMediaBackend.testCdgDecode(); // todo: andth
        }
        ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
    });
    m_timerTest.start(2000);
#endif
}

void MainWindow::on_actionWrite_Gstreamer_pipeline_dot_files_triggered() {
    QString outputFolder = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0);
    kMediaBackend.writePipelinesGraphToFile(outputFolder);
    bmMediaBackend.writePipelinesGraphToFile(outputFolder);
    sfxMediaBackend.writePipelinesGraphToFile(outputFolder);
}

void MainWindow::on_comboBoxSearchType_currentIndexChanged(int index) {
    switch (index) {
        case 1:
            karaokeSongsModel.setSearchType(TableModelKaraokeSongs::SEARCH_TYPE_ARTIST);
            break;
        case 2:
            karaokeSongsModel.setSearchType(TableModelKaraokeSongs::SEARCH_TYPE_TITLE);
            break;
        default:
            karaokeSongsModel.setSearchType(TableModelKaraokeSongs::SEARCH_TYPE_ALL);
            break;
    }
}

void MainWindow::on_actionDocumentation_triggered() {
    QDesktopServices::openUrl(QUrl("https://docs.openkj.org"));
}

void MainWindow::on_btnToggleCdgWindow_clicked(bool checked) {
    if (!checked) {
        cdgWindow->hide();
    } else {
        cdgWindow->show();
    }
}

void MainWindow::on_pushButtonHistoryPlay_clicked() {
    auto selRows = ui->tableViewHistory->selectionModel()->selectedRows();
    if (selRows.empty())
        return;
    auto index = selRows.at(0);
    k2kTransition = false;
    if (kMediaBackend.state() == MediaBackend::PlayingState) {
        if (settings.showSongInterruptionWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(settings.showSongInterruptionWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Interrupt currenly playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &settings, &Settings::setShowSongInterruptionWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        k2kTransition = true;
    }
    if (kMediaBackend.state() == MediaBackend::PausedState) {
        if (settings.karaokeAutoAdvance()) {
            kAASkip = true;
            cdgWindow->showAlert(false);
        }
        audioRecorder.stop();
        kMediaBackend.stop(true);
    }
    int curSingerId = rotModel.getSingerId(historySongsModel.currentSingerName());
    curSinger = rotModel.getSingerName(curSingerId);
    curArtist = index.sibling(index.row(), 3).data().toString();
    curTitle = index.sibling(index.row(), 4).data().toString();
    QString curSongId = index.sibling(index.row(), 5).data().toString();
    QString filePath = index.sibling(index.row(), 2).data().toString();
    int curKeyChange = index.sibling(index.row(), 6).data().toInt();
    ui->labelSinger->setText(curSinger);
    ui->labelArtist->setText(curArtist);
    ui->labelTitle->setText(curTitle);
    karaokeSongsModel.updateSongHistory(karaokeSongsModel.getIdForPath(filePath));
    play(filePath, k2kTransition);
    if (settings.treatAllSingersAsRegs() || rotModel.singerIsRegular(curSingerId))
        historySongsModel.saveSong(curSinger, filePath, curArtist, curTitle, curSongId, curKeyChange);
    kMediaBackend.setPitchShift(curKeyChange);
    rotModel.setCurrentSinger(curSingerId);
    rotDelegate.setCurrentSinger(curSingerId);
    if (settings.rotationAltSortOrder()) {
        auto curSingerPos = rotModel.getSingerPosition(curSingerId);
        m_curSingerOriginalPosition = curSingerPos;
        if (curSingerPos != 0)
            rotModel.singerMove(curSingerPos, 0);
    }
}

void MainWindow::on_pushButtonHistoryToQueue_clicked() {
    auto selRows = ui->tableViewHistory->selectionModel()->selectedRows();
    if (selRows.empty())
        return;

    std::for_each(selRows.begin(), selRows.end(), [&](auto index) {
        auto path = index.sibling(index.row(), 2).data().toString();
        int curSingerId = rotModel.getSingerId(historySongsModel.currentSingerName());
        int key = index.sibling(index.row(), 6).data().toInt();
        int dbSongId = karaokeSongsModel.getIdForPath(path);
        if (dbSongId == -1) {
            QMessageBox::warning(this,
                                 "Unable to add history song",
                                 "Unable to find a matching song in the database for the selected "
                                 "history song.  This usually happens when a file that was once in "
                                 "OpenKJ's database has been removed or renamed outside of OpenKJ\n\n"
                                 "Song: " + path
            );
            return;
        }
        qModel.songAddSlot(dbSongId, curSingerId, key);
    });
    ui->tableViewHistory->clearSelection();
    ui->tabWidgetQueue->setCurrentIndex(0);
}

void MainWindow::on_tableViewHistory_doubleClicked([[maybe_unused]]const QModelIndex &index) {
    switch (ui->comboBoxHistoryDblClick->currentIndex()) {
        case 0:
            on_pushButtonHistoryToQueue_clicked();
            break;
        case 1:
            on_pushButtonHistoryPlay_clicked();
            break;
    }
}

void MainWindow::on_tableViewHistory_customContextMenuRequested(const QPoint &pos) {
    int selCount = ui->tableViewHistory->selectionModel()->selectedRows().size();
    if (selCount == 1) {
        QModelIndex index = ui->tableViewHistory->indexAt(pos);
        if (index.isValid()) {
            QMenu contextMenu(this);
            contextMenu.addAction("Preview", [&]() {
                QString filename = index.sibling(index.row(), 2).data().toString();
                if (!QFile::exists(filename)) {
                    QMessageBox::warning(this, tr("Missing File!"),
                                         "Specified karaoke file missing, preview aborted!\n\n" + dbRtClickFile,
                                         QMessageBox::Ok);
                    return;
                }
                auto *videoPreview = new DlgVideoPreview(filename, this);
                videoPreview->setAttribute(Qt::WA_DeleteOnClose);
                videoPreview->show();
            });
            contextMenu.addAction("Play", this, &MainWindow::on_pushButtonHistoryPlay_clicked);
            contextMenu.addAction("Add to queue", this, &MainWindow::on_pushButtonHistoryToQueue_clicked);
            contextMenu.addSeparator();
            contextMenu.addAction("Delete", [&]() {
                QMessageBox msgBox;
                msgBox.setText("Delete song from singer history?");
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setInformativeText("Are you sure you want to remove this song from the singer's history?");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Cancel);
                int ret = msgBox.exec();
                if (ret == QMessageBox::Cancel)
                    return;
                int songIndex = index.sibling(index.row(), 0).data().toInt();
                historySongsModel.deleteSong(songIndex);
                ui->tableViewHistory->clearSelection();
            });
            contextMenu.exec(QCursor::pos());
        }
    } else if (selCount > 1) {
        QMenu contextMenu(this);
        contextMenu.addAction("Add to queue", this, &MainWindow::on_pushButtonHistoryToQueue_clicked);
        contextMenu.addSeparator();
        contextMenu.addAction("Delete", [&]() {
            QMessageBox msgBox;
            msgBox.setText("Delete songs from singer history?");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setInformativeText("Are you sure you want to remove these songs from the singer's history?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Cancel)
                return;
            auto indexes = ui->tableViewHistory->selectionModel()->selectedRows();
            std::for_each(indexes.rbegin(), indexes.rend(), [&](QModelIndex index) {
                historySongsModel.deleteSong(index.data().toInt());
            });
            ui->tableViewHistory->clearSelection();
        });

        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::on_actionBreak_music_torture_triggered() {
    bmPlaylistsModel->select();
    bmAddPlaylist("torture");
    bmPlaylistsModel->select();
    ui->comboBoxBmPlaylists->setCurrentText("torture");
    ui->tableViewBmDb->selectAll();
    auto mimedata = bmDbModel.mimeData(ui->tableViewBmDb->selectionModel()->selectedIndexes());
    playlistSongsModel.dropMimeData(mimedata, Qt::CopyAction, 0, 3, QModelIndex());

    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static int runs = 0;
        logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        ui->tableViewBmPlaylist->selectRow(0);
        on_tableViewBmPlaylist_doubleClicked(ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0));
        logger->info("{} test runs: ", m_loggingPrefix, ++runs);
    });
    m_timerTest.start(2000);
}

void MainWindow::on_tableViewBmDb_clicked([[maybe_unused]]const QModelIndex &index) {
#ifdef Q_OS_WIN
    ui->tableViewBmPlaylist->setAttribute(Qt::WA_AcceptDrops, false);
    ui->tableViewBmPlaylist->setAttribute(Qt::WA_AcceptDrops, true);
#endif
}

void MainWindow::resetBmLabels() {
    ui->labelBmPlaying->setText("None");
    ui->labelBmNext->setText("None");
    ui->labelBmDuration->setText("00:00");
    ui->labelBmRemaining->setText("00:00");
    ui->labelBmPosition->setText("00:00");
    ui->sliderBmPosition->setValue(0);
}

void MainWindow::on_actionBurn_in_EOS_Jump_triggered() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    m_testMode = true;
    emit ui->buttonClearRotation->clicked();
    for (auto i = 0; i < 21; i++) {
        auto singerName = "Test Singer " + QString::number(i);
        rotModel.singerAdd(singerName);
        // rotModel.regularDelete(singerName);
    }
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        rotModel.singerMove(QRandomGenerator::global()->bounded(0, 19), QRandomGenerator::global()->bounded(0, 19));
        ui->tableViewRotation->selectRow(QRandomGenerator::global()->bounded(0, 19));
        int randno{0};
        if (karaokeSongsModel.rowCount(QModelIndex()) > 1)
            randno = QRandomGenerator::global()->bounded(0, karaokeSongsModel.rowCount(QModelIndex()) - 1);
        logger->info("{} randno: {}", m_loggingPrefix, randno);
        ui->tableViewDB->selectRow(randno);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        emit ui->tableViewDB->doubleClicked(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        ui->tableViewQueue->selectRow(qModel.rowCount() - 1);
        ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        if (qModel.rowCount() > 2) {
            auto newPos = QRandomGenerator::global()->bounded(0, qModel.rowCount() - 1);
            qModel.move(qModel.rowCount() - 1, newPos);
            ui->tableViewQueue->selectRow(newPos);
            ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        }
        auto idx = ui->tableViewQueue->selectionModel()->selectedRows().at(0);
        emit ui->tableViewQueue->doubleClicked(idx);
        ui->tableViewQueue->selectRow(idx.row());
        playing = true;
        QTimer::singleShot(500, [&]() {
            auto duration = kMediaBackend.duration();
            auto jumpPoint = duration - 10000;
            kMediaBackend.setPosition(jumpPoint);
        });
        logger->info("{} Burn in test cycle: {}", m_loggingPrefix, ++runs);
        ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
    });
    m_timerTest.start(13000);
#endif
}

void MainWindow::on_actionSong_Shop_triggered() {
    on_pushButtonShop_clicked();
}



