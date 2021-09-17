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
#include <memory>
#include "dlgaddsong.h"
#include <spdlog/version.h>
#include <taglib.h>
#include <miniz/miniz.h>
#include "okjtypes.h"

#ifdef _MSC_VER
#define NOMINMAX
#include <Windows.h>
#include <timeapi.h>
#endif


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
void MainWindow::addSfxButton(const QString &filename, const QString &label, bool reset) {
    static int numButtons = 0;
    if (reset)
        numButtons = 0;
    int col{0};
    if (numButtons % 2)
        col = 1;
    int row = numButtons / 2;
    auto button = new SoundFxButton(filename, label);
    ui->sfxButtonGrid->addWidget(button, row, col);
    connect(button, &SoundFxButton::clicked, this, &MainWindow::sfxButtonPressed);
    connect(button, &SoundFxButton::customContextMenuRequested, this,
            &MainWindow::sfxButtonContextMenuRequested);
    numButtons++;
}
#pragma clang diagnostic pop

void MainWindow::refreshSfxButtons() {
    QLayoutItem *item;
    while ((item = ui->sfxButtonGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    SfxEntryList list = m_settings.getSfxEntries();
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
    QString thm = (m_settings.theme() == 1) ? ":/theme/Icons/okjbreeze-dark/" : ":/theme/Icons/okjbreeze/";
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
    treatAllSingersAsRegsChanged(m_settings.treatAllSingersAsRegs());
}

void MainWindow::setupShortcuts() {

    m_scutKSelectNextSinger.setKey(m_settings.loadShortcutKeySequence("kSelectNextSinger"));
    m_scutKSelectNextSinger.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKSelectNextSinger, &QShortcut::activated, [&]() {
        okj::RotationSinger nextSinger;
        QString nextSongPath;
        bool empty{false};
        int curSingerId{m_rotModel.currentSinger()};
        int curPos{m_rotModel.getSinger(curSingerId).position};
        if (curSingerId == -1)
            curPos = static_cast<int>(m_rotModel.singerCount() - 1);
        int loops = 0;
        while ((nextSongPath == "") && (!empty)) {
            if (loops > m_rotModel.singerCount()) {
                empty = true;
            } else {
                if (++curPos >= m_rotModel.singerCount()) {
                    curPos = 0;
                }
                nextSinger = m_rotModel.getSingerAtPosition(curPos);
                nextSongPath = nextSinger.nextSongPath();
                loops++;
            }
        }
        if (empty) {
            QMessageBox::information(this, "Unable to select next",
                                     "Sorry, no unsung karaoke songs are currently in any singer's queue");
            return;
        }
        ui->tableViewRotation->clearSelection();
        ui->tableViewRotation->selectRow(nextSinger.position);
    });

    m_scutKPlayNextUnsung.setKey(m_settings.loadShortcutKeySequence("kPlayNextUnsung"));
    m_scutKPlayNextUnsung.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKPlayNextUnsung, &QShortcut::activated, [&]() {
        if (auto state = m_mediaBackendKar.state(); state == MediaBackend::PlayingState ||
                                                    state == MediaBackend::PausedState) {
            if (m_settings.showSongInterruptionWarning()) {
                QMessageBox msgBox(this);
                auto *cb = new QCheckBox("Show this warning in the future");
                cb->setChecked(m_settings.showSongInterruptionWarning());
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Interrupt currently playing karaoke song?");
                msgBox.setInformativeText(
                        "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
                QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.setCheckBox(cb);
                connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSongInterruptionWarning);
                msgBox.exec();
                if (msgBox.clickedButton() != yesButton) {
                    return;
                }
            }
            m_mediaBackendKar.stop();
        }
        okj::RotationSinger nextSinger;
        QString nextSongPath;
        bool empty{false};
        int curSingerId{m_rotModel.currentSinger()};
        int curPos{m_rotModel.getSinger(curSingerId).position};
        if (curSingerId == -1)
            curPos = static_cast<int>(m_rotModel.singerCount() - 1);
        int loops = 0;
        while ((nextSongPath == "") && (!empty)) {
            if (loops > m_rotModel.singerCount()) {
                empty = true;
            } else {
                if (++curPos >= m_rotModel.singerCount()) {
                    curPos = 0;
                }
                nextSinger = m_rotModel.getSingerAtPosition(curPos);
                nextSongPath = nextSinger.nextSongPath();
                loops++;
            }
        }
        if (empty) {
            QMessageBox::information(this, "Unable to play next",
                                     "Sorry, no unsung karaoke songs are currently in any singer's queue");
            return;
        }
        m_curSinger = nextSinger.name;
        m_curArtist = nextSinger.nextSongArtist();
        m_curTitle = nextSinger.nextSongTitle();

        if (m_settings.treatAllSingersAsRegs() || nextSinger.regular) {
            m_historySongsModel.saveSong(
                    nextSinger.name,
                    nextSongPath,
                    m_curArtist,
                    m_curTitle,
                    nextSinger.nextSongSongId(),
                    nextSinger.nextSongKeyChg()
            );
        }
        m_karaokeSongsModel.updateSongHistory(m_karaokeSongsModel.getIdForPath(nextSongPath));
        play(nextSongPath);
        m_mediaBackendKar.setPitchShift(nextSinger.nextSongKeyChg());
        m_qModel.setPlayed(nextSinger.nextSongQueueId());
        m_rotModel.setCurrentSinger(nextSinger.id);
        m_rotDelegate.setCurrentSinger(nextSinger.id);
        if (m_settings.rotationAltSortOrder()) {
            auto curSingerPos = nextSinger.position;
            m_curSingerOriginalPosition = curSingerPos;
            if (curSingerPos != 0)
                m_rotModel.singerMove(curSingerPos, 0);
        }
        ui->labelArtist->setText(m_curArtist);
        ui->labelTitle->setText(m_curTitle);
        ui->labelSinger->setText(nextSinger.name);
        ui->tableViewRotation->clearSelection();
        ui->tableViewRotation->selectRow(m_rotModel.getSinger(m_rotModel.currentSinger()).position);
    });

    m_scutAddSinger.setKey(m_settings.loadShortcutKeySequence("addSinger"));
    m_scutAddSinger.setContext(Qt::ApplicationShortcut);
    connect(&m_scutAddSinger, &QShortcut::activated, this, &MainWindow::showAddSingerDialog);

    m_scutBFfwd.setKey(m_settings.loadShortcutKeySequence("bFfwd"));
    m_scutBFfwd.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBFfwd, &QShortcut::activated, [&]() {
        auto mediaState = m_mediaBackendBm.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            auto curPos = m_mediaBackendBm.position();
            auto duration = m_mediaBackendBm.duration();
            if (curPos + 5000 < duration)
                m_mediaBackendBm.setPosition(curPos + 5000);
        }
    });

    m_scutBPause.setKey(m_settings.loadShortcutKeySequence("bPause"));
    m_scutBPause.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBPause, &QShortcut::activated, [&]() {
        auto mediaState = m_mediaBackendBm.state();
        if (mediaState == MediaBackend::PlayingState) {
            m_mediaBackendBm.pause();
            ui->buttonBmPause->setChecked(true);
        } else if (mediaState == MediaBackend::PausedState) {
            m_mediaBackendBm.play();
            ui->buttonBmPause->setChecked(false);
        }
    });

    m_scutBRestartSong.setKey(m_settings.loadShortcutKeySequence("bRestartSong"));
    m_scutBRestartSong.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBRestartSong, &QShortcut::activated, [&]() {
        auto mediaState = m_mediaBackendBm.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            m_mediaBackendBm.setPosition(0);
        }
    });
    m_scutBRwnd.setKey(m_settings.loadShortcutKeySequence("bRwnd"));
    m_scutBRwnd.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBRwnd, &QShortcut::activated, [&]() {
        auto mediaState = m_mediaBackendBm.state();
        if (mediaState == MediaBackend::PlayingState || mediaState == MediaBackend::PausedState) {
            auto curPos = m_mediaBackendBm.position();
            if (curPos - 5000 > 0)
                m_mediaBackendBm.setPosition(curPos - 5000);
            else
                m_mediaBackendBm.setPosition(0);
        }
    });

    m_scutBStop.setKey(m_settings.loadShortcutKeySequence("bStop"));
    m_scutBStop.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBStop, &QShortcut::activated, [&]() {
        m_mediaBackendBm.stop();
    });

    m_scutBVolDn.setKey(m_settings.loadShortcutKeySequence("bVolDn"));
    m_scutBVolDn.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBVolDn, &QShortcut::activated, [&]() {
        if (int curVol = m_mediaBackendBm.getVolume(); curVol > 0)
            m_mediaBackendBm.setVolume(curVol - 1);
    });

    m_scutBVolMute.setKey(m_settings.loadShortcutKeySequence("bVolMute"));
    m_scutBVolMute.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBVolMute, &QShortcut::activated, [&]() {
        m_mediaBackendBm.setMuted(!m_mediaBackendBm.isMuted());
    });

    m_scutBVolUp.setKey(m_settings.loadShortcutKeySequence("bVolUp"));
    m_scutBVolUp.setContext(Qt::ApplicationShortcut);
    connect(&m_scutBVolUp, &QShortcut::activated, [&]() {
        if (int curVol = m_mediaBackendBm.getVolume(); curVol < 100)
            m_mediaBackendBm.setVolume(curVol + 1);
    });

    m_scutJumpToSearch.setKey(m_settings.loadShortcutKeySequence("jumpToSearch"));
    m_scutJumpToSearch.setContext(Qt::ApplicationShortcut);
    connect(&m_scutJumpToSearch, &QShortcut::activated, [&]() {
        activateWindow();
        if (!hasFocus() && !ui->lineEdit->hasFocus())
            setFocus();
        if (ui->lineEdit->hasFocus())
            ui->lineEdit->clear();
        else
            ui->lineEdit->setFocus();
    });

    m_scutKFfwd.setKey(m_settings.loadShortcutKeySequence("kFfwd"));
    m_scutKFfwd.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKFfwd, &QShortcut::activated, [&]() {
        if (auto state = m_mediaBackendKar.state(); state == MediaBackend::PlayingState ||
                                                    state == MediaBackend::PausedState) {
            if (auto curPos = m_mediaBackendKar.position(); curPos + 5000 < m_mediaBackendKar.duration())
                m_mediaBackendKar.setPosition(curPos + 5000);
        }
    });

    m_scutKPause.setKey(m_settings.loadShortcutKeySequence("kPause"));
    m_scutKPause.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKPause, &QShortcut::activated, this, &MainWindow::buttonPauseClicked);

    m_scutKRestartSong.setKey(m_settings.loadShortcutKeySequence("kRestartSong"));
    m_scutKRestartSong.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKRestartSong, &QShortcut::activated, [&]() {
        if (auto state = m_mediaBackendKar.state(); state == MediaBackend::PlayingState ||
                                                    state == MediaBackend::PausedState)
            m_mediaBackendKar.setPosition(0);
    });

    m_scutKRwnd.setKey(m_settings.loadShortcutKeySequence("kRwnd"));
    m_scutKRwnd.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKRwnd, &QShortcut::activated, [&]() {
        if (auto state = m_mediaBackendKar.state(); state == MediaBackend::PlayingState ||
                                                    state == MediaBackend::PausedState) {

            if (auto curPos = m_mediaBackendKar.position(); curPos - 5000 > 0)
                m_mediaBackendKar.setPosition(curPos - 5000);
            else
                m_mediaBackendKar.setPosition(0);
        }
    });

    m_scutKStop.setKey(m_settings.loadShortcutKeySequence("kStop"));
    m_scutKStop.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKStop, &QShortcut::activated, [&]() {
        m_mediaBackendKar.stop();
    });

    m_scutKVolDn.setKey(m_settings.loadShortcutKeySequence("kVolDn"));
    m_scutKVolDn.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKVolDn, &QShortcut::activated, [&]() {
        int curVol = m_mediaBackendKar.getVolume();
        if (curVol > 0)
            m_mediaBackendKar.setVolume(curVol - 1);
    });

    m_scutKVolMute.setKey(m_settings.loadShortcutKeySequence("kVolMute"));
    m_scutKVolMute.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKVolMute, &QShortcut::activated, [&]() {
        m_mediaBackendKar.setMuted(!m_mediaBackendKar.isMuted());
    });

    m_scutKVolUp.setKey(m_settings.loadShortcutKeySequence("kVolUp"));
    m_scutKVolUp.setContext(Qt::ApplicationShortcut);
    connect(&m_scutKVolUp, &QShortcut::activated, [&]() {
        if (int curVol = m_mediaBackendKar.getVolume(); curVol < 100)
            m_mediaBackendKar.setVolume(curVol + 1);
    });

    m_scutLoadRegularSinger.setKey(m_settings.loadShortcutKeySequence("loadRegularSinger"));
    m_scutLoadRegularSinger.setContext(Qt::ApplicationShortcut);
    connect(&m_scutLoadRegularSinger, &QShortcut::activated, &m_dlgRegularSingers,
            &DlgRegularSingers::toggleVisibility);

    m_scutToggleSingerWindow.setKey(m_settings.loadShortcutKeySequence("toggleSingerWindow"));
    m_scutToggleSingerWindow.setContext(Qt::ApplicationShortcut);
    connect(&m_scutToggleSingerWindow, &QShortcut::activated, [&]() {
        cdgWindow->setVisible(!cdgWindow->isVisible());
    });

    m_scutRequests.setKey(m_settings.loadShortcutKeySequence("showIncomingRequests"));
    m_scutRequests.setContext(Qt::ApplicationShortcut);
    connect(&m_scutRequests, &QShortcut::activated, requestsDialog.get(), &DlgRequests::show);

    m_scutDeleteSinger.setParent(ui->tableViewRotation);
    m_scutDeleteSinger.setKey(QKeySequence::Delete);
    m_scutDeleteSinger.setContext(Qt::WidgetShortcut);
    connect(&m_scutDeleteSinger, &QShortcut::activated, [&]() {
        auto indexes = ui->tableViewRotation->selectionModel()->selectedRows(0);
        std::vector<int> singerIds;
        std::for_each(indexes.begin(), indexes.end(), [&](auto index) {
            singerIds.emplace_back(index.data(Qt::UserRole).toInt());
        });
        if (singerIds.empty())
            return;
        if (m_settings.showSingerRemovalWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(m_settings.showSingerRemovalWarning());
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
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSingerRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }

        m_qModel.loadSinger(-1);
        std::for_each(singerIds.begin(), singerIds.end(), [&](auto singerId) {
            if (m_rotModel.currentSinger() == singerId) {
                m_rotModel.setCurrentSinger(-1);
                m_rotDelegate.setCurrentSinger(-1);
            }
            m_rotModel.singerDelete(singerId);
        });
        ui->tableViewRotation->clearSelection();
        ui->tableViewQueue->clearSelection();
    });

    m_scutDeleteSong.setParent(ui->tableViewQueue);
    m_scutDeleteSong.setKey(QKeySequence::Delete);
    m_scutDeleteSong.setContext(Qt::WidgetShortcut);
    connect(&m_scutDeleteSong, &QShortcut::activated, [&]() {
        auto indexes = ui->tableViewQueue->selectionModel()->selectedRows(0);
        bool containsUnplayed{false};
        std::vector<int> songIds;
        std::for_each(indexes.begin(), indexes.end(), [&](auto index) {
            songIds.emplace_back(index.data().toInt());
            if (!m_qModel.getPlayed(index.data().toInt()))
                containsUnplayed = true;
        });
        if ((m_settings.showQueueRemovalWarning()) && containsUnplayed) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(m_settings.showQueueRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Removing un-played song from queue");
            msgBox.setInformativeText("This song has not been played yet, are you sure you want to remove it?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowQueueRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton)
                return;
        }
        std::for_each(songIds.begin(), songIds.end(), [&](auto songId) {
            m_qModel.remove(songId);
        });
    });

    m_scutDeletePlSong.setParent(ui->tableViewBmPlaylist);
    m_scutDeleteSong.setKey(QKeySequence::Delete);
    m_scutDeleteSong.setContext(Qt::WidgetShortcut);
    connect(&m_scutDeletePlSong, &QShortcut::activated, [&]() {
        auto rows = ui->tableViewBmPlaylist->selectionModel()->selectedRows(0);
        std::vector<int> positions;
        bool curPlayingSelected{false};
        std::for_each(rows.begin(), rows.end(), [&](auto index) {
            positions.emplace_back(index.row());
            if (m_tableModelPlaylistSongs.isCurrentlyPlayingSong(index.data(Qt::UserRole).toInt()))
                curPlayingSelected = true;
        });
        auto state = m_mediaBackendBm.state();
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
            m_tableModelPlaylistSongs.deleteSong(position);
        });
        if (curPlayingSelected) {
            m_tableModelPlaylistSongs.setCurrentPosition(-1);
        }
        m_tableModelPlaylistSongs.savePlaylistChanges();
        if (state != MediaBackend::PlayingState && state != MediaBackend::PausedState)
            return;
        if (ui->checkBoxBmBreak->isChecked())
            ui->labelBmNext->setText("None - Breaking after current song");
        else {
            auto nextSong = m_tableModelPlaylistSongs.getNextPlSong();
            if (nextSong.has_value())
                ui->labelBmNext->setText(nextSong->get().artist + " - " + nextSong->get().title);
            else
                ui->labelBmNext->setText("None - Breaking after current song");
        }
    });

}

void MainWindow::shortcutsUpdated() {
    m_scutKSelectNextSinger.setKey(m_settings.loadShortcutKeySequence("kSelectNextSinger"));
    m_scutKPlayNextUnsung.setKey(m_settings.loadShortcutKeySequence("kPlayNextUnsung"));
    m_scutAddSinger.setKey(m_settings.loadShortcutKeySequence("addSinger"));
    m_scutBFfwd.setKey(m_settings.loadShortcutKeySequence("bFfwd"));
    m_scutBPause.setKey(m_settings.loadShortcutKeySequence("bPause"));
    m_scutBRestartSong.setKey(m_settings.loadShortcutKeySequence("bRestartSong"));
    m_scutBRwnd.setKey(m_settings.loadShortcutKeySequence("bRwnd"));
    m_scutBStop.setKey(m_settings.loadShortcutKeySequence("bStop"));
    m_scutBVolDn.setKey(m_settings.loadShortcutKeySequence("bVolDn"));
    m_scutBVolMute.setKey(m_settings.loadShortcutKeySequence("bVolMute"));
    m_scutBVolUp.setKey(m_settings.loadShortcutKeySequence("bVolUp"));
    m_scutJumpToSearch.setKey(m_settings.loadShortcutKeySequence("jumpToSearch"));
    m_scutKFfwd.setKey(m_settings.loadShortcutKeySequence("kFfwd"));
    m_scutKPause.setKey(m_settings.loadShortcutKeySequence("kPause"));
    m_scutKRestartSong.setKey(m_settings.loadShortcutKeySequence("kRestartSong"));
    m_scutKRwnd.setKey(m_settings.loadShortcutKeySequence("kRwnd"));
    m_scutKStop.setKey(m_settings.loadShortcutKeySequence("kStop"));
    m_scutKVolDn.setKey(m_settings.loadShortcutKeySequence("kVolDn"));
    m_scutKVolMute.setKey(m_settings.loadShortcutKeySequence("kVolMute"));
    m_scutKVolUp.setKey(m_settings.loadShortcutKeySequence("kVolUp"));
    m_scutLoadRegularSinger.setKey(m_settings.loadShortcutKeySequence("loadRegularSinger"));
    m_scutRequests.setKey(m_settings.loadShortcutKeySequence("showIncomingRequests"));
    m_scutToggleSingerWindow.setKey(m_settings.loadShortcutKeySequence("toggleSingerWindow"));
}

void MainWindow::treatAllSingersAsRegsChanged(bool enabled) {
    if (enabled) {
        ui->tableViewRotation->hideColumn(TableModelRotation::COL_REGULAR);
        if (ui->tabWidgetQueue->count() == 1)
            ui->tabWidgetQueue->addTab(m_historyTabWidget, "History");
    } else {
        ui->tableViewRotation->showColumn(TableModelRotation::COL_REGULAR);
        int curSelSingerId{-1};
        if (ui->tableViewRotation->selectionModel()->selectedRows().count() > 1) {
            curSelSingerId = ui->tableViewRotation->selectionModel()->selectedRows(0).at(
                    TableModelRotation::COL_ID).data(Qt::UserRole).toInt();
        }
        if (!m_rotModel.getSinger(curSelSingerId).regular && ui->tabWidgetQueue->count() == 2)
            ui->tabWidgetQueue->removeTab(1);
    }
    autosizeRotationCols();
}

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow) {
    m_logger = spdlog::get("logger");
#ifdef _MSC_VER
    timeBeginPeriod(1);
#endif
    QCoreApplication::setOrganizationName("OpenKJ");
    QCoreApplication::setOrganizationDomain("OpenKJ.org");
    QCoreApplication::setApplicationName("OpenKJ");
    ui->setupUi(this);
    setMouseTracking(true);
    m_songShop = std::make_unique<SongShop>(this);
    m_lazyDurationUpdater = std::make_unique<LazyDurationUpdateController>(this);
    ui->tableViewBmPlaylist->setMouseTracking(true);
    m_historyTabWidget = ui->tabWidgetQueue->widget(1);
    ui->actionShow_Debug_Log->setChecked(m_settings.logShow());
#ifdef Q_OS_WIN
    ui->sliderBmPosition->setMaximumHeight(12);
    ui->sliderBmVolume->setMaximumWidth(12);
    ui->sliderProgress->setMaximumHeight(12);
#endif
    QDir okjDataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!okjDataDir.exists()) {
        okjDataDir.mkpath(okjDataDir.absolutePath());
    }
    dbInit(okjDataDir);
    ui->videoPreviewBm->hide();
    ui->pushButtonKeyDn->setEnabled(false);
    ui->pushButtonKeyUp->setEnabled(false);
    ui->pushButtonTempoDn->setEnabled(false);
    ui->pushButtonTempoUp->setEnabled(false);
    m_karaokeSongsModel.loadData();
    m_rotModel.loadData();
    ui->comboBoxHistoryDblClick->addItems(QStringList{"Adds to queue", "Plays song"});
    ui->tabWidgetQueue->setCurrentIndex(0);
    ui->tableViewHistory->setModel(&m_historySongsModel);
    ui->tableViewHistory->hideColumn(0);
    ui->tableViewHistory->hideColumn(1);
    ui->tableViewHistory->hideColumn(2);
    ui->tableViewHistory->sortByColumn(3, Qt::AscendingOrder);
    ui->tableViewHistory->resizeColumnsToContents();
    ui->comboBoxSearchType->addItems({QString("All"), QString("Artist"), QString("Title")});
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewRotation->setModel(&m_rotModel);
    ui->tableViewRotation->setItemDelegate(&m_rotDelegate);
    ui->tableViewRotation->hideColumn(TableModelRotation::COL_ADDTS);
    ui->tableViewRotation->hideColumn(TableModelRotation::COL_POSITION);
    if (m_settings.treatAllSingersAsRegs())
        ui->tableViewRotation->hideColumn(TableModelRotation::COL_REGULAR);
    if (m_settings.rotationShowNextSong()) {
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
    ui->tableViewQueue->setModel(&m_qModel);
    ui->tableViewQueue->setItemDelegate(&m_qDelegate);
    ui->tableViewQueue->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewQueue));
    ui->labelNoSinger->setVisible(true);
    ui->tabWidgetQueue->setVisible(false);
    m_mediaTempDir = std::make_unique<QTemporaryDir>();
    dbDialog = std::make_unique<DlgDatabase>(m_karaokeSongsModel, this);
    dlgKeyChange = std::make_unique<DlgKeyChange>(&m_qModel, this);
    requestsDialog = std::make_unique<DlgRequests>(m_rotModel, m_songbookApi);
    requestsDialog->setModal(false);
    dlgSongShop = std::make_unique<DlgSongShop>(m_songShop);
    dlgSongShop->setModal(false);
    ui->tableViewDB->setModel(&m_karaokeSongsModel);
    ui->tableViewDB->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewDB));
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
    cdgWindow = std::make_unique<DlgCdg>(m_mediaBackendKar, m_mediaBackendBm, nullptr, Qt::Window);
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_ID);
    ui->tableViewDB->hideColumn(TableModelKaraokeSongs::COL_FILENAME);
    ui->tableViewQueue->hideColumn(TableModelQueueSongs::COL_ID);
    ui->tableViewQueue->hideColumn(TableModelQueueSongs::COL_DBSONGID);
    if (!MediaBackend::canPitchShift()) {
        ui->tableViewQueue->hideColumn(TableModelQueueSongs::COL_KEY);
    }
    m_rotModel.setHeaderData(0, Qt::Horizontal, "");
    m_rotModel.setHeaderData(1, Qt::Horizontal, "Singer");
    m_rotModel.setHeaderData(3, Qt::Horizontal, "");
    m_rotModel.setHeaderData(4, Qt::Horizontal, "");
    m_logger->info("{} Adding singer count to status bar", m_loggingPrefix);
    ui->statusBar->addWidget(&m_labelSingerCount);
    ui->statusBar->addWidget(&m_labelRotationDuration);
    m_tableModelPlaylists = std::make_unique<QSqlTableModel>(this, m_database);
    m_tableModelPlaylists->setTable("bmplaylists");
    m_tableModelPlaylists->sort(2, Qt::AscendingOrder);
    bmDbDialog = std::make_unique<BmDbDialog>(this);
    ui->comboBoxBmPlaylists->setModel(m_tableModelPlaylists.get());
    ui->comboBoxBmPlaylists->setModelColumn(1);
    if (m_tableModelPlaylists->rowCount() == 0) {
        bmAddPlaylist("Default");
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
    }
    ui->tableViewBmDb->setModel(&m_tableModelBreakSongs);
    m_tableModelBreakSongs.loadDatabase();
    ui->tableViewBmDb->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewBmDb));
    ui->tableViewBmPlaylist->setModel(&m_tableModelPlaylistSongs);
    ui->tableViewBmPlaylist->viewport()->installEventFilter(new TableViewToolTipFilter(ui->tableViewBmPlaylist));
    ui->tableViewBmPlaylist->setItemDelegate(&m_itemDelegatePlSongs);
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_ID, true);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_POSITION, true);
    m_updateChecker = std::make_unique<UpdateChecker>(this);
    m_updateChecker->checkForUpdates();
    m_timerButtonFlash.start(1000);
    m_logger->info("{} Initial UI setup complete", m_loggingPrefix);
    QApplication::processEvents();
    appFontChanged(m_settings.applicationFont());
    QTimer::singleShot(500, [&]() {
        autosizeViews();
        autosizeBmViews();
    });
    m_dlgRegularSingers.regularsChanged();
    m_dlgRegularSingers.setModal(false);
    updateRotationDuration();
    if (m_settings.dbLazyLoadDurations())
        m_lazyDurationUpdater->getDurations();
    ui->labelVolume->setPixmap(QIcon::fromTheme("player-volume").pixmap(QSize(22, 22)));
    ui->labelVolumeBm->setPixmap(QIcon::fromTheme("player-volume").pixmap(QSize(22, 22)));
    updateIcons();

    std::vector<QWidget *> videoWidgets{cdgWindow->getVideoDisplay(), ui->videoPreview};
    m_mediaBackendBm.setVideoOutputWidgets({cdgWindow->getVideoDisplayBm(), ui->videoPreviewBm});
    m_mediaBackendKar.setVideoOutputWidgets(videoWidgets);
    m_settings.setStartupOk(true);
    m_mediaBackendBm.stop(true);

    loadSettings();
    setupShortcuts();
    setupConnections();
    m_timerSlowUiUpdate.start(10000);
}

void MainWindow::loadSettings() {
    if (m_settings.theme() != 0) {
        ui->pushButtonIncomingRequests->setStyleSheet("");
        update();
    }
    ui->sliderBmVolume->setValue(m_settings.bmVolume());
    m_mediaBackendBm.setVolume(m_settings.bmVolume());
    ui->sliderVolume->setValue(m_settings.audioVolume());
    m_mediaBackendKar.setVolume(m_settings.audioVolume());
    m_logger->debug("{} Initial volumes - K: {} - BM {}", m_loggingPrefix, m_settings.audioVolume(), m_settings.bmVolume());
    ui->comboBoxHistoryDblClick->setCurrentIndex(m_settings.historyDblClickAction());
    if (m_settings.rotationShowNextSong())
        m_settings.restoreColumnWidths(ui->tableViewRotation);
    m_settings.restoreWindowState(cdgWindow.get());
    m_mediaBackendKar.setUseFader(m_settings.audioUseFader());
    m_mediaBackendKar.setUseSilenceDetection(m_settings.audioDetectSilence());
    m_mediaBackendBm.setUseFader(m_settings.audioUseFaderBm());
    m_mediaBackendBm.setUseSilenceDetection(m_settings.audioDetectSilenceBm());
    m_mediaBackendKar.setDownmix(m_settings.audioDownmix());
    m_mediaBackendBm.setDownmix(m_settings.audioDownmixBm());
    m_settings.restoreWindowState(requestsDialog.get());
    m_settings.restoreWindowState(dbDialog.get());
    m_settings.restoreSplitterState(ui->splitter);
    m_settings.restoreSplitterState(ui->splitter_2);
    m_settings.restoreSplitterState(ui->splitterBm);
    m_settings.restoreWindowState(dlgSongShop.get());
    m_bmCurrentPlaylist = m_settings.bmPlaylistIndex();
    ui->comboBoxBmPlaylists->setCurrentIndex(m_settings.bmPlaylistIndex());
    ui->actionDisplay_Filenames->setChecked(m_settings.bmShowFilenames());
    ui->actionDisplay_Metadata->setChecked(m_settings.bmShowMetadata());
    actionDisplayFilenamesToggled(m_settings.bmShowFilenames());
    actionDisplayMetadataToggled(m_settings.bmShowMetadata());
    m_settings.restoreSplitterState(ui->splitterBm);
    m_settings.restoreSplitterState(ui->splitter_3);
    if (m_settings.mplxMode() == Multiplex_Normal)
        pushButtonMplxBothToggled(true);
    else if (m_settings.mplxMode() == Multiplex_LeftChannel)
        pushButtonMplxLeftToggled(true);
    else if (m_settings.mplxMode() == Multiplex_RightChannel)
        pushButtonMplxRightToggled(true);
    ui->actionAutoplay_mode->setChecked(m_settings.karaokeAutoAdvance());
    m_mediaBackendKar.setEnforceAspectRatio(m_settings.enforceAspectRatio());
    m_mediaBackendBm.setEnforceAspectRatio(m_settings.enforceAspectRatio());
    m_mediaBackendKar.setEqBypass(m_settings.eqKBypass());
    m_mediaBackendBm.setEqBypass(m_settings.eqBBypass());
    for (int band = 0; band < 10; band++) {
        m_mediaBackendKar.setEqLevel(band, m_settings.getEqKLevel(band));
        m_mediaBackendBm.setEqLevel(band, m_settings.getEqBLevel(band));
    }
    ui->pushButtonIncomingRequests->setVisible(m_settings.requestServerEnabled());
    ui->btnToggleCdgWindow->setChecked(m_settings.showCdgWindow());
    ui->groupBoxNowPlaying->setVisible(m_settings.showMainWindowNowPlaying());
    ui->groupBoxSoundClips->setVisible(m_settings.showMainWindowSoundClips());
    if (m_settings.showMainWindowSoundClips()) {
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
    SfxEntryList list = m_settings.getSfxEntries();
    foreach (SfxEntry entry, list) {
        addSfxButton(entry.path, entry.name);
    }
    m_rotModel.setCurrentSinger(m_settings.currentRotationPosition());
    m_rotDelegate.setCurrentSinger(m_settings.currentRotationPosition());
    ui->videoPreview->setVisible(m_settings.showMainWindowVideo());
    ui->actionNow_Playing->setChecked(m_settings.showMainWindowNowPlaying());
    ui->actionSound_Clips->setChecked(m_settings.showMainWindowSoundClips());
    ui->actionVideo_Output_2->setChecked(m_settings.showMainWindowVideo());
    ui->actionMultiplex_Controls->setChecked(m_settings.showMplxControls());
    ui->widgetMplxControls->setVisible(m_settings.showMplxControls());
    ui->menuTesting->menuAction()->setVisible(m_settings.testingEnabled());
    switch (m_settings.mainWindowVideoSize()) {
        case Settings::Small:
            actionVideoSmallTriggered();
            break;
        case Settings::Medium:
            actionVideoMediumTriggered();
            break;
        case Settings::Large:
            actionVideoLargeTriggered();
            break;
    }
    comboBoxBmPlaylistsIndexChanged(m_settings.bmPlaylistIndex());
    QTimer::singleShot(250, [&]() {
        m_settings.restoreWindowState(this);
        m_initialUiSetupDone = true;
        rotationDataChanged();
    });
    if (m_settings.bmAutoStart()) {
        QTimer::singleShot(1000, [&]() {
            if (m_tableModelPlaylistSongs.rowCount() > 0)
            {
                m_tableModelPlaylistSongs.setCurrentPosition(0);
                auto plSong = m_tableModelPlaylistSongs.getCurrentSong();
                if (plSong.has_value()) {
                    if (QFile::exists(plSong->get().path)) {
                        m_mediaBackendBm.setMedia(plSong->get().path);
                        m_mediaBackendBm.play();
                        m_mediaBackendBm.setVolume(ui->sliderBmVolume->value());
                    } else {
                        QMessageBox::warning(this, tr("Break music autostart failure"),
                                             tr("Break music is set to autostart but the first song in the current playlist was not found.\n\nAborting playback."),
                                             QMessageBox::Ok);
                    }
                }
            }
        });
    }
    // Without this, QAction items aren't getting set properly on program startup.  Not sure why, but it must be
    // something to do with it needing to be set after the window is finished building.
    QTimer::singleShot(250, [&] () {
        appFontChanged(m_settings.applicationFont());
        m_settings.restoreColumnWidths(ui->tableViewDB);
        m_settings.restoreColumnWidths(ui->tableViewQueue);
    });
}

void MainWindow::setupConnections() {
    connect(ui->tabWidgetQueue, &QTabWidget::currentChanged, [&] (auto tab) {
       if (tab == 1) {
           QApplication::processEvents();
           ui->tableViewHistory->resizeColumnsToContents();
       }
    });
    connect(cdgWindow.get(), &DlgCdg::visibilityChanged, ui->btnToggleCdgWindow, &QPushButton::setChecked);
    connect(ui->comboBoxHistoryDblClick, QOverload<int>::of(&QComboBox::currentIndexChanged), &m_settings,
            &Settings::setHistoryDblClickAction);
    connect(&m_rotModel, &TableModelRotation::songDroppedOnSinger, this, &MainWindow::songDroppedOnSinger);
    connect(dbDialog.get(), &DlgDatabase::databaseUpdateComplete, this, &MainWindow::databaseUpdated);
    connect(dbDialog.get(), &DlgDatabase::databaseSongAdded, &m_karaokeSongsModel, &TableModelKaraokeSongs::loadData);
    connect(dbDialog.get(), &DlgDatabase::databaseSongAdded, requestsDialog.get(), &DlgRequests::databaseSongAdded);
    connect(dbDialog.get(), &DlgDatabase::databaseCleared, this, &MainWindow::databaseCleared);
    connect(&m_mediaBackendKar, &MediaBackend::volumeChanged, ui->sliderVolume, &QSlider::setValue);
    connect(&m_mediaBackendKar, &MediaBackend::positionChanged, this, &MainWindow::karaokeMediaBackend_positionChanged);
    connect(&m_mediaBackendKar, &MediaBackend::durationChanged, this, &MainWindow::karaokeMediaBackend_durationChanged);
    connect(&m_mediaBackendKar, &MediaBackend::stateChanged, this, &MainWindow::karaokeMediaBackend_stateChanged);
    connect(&m_mediaBackendKar, &MediaBackend::hasActiveVideoChanged, [=](const bool &isActive) {
        m_kHasActiveVideo = isActive;
        hasActiveVideoChanged();
    });
    connect(&m_mediaBackendKar, &MediaBackend::pitchChanged, ui->spinBoxKey, &QSpinBox::setValue);
    connect(&m_mediaBackendKar, &MediaBackend::audioError, this, &MainWindow::audioError);
    connect(&m_mediaBackendKar, &MediaBackend::silenceDetected, this, &MainWindow::silenceDetectedKar);
    connect(&m_mediaBackendBm, &MediaBackend::audioError, this, &MainWindow::audioError);
    connect(&m_mediaBackendBm, &MediaBackend::silenceDetected, this, &MainWindow::silenceDetectedBm);
    connect(&m_mediaBackendBm, &MediaBackend::hasActiveVideoChanged, [=](const bool &isActive) {
        m_bmHasActiveVideo = isActive;
        hasActiveVideoChanged();
    });
    connect(&m_mediaBackendSfx, &MediaBackend::positionChanged, this, &MainWindow::sfxAudioBackend_positionChanged);
    connect(&m_mediaBackendSfx, &MediaBackend::durationChanged, this, &MainWindow::sfxAudioBackend_durationChanged);
    connect(&m_mediaBackendSfx, &MediaBackend::stateChanged, this, &MainWindow::sfxAudioBackend_stateChanged);
    connect(&m_rotModel, &TableModelRotation::rotationModified, this, &MainWindow::rotationDataChanged, Qt::QueuedConnection);
    connect(m_songShop.get(), &SongShop::karaokeSongDownloaded, dbDialog.get(), &DlgDatabase::singleSongAdd);
    connect(ui->pushButtonTempoDn, &QPushButton::clicked, ui->spinBoxTempo, &QSpinBox::stepDown);
    connect(ui->pushButtonTempoUp, &QPushButton::clicked, ui->spinBoxTempo, &QSpinBox::stepUp);
    connect(ui->pushButtonKeyDn, &QPushButton::clicked, ui->spinBoxKey, &QSpinBox::stepDown);
    connect(ui->pushButtonKeyUp, &QPushButton::clicked, ui->spinBoxKey, &QSpinBox::stepUp);
    connect(requestsDialog.get(), &DlgRequests::addRequestSong, &m_qModel, &TableModelQueueSongs::songAddSlot);
    connect(&m_mediaBackendBm, &MediaBackend::stateChanged, this, &MainWindow::bmMediaStateChanged);
    connect(&m_mediaBackendBm, &MediaBackend::positionChanged, this, &MainWindow::bmMediaPositionChanged);
    connect(&m_mediaBackendBm, &MediaBackend::durationChanged, this, &MainWindow::bmMediaDurationChanged);
    connect(&m_mediaBackendBm, &MediaBackend::volumeChanged, ui->sliderBmVolume, &QSlider::setValue);
    connect(bmDbDialog.get(), &BmDbDialog::bmDbUpdated, this, &MainWindow::bmDbUpdated);
    connect(bmDbDialog.get(), &BmDbDialog::bmDbCleared, this, &MainWindow::bmDbCleared);
    connect(bmDbDialog.get(), &BmDbDialog::bmDbAboutToUpdate, this, &MainWindow::bmDatabaseAboutToUpdate);
    connect(&m_timerKaraokeAA, &QTimer::timeout, this, &MainWindow::karaokeAATimerTimeout);
    connect(ui->actionAutoplay_mode, &QAction::toggled, &m_settings, &Settings::setKaraokeAutoAdvance);


    connect(ui->lineEdit, &CustomLineEdit::escapePressed, ui->lineEdit, &CustomLineEdit::clear);
    connect(ui->lineEditBmSearch, &CustomLineEdit::escapePressed, ui->lineEditBmSearch, &CustomLineEdit::clear);
    connect(&m_qModel, &TableModelQueueSongs::songDroppedWithoutSinger, this, &MainWindow::songDropNoSingerSel);
    connect(ui->splitter_3, &QSplitter::splitterMoved, [&]() { autosizeViews(); });
    connect(ui->splitterBm, &QSplitter::splitterMoved, [&]() { autosizeBmViews(); });
    connect(m_updateChecker.get(), &UpdateChecker::newVersionAvailable, this, &MainWindow::newVersionAvailable);
    connect(&m_timerButtonFlash, &QTimer::timeout, this, &MainWindow::timerButtonFlashTimeout);

    connect(ui->actionSong_Shop, &QAction::triggered, [&]() { show(); });
    connect(&m_qModel, &TableModelQueueSongs::filesDroppedOnSinger, this, &MainWindow::filesDroppedOnQueue);
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &MainWindow::tableViewRotationCurrentChanged);
    connect(&m_tableModelPlaylistSongs, &TableModelPlaylistSongs::bmSongMoved, this, &MainWindow::bmSongMoved);
    connect(&m_songbookApi, &OKJSongbookAPI::alertRecieved, this, &MainWindow::showAlert);
    connect(&m_dlgRegularSingers.historySingersModel(), &TableModelHistorySingers::historySingersModified, [&]() {
        m_historySongsModel.refresh();
    });
    connect(&m_qModel, &TableModelQueueSongs::queueModified, &m_dlgRegularSingers, &DlgRegularSingers::regularsChanged);
    connect(&m_timerSlowUiUpdate, &QTimer::timeout, this, &MainWindow::updateRotationDuration);
    connect(&m_qModel, &TableModelQueueSongs::queueModified, [&]() {
        updateRotationDuration();
        m_rotModel.layoutChanged();
    });
    connect(m_lazyDurationUpdater.get(), &LazyDurationUpdateController::gotDuration, &m_karaokeSongsModel,
            &TableModelKaraokeSongs::setSongDuration);
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::rotationSelectionChanged);
    connect(ui->tableViewQueue->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::tableViewQueueSelChanged);
    connect(ui->tableViewBmPlaylist->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::tableViewPlaylistSelectionChanged);
    connect(ui->tableViewRotation->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::tableViewRotationSelChanged);
    connect(&m_tableModelPlaylistSongs, &TableModelPlaylistSongs::bmPlSongsMoved,
            [&](auto startRow, auto startCol, auto endRow, auto endCol) {
        auto topLeft = ui->tableViewBmPlaylist->model()->index(startRow, startCol);
        auto bottomRight = ui->tableViewBmPlaylist->model()->index(endRow, endCol);
        ui->tableViewBmPlaylist->clearSelection();
        ui->tableViewBmPlaylist->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                          QItemSelectionModel::Select);
        auto nextPlSong = m_tableModelPlaylistSongs.getNextPlSong();
        if (nextPlSong.has_value() && !ui->checkBoxBmBreak->isChecked()) {
            ui->labelBmNext->setText(nextPlSong->get().artist + " - " + nextPlSong->get().title);
        } else {
            ui->labelBmNext->setText("Breaking after current song");
        }

    });
    connect(&m_tableModelPlaylistSongs, &TableModelPlaylistSongs::playingPlSongIdChanged, &m_itemDelegatePlSongs,
            &ItemDelegatePlaylistSongs::setPlayingPlSongId);
    connect(&m_qModel, &TableModelQueueSongs::qSongsMoved, [&](auto startRow, auto startCol, auto endRow, auto endCol) {
        auto topLeft = ui->tableViewQueue->model()->index(startRow, startCol);
        auto bottomRight = ui->tableViewQueue->model()->index(endRow, endCol);
        ui->tableViewQueue->clearSelection();
        ui->tableViewQueue->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    });
    connect(&m_rotModel, &TableModelRotation::singersMoved,
            [&](auto startRow, auto startCol, auto endRow, auto endCol) {
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
    connect(ui->buttonStop, &QPushButton::clicked, this, &MainWindow::buttonStopClicked);
    connect(ui->buttonPause, &QPushButton::clicked, this, &MainWindow::buttonPauseClicked);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &MainWindow::search);
    connect(ui->lineEditBmSearch, &QLineEdit::returnPressed, this, &MainWindow::searchBreakMusic);
    connect(ui->tableViewDB, &QTableView::doubleClicked, this, &MainWindow::tableViewDbDoubleClicked);
    connect(ui->buttonAddSinger, &QPushButton::clicked, this, &MainWindow::showAddSingerDialog);
    connect(ui->tableViewRotation, &QTableView::doubleClicked, this, &MainWindow::tableViewRotationDoubleClicked);
    connect(ui->tableViewRotation, &QTableView::clicked, this, &MainWindow::tableViewRotationClicked);
    connect(ui->tableViewQueue, &QTableView::doubleClicked, this, &MainWindow::tableViewQueueDoubleClicked);
    connect(ui->tableViewQueue, &QTableView::clicked, this, &MainWindow::tableViewQueueClicked);
    connect(ui->tableViewBmPlaylist, &QTableView::clicked, this, &MainWindow::tableViewBmPlaylistClicked);
    connect(ui->tableViewBmPlaylist, &QTableView::doubleClicked, this, &MainWindow::tableViewBmPlaylistDoubleClicked);
    connect(ui->tableViewBmDb, &QTableView::doubleClicked, this, &MainWindow::tableViewBmDbDoubleClicked);
    connect(ui->tableViewBmDb, &QTableView::clicked, this, &MainWindow::tableViewBmDbClicked);
    connect(ui->actionManage_DB, &QAction::triggered, dbDialog.get(), &DlgDatabase::showNormal);
    connect(ui->actionManage_Karaoke_DB, &QAction::triggered, dbDialog.get(), &DlgDatabase::showNormal);
    connect(ui->actionExport_Regulars, &QAction::triggered, this, &MainWindow::actionExportRegularsTriggered);
    connect(ui->actionImport_Regulars, &QAction::triggered, this, &MainWindow::actionImportRegularsTriggered);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::actionSettingsTriggered);
    connect(ui->actionRegulars, &QAction::triggered, &m_dlgRegularSingers, &DlgRegularSingers::showNormal);
    connect(ui->actionIncoming_Requests, &QAction::triggered, requestsDialog.get(), &DlgRequests::show);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::search);
    connect(ui->buttonClearRotation, &QPushButton::clicked, this, &MainWindow::clearRotation);
    connect(ui->buttonClearQueue, &QPushButton::clicked, this, &MainWindow::clearSingerQueue);
    connect(ui->spinBoxKey, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::spinBoxKeyValueChanged);
    connect(ui->buttonRegulars, &QPushButton::clicked, &m_dlgRegularSingers, &DlgRegularSingers::toggleVisibility);
    connect(ui->tableViewDB, &QTableView::customContextMenuRequested, this,
            &MainWindow::tableViewDBContextMenuRequested);
    connect(ui->tableViewQueue, &QTableView::customContextMenuRequested, this,
            &MainWindow::tableViewQueueContextMenuRequested);
    connect(ui->tableViewRotation, &QTableView::customContextMenuRequested, this,
            &MainWindow::tableViewRotationContextMenuRequested);
    connect(ui->sliderProgress, &QSlider::sliderPressed, this, &MainWindow::sliderProgressPressed);
    connect(ui->sliderProgress, &QSlider::sliderReleased, this, &MainWindow::sliderProgressReleased);
    connect(ui->actionManage_Break_DB, &QAction::triggered, bmDbDialog.get(), &BmDbDialog::show);
    connect(ui->comboBoxBmPlaylists, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MainWindow::comboBoxBmPlaylistsIndexChanged);
    connect(ui->checkBoxBmBreak, &QCheckBox::toggled, this, &MainWindow::checkBoxBmBreakToggled);
    connect(ui->buttonBmStop, &QPushButton::clicked, this, &MainWindow::buttonBmStopClicked);
    connect(ui->buttonBmPause, &QPushButton::clicked, this, &MainWindow::buttonBmPauseClicked);
    connect(ui->actionDisplay_Filenames, &QAction::toggled, this, &MainWindow::actionDisplayFilenamesToggled);
    connect(ui->actionDisplay_Metadata, &QAction::toggled, this, &MainWindow::actionDisplayMetadataToggled);
    connect(ui->actionPlaylistNew, &QAction::triggered, this, &MainWindow::actionPlaylistNewTriggered);
    connect(ui->actionPlaylistImport, &QAction::triggered, this, &MainWindow::actionPlaylistImportTriggered);
    connect(ui->actionPlaylistExport, &QAction::triggered, this, &MainWindow::actionPlaylistExportTriggered);
    connect(ui->actionPlaylistDelete, &QAction::triggered, this, &MainWindow::actionPlaylistDeleteTriggered);
    connect(ui->buttonBmSearch, &QPushButton::clicked, this, &MainWindow::searchBreakMusic);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::actionAboutTriggered);
    connect(ui->pushButtonMplxBoth, &QPushButton::toggled, this, &MainWindow::pushButtonMplxBothToggled);
    connect(ui->pushButtonMplxLeft, &QPushButton::toggled, this, &MainWindow::pushButtonMplxLeftToggled);
    connect(ui->pushButtonMplxRight, &QPushButton::toggled, this, &MainWindow::pushButtonMplxRightToggled);
    connect(ui->lineEdit, &QLineEdit::textChanged, this, &MainWindow::lineEditSearchTextChanged);
    connect(ui->spinBoxTempo, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::spinBoxTempoValueChanged);
    connect(ui->actionSongbook_Generator, &QAction::triggered, [&] () {
        auto dlgBookCreator = findChild<DlgBookCreator *>(QString(), Qt::FindDirectChildrenOnly);
        if (dlgBookCreator == nullptr) {
            dlgBookCreator = new DlgBookCreator(this);
            connect(dlgBookCreator, &DlgBookCreator::finished, dlgBookCreator, &DlgBookCreator::deleteLater);
            dlgBookCreator->show();
        }
        else
            dlgBookCreator->raise();
    });
    connect(ui->actionEqualizer, &QAction::triggered, [&] () {
        auto dlgEq = findChild<DlgEq *>(QString(), Qt::FindDirectChildrenOnly);
        if (dlgEq == nullptr) {
            dlgEq = new DlgEq(this);
            connect(dlgEq, &DlgEq::finished, dlgEq, &DlgEq::deleteLater);
            connect(dlgEq, &DlgEq::bmEqBypassChanged, &m_mediaBackendBm, &MediaBackend::setEqBypass);
            connect(dlgEq, &DlgEq::karEqBypassChanged, &m_mediaBackendKar, &MediaBackend::setEqBypass);
            connect(dlgEq, &DlgEq::bmEqLevelChanged, &m_mediaBackendBm, &MediaBackend::setEqLevel);
            connect(dlgEq, &DlgEq::karEqLevelChanged, &m_mediaBackendKar, &MediaBackend::setEqLevel);
            dlgEq->show();
        } else
            dlgEq->raise();
    });
    connect(ui->pushButtonIncomingRequests, &QPushButton::clicked, requestsDialog.get(), &DlgRequests::show);
    connect(ui->pushButtonShop, &QPushButton::clicked, dlgSongShop.get(), &DlgSongShop::show);
    connect(ui->actionSong_Shop, &QAction::triggered, dlgSongShop.get(), &DlgSongShop::show);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::tabWidgetCurrentChanged);
    connect(ui->sliderBmPosition, &QSlider::sliderPressed, this, &MainWindow::sliderBmPositionPressed);
    connect(ui->sliderBmPosition, &QSlider::sliderReleased, this, &MainWindow::sliderBmPositionReleased);
    connect(ui->btnAddSfx, &QPushButton::clicked, this, &MainWindow::addSfxButtonPressed);
    connect(ui->btnSfxStop, &QPushButton::clicked, this, &MainWindow::stopSfxPlayback);
    connect(ui->lineEditBmSearch, &QLineEdit::textChanged, this, &MainWindow::lineEditBmSearchChanged);
    connect(ui->btnRotUp, &QPushButton::clicked, this, &MainWindow::btnRotUpClicked);
    connect(ui->btnRotTop, &QPushButton::clicked, this, &MainWindow::btnRotTopClicked);
    connect(ui->btnRotDown, &QPushButton::clicked, this, &MainWindow::btnRotDownClicked);
    connect(ui->btnRotBottom, &QPushButton::clicked, this, &MainWindow::btnRotBottomClicked);
    connect(ui->btnQUp, &QPushButton::clicked, this, &MainWindow::btnQUpClicked);
    connect(ui->btnQTop, &QPushButton::clicked, this, &MainWindow::btnQTopClicked);
    connect(ui->btnQDown, &QPushButton::clicked, this, &MainWindow::btnQDownClicked);
    connect(ui->btnQBottom, &QPushButton::clicked, this, &MainWindow::btnQBottomClicked);
    connect(ui->btnPlUp, &QPushButton::clicked, this, &MainWindow::btnPlUpClicked);
    connect(ui->btnPlTop, &QPushButton::clicked, this, &MainWindow::btnPlTopClicked);
    connect(ui->btnPlDown, &QPushButton::clicked, this, &MainWindow::btnPlDownClicked);
    connect(ui->btnPlBottom, &QPushButton::clicked, this, &MainWindow::btnPlBottomClicked);
    connect(ui->btnBmPlRandomize, &QPushButton::clicked, this, &MainWindow::btnBmPlRandomizeClicked);
    connect(ui->actionSound_Clips, &QAction::triggered, this, &MainWindow::actionSoundClipsTriggered);
    connect(ui->actionNow_Playing, &QAction::triggered, this, &MainWindow::actionNowPlayingTriggered);
    connect(ui->actionVideoLarge, &QAction::triggered, this, &MainWindow::actionVideoLargeTriggered);
    connect(ui->actionVideoSmall, &QAction::triggered, this, &MainWindow::actionVideoSmallTriggered);
    connect(ui->actionVideoMedium, &QAction::triggered, this, &MainWindow::actionVideoMediumTriggered);
    connect(ui->actionVideo_Output_2, &QAction::toggled, ui->videoPreview, &VideoDisplay::setVisible);
    connect(ui->actionVideo_Output_2, &QAction::toggled, &m_settings, &Settings::setShowMainWindowVideo);
    connect(ui->actionKaraoke_torture, &QAction::triggered, this, &MainWindow::actionKaraokeTorture);
    connect(ui->actionK_B_torture, &QAction::triggered, this, &MainWindow::actionKAndBTorture);
    connect(ui->actionBurn_in, &QAction::triggered, this, &MainWindow::actionBurnIn);
    connect(ui->actionPreview_burn_in, &QAction::triggered, this, &MainWindow::actionPreviewBurnIn);
    connect(ui->actionMultiplex_Controls, &QAction::toggled, ui->widgetMplxControls, &QWidget::setVisible);
    connect(ui->actionMultiplex_Controls, &QAction::toggled, &m_settings, &Settings::setShowMplxControls);
    connect(ui->actionCDG_Decode_Torture, &QAction::triggered, this, &MainWindow::actionCdgDecodeTorture);
    connect(ui->actionWrite_Gstreamer_pipeline_dot_files, &QAction::triggered, this,
            &MainWindow::writeGstPipelineDiagramToDisk);
    connect(ui->comboBoxSearchType, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &MainWindow::comboBoxSearchTypeIndexChanged);
    connect(ui->actionDocumentation, &QAction::triggered, this, &MainWindow::actionDocumentation);
    connect(ui->btnToggleCdgWindow, &QPushButton::toggled, cdgWindow.get(), &DlgCdg::setVisible);
    connect(ui->tableViewBmPlaylist, &QTableView::customContextMenuRequested, this,
            &MainWindow::tableViewBmPlaylistContextMenu);
    connect(ui->pushButtonHistoryPlay, &QPushButton::clicked, this, &MainWindow::buttonHistoryPlayClicked);
    connect(ui->pushButtonHistoryToQueue, &QPushButton::clicked, this, &MainWindow::buttonHistoryToQueueClicked);
    connect(ui->tableViewHistory, &QTableView::doubleClicked, this, &MainWindow::tableViewHistoryDoubleClicked);
    connect(ui->tableViewHistory, &QTableView::customContextMenuRequested, this,
            &MainWindow::tableViewHistoryContextMenu);
    connect(ui->actionBreak_music_torture, &QAction::triggered, this, &MainWindow::actionBreakMusicTorture);
    connect(ui->actionBurn_in_EOS_Jump, &QAction::triggered, this, &MainWindow::actionBurnInEosJump);
    connect(ui->sliderVolume, &QSlider::valueChanged, this, &MainWindow::sliderVolumeChanged);
    connect(ui->sliderBmVolume, &QSlider::valueChanged, this, &MainWindow::sliderBmVolumeChanged);
    connect(ui->tabWidgetQueue, &QTabWidget::currentChanged, [&] (auto current) {
        if (current == 1) {
            m_settings.saveColumnWidths(ui->tableViewQueue);
            if (!m_settings.restoreColumnWidths(ui->tableViewHistory))
                autosizeHistoryCols();
        } else {
            m_settings.saveColumnWidths(ui->tableViewHistory);
            if (!m_settings.restoreColumnWidths(ui->tableViewQueue))
                autosizeQueueCols();
        }
    });
}

void MainWindow::tableViewRotationSelChanged() {
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
}

void MainWindow::tableViewPlaylistSelectionChanged() {
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
}

void MainWindow::tableViewQueueSelChanged() {
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
}

void MainWindow::dbInit(const QDir &okjDataDir) {
    m_database = QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));
    m_database.setDatabaseName(okjDataDir.absolutePath() + QDir::separator() + "openkj.sqlite");
    m_database.open();
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
    m_logger->info("{} Database schema version: {}", m_loggingPrefix, schemaVersion);

    if (schemaVersion < 100) {
        m_logger->info("{} Updating database schema to version 101", m_loggingPrefix);
        query.exec("ALTER TABLE sourceDirs ADD COLUMN custompattern INTEGER");
        query.exec("PRAGMA user_version = 100");
        m_logger->info("{} DB Schema update to v100 completed", m_loggingPrefix);
    }
    if (schemaVersion < 101) {
        m_logger->info("{} Updating database schema to version 101", m_loggingPrefix);
        query.exec(
                "CREATE TABLE custompatterns ( patternid INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, artistregex TEXT, artistcapturegrp INT, titleregex TEXT, titlecapturegrp INT, discidregex TEXT, discidcapturegrp INT)");
        query.exec("PRAGMA user_version = 101");
        m_logger->info("{} DB Schema update to v101 completed", m_loggingPrefix);
    }
    if (schemaVersion < 102) {
        m_logger->info("{} Updating database schema to version 102", m_loggingPrefix);
        query.exec("CREATE UNIQUE INDEX idx_path ON dbsongs(path)");
        query.exec("PRAGMA user_version = 102");
        m_logger->info("{} DB Schema update to v102 completed", m_loggingPrefix);
    }
    if (schemaVersion < 103) {
        m_logger->info("{} Updating database schema to version 103", m_loggingPrefix);
        query.exec("ALTER TABLE dbsongs ADD COLUMN searchstring TEXT");
        query.exec("UPDATE dbsongs SET searchstring = filename || ' ' || artist || ' ' || title || ' ' || discid");
        query.exec("PRAGMA user_version = 103");
        m_logger->info("{} DB Schema update to v103 completed", m_loggingPrefix);

    }
    if (schemaVersion < 105) {
        m_logger->info("{} Updating database schema to version 105", m_loggingPrefix);
        query.exec("ALTER TABLE rotationSingers ADD COLUMN addts TIMESTAMP");
        query.exec("PRAGMA user_version = 105");
        m_logger->info("{} DB Schema update to v105 completed", m_loggingPrefix);
    }
    if (schemaVersion < 106) {
        m_logger->info("{} Updating database schema to version 106", m_loggingPrefix);
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
        m_logger->info("{} DB Schema update to v106 completed", m_loggingPrefix);
        m_logger->info("{} Importing old regular singers data into singer history", m_loggingPrefix);
        QSqlQuery songImportQuery;
        songImportQuery.prepare(
                "INSERT INTO historySongs (historySinger, filepath, artist, title, songid, keychange) values(:historySinger, :filepath, :artist, :title, :songid, :keychange)");
        QSqlQuery singersQuery;
        singersQuery.exec("SELECT regsingerid,name FROM regularSingers");
        while (singersQuery.next()) {
            m_logger->info("{} Running import for singer: {}", m_loggingPrefix,
                           singersQuery.value("name").toString().toStdString());
            QSqlQuery songsQuery;
            songsQuery.exec(
                    "SELECT dbsongs.artist,dbsongs.title,dbsongs.discid,regularsongs.keychg,dbsongs.path FROM regularsongs,dbsongs WHERE dbsongs.songid == regularsongs.songid AND regularsongs.regsingerid == " +
                    singersQuery.value("regsingerid").toString() + " ORDER BY regularsongs.position");
            while (songsQuery.next()) {
                m_logger->info("{} Importing song: {}", m_loggingPrefix, songsQuery.value(4).toString().toStdString());
                m_historySongsModel.saveSong(
                        singersQuery.value("name").toString(),
                        songsQuery.value(4).toString(),
                        songsQuery.value(0).toString(),
                        songsQuery.value(1).toString(),
                        songsQuery.value(2).toString(),
                        songsQuery.value(3).toInt()
                );
            }
            m_logger->info("{} Import complete for singer: {}", m_loggingPrefix,
                           singersQuery.value("name").toString().toStdString());
        }
    }
}


void MainWindow::play(const QString &karaokeFilePath, const bool &k2k) {
    m_mediaTempDir = std::make_unique<QTemporaryDir>();
    if (m_mediaBackendKar.state() != MediaBackend::PausedState) {
        m_logger->info("{} Playing file: {}", m_loggingPrefix, karaokeFilePath.toStdString());
        if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
            if (m_settings.karaokeAutoAdvance()) {
                m_kAASkip = true;
                cdgWindow->showAlert(false);
            }
            m_mediaBackendKar.stop();
            if (m_k2kTransition && m_settings.rotationAltSortOrder())
                m_rotModel.singerMove(0, static_cast<int>(m_rotModel.singerCount() - 1));
            ui->spinBoxTempo->setValue(100);
        }
        if (karaokeFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
            MzArchive archive(karaokeFilePath);
            if ((archive.checkCDG()) && (archive.checkAudio())) {
                if (archive.checkAudio()) {
                    if (!archive.extractAudio(m_mediaTempDir->path(), "tmp" + archive.audioExtension())) {
                        m_timerTest.stop();
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract audio file."),
                                             QMessageBox::Ok);
                        return;
                    }
                    if (!archive.extractCdg(m_mediaTempDir->path(), "tmp.cdg")) {
                        m_timerTest.stop();
                        QMessageBox::warning(this, tr("Bad karaoke file"), tr("Failed to extract CDG file."),
                                             QMessageBox::Ok);
                        return;
                    }
                    QString audioFile = m_mediaTempDir->path() + QDir::separator() + "tmp" + archive.audioExtension();
                    QString cdgFile = m_mediaTempDir->path() + QDir::separator() + "tmp.cdg";
                    m_logger->info("{} Extracted audio file size: {}", m_loggingPrefix, QFileInfo(audioFile).size());
                    m_logger->info("{} Setting karaoke backend source file to: {}", m_loggingPrefix,
                                   audioFile.toStdString());
                    m_mediaBackendKar.setMediaCdg(cdgFile, audioFile);
                    if (!k2k)
                        m_mediaBackendBm.fadeOut(!m_settings.bmKCrossFade());
                    m_logger->info("{} Beginning playback of file: {}", m_loggingPrefix, audioFile.toStdString());
                    QApplication::setOverrideCursor(Qt::WaitCursor);
                    m_mediaBackendKar.play();
                    QApplication::restoreOverrideCursor();
                    m_mediaBackendKar.fadeInImmediate();
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
            QString audioFilename = findMatchingAudioFile(karaokeFilePath);
            if (audioFilename == "") {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file missing."), QMessageBox::Ok);
                return;
            }
            QFile audioFile(audioFilename);
            if (audioFile.size() == 0) {
                m_timerTest.stop();
                QMessageBox::warning(this, tr("Bad karaoke file"), tr("Audio file contains no data"), QMessageBox::Ok);
                return;
            }
            cdgFile.copy(m_mediaTempDir->path() + QDir::separator() + cdgTmpFile);
            QFile::copy(audioFilename, m_mediaTempDir->path() + QDir::separator() + audTmpFile);
            m_mediaBackendKar.setMediaCdg(m_mediaTempDir->path() + QDir::separator() + cdgTmpFile,
                                          m_mediaTempDir->path() + QDir::separator() + audTmpFile);
            if (!k2k)
                m_mediaBackendBm.fadeOut(!m_settings.bmKCrossFade());
            QApplication::setOverrideCursor(Qt::WaitCursor);
            m_mediaBackendKar.play();
            QApplication::restoreOverrideCursor();
            m_mediaBackendKar.fadeInImmediate();
        } else {
            // Close CDG if open to avoid double video playback
            m_logger->info("{} Playing non-CDG video file: {}", m_loggingPrefix, karaokeFilePath.toStdString());
            QString tmpFileName = m_mediaTempDir->path() + QDir::separator() + "tmpvid." + karaokeFilePath.right(4);
            QFile::copy(karaokeFilePath, tmpFileName);
            m_logger->info("{} Playing temporary copy to avoid bad filename stuff w/ gstreamer: {}", m_loggingPrefix,
                           tmpFileName.toStdString());
            m_mediaBackendKar.setMedia(tmpFileName);
            if (!k2k)
                m_mediaBackendBm.fadeOut();
            m_mediaBackendKar.play();
            m_mediaBackendKar.fadeInImmediate();
        }
        m_mediaBackendKar.setTempo(ui->spinBoxTempo->value());
        if (m_settings.recordingEnabled()) {
            m_logger->info("{} Starting recording", m_loggingPrefix);
            QString timeStamp = QDateTime::currentDateTime().toString("yyyy-MM-dd-hhmm");
            audioRecorder.record(m_curSinger + " - " + m_curArtist + " - " + m_curTitle + " - " + timeStamp);
        }


    } else if (m_mediaBackendKar.state() == MediaBackend::PausedState) {
        if (m_settings.recordingEnabled())
            audioRecorder.unpause();
        m_mediaBackendKar.play();
        m_mediaBackendKar.fadeIn(false);
    }
    m_k2kTransition = false;
    if (m_settings.karaokeAutoAdvance())
        m_kAASkip = false;
}

MainWindow::~MainWindow() {
    m_shuttingDown = true;
    cdgWindow->stopTicker();
#ifdef _MSC_VER
    timeEndPeriod(1);
#endif
    m_lazyDurationUpdater->stopWork();
    m_settings.bmSetVolume(ui->sliderBmVolume->value());
    m_settings.setAudioVolume(ui->sliderVolume->value());
    m_logger->info("{} Saving volumes - K: {} BM {}", m_loggingPrefix, m_settings.audioVolume(), m_settings.bmVolume());
    m_logger->info("{} Saving window and widget sizing and positioning info", m_loggingPrefix);
    m_settings.saveSplitterState(ui->splitter);
    m_settings.saveSplitterState(ui->splitter_2);
    m_settings.saveSplitterState(ui->splitter_3);
    m_settings.saveColumnWidths(ui->tableViewDB);
    m_settings.saveColumnWidths(ui->tableViewRotation);
    m_settings.saveWindowState(requestsDialog.get());
    m_settings.saveWindowState(dlgSongShop.get());
    m_settings.saveWindowState(dbDialog.get());
    m_settings.saveWindowState(this);
    m_settings.saveSplitterState(ui->splitterBm);
    m_settings.saveColumnWidths(ui->tableViewBmDb);
    m_settings.saveColumnWidths(ui->tableViewBmPlaylist);
    m_settings.bmSetPlaylistIndex(ui->comboBoxBmPlaylists->currentIndex());
    m_settings.sync();
    m_logger->info("{} Program shutdown complete", m_loggingPrefix);
}

void MainWindow::search() {
    ui->tableViewDB->scrollToTop();
    m_karaokeSongsModel.search(ui->lineEdit->text());
}

void MainWindow::databaseUpdated() {
    m_karaokeSongsModel.loadData();
    search();
    autosizeViews();
    m_settings.restoreColumnWidths(ui->tableViewDB);
    requestsDialog->databaseUpdateComplete();
    m_lazyDurationUpdater->stopWork();
    m_lazyDurationUpdater->deleteLater();
    m_lazyDurationUpdater = std::make_unique<LazyDurationUpdateController>(this);
    connect(m_lazyDurationUpdater.get(), &LazyDurationUpdateController::gotDuration, &m_karaokeSongsModel,
            &TableModelKaraokeSongs::setSongDuration);
    m_lazyDurationUpdater->getDurations();
}

void MainWindow::databaseCleared() {
    m_lazyDurationUpdater->stopWork();
    m_karaokeSongsModel.loadData();
    m_rotModel.loadData();
    m_qModel.loadSinger(-1);
    ui->tableViewQueue->reset();
    autosizeViews();
    rotationDataChanged();


}

void MainWindow::buttonStopClicked() {
    if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
        if (m_settings.showSongPauseStopWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show warning on pause/stop in the future");
            cb->setChecked(m_settings.showSongPauseStopWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Stop currently playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSongPauseStopWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
    }
    m_kAASkip = true;
    cdgWindow->showAlert(false);
    audioRecorder.stop();
    if (m_settings.bmKCrossFade()) {
        m_mediaBackendBm.fadeIn(false);
        m_mediaBackendKar.stop();
    } else {
        m_mediaBackendKar.stop();
        m_mediaBackendBm.fadeIn();
    }
}

void MainWindow::buttonPauseClicked() {
    if (m_mediaBackendKar.state() == MediaBackend::PausedState) {
        m_mediaBackendKar.play();
    } else if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
        if (m_settings.showSongPauseStopWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show warning on pause/stop in the future");
            cb->setChecked(m_settings.showSongPauseStopWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Pause currently playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be paused.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSongPauseStopWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        m_mediaBackendKar.pause();
    }
}

void MainWindow::tableViewDbDoubleClicked(const QModelIndex &index) {
    if (!index.isValid())
        return;
    auto song = qvariant_cast<std::shared_ptr<okj::KaraokeSong>>(index.data(Qt::UserRole));
    if (m_settings.dbDoubleClickAddsSong()) {
        auto addSongDlg = new DlgAddSong(m_rotModel, m_qModel, song->id, this);
        connect(addSongDlg, &DlgAddSong::newSingerAdded, [&](auto pos) {
            ui->tableViewRotation->selectRow(pos);
            ui->lineEdit->setFocus();
        });
        addSongDlg->setModal(true);
        addSongDlg->show();
        return;
    }
    if (m_qModel.getSingerId() >= 0 && ui->tableViewRotation->selectionModel()->hasSelection()) {
        m_qModel.add(song->id);
        updateRotationDuration();
    } else {
        QMessageBox msgBox;
        msgBox.setText("No singer selected.  You must select a singer before you can double-click to add to a queue.");
        msgBox.exec();
    }
}

void MainWindow::tableViewRotationDoubleClicked(const QModelIndex &index) {
    if (index.column() <= 3) {
        m_k2kTransition = false;
        int singerId = index.data(Qt::UserRole).toInt();
        QString nextSongPath = m_rotModel.getSinger(singerId).nextSongPath();
        if (nextSongPath != "") {
            if ((m_mediaBackendKar.state() == MediaBackend::PlayingState) && (m_settings.showSongInterruptionWarning())) {
                QMessageBox msgBox(this);
                auto *cb = new QCheckBox("Show this warning in the future");
                cb->setChecked(m_settings.showSongInterruptionWarning());
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Interrupt currently playing karaoke song?");
                msgBox.setInformativeText(
                        "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
                QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.setCheckBox(cb);
                connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSongInterruptionWarning);
                msgBox.exec();
                if (msgBox.clickedButton() != yesButton) {
                    return;
                }
                m_k2kTransition = true;
            }
            if (m_mediaBackendKar.state() == MediaBackend::PausedState) {
                if (m_settings.karaokeAutoAdvance()) {
                    m_kAASkip = true;
                    cdgWindow->showAlert(false);
                }
                audioRecorder.stop();
                m_mediaBackendKar.stop(true);
            }
            //           play(nextSongPath);
            //           kAudioBackend.setPitchShift(m_rotModel.nextSongKeyChg(singerId));
            auto &singer = m_rotModel.getSinger(singerId);
            m_curSinger = singer.name;
            m_curArtist = singer.nextSongArtist();
            m_curTitle = singer.nextSongTitle();
            QString curSongId = singer.nextSongSongId();
            int curKeyChange = singer.nextSongKeyChg();

            m_karaokeSongsModel.updateSongHistory(m_karaokeSongsModel.getIdForPath(nextSongPath));
            play(nextSongPath, m_k2kTransition);
            ui->labelArtist->setText(m_curArtist);
            ui->labelTitle->setText(m_curTitle);
            ui->labelSinger->setText(m_curSinger);
            if (m_settings.treatAllSingersAsRegs() || m_rotModel.getSinger(singerId).regular)
                m_historySongsModel.saveSong(m_curSinger, nextSongPath, m_curArtist, m_curTitle, curSongId,
                                             curKeyChange);
            m_mediaBackendKar.setPitchShift(curKeyChange);
            m_qModel.setPlayed(singer.nextSongQueueId());
            m_rotDelegate.setCurrentSinger(singerId);
            m_rotModel.setCurrentSinger(singerId);
            if (m_settings.rotationAltSortOrder()) {
                auto curSingerPos = m_rotModel.getSinger(singerId).position;
                m_curSingerOriginalPosition = curSingerPos;
                if (curSingerPos != 0) {
                    m_rotModel.singerMove(curSingerPos, 0);
                    ui->tableViewRotation->clearSelection();
                    ui->tableViewRotation->selectRow(0);
                }
            }
        }
    }
}

void MainWindow::tableViewRotationClicked(const QModelIndex &index) {
    if (index.column() == TableModelRotation::COL_DELETE) {
        if (m_settings.showSingerRemovalWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(m_settings.showSingerRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Are you sure you want to remove this singer?");
            msgBox.setInformativeText(
                    "Unless this singer is a tracked regular, you will be unable retrieve any queue data for this singer once they are deleted.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSingerRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        int singerId = index.data(Qt::UserRole).toInt();
        m_logger->info("{} Singer id selected: {}", m_loggingPrefix, singerId);
        m_qModel.loadSinger(-1);
        if (m_rotModel.currentSinger() == singerId) {
            m_rotModel.setCurrentSinger(-1);
            m_rotDelegate.setCurrentSinger(-1);
        }
        m_rotModel.singerDelete(singerId);
        ui->tableViewRotation->clearSelection();
        ui->tableViewQueue->clearSelection();
        return;

    }
    if (index.column() == TableModelRotation::COL_REGULAR) {
        if (!m_rotModel.getSinger(index.data(Qt::UserRole).toInt()).regular) {
            QString name = index.sibling(index.row(), TableModelRotation::COL_NAME).data().toString();
            if (m_rotModel.historySingerExists(name)) {
                auto answer = QMessageBox::question(this,
                                                    "A regular singer with this name already exists!",
                                                    "There is already a regular singer saved with this name. Would you like to load "
                                                    "the matching regular singer's history for this singer?",
                                                    QMessageBox::StandardButtons(
                                                            QMessageBox::Yes | QMessageBox::Cancel),
                                                    QMessageBox::Cancel
                );
                if (answer == QMessageBox::Yes)
                    m_rotModel.singerMakeRegular(m_rotModel.getSingerByName(name).id);
            } else
                m_rotModel.singerMakeRegular(index.data(Qt::UserRole).toInt());
        } else {
            QMessageBox msgBox(this);
            msgBox.setText("Are you sure you want to disable regular tracking for this singer?");
            msgBox.setInformativeText(
                    "Doing so will not remove the regular singer entry, but it will prevent any changes made to the singer's queue from being saved to the regular singer until the regular singer is either reloaded or the rotation singer is re-merged with the regular singer.");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton) {
                m_rotModel.singerDisableRegularTracking(index.data(Qt::UserRole).toInt());
            }
        }
    }
}

void MainWindow::tableViewQueueDoubleClicked(const QModelIndex &index) {
    if (!index.isValid())
        return;
    auto song = qvariant_cast<okj::QueueSong>(index.data(Qt::UserRole));
    auto singer = m_rotModel.getSinger(song.singerId);
    m_k2kTransition = false;
    if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
        if (m_settings.showSongInterruptionWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(m_settings.showSongInterruptionWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Interrupt currently playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSongInterruptionWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        m_k2kTransition = true;
        m_mediaBackendKar.stop();
        while (m_mediaBackendKar.state() == MediaBackend::PlayingState)
            QApplication::processEvents();
        if (m_settings.rotationAltSortOrder())
            m_rotModel.singerMove(0, static_cast<int>(m_rotModel.singerCount() -1), true);
    }
    else if (m_mediaBackendKar.state() == MediaBackend::PausedState) {
        if (m_settings.karaokeAutoAdvance()) {
            m_kAASkip = true;
            cdgWindow->showAlert(false);
        }
        audioRecorder.stop();
        m_mediaBackendKar.stop(true);
    }
    m_curSinger = singer.name;
    m_curArtist = song.artist;
    m_curTitle = song.title;
    ui->labelSinger->setText(singer.name);
    ui->labelArtist->setText(song.artist);
    ui->labelTitle->setText(song.title);
    m_karaokeSongsModel.updateSongHistory(song.dbSongId);
    play(song.path, m_k2kTransition);
    if (m_settings.treatAllSingersAsRegs() || singer.regular)
        m_historySongsModel.saveSong(singer.name, song.path, song.artist, song.title, song.songId, song.keyChange);
    m_mediaBackendKar.setPitchShift(song.keyChange);
    m_qModel.setPlayed(song.id);
    m_rotModel.setCurrentSinger(singer.id);
    m_rotDelegate.setCurrentSinger(singer.id);
    if (m_settings.rotationAltSortOrder()) {
        // Need a new copy of the singer since the rotation has changed since we last grabbed them
        singer = m_rotModel.getSinger(singer.id);
        m_curSingerOriginalPosition = singer.position;
        if (singer.position != 0) {
            m_rotModel.singerMove(singer.position, 0);
            ui->tableViewRotation->clearSelection();
            ui->tableViewRotation->selectRow(0);
        }
    }
}

void MainWindow::actionExportRegularsTriggered() {
    auto exportDlg = new DlgRegularExport(m_karaokeSongsModel, this);
    exportDlg->setModal(true);
    exportDlg->show();
}

void MainWindow::actionImportRegularsTriggered() {
    auto iDialog = new DlgRegularImport(m_karaokeSongsModel, this);
    iDialog->setModal(true);
    iDialog->show();
}

void MainWindow::actionSettingsTriggered() {
    auto settingsDialog = new DlgSettings(m_mediaBackendKar, m_mediaBackendBm, m_songbookApi, this);
    settingsDialog->setModal(true);

    connect(settingsDialog, &DlgSettings::videoOffsetChanged, [&](auto offsetMs) {
        m_mediaBackendKar.setVideoOffset(offsetMs);
        m_mediaBackendBm.setVideoOffset(offsetMs);
    });
    connect(settingsDialog, &DlgSettings::requestServerEnableChanged, ui->pushButtonIncomingRequests,
            &QPushButton::setVisible);
    connect(settingsDialog, &DlgSettings::rotationShowNextSongChanged, [&]() { autosizeRotationCols(); });
    connect(settingsDialog, &DlgSettings::rotationDurationSettingsModified, this, &MainWindow::updateRotationDuration);
    connect(settingsDialog, &DlgSettings::requestServerIntervalChanged, &m_songbookApi, &OKJSongbookAPI::setInterval);
    connect(settingsDialog, &DlgSettings::shortcutsChanged, this, &MainWindow::shortcutsUpdated);
    connect(settingsDialog, &DlgSettings::treatAllSingersAsRegsChanged, this, &MainWindow::treatAllSingersAsRegsChanged);
    connect(settingsDialog, &DlgSettings::enforceAspectRatioChanged, &m_mediaBackendKar, &MediaBackend::setEnforceAspectRatio);
    connect(settingsDialog, &DlgSettings::enforceAspectRatioChanged, &m_mediaBackendBm, &MediaBackend::setEnforceAspectRatio);
    connect(settingsDialog, &DlgSettings::karaokeAutoAdvanceChanged, ui->actionAutoplay_mode, &QAction::setChecked);
    connect(settingsDialog, &DlgSettings::audioUseFaderChanged, &m_mediaBackendKar, &MediaBackend::setUseFader);
    connect(settingsDialog, &DlgSettings::audioSilenceDetectChanged, &m_mediaBackendKar,
            &MediaBackend::setUseSilenceDetection);
    connect(settingsDialog, &DlgSettings::audioUseFaderChangedBm, &m_mediaBackendBm, &MediaBackend::setUseFader);
    connect(settingsDialog, &DlgSettings::audioSilenceDetectChangedBm, &m_mediaBackendBm,
            &MediaBackend::setUseSilenceDetection);
    connect(settingsDialog, &DlgSettings::audioDownmixChanged, &m_mediaBackendKar, &MediaBackend::setDownmix);
    connect(settingsDialog, &DlgSettings::audioDownmixChangedBm, &m_mediaBackendBm, &MediaBackend::setDownmix);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, &m_itemDelegatePlSongs, &ItemDelegatePlaylistSongs::resizeIconsForFont);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, &m_qDelegate, &ItemDelegateQueueSongs::resizeIconsForFont);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, &m_rotDelegate, &ItemDelegateRotation::resizeIconsForFont);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, &m_karaokeSongsModel, &TableModelKaraokeSongs::resizeIconsForFont);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, &m_qModel, &TableModelQueueSongs::setFont);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, &m_historySongsModel, &TableModelHistorySongs::setFont);
    connect(settingsDialog, &DlgSettings::applicationFontChanged, this, &MainWindow::appFontChanged);
    connect(settingsDialog, &DlgSettings::alertBgColorChanged, cdgWindow.get(), &DlgCdg::alertBgColorChanged);
    connect(settingsDialog, &DlgSettings::alertTxtColorChanged, cdgWindow.get(), &DlgCdg::alertTxtColorChanged);
    connect(settingsDialog, &DlgSettings::bgModeChanged, cdgWindow.get(), &DlgCdg::applyBackgroundImageMode);
    connect(settingsDialog, &DlgSettings::bgSlideShowDirChanged, cdgWindow.get(), &DlgCdg::applyBackgroundImageMode);
    connect(settingsDialog, &DlgSettings::cdgBgImageChanged, cdgWindow.get(), &DlgCdg::applyBackgroundImageMode);
    connect(settingsDialog, &DlgSettings::cdgOffsetsChanged, cdgWindow.get(), &DlgCdg::cdgOffsetsChanged);
    connect(settingsDialog, &DlgSettings::cdgRemainBgColorChanged, cdgWindow->durationWidget(), &TransparentWidget::setBackgroundColor);
    connect(settingsDialog, &DlgSettings::cdgRemainEnabledChanged, cdgWindow->durationWidget(), &TransparentWidget::setVisible);
    connect(settingsDialog, &DlgSettings::cdgRemainFontChanged, cdgWindow->durationWidget(), &TransparentWidget::setTextFont);
    connect(settingsDialog, &DlgSettings::cdgRemainTextColorChanged, cdgWindow->durationWidget(), &TransparentWidget::setTextColor);
    connect(settingsDialog, &DlgSettings::durationPositionReset, cdgWindow->durationWidget(), &TransparentWidget::resetPosition);
    connect(settingsDialog, &DlgSettings::karaokeAAAlertFontChanged, cdgWindow.get(), &DlgCdg::alertFontChanged);
    connect(settingsDialog, &DlgSettings::slideShowIntervalChanged, cdgWindow.get(), &DlgCdg::setSlideshowInterval);
    connect(settingsDialog, &DlgSettings::tickerBgColorChanged, cdgWindow.get(), &DlgCdg::tickerBgColorChanged);
    connect(settingsDialog, &DlgSettings::tickerEnableChanged, cdgWindow.get(), &DlgCdg::tickerEnableChanged);
    connect(settingsDialog, &DlgSettings::tickerEnableChanged, this, &MainWindow::rotationDataChanged);
    connect(settingsDialog, &DlgSettings::tickerFontChanged, cdgWindow.get(), &DlgCdg::tickerFontChanged);
    connect(settingsDialog, &DlgSettings::tickerSpeedChanged, cdgWindow.get(), &DlgCdg::tickerSpeedChanged);
    connect(settingsDialog, &DlgSettings::tickerTextColorChanged, cdgWindow.get(), &DlgCdg::tickerTextColorChanged);
    connect(settingsDialog, &DlgSettings::tickerOutputModeChanged, this, &MainWindow::rotationDataChanged);
    connect(settingsDialog, &DlgSettings::tickerCustomStringChanged, this, &MainWindow::rotationDataChanged);
    connect(settingsDialog, &DlgSettings::shortcutsChanged, this, &MainWindow::shortcutsUpdated);

    settingsDialog->show();
}

void MainWindow::songDroppedOnSinger(const int &singerId, const int &songId, const int &dropRow) {
    m_qModel.loadSinger(singerId);
    m_qModel.add(songId);
    ui->tableViewRotation->clearSelection();
    auto selectionModel = ui->tableViewRotation->selectionModel();
    QModelIndex topLeft;
    QModelIndex bottomRight;
    topLeft = m_rotModel.index(dropRow, 0, QModelIndex());
    bottomRight = m_rotModel.index(dropRow, 4, QModelIndex());
    QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, QItemSelectionModel::Select);
}

void MainWindow::tableViewQueueClicked(const QModelIndex &index) {
    if (index.column() == TableModelQueueSongs::COL_PATH) {
        if ((m_settings.showQueueRemovalWarning()) &&
            (!m_qModel.getPlayed(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt()))) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(m_settings.showQueueRemovalWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Removing un-played song from queue");
            msgBox.setInformativeText("This song has not been played yet, are you sure you want to remove it?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowQueueRemovalWarning);
            msgBox.exec();
            if (msgBox.clickedButton() == yesButton) {
                m_qModel.remove(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt());
            }
        } else {
            m_qModel.remove(index.sibling(index.row(), TableModelQueueSongs::COL_ID).data().toInt());
        }
    }
}

void MainWindow::clearRotation() {
    if (m_testMode) {
        m_settings.setCurrentRotationPosition(-1);
        m_rotModel.clearRotation();
        m_rotDelegate.setCurrentSinger(-1);
        m_qModel.loadSinger(-1);
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
        m_settings.setCurrentRotationPosition(-1);
        m_rotModel.clearRotation();
        m_rotDelegate.setCurrentSinger(-1);
        m_qModel.loadSinger(-1);
    }
}

void MainWindow::clearSingerQueue() {
    if (m_testMode) {
        m_qModel.removeAll();
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
        m_qModel.removeAll();
    }
}

void MainWindow::spinBoxKeyValueChanged(const int &arg1) {
    m_mediaBackendKar.setPitchShift(arg1);
    if (arg1 > 0)
        ui->spinBoxKey->setPrefix("+");
    else
        ui->spinBoxKey->setPrefix("");
    QTimer::singleShot(20, [&]() {
        ui->spinBoxKey->findChild<QLineEdit *>()->deselect();
    });
}

void MainWindow::karaokeMediaBackend_positionChanged(const qint64 &position) {
    if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
        if (!m_sliderPositionPressed) {
            ui->sliderProgress->setMaximum((int) m_mediaBackendKar.duration());
            ui->sliderProgress->setValue((int) position);
        }
        ui->labelElapsedTime->setText(MediaBackend::msToMMSS(position));
        ui->labelRemainTime->setText(MediaBackend::msToMMSS(m_mediaBackendKar.duration() - position));
        m_rotModel.setCurRemainSecs((int) (m_mediaBackendKar.duration() - position) / 1000);
    }
}

void MainWindow::karaokeMediaBackend_durationChanged(const qint64 &duration) {
    ui->labelTotalTime->setText(MediaBackend::msToMMSS(duration));
}

void MainWindow::karaokeMediaBackend_stateChanged(const MediaBackend::State &state) {
    if (m_shuttingDown)
        return;
    if (state == MediaBackend::StoppedState) {
        m_logger->info("{} MainWindow - audio backend state is now STOPPED", m_loggingPrefix);
        if (ui->labelTotalTime->text() == "0:00") {
            return;
        }
        m_logger->info("{} KAudio entered StoppedState", m_loggingPrefix);
        audioRecorder.stop();
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
        if (state == m_lastAudioState || m_k2kTransition)
            return;
        m_lastAudioState = state;
        m_mediaBackendBm.fadeIn(false);
        if (m_settings.karaokeAutoAdvance()) {
            m_logger->info("{}  - Karaoke Autoplay is enabled", m_loggingPrefix);
            if (m_kAASkip) {
                m_kAASkip = false;
                m_logger->info("{}  - Karaoke Autoplay set to skip, bailing out", m_loggingPrefix);
            } else {
                okj::RotationSinger nextSinger;
                QString nextSongPath;
                bool empty = false;

                int curSingerId = m_rotModel.currentSinger();

                int curPos = m_rotModel.getSinger(curSingerId).position;
                if (m_settings.rotationAltSortOrder())
                    curPos = m_curSingerOriginalPosition;
                if (curSingerId == -1)
                    curPos = static_cast<int>(m_rotModel.singerCount() - 1);
                int loops = 0;
                while ((nextSongPath == "") && (!empty)) {
                    if (loops > m_rotModel.singerCount()) {
                        empty = true;
                    } else {
                        if (++curPos >= m_rotModel.singerCount()) {
                            curPos = 0;
                        }
                        nextSinger = m_rotModel.getSingerAtPosition(curPos);
                        nextSongPath = nextSinger.nextSongPath();
                        loops++;
                    }
                }
                if (empty)
                    m_logger->info("{} KaraokeAA - No more songs to play, giving up", m_loggingPrefix);
                else {
                    m_kAANextSinger = nextSinger.id;
                    m_kAANextSongPath = nextSongPath;
                    m_logger->info("{} KaraokeAA - Will play: {} - {}", m_loggingPrefix,
                                   nextSinger.name.toStdString(), nextSongPath.toStdString());
                    m_logger->info("{} KaraokeAA - Starting {} second timer", m_loggingPrefix,
                                   m_settings.karaokeAATimeout());
                    m_timerKaraokeAA.start(m_settings.karaokeAATimeout() * 1000);
                    cdgWindow->setNextSinger(nextSinger.name);
                    cdgWindow->setNextSong(nextSinger.nextSongArtistTitle());
                    cdgWindow->setCountdownSecs(m_settings.karaokeAATimeout());
                    cdgWindow->showAlert(true);
                }
            }
        }
        if (m_settings.rotationAltSortOrder()) {
            m_rotModel.singerMove(0, static_cast<int>(m_rotModel.singerCount() - 1));
            m_rotModel.setCurrentSinger(-1);
            m_rotDelegate.setCurrentSinger(-1);
            ui->tableViewRotation->clearSelection();
            ui->tableViewRotation->selectRow(0);
            m_rotModel.setCurRemainSecs(0);
        }
    }
    if (state == MediaBackend::EndOfMediaState) {
        m_logger->info("{} KAudio entered EndOfMediaState", m_loggingPrefix);
        audioRecorder.stop();
//        ipcClient->send_MessageToServer(KhIPCClient::CMD_FADE_IN);
        //m_mediaBackendBm.setVideoEnabled(true);
        m_mediaBackendKar.stop(true);
        m_mediaBackendBm.fadeIn(false);
    }
    if (state == MediaBackend::PausedState) {
        m_logger->info("{} KAudio entered PausedState", m_loggingPrefix);
        audioRecorder.pause();
    }
    if (state == MediaBackend::PlayingState) {
        m_logger->info("{} KAudio entered PlayingState", m_loggingPrefix);
        m_lastAudioState = state;
        //m_mediaBackendBm.setVideoEnabled(false);
        ui->pushButtonKeyUp->setEnabled(true);
        ui->pushButtonKeyDn->setEnabled(true);
        ui->pushButtonTempoDn->setEnabled(true);
        ui->pushButtonTempoUp->setEnabled(true);
    }
    if (state == MediaBackend::UnknownState) {
        m_logger->info("{} KAudio entered UnknownState", m_loggingPrefix);
    }
    rotationDataChanged();
}

void MainWindow::sfxAudioBackend_positionChanged(const qint64 &position) {
    ui->sliderSfxPos->setValue((int) position);
}

void MainWindow::sfxAudioBackend_durationChanged(const qint64 &duration) {
    ui->sliderSfxPos->setMaximum((int) duration);
}

void MainWindow::sfxAudioBackend_stateChanged(const MediaBackend::State &state) {
    if (state == MediaBackend::EndOfMediaState) {
        ui->sliderSfxPos->setValue(0);
        m_mediaBackendSfx.stop();
    }
    if (state == MediaBackend::StoppedState || state == MediaBackend::UnknownState)
        ui->sliderSfxPos->setValue(0);
}

void MainWindow::hasActiveVideoChanged() {
    ui->videoPreview->setHasActiveVideo(m_kHasActiveVideo);
    ui->videoPreviewBm->setHasActiveVideo(m_bmHasActiveVideo);
    cdgWindow->getVideoDisplay()->setHasActiveVideo(m_kHasActiveVideo);
    cdgWindow->getVideoDisplayBm()->setHasActiveVideo(m_bmHasActiveVideo);
    if (m_timerKaraokeAA.isActive() && m_settings.karaokeAAAlertEnabled())
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

void MainWindow::rotationDataChanged() {
    if (m_shuttingDown)
        return;
    m_logger->trace("{} [{}] Called", m_loggingPrefix, __func__);
    auto st = std::chrono::high_resolution_clock::now();
    if (m_settings.rotationShowNextSong())
        autosizeRotationCols();
    updateRotationDuration();
    QString sep = "•";
    requestsDialog->rotationChanged();
    QString statusBarText = "Singers: ";
    statusBarText += QString::number(m_rotModel.singerCount());
    m_labelSingerCount.setText(statusBarText);
    QString tickerText;
    if (m_settings.tickerCustomString() != "") {
        tickerText += m_settings.tickerCustomString() + " " + sep + " ";
        QString cs = m_rotModel.getSinger(m_rotModel.currentSinger()).name;
        int nsPos;
        if (cs == "") {
            cs = m_rotModel.getSingerAtPosition(0).name;
            if (cs == "")
                cs = "[nobody]";
            nsPos = 0;
        } else
            nsPos = m_rotModel.getSinger(m_rotModel.currentSinger()).position;
        QString ns = "[nobody]";
        if (m_rotModel.singerCount() > 0) {
            if (nsPos + 1 < m_rotModel.singerCount())
                nsPos++;
            else
                nsPos = 0;
            ns = m_rotModel.getSingerAtPosition(nsPos).name;
        }
        tickerText.replace("%cs", cs);
        tickerText.replace("%ns", ns);
        tickerText.replace("%rc", QString::number(m_rotModel.singerCount()));
        if (ui->labelArtist->text() == "None" && ui->labelTitle->text() == "None")
            tickerText.replace("%curSong", "None");
        else
            tickerText.replace("%curSong", ui->labelArtist->text() + " - " + ui->labelTitle->text());
        tickerText.replace("%m_curArtist", ui->labelArtist->text());
        tickerText.replace("%m_curTitle", ui->labelTitle->text());
        tickerText.replace("%m_curSinger", cs);
        tickerText.replace("%nextSinger", ns);

    }
    if (m_settings.tickerShowRotationInfo()) {
        tickerText += "Singers: ";
        tickerText += QString::number(m_rotModel.singerCount());
        tickerText += " " + sep + " Current: ";
        int displayPos;
        QString curSingerName = m_rotModel.getSinger(m_rotModel.currentSinger()).name;
        if (m_rotModel.currentSinger() < 0)
            curSingerName = "None";
        if (curSingerName != "") {
            tickerText += curSingerName;
            displayPos = m_rotModel.getSinger(m_rotModel.currentSinger()).position;
        } else {
            tickerText += "None ";
            displayPos = -1;
        }
        int listSize;
        if (m_settings.tickerFullRotation() || (m_rotModel.singerCount() < m_settings.tickerShowNumSingers())) {
            if (curSingerName == "")
                listSize = static_cast<int>(m_rotModel.singerCount());
            else
                listSize = static_cast<int>(m_rotModel.singerCount() - 1);
            if (listSize > 0)
                tickerText += " " + sep + " Upcoming: ";
        } else {
            listSize = m_settings.tickerShowNumSingers();
            tickerText += " " + sep + " Next ";
            tickerText += QString::number(m_settings.tickerShowNumSingers());
            tickerText += " Singers: ";
        }
        for (int i = 0; i < listSize; i++) {
            if (displayPos + 1 < m_rotModel.singerCount())
                displayPos++;
            else
                displayPos = 0;
            tickerText += QString::number(i + 1);
            tickerText += ") ";
            tickerText += m_rotModel.getSingerAtPosition(displayPos).name;
            if (i < listSize - 1)
                tickerText += " ";
        }
    }
    cdgWindow->setTickerText(tickerText);

    m_logger->trace("{} [{}] finished in {}ms",
                    m_loggingPrefix,
                    __func__,
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count()
    );
}

void MainWindow::silenceDetectedKar() {
    m_logger->info("{} Karaoke music silence detected", m_loggingPrefix);
    m_mediaBackendKar.rawStop();
    if (m_settings.karaokeAutoAdvance())
        m_kAASkip = false;
    m_mediaBackendBm.fadeIn();
}

void MainWindow::silenceDetectedBm() {
    if (m_mediaBackendBm.position() > 10000 && m_mediaBackendBm.position() < (m_mediaBackendBm.duration() - 3)) {
        m_logger->info("{} Break music silence detected, reporting EndOfMediaState to trigger playlist advance",
                       m_loggingPrefix);
        bmMediaStateChanged(MediaBackend::EndOfMediaState);
    }
}

void MainWindow::tableViewDBContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->tableViewDB->indexAt(pos);
    if (!index.isValid())
        return;
    auto song = qvariant_cast<std::shared_ptr<okj::KaraokeSong>>(index.data(Qt::UserRole));
    QMenu contextMenu(this);
    contextMenu.addAction("Preview", [&]() {
        previewKaraokeSong(song->path);
    });
    contextMenu.addSeparator();
    contextMenu.addAction("Edit", [&] () { editSong(song); });
    contextMenu.addAction("Mark bad", [&] () { markSongBad(song); });
    contextMenu.exec(QCursor::pos());
}

void MainWindow::tableViewRotationContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->tableViewRotation->indexAt(pos);
    if (index.isValid()) {
        m_rtClickRotationSingerId = index.data(Qt::UserRole).toInt();
        QMenu contextMenu(this);
        if (ui->tableViewRotation->selectionModel()->selectedRows().size() > 1) {
            contextMenu.addAction("Delete", &m_scutDeleteSinger, &QShortcut::activated);
        } else {
            contextMenu.addAction("Rename", this, &MainWindow::renameSinger);
            contextMenu.addAction("Set as top of rotation", [&]() {
                m_rotModel.setRotationTopSingerId(m_rtClickRotationSingerId);
            });
        }
        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::sfxButtonContextMenuRequested([[maybe_unused]]const QPoint &pos) {
    auto *btn = (SoundFxButton *) sender();
    m_lastRtClickedSfxBtn.path = btn->buttonData().toString();
    m_lastRtClickedSfxBtn.name = btn->text();
    QMenu contextMenu(this);
    contextMenu.addAction("Remove", this, &MainWindow::removeSfxButton);
    contextMenu.exec(QCursor::pos());
}

void MainWindow::renameSinger() {
    bool ok;
    QString currentName = m_rotModel.getSinger(m_rtClickRotationSingerId).name;
    QString name = QInputDialog::getText(this, "Rename singer", "New name:", QLineEdit::Normal, currentName, &ok);
    if (ok && !name.isEmpty()) {
        if ((name.toLower() == currentName.toLower() && name != currentName) || !m_rotModel.singerExists(name)) {
            m_rotModel.singerSetName(m_rtClickRotationSingerId, name);
        } else if (m_rotModel.singerExists(name)) {
            QMessageBox::warning(this, "Singer exists!", "A singer named " + name +
                                                         " already exists. Please choose a unique name and try again. The operation has been cancelled.",
                                 QMessageBox::Ok);
        }

    }
}

void MainWindow::tableViewBmPlaylistContextMenu([[maybe_unused]]const QPoint &pos) {
    QMenu contextMenu(this);
    contextMenu.addAction("Delete", &m_scutDeletePlSong, &QShortcut::activated);
    contextMenu.exec(QCursor::pos());
}

void MainWindow::tableViewQueueContextMenuRequested(const QPoint &pos) {
    int selCount = ui->tableViewQueue->selectionModel()->selectedRows().size();
    if (selCount == 1) {
        QModelIndex index = ui->tableViewQueue->indexAt(pos);
        if (index.isValid()) {
            m_dbRtClickFile = index.sibling(index.row(), TableModelQueueSongs::COL_PATH).data().toString();
            m_rtClickQueueSongId = index.sibling(index.row(), 0).data().toInt();
            dlgKeyChange->setActiveSong(m_rtClickQueueSongId);
            QMenu contextMenu(this);
            contextMenu.addAction("Preview", [&] () {
                previewKaraokeSong(index.sibling(index.row(), TableModelQueueSongs::COL_PATH).data().toString());
            });
            contextMenu.addSeparator();
            contextMenu.addAction("Set Key Change", this, &MainWindow::setKeyChange);
            contextMenu.addAction("Toggle played", this, &MainWindow::toggleQueuePlayed);
            contextMenu.addSeparator();
            contextMenu.addAction("Delete", &m_scutDeleteSong, &QShortcut::activated);
            contextMenu.exec(QCursor::pos());
        }
    } else if (selCount > 1) {
        QMenu contextMenu(this);
        contextMenu.addAction("Set Played", this, &MainWindow::setMultiPlayed);
        contextMenu.addAction("Set Unplayed", this, &MainWindow::setMultiUnplayed);
        contextMenu.addSeparator();
        contextMenu.addAction("Delete", &m_scutDeleteSong, &QShortcut::activated);

        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::sliderProgressPressed() {
    m_sliderPositionPressed = true;
}

void MainWindow::sliderProgressReleased() {
    m_mediaBackendKar.setPosition(ui->sliderProgress->value());
    m_sliderPositionPressed = false;
}

void MainWindow::setKeyChange() {
    dlgKeyChange->show();
}

void MainWindow::toggleQueuePlayed() {
    m_qModel.setPlayed(m_rtClickQueueSongId, !m_qModel.getPlayed(m_rtClickQueueSongId));
    updateRotationDuration();
}

void MainWindow::previewKaraokeSong(const QString &path) {
    if (path.isEmpty())
        return;
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, tr("Missing File!"),
                             "Specified karaoke file missing, preview aborted!\n\n" + path, QMessageBox::Ok);
        return;
    }
    auto *videoPreview = new DlgVideoPreview(path, this);
    if (m_testMode)
        videoPreview->setPlaybackTimeLimit(3);
    videoPreview->setAttribute(Qt::WA_DeleteOnClose);
    videoPreview->show();
}

void MainWindow::editSong(const std::shared_ptr<okj::KaraokeSong>& song) {
    QSqlQuery query;
    bool isCdg = false;
    if (QFileInfo(song->path).suffix().toLower() == "cdg")
        isCdg = true;
    QString mediaFile;
    if (isCdg)
        mediaFile = DbUpdater::findMatchingAudioFile(song->path);
    TableModelKaraokeSourceDirs model;
    SourceDir srcDir = model.getDirByPath(song->path);
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
    DlgEditSong dlg(song->artist, song->title, song->songid, showSongId, allowRename, this);
    int result = dlg.exec();
    if (result != QDialog::Accepted)
        return;
    if (song->artist == dlg.artist() && song->title == dlg.title() && song->songid == dlg.songId())
        return;
    if (dlg.renameFile()) {
        if (!QFileInfo(song->path).isWritable()) {
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
                        QFileInfo(song->path).suffix();
                if (isCdg)
                    newMediaFn = dlg.songId() + " - " + dlg.artist() + " - " + dlg.title() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::STA:
                newFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." +
                        QFileInfo(song->path).suffix();
                if (isCdg)
                    newMediaFn = dlg.songId() + " - " + dlg.title() + " - " + dlg.artist() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::ATS:
                newFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." +
                        QFileInfo(song->path).suffix();
                if (isCdg)
                    newMediaFn = dlg.artist() + " - " + dlg.title() + " - " + dlg.songId() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::TAS:
                newFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." +
                        QFileInfo(song->path).suffix();
                if (isCdg)
                    newMediaFn = dlg.title() + " - " + dlg.artist() + " - " + dlg.songId() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::S_T_A:
                newFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." +
                        QFileInfo(song->path).suffix();
                if (isCdg)
                    newMediaFn = dlg.songId() + "_" + dlg.title() + "_" + dlg.artist() + "." +
                                 QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::AT:
                newFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(song->path).suffix();
                if (isCdg)
                    newMediaFn = dlg.artist() + " - " + dlg.title() + "." + QFileInfo(mediaFile).suffix();
                break;
            case SourceDir::TA:
                newFn = dlg.title() + " - " + dlg.artist() + "." + QFileInfo(song->path).suffix();
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
        QString newFilePath = QFileInfo(song->path).absoluteDir().absolutePath() + "/" + newFn;
        if (newFilePath != song->path) {
            if (!QFile::rename(song->path,
                               QFileInfo(song->path).absoluteDir().absolutePath() + "/" + newFn)) {
                QMessageBox msgBoxErr;
                msgBoxErr.setText("Error while renaming file!");
                msgBoxErr.setInformativeText("An unknown error occurred while renaming the file. Operation cancelled.");
                msgBoxErr.setStandardButtons(QMessageBox::Ok);
                msgBoxErr.exec();
                return;
            }
            if (isCdg) {
                if (!QFile::rename(mediaFile,
                                   QFileInfo(song->path).absoluteDir().absolutePath() + "/" + newMediaFn)) {
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
        m_logger->info("{} New filename: {}", m_loggingPrefix, newFn.toStdString());
        query.prepare(
                "UPDATE dbsongs SET artist = :artist, title = :title, discid = :songid, path = :path, filename = :filename, searchstring = :searchstring WHERE songid = :rowid");
        QString newArtist = dlg.artist();
        QString newTitle = dlg.title();
        QString newSongId = dlg.songId();
        QString newPath = QFileInfo(song->path).absoluteDir().absolutePath() + "/" + newFn;
        QString newSearchString =
                QFileInfo(newPath).completeBaseName() + " " + newArtist + " " + newTitle + " " + newSongId;
        query.bindValue(":artist", newArtist);
        query.bindValue(":title", newTitle);
        query.bindValue(":songid", newSongId);
        query.bindValue(":path", newPath);
        query.bindValue(":filename", newFn);
        query.bindValue(":searchstring", newSearchString);
        query.bindValue(":rowid", song->id);
        query.exec();
        if (auto error = query.lastError(); error.type() != QSqlError::NoError) {
            m_logger->error("{} Database error: {}", m_loggingPrefix, error.text().toStdString());
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error while updating the database!");
            msgBoxErr.setInformativeText(query.lastError().text());
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        } else {
            QMessageBox msgBoxInfo;
            msgBoxInfo.setText("Edit successful");
            msgBoxInfo.setInformativeText("The file has been renamed and the database has been updated successfully.");
            msgBoxInfo.setStandardButtons(QMessageBox::Ok);
            msgBoxInfo.exec();
            m_karaokeSongsModel.loadData();
            return;
        }
    } else {
        query.prepare(
                "UPDATE dbsongs SET artist = :artist, title = :title, discid = :songid, searchstring = :searchstring WHERE songid = :rowid");
        QString newArtist = dlg.artist();
        QString newTitle = dlg.title();
        QString newSongId = dlg.songId();
        QString newSearchString =
                QFileInfo(song->path).completeBaseName() + " " + newArtist + " " + newTitle + " " + newSongId;
        query.bindValue(":artist", newArtist);
        query.bindValue(":title", newTitle);
        query.bindValue(":songid", newSongId);
        query.bindValue(":searchstring", newSearchString);
        query.bindValue(":rowid", song->id);
        query.exec();
        if (auto error = query.lastError(); error.type() != QSqlError::NoError) {
            m_logger->error("{} Database error: {}", m_loggingPrefix, error.text().toStdString());
            QMessageBox msgBoxErr;
            msgBoxErr.setText("Error while updating the database!");
            msgBoxErr.setInformativeText(query.lastError().text());
            msgBoxErr.setStandardButtons(QMessageBox::Ok);
            msgBoxErr.exec();
            return;
        } else {
            QMessageBox msgBoxInfo;
            msgBoxInfo.setText("Edit successful");
            msgBoxInfo.setInformativeText("The database has been updated successfully.");
            msgBoxInfo.setStandardButtons(QMessageBox::Ok);
            msgBoxInfo.exec();
            m_karaokeSongsModel.loadData();
            return;
        }
    }
}

void MainWindow::markSongBad(const std::shared_ptr<okj::KaraokeSong>& song) {
    QMessageBox msgBox;
    QMessageBox msgBoxResult;
    msgBox.setWindowTitle("Mark song as bad?");
    msgBox.setText("Would you like mark the file as bad in the DB, or remove it from disk permanently?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setInformativeText(song->path);
    auto markBadButton = msgBox.addButton(tr("Mark Bad"), QMessageBox::ActionRole);
    auto removeFileButton = msgBox.addButton(tr("Remove File"), QMessageBox::ActionRole);
    auto cancelButton = msgBox.addButton(QMessageBox::Cancel);
    msgBox.exec();
    if (msgBox.clickedButton() == markBadButton) {
        m_karaokeSongsModel.markSongBad(song->path);
        msgBoxResult.setText("File marked as bad and will no longer show up in searches.");
        msgBoxResult.setIcon(QMessageBox::Information);
        msgBoxResult.exec();
    } else if (msgBox.clickedButton() == removeFileButton) {
        bool isCdg = false;
        if (QFileInfo(song->path).suffix().toLower() == "cdg")
            isCdg = true;
        QString mediaFile;
        if (isCdg)
            mediaFile = DbUpdater::findMatchingAudioFile(song->path);
        QFile file(song->path);
        auto ret = m_karaokeSongsModel.removeBadSong(song->path);
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
    m_logger->info("{} KaraokeAA - timer timeout", m_loggingPrefix);
    m_timerKaraokeAA.stop();
    cdgWindow->showAlert(false);
    if (m_kAASkip) {
        m_logger->info("{} KaraokeAA - Aborted via stop button", m_loggingPrefix);
        m_kAASkip = false;
    } else {
        auto &singer = m_rotModel.getSinger(m_kAANextSinger);
        m_curSinger = singer.name;
        m_curArtist = singer.nextSongArtist();
        m_curTitle = singer.nextSongTitle();
        ui->labelArtist->setText(m_curArtist);
        ui->labelTitle->setText(m_curTitle);
        ui->labelSinger->setText(m_curSinger);
        if (m_settings.treatAllSingersAsRegs() || m_rotModel.getSinger(m_kAANextSinger).regular) {
            m_historySongsModel.saveSong(
                    m_curSinger,
                    m_kAANextSongPath,
                    m_curArtist,
                    m_curTitle,
                    singer.nextSongSongId(),
                    singer.nextSongKeyChg()
            );
        }
        m_karaokeSongsModel.updateSongHistory(m_karaokeSongsModel.getIdForPath(m_kAANextSongPath));
        play(m_kAANextSongPath);
        m_mediaBackendKar.setPitchShift(singer.nextSongKeyChg());
        m_qModel.setPlayed(singer.nextSongQueueId());
        m_rotModel.setCurrentSinger(m_kAANextSinger);
        m_rotDelegate.setCurrentSinger(m_kAANextSinger);
        if (m_settings.rotationAltSortOrder()) {
            m_curSingerOriginalPosition = singer.position;
            if (singer.position != 0)
                m_rotModel.singerMove(singer.position, 0);
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
        if (m_settings.theme() != 0) {
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
        if (m_settings.theme() != 0) {
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
    if (m_tableModelPlaylists->insertRow(m_tableModelPlaylists->rowCount())) {
        QModelIndex index = m_tableModelPlaylists->index(m_tableModelPlaylists->rowCount() - 1, 1);
        m_tableModelPlaylists->setData(index, title);
        m_tableModelPlaylists->submitAll();
        m_tableModelPlaylists->select();
        ui->comboBoxBmPlaylists->setCurrentIndex(index.row());
    }
}

void MainWindow::bmDbUpdated() {
    m_tableModelBreakSongs.loadDatabase();
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
}

void MainWindow::bmDbCleared() {
    m_logger->info("{} bmDbCleared fired", m_loggingPrefix);
    m_tableModelBreakSongs.loadDatabase();
    bmAddPlaylist("Default");
    ui->comboBoxBmPlaylists->setCurrentIndex(0);
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
                m_mediaBackendBm.stop(true);
                resetBmLabels();
                return;
            }
            auto plSong = m_tableModelPlaylistSongs.getNextPlSong();
            if (plSong.has_value()) {
                m_mediaBackendBm.setMedia(plSong->get().path);
                m_tableModelPlaylistSongs.setCurrentPosition(plSong->get().position);
                m_logger->info("{} Break music auto-advancing to song: {}", m_loggingPrefix,
                               plSong->get().path.toStdString());
                m_mediaBackendBm.stop(true);
                m_mediaBackendBm.play();
                if (m_mediaBackendKar.state() == MediaBackend::PlayingState)
                    m_mediaBackendBm.fadeOutImmediate();
            } else {
                m_mediaBackendBm.stop(true);
                resetBmLabels();
            }
            break;
        }
        case MediaBackend::PlayingState: {
            auto plSong = m_tableModelPlaylistSongs.getCurrentSong();
            if (plSong.has_value())
                ui->labelBmPlaying->setText(plSong->get().artist + " - " + plSong->get().title);
            auto plNextSong = m_tableModelPlaylistSongs.getNextPlSong();
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
    if (!m_sliderBmPositionPressed) {
        ui->sliderBmPosition->setValue((int) position);
    }
    ui->labelBmPosition->setText(QTime(0, 0, 0, 0).addMSecs((int) position).toString("m:ss"));
    ui->labelBmRemaining->setText(
            QTime(0, 0, 0, 0).addMSecs((int) (m_mediaBackendBm.duration() - position)).toString("m:ss"));
}

void MainWindow::bmMediaDurationChanged(const qint64 &duration) {
    ui->sliderBmPosition->setMaximum((int) duration);
    ui->labelBmDuration->setText(QTime(0, 0, 0, 0).addMSecs((int) duration).toString("m:ss"));
}

void MainWindow::tableViewBmPlaylistClicked(const QModelIndex &index) {
    if (index.column() == TableModelPlaylistSongs::COL_PATH) {
        if (m_tableModelPlaylistSongs.isCurrentlyPlayingSong(index.data(Qt::UserRole).toInt())) {
            if (m_mediaBackendBm.state() == MediaBackend::PlayingState ||
                m_mediaBackendBm.state() == MediaBackend::PausedState) {
                QMessageBox msgBox;
                msgBox.setWindowTitle("Unable to remove");
                msgBox.setText(
                        "The playlist song you are trying to remove is currently playing and can not be removed.");
                msgBox.exec();
                return;
            }
            m_tableModelPlaylistSongs.setCurrentPosition(-1);
            resetBmLabels();
        }
        m_tableModelPlaylistSongs.deleteSong(index.row());
        m_tableModelPlaylistSongs.savePlaylistChanges();
        if (m_tableModelPlaylistSongs.currentPosition() > index.row()) {
            m_tableModelPlaylistSongs.setCurrentPosition(m_tableModelPlaylistSongs.currentPosition() - 1);
        }
        if (ui->checkBoxBmBreak->isChecked()) {
            ui->labelBmNext->setText("None - Breaking after current song");
            return;
        }
        auto nextSong = m_tableModelPlaylistSongs.getNextPlSong();
        if (nextSong.has_value())
            ui->labelBmNext->setText(nextSong->get().artist + " - " + nextSong->get().title);
        else
            ui->labelBmNext->setText("None - Breaking after current song");
    }
}

void MainWindow::comboBoxBmPlaylistsIndexChanged(const int &index) {
    m_bmCurrentPlaylist = m_tableModelPlaylists->index(index, 0).data().toInt();
    m_tableModelPlaylistSongs.setCurrentPlaylist(m_bmCurrentPlaylist);
    auto nextPlSong = m_tableModelPlaylistSongs.getNextPlSong();
    if (nextPlSong.has_value())
        ui->labelBmNext->setText(nextPlSong->get().artist + " - " + nextPlSong->get().title);
    else
        ui->labelBmNext->setText("None - Breaking after current song");
    ui->tableViewBmPlaylist->clearSelection();
}

void MainWindow::checkBoxBmBreakToggled(const bool &checked) {
    if (!checked) {
        auto nextSong = m_tableModelPlaylistSongs.getNextPlSong();
        if (nextSong.has_value())
            ui->labelBmNext->setText(nextSong->get().artist + " - " + nextSong->get().title);
        return;
    }
    ui->labelBmNext->setText("None - Stopping after current song");
}

void MainWindow::tableViewBmDbDoubleClicked(const QModelIndex &index) {
    int songId = index.sibling(index.row(), 0).data().toInt();
    m_tableModelPlaylistSongs.addSong(songId);
    m_tableModelPlaylistSongs.savePlaylistChanges();
}

void MainWindow::buttonBmStopClicked() {
    m_mediaBackendBm.stop(false);
}

void MainWindow::searchBreakMusic() {
    m_tableModelBreakSongs.search(ui->lineEditBmSearch->text());
}

void MainWindow::tableViewBmPlaylistDoubleClicked(const QModelIndex &index) {
    if (m_mediaBackendBm.state() == MediaBackend::PlayingState || m_mediaBackendBm.state() == MediaBackend::PausedState)
        m_mediaBackendBm.stop(false);
    m_tableModelPlaylistSongs.setCurrentPosition(index.row());
    auto plSong = m_tableModelPlaylistSongs.getCurrentSong();
    if (!plSong.has_value())
        return;
    m_mediaBackendBm.setMedia(plSong->get().path);
    m_mediaBackendBm.play();
    if (m_mediaBackendKar.state() != MediaBackend::PlayingState)
        m_mediaBackendBm.fadeInImmediate();
}

void MainWindow::buttonBmPauseClicked(const bool &checked) {
    if (checked)
        m_mediaBackendBm.pause();
    else
        m_mediaBackendBm.play();
}

bool MainWindow::bmPlaylistExists(const QString &name) {
    for (int i = 0; i < m_tableModelPlaylists->rowCount(); i++) {
        if (m_tableModelPlaylists->index(i, 1).data().toString().toLower() == name.toLower())
            return true;
    }
    return false;
}

void MainWindow::actionDisplayMetadataToggled(const bool &arg1) {
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_ARTIST, !arg1);
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_TITLE, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_ARTIST, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_TITLE, !arg1);
    m_settings.bmSetShowMetadata(arg1);
    autosizeBmViews();
}

void MainWindow::actionDisplayFilenamesToggled(const bool &arg1) {
    ui->tableViewBmDb->setColumnHidden(TableModelBreakSongs::COL_FILENAME, !arg1);
    ui->tableViewBmPlaylist->setColumnHidden(TableModelPlaylistSongs::COL_FILENAME, !arg1);
    m_settings.bmSetShowFilenames(arg1);
    autosizeBmViews();
}

void MainWindow::actionPlaylistNewTriggered() {
    bool ok;
    QString title = QInputDialog::getText(this, tr("New Playlist"), tr("Playlist title:"), QLineEdit::Normal,
                                          tr("New Playlist"), &ok);
    if (ok && !title.isEmpty()) {
        bmAddPlaylist(title);
    }
}

void MainWindow::actionPlaylistImportTriggered() {
#ifdef Q_OS_LINUX
    QString importFile = QFileDialog::getOpenFileName(
            this,
            tr("Select playlist to import"),
            QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0),
            tr("m3u playlist(*.m3u)"),
            nullptr,
            QFileDialog::DontUseNativeDialog
    );
#else
    QString importFile = QFileDialog::getOpenFileName(
            this,
            tr("Select playlist to import"),
            QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0),
            tr("m3u playlist(*.m3u)"),
            nullptr
    );
#endif
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
                if (m_tableModelPlaylists->insertRow(m_tableModelPlaylists->rowCount())) {
                    QModelIndex index = m_tableModelPlaylists->index(m_tableModelPlaylists->rowCount() - 1, 1);
                    m_tableModelPlaylists->setData(index, plTitle);
                    m_tableModelPlaylists->submitAll();
                    m_tableModelPlaylists->select();
                    ui->comboBoxBmPlaylists->setCurrentIndex(index.row());
                    m_tableModelPlaylistSongs.setCurrentPlaylist(
                            m_tableModelPlaylists->index(index.row(), 0).data().toInt());

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
        m_tableModelBreakSongs.loadDatabase();
        QApplication::processEvents();
        QList<int> songIds;
        for (int i = 0; i < files.size(); i++) {
            int songId = m_tableModelBreakSongs.getSongId(files.at(i));
            if (songId >= 0) {
                songIds.push_back(songId);
            } else {
                songId = m_tableModelBreakSongs.getSongId(importPath + "/" + files.at(i));
                if (songId >= 0) {
                    songIds.push_back(songId);
                }
            }
        }
        m_tableModelPlaylistSongs.setCurrentPlaylist(m_bmCurrentPlaylist);
        std::for_each(songIds.begin(), songIds.end(), [&](int songId) {
            m_tableModelPlaylistSongs.addSong(songId);
        });
        m_tableModelPlaylistSongs.savePlaylistChanges();
    }
}

void MainWindow::actionPlaylistExportTriggered() {
    QString defaultFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() +
                              ui->comboBoxBmPlaylists->currentText() + ".m3u";
    m_logger->debug("{} Default save location: {}", m_loggingPrefix, defaultFilePath.toStdString());
#ifdef Q_OS_LINUX
    QString saveFilePath = QFileDialog::getSaveFileName(
            this,
            tr("Select filename to save playlist as"),
            defaultFilePath,
            tr("m3u playlist(*.m3u)"),
            nullptr,
            QFileDialog::DontUseNativeDialog
    );
#else
    QString saveFilePath = QFileDialog::getSaveFileName(
            this,
            tr("Select filename to save playlist as"),
            defaultFilePath,
            tr("m3u playlist(*.m3u)"),
            nullptr
    );
#endif
    if (saveFilePath != "") {
        QFile file(saveFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("Error saving file"),
                                 tr("Unable to open selected file for writing.  Please verify that you have the proper permissions to write to that location."),
                                 QMessageBox::Close);
            return;
        }
        QTextStream out(&file);
        for (int i = 0; i < m_tableModelPlaylistSongs.rowCount(); i++) {
            out << m_tableModelPlaylistSongs.index(i, TableModelPlaylistSongs::COL_PATH).data().toString() << "\n";
        }
    }
}

void MainWindow::actionPlaylistDeleteTriggered() {
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
        query.exec("DELETE FROM bmplsongs WHERE playlist == " +
                   QString::number(m_tableModelPlaylistSongs.currentPlaylist()));
        query.exec(
                "DELETE FROM bmplaylists WHERE playlistid == " +
                QString::number(m_tableModelPlaylistSongs.currentPlaylist()));
        m_tableModelPlaylists->select();
        if (m_tableModelPlaylists->rowCount() == 0) {
            bmAddPlaylist("Default");
        }
        ui->comboBoxBmPlaylists->setCurrentIndex(0);
        m_tableModelPlaylistSongs.setCurrentPlaylist(m_tableModelPlaylists->index(0, 0).data().toInt());
    }
}

void MainWindow::actionAboutTriggered() {
    QString title;
    QString text;
    QString date = QString::fromLocal8Bit(__DATE__) + " " + QString(__TIME__);
    title = "About OpenKJ";
    text =  "OpenKJ\nVersion: " + QString(OKJ_VERSION_STRING) + " " + QString(OKJ_VERSION_BRANCH);
    text += "\nLicense: GNU GPL v3+";
    text += "\nBuilt: " + date;
    text += "\n\nIncluded library info";
    text += "\n\nQt version: " + QString(QT_VERSION_STR);
    text += "\nLicense: GNU GPL v3";
    text += "\n\nGStreamer version: " + QString::number(GST_VERSION_MAJOR) + '.' + QString::number(GST_VERSION_MINOR) + '.' + QString::number(GST_VERSION_MICRO);
    text += "\nLicense: GNU LGPL v2.1";
    text += "\n\nspdlog version: " + QString::number(SPDLOG_VER_MAJOR) + '.' + QString::number(SPDLOG_VER_MINOR) + '.' + QString::number(SPDLOG_VER_PATCH);
    text += "\nLicense: MIT";
    text += "\n\nTagLib version: " + QString::number(TAGLIB_MAJOR_VERSION) + '.' + QString::number(TAGLIB_MINOR_VERSION) + '.' + QString::number(TAGLIB_PATCH_VERSION);
    text += "\nLicense: GNU LGPL v2.1";
    text += "\n\nMiniZ version: " + QString(MZ_VERSION);
    text += "\nLicense: MIT";
    QMessageBox::about(this, title, text);
}

void MainWindow::pushButtonMplxLeftToggled(const bool &checked) {
    if (!checked)
        return;
    m_settings.setMplxMode(Multiplex_LeftChannel);
    m_mediaBackendKar.setMplxMode(Multiplex_LeftChannel);

}

void MainWindow::pushButtonMplxBothToggled(const bool &checked) {
    if (!checked)
        return;
    m_settings.setMplxMode(Multiplex_Normal);
    m_mediaBackendKar.setMplxMode(Multiplex_Normal);
}

void MainWindow::pushButtonMplxRightToggled(const bool &checked) {
    if (!checked)
        return;
    m_settings.setMplxMode(Multiplex_RightChannel);
    m_mediaBackendKar.setMplxMode(Multiplex_RightChannel);
}

void MainWindow::lineEditSearchTextChanged(const QString &arg1) {
    if (!m_settings.progressiveSearchEnabled())
        return;
    ui->tableViewDB->scrollToTop();
    static QString lastVal;
    if (arg1.trimmed() != lastVal) {
        m_karaokeSongsModel.search(arg1);
        lastVal = arg1.trimmed();
    }
}

void MainWindow::setMultiPlayed() {
    QModelIndexList indexes = ui->tableViewQueue->selectionModel()->selectedIndexes();
    QModelIndex index;

            foreach(index, indexes) {
            if (index.column() == 0) {
                int queueId = index.sibling(index.row(), 0).data().toInt();
                m_logger->info("{} Selected row: {} - queueId: {}", m_loggingPrefix, index.row(), queueId);
                m_qModel.setPlayed(queueId);
            }
        }
}

void MainWindow::setMultiUnplayed() {
    QModelIndexList indexes = ui->tableViewQueue->selectionModel()->selectedIndexes();
    QModelIndex index;

            foreach(index, indexes) {
            if (index.column() == 0) {
                int queueId = index.sibling(index.row(), 0).data().toInt();
                m_logger->info("{} Selected row: {} - queueId: {}", m_loggingPrefix, index.row(), queueId);
                m_qModel.setPlayed(queueId, false);
            }
        }
}

void MainWindow::spinBoxTempoValueChanged(const int &arg1) {
    m_mediaBackendKar.setTempo(arg1);
    QTimer::singleShot(20, [&]() {
        ui->spinBoxTempo->findChild<QLineEdit *>()->deselect();
    });
}

void MainWindow::audioError(const QString &msg) {
    QMessageBox msgBox;
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText("Audio playback error! - " + msg);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
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
    if (!m_settings.cdgWindowFullscreen())
        m_settings.saveWindowState(cdgWindow.get());
    m_settings.setShowCdgWindow(cdgWindow->isVisible());
    cdgWindow->setVisible(false);
    dlgSongShop->setVisible(false);
    requestsDialog->setVisible(false);
    event->accept();
}

void MainWindow::sliderVolumeChanged(int value) {
    m_mediaBackendKar.setVolume(value);
    m_mediaBackendKar.fadeInImmediate();
}

void MainWindow::sliderBmVolumeChanged(int value) {
    m_mediaBackendBm.setVolume(value);
    if (m_mediaBackendKar.state() != MediaBackend::PlayingState)
        m_mediaBackendBm.fadeInImmediate();
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
    if (m_updateChecker->getOS() == "Linux") {
        msgBox.setInformativeText(
                "To install the update, please use your distribution's package manager or download and build the current source.");
    }
    if (m_updateChecker->getOS() == "Win32" || m_updateChecker->getOS() == "Win64") {
        msgBox.setInformativeText(
                "You can download the new version at <a href=https://openkj.org/software>https://openkj.org/software</a>");
    }
    if (m_updateChecker->getOS() == "MacOS") {
        msgBox.setInformativeText(
                "You can download the new version at <a href=https://openkj.org/software>https://openkj.org/software</a>");
    }
    msgBox.exec();
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
                    if (m_karaokeSongsModel.findCdgAudioFile(file) == QString()) {
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
                m_logger->info("{} Karaoke file dropped. Singer: {} Pos: {} Path: {}", m_loggingPrefix, singerId,
                               position, file.toStdString());
                QFileInfo dFileInfo(file);

                okj::KaraokeSong droppedSong{
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
                int songId = m_karaokeSongsModel.addSong(droppedSong);
                m_logger->info("{} addSong returned songid: {}", m_loggingPrefix, songId);
                if (songId == -1)
                    continue;
                m_qModel.insert(songId, position);
            }
        }
}

void MainWindow::appFontChanged(const QFont &font) {
    QApplication::setFont(font);
    ui->actionAbout->setFont(font);
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

    setFont(font);
    QFontMetrics fm(m_settings.applicationFont());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int cvwWidth = std::max(300, fm.horizontalAdvance("Total: 00:00  Current:00:00  Remain: 00:00"));
#else
    int cvwWidth = std::max(300, fm.width("Total: 00:00  Current:00:00  Remain: 00:00"));
#endif
    m_logger->info("{} Resizing videoPreview to width: {}", m_loggingPrefix, cvwWidth);
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

void MainWindow::autosizeRotationCols() {
    if (m_settings.rotationShowNextSong()) {
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
    autosizeKaraokeDbCols();
    autosizeRotationCols();
    autosizeQueueCols();
    autosizeHistoryCols();
}

void MainWindow::autosizeKaraokeDbCols() const {
    // Toggling the mode on these to get the initial size then allowing manual resize
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_LASTPLAY, QHeaderView::ResizeToContents);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_PLAYS, QHeaderView::ResizeToContents);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_DURATION, QHeaderView::ResizeToContents);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_ARTIST, QHeaderView::Stretch);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_TITLE, QHeaderView::Stretch);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_SONGID, QHeaderView::ResizeToContents);
    QApplication::processEvents();
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_ARTIST, QHeaderView::Interactive);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_TITLE, QHeaderView::Interactive);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_SONGID, QHeaderView::Interactive);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_LASTPLAY, QHeaderView::Interactive);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_PLAYS, QHeaderView::Interactive);
    ui->tableViewDB->horizontalHeader()->setSectionResizeMode(TableModelKaraokeSongs::COL_DURATION, QHeaderView::Interactive);
}

void MainWindow::autosizeQueueCols() {
    int fH = QFontMetrics(m_settings.applicationFont()).height();
    int iconWidth = fH + fH;
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_PATH, QHeaderView::Fixed);
    ui->tableViewQueue->horizontalHeader()->resizeSection(TableModelQueueSongs::COL_PATH, iconWidth);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_ARTIST, QHeaderView::Stretch);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_TITLE, QHeaderView::Stretch);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_SONGID, QHeaderView::ResizeToContents);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_DURATION, QHeaderView::ResizeToContents);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_KEY, QHeaderView::ResizeToContents);
    QApplication::processEvents();
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_ARTIST, QHeaderView::Interactive);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_TITLE, QHeaderView::Interactive);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_SONGID, QHeaderView::Interactive);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_DURATION, QHeaderView::Interactive);
    ui->tableViewQueue->horizontalHeader()->setSectionResizeMode(TableModelQueueSongs::COL_KEY, QHeaderView::Interactive);
}

void MainWindow::autosizeHistoryCols() {
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::LAST_SUNG, QHeaderView::ResizeToContents);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::SUNG_COUNT, QHeaderView::ResizeToContents);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::KEY_CHANGE, QHeaderView::ResizeToContents);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::ARTIST, QHeaderView::Stretch);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::TITLE, QHeaderView::Stretch);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::SONGID, QHeaderView::ResizeToContents);
    QApplication::processEvents();
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::LAST_SUNG, QHeaderView::Interactive);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::SUNG_COUNT, QHeaderView::Interactive);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::KEY_CHANGE, QHeaderView::Interactive);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::ARTIST, QHeaderView::Interactive);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::TITLE, QHeaderView::Interactive);
    ui->tableViewHistory->horizontalHeader()->setSectionResizeMode(TableModelHistorySongs::SONGID, QHeaderView::Interactive);
}

void MainWindow::autosizeBmViews() {

    int fH = QFontMetrics(m_settings.applicationFont()).height();
    int iconWidth = fH + fH;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    int durationColSize = QFontMetrics(m_settings.applicationFont()).horizontalAdvance("Duration") + fH;
#else
    int durationColSize = QFontMetrics(m_settings.applicationFont()).width("Duration") + fH;
#endif
    // 4 = filename 1 = metadata artist 2 = metadata title

    int artistColSize = 0;
    int titleColSize = 0;
    int fileNameColSize = 0;
    int remainingSpace = ui->tableViewBmDb->width() - durationColSize - 15;
    if (m_settings.bmShowMetadata() && m_settings.bmShowFilenames()) {
        artistColSize = (int) ((float) remainingSpace * .25);
        titleColSize = (int) ((float) remainingSpace * .25);
        fileNameColSize = (int) ((float) remainingSpace * .5);
    } else if (m_settings.bmShowMetadata()) {
        artistColSize = (int) ((float) remainingSpace * .5);
        titleColSize = (int) ((float) remainingSpace * .5);
    } else if (m_settings.bmShowFilenames()) {
        fileNameColSize = remainingSpace;
    }
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_ARTIST, artistColSize);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_TITLE, titleColSize);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_FILENAME, fileNameColSize);
    ui->tableViewBmDb->horizontalHeader()->setSectionResizeMode(TableModelBreakSongs::COL_DURATION, QHeaderView::Fixed);
    ui->tableViewBmDb->horizontalHeader()->resizeSection(TableModelBreakSongs::COL_DURATION, durationColSize);


    remainingSpace = ui->tableViewBmPlaylist->width() - durationColSize - (iconWidth * 2) - 15;
    if (m_settings.bmShowMetadata() && m_settings.bmShowFilenames()) {
        artistColSize = (int) ((float) remainingSpace * .25);
        titleColSize = (int) ((float) remainingSpace * .25);
        fileNameColSize = (int) ((float) remainingSpace * .5);
    } else if (m_settings.bmShowMetadata()) {
        artistColSize = (int) ((float) remainingSpace * .5);
        titleColSize = (int) ((float) remainingSpace * .5);
    } else if (m_settings.bmShowFilenames()) {
        fileNameColSize = remainingSpace;
    }
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_ARTIST, artistColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_TITLE, titleColSize);
    ui->tableViewBmPlaylist->horizontalHeader()->resizeSection(TableModelPlaylistSongs::COL_FILENAME, fileNameColSize);
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
        m_bNeedAutoSize = true;
        m_kNeedAutoSize = false;
    }
    if (ui->tabWidget->currentIndex() == 1) {
        autosizeBmViews();
        m_bNeedAutoSize = false;
        m_kNeedAutoSize = true;
    }
    m_settings.saveWindowState(this);
}

void MainWindow::tabWidgetCurrentChanged(const int &index) {
    if (m_bNeedAutoSize && index == 1) {
        autosizeBmViews();
        m_bNeedAutoSize = false;
    }
    if (m_kNeedAutoSize && index == 0) {
        autosizeViews();
        m_kNeedAutoSize = false;
    }
}

void MainWindow::bmDatabaseAboutToUpdate() {
    m_tableModelPlaylists->revertAll();
    m_tableModelPlaylists->setTable("");
}

void MainWindow::bmSongMoved(const int &oldPos, const int &newPos) {
    int curPlPos = m_tableModelPlaylistSongs.currentPosition();
    if (oldPos < curPlPos && newPos >= curPlPos)
        curPlPos--;
    else if (oldPos > curPlPos && newPos <= curPlPos)
        curPlPos++;
    else if (oldPos == curPlPos)
        curPlPos = newPos;
    m_tableModelPlaylistSongs.setCurrentPosition(curPlPos);
    auto nextPlSong = m_tableModelPlaylistSongs.getNextPlSong();
    if (!ui->checkBoxBmBreak->isChecked() && nextPlSong.has_value()) {
        ui->labelBmNext->setText(nextPlSong->get().artist + " - " + nextPlSong->get().title);
    } else
        ui->labelBmNext->setText("None - Breaking after current song");
}

void MainWindow::sliderBmPositionPressed() {
    m_logger->trace("{} BM slider down", m_loggingPrefix);
    m_sliderBmPositionPressed = true;
}

void MainWindow::sliderBmPositionReleased() {
    m_mediaBackendBm.setPosition(ui->sliderBmPosition->value());
    m_sliderBmPositionPressed = false;
    m_logger->trace("{} BM slider up.  Position: {}", m_loggingPrefix, ui->sliderBmPosition->value());
}

void MainWindow::sfxButtonPressed() {
    auto *btn = (SoundFxButton *) sender();
    m_mediaBackendSfx.setMedia(btn->buttonData().toString());
    m_mediaBackendSfx.setVolume(ui->sliderVolume->value());
    m_mediaBackendSfx.play();
}

void MainWindow::addSfxButtonPressed() {
#ifdef Q_OS_LINUX
    QString path = QFileDialog::getOpenFileName(
            this,
            "Select audio file",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            "Audio (*.mp3 *.ogg *.wav *.wma)",
            nullptr,
            QFileDialog::DontUseNativeDialog
    );
#else
    QString path = QFileDialog::getOpenFileName(
            this,
            "Select audio file",
            QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0),
            "Audio (*.mp3 *.ogg *.wav *.wma)",
            nullptr
    );
#endif
    if (path != "") {
        bool ok;
        QString name = QInputDialog::getText(this, tr("Button Text"), tr("Enter button text:"), QLineEdit::Normal,
                                             QString(), &ok);
        if (!ok || name.isEmpty())
            return;
        SfxEntry entry;
        entry.name = name;
        entry.path = path;
        m_settings.addSfxEntry(entry);
        addSfxButton(path, name);
    }

}

void MainWindow::stopSfxPlayback() {
    m_mediaBackendSfx.stop(true);
}

void MainWindow::removeSfxButton() {
    SfxEntryList entries = m_settings.getSfxEntries();
    SfxEntryList newEntries;
            foreach (SfxEntry entry, entries) {
            if (entry.name == m_lastRtClickedSfxBtn.name && entry.path == m_lastRtClickedSfxBtn.path)
                continue;
            newEntries.push_back(entry);
        }
    m_settings.setSfxEntries(newEntries);
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
    m_qModel.loadSinger(cur.data(Qt::UserRole).toInt());
    m_historySongsModel.loadSinger(m_rotModel.getSinger(cur.data(Qt::UserRole).toInt()).name);
    if (!m_settings.treatAllSingersAsRegs() && !cur.sibling(cur.row(), TableModelRotation::COL_REGULAR).data().toBool())
        ui->tabWidgetQueue->removeTab(1);
    else if (ui->tabWidgetQueue->count() == 1) {
        ui->tabWidgetQueue->addTab(m_historyTabWidget, "History");
    }
    ui->gbxQueue->setTitle(
            QString("Song Queue - " + cur.sibling(cur.row(), TableModelRotation::COL_NAME).data().toString()));
    if (!ui->tabWidgetQueue->isVisible()) {
        ui->tabWidgetQueue->setVisible(true);
        ui->labelNoSinger->setVisible(false);
        QApplication::processEvents();
        autosizeQueueCols();
        m_settings.restoreColumnWidths(ui->tableViewQueue);
    }
}

void MainWindow::updateRotationDuration() {
    QString text;
    int secs = m_rotModel.rotationDuration();
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
    m_labelRotationDuration.setText(text);
}

void MainWindow::rotationSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    if (selected.empty()) {
        m_logger->trace("{} Rotation Selection Cleared!", m_loggingPrefix);
        m_qModel.loadSinger(-1);
        ui->tableViewRotation->reset();
        ui->gbxQueue->setTitle("Song Queue");
        ui->tabWidgetQueue->setVisible(false);
        ui->labelNoSinger->setVisible(true);
    }
    m_logger->trace("{} Rotation Selection Changed", m_loggingPrefix);
}

void MainWindow::lineEditBmSearchChanged(const QString &arg1) {
    if (!m_settings.progressiveSearchEnabled())
        return;
    static QString lastVal;
    if (arg1.trimmed() != lastVal) {
        m_tableModelBreakSongs.search(arg1);
        lastVal = arg1.trimmed();
    }
}

void MainWindow::btnRotTopClicked() {
    auto indexes = ui->tableViewRotation->selectionModel()->selectedRows();
    std::vector<int> singerIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        singerIds.emplace_back(index.data(Qt::UserRole).toInt());
    });
    std::for_each(singerIds.rbegin(), singerIds.rend(), [&](auto singerId) {
        m_rotModel.singerMove(m_rotModel.getSinger(singerId).position, 0);
    });
    auto topLeft = ui->tableViewRotation->model()->index(0, 0);
    auto bottomRight = ui->tableViewRotation->model()->index((int) singerIds.size() - 1, m_rotModel.columnCount(QModelIndex()) - 1);
    ui->tableViewRotation->clearSelection();
    ui->tableViewRotation->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::btnRotUpClicked() {
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    m_rotModel.singerMove(curPos, curPos - 1);
    ui->tableViewRotation->selectRow(curPos - 1);
    rotationDataChanged();
}

void MainWindow::btnRotDownClicked() {
    if (ui->tableViewRotation->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewRotation->selectionModel()->selectedRows().at(0).row();
    if (curPos == m_rotModel.singerCount() - 1)
        return;
    m_rotModel.singerMove(curPos, curPos + 1);
    ui->tableViewRotation->selectRow(curPos + 1);
    rotationDataChanged();
}

void MainWindow::btnRotBottomClicked() {
    auto indexes = ui->tableViewRotation->selectionModel()->selectedRows();
    std::vector<int> singerIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        singerIds.emplace_back(index.data(Qt::UserRole).toInt());
    });
    std::for_each(singerIds.begin(), singerIds.end(), [&](auto singerId) {
        m_rotModel.singerMove(m_rotModel.getSinger(singerId).position, m_rotModel.singerCount() - 1);
    });
    auto topLeft = ui->tableViewRotation->model()->index((int) (m_rotModel.singerCount() - singerIds.size()), 0);
    auto bottomRight = ui->tableViewRotation->model()->index(static_cast<int>(m_rotModel.singerCount() - 1), m_rotModel.columnCount(QModelIndex()) - 1);
    ui->tableViewRotation->clearSelection();
    ui->tableViewRotation->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::btnQTopClicked() {
    auto indexes = ui->tableViewQueue->selectionModel()->selectedRows();
    std::vector<int> songIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        songIds.emplace_back(index.data().toInt());
    });
    std::for_each(songIds.rbegin(), songIds.rend(), [&](auto songId) {
        m_qModel.moveSongId(songId, 0);
    });
    auto topLeft = ui->tableViewQueue->model()->index(0, 0);
    auto bottomRight = ui->tableViewQueue->model()->index((int) songIds.size() - 1, m_qModel.columnCount() - 1);
    ui->tableViewQueue->clearSelection();
    ui->tableViewQueue->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::btnQUpClicked() {
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    m_qModel.move(curPos, curPos - 1);
    ui->tableViewQueue->selectRow(curPos - 1);
    rotationDataChanged();
}

void MainWindow::btnQDownClicked() {
    if (ui->tableViewQueue->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewQueue->selectionModel()->selectedRows().at(0).row();
    if (curPos == ui->tableViewQueue->model()->rowCount() - 1)
        return;
    m_qModel.move(curPos, curPos + 1);
    ui->tableViewQueue->selectRow(curPos + 1);
    rotationDataChanged();
}

void MainWindow::btnQBottomClicked() {
    auto indexes = ui->tableViewQueue->selectionModel()->selectedRows();
    std::vector<int> songIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        songIds.emplace_back(index.data().toInt());
    });
    std::for_each(songIds.begin(), songIds.end(), [&](auto songId) {
        m_qModel.moveSongId(songId, m_qModel.rowCount() - 1);
    });
    auto topLeft = ui->tableViewQueue->model()->index((int) (m_qModel.rowCount() - songIds.size()), 0);
    auto bottomRight = ui->tableViewQueue->model()->index(m_qModel.rowCount() - 1, m_qModel.columnCount() - 1);
    ui->tableViewQueue->clearSelection();
    ui->tableViewQueue->selectionModel()->select(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    rotationDataChanged();
}

void MainWindow::btnBmPlRandomizeClicked() {
    if (m_tableModelPlaylistSongs.rowCount() < 2)
        return;
    m_tableModelPlaylistSongs.randomizePlaylist();
}

void MainWindow::btnPlTopClicked() {
    auto indexes = ui->tableViewBmPlaylist->selectionModel()->selectedRows();
    std::vector<int> plSongIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        plSongIds.emplace_back(index.data().toInt());
    });
    std::for_each(plSongIds.rbegin(), plSongIds.rend(), [&](auto plSongId) {
        m_tableModelPlaylistSongs.moveSong(m_tableModelPlaylistSongs.getSongPositionById(plSongId), 0);
    });
    m_tableModelPlaylistSongs.savePlaylistChanges();
    auto topLeft = ui->tableViewBmPlaylist->model()->index(0, 0);
    auto bottomRight = ui->tableViewBmPlaylist->model()->index((int) plSongIds.size() - 1, 7);
    ui->tableViewBmPlaylist->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                      QItemSelectionModel::Select);
}

void MainWindow::btnPlUpClicked() {
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == 0)
        return;
    m_tableModelPlaylistSongs.moveSong(curPos, curPos - 1);
    m_tableModelPlaylistSongs.savePlaylistChanges();
    ui->tableViewBmPlaylist->selectRow(curPos - 1);
}

void MainWindow::btnPlDownClicked() {
    int maxPos = ui->tableViewBmPlaylist->model()->rowCount() - 1;
    if (ui->tableViewBmPlaylist->selectionModel()->selectedRows().count() < 1)
        return;
    int curPos = ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0).row();
    if (curPos == maxPos)
        return;
    m_tableModelPlaylistSongs.moveSong(curPos, curPos + 1);
    m_tableModelPlaylistSongs.savePlaylistChanges();
    ui->tableViewBmPlaylist->selectRow(curPos + 1);
}

void MainWindow::btnPlBottomClicked() {
    auto indexes = ui->tableViewBmPlaylist->selectionModel()->selectedRows();
    std::vector<int> plSongIds;
    std::for_each(indexes.begin(), indexes.end(), [&](QModelIndex index) {
        plSongIds.emplace_back(index.data().toInt());
    });
    std::for_each(plSongIds.begin(), plSongIds.end(), [&](auto plSongId) {
        m_tableModelPlaylistSongs.moveSong(m_tableModelPlaylistSongs.getSongPositionById(plSongId),
                                           m_tableModelPlaylistSongs.rowCount() - 1);
    });
    m_tableModelPlaylistSongs.savePlaylistChanges();
    auto topLeft = ui->tableViewBmPlaylist->model()->index(
            (int) (m_tableModelPlaylistSongs.rowCount() - plSongIds.size()), 0);
    auto bottomRight = ui->tableViewBmPlaylist->model()->index(m_tableModelPlaylistSongs.rowCount() - 1, 7);
    ui->tableViewBmPlaylist->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                                      QItemSelectionModel::Select);
}

void MainWindow::actionSoundClipsTriggered(const bool &checked) {
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
    m_settings.setShowMainWindowSoundClips(checked);
}

void MainWindow::actionNowPlayingTriggered(const bool &checked) {
    ui->groupBoxNowPlaying->setVisible(checked);
    m_settings.setShowMainWindowNowPlaying(checked);
}

void MainWindow::actionVideoSmallTriggered() {
    ui->videoPreview->setMinimumSize(QSize(256, 144));
    ui->videoPreview->setMaximumSize(QSize(256, 144));
    ui->videoPreviewBm->setMinimumSize(QSize(256, 144));
    ui->videoPreviewBm->setMaximumSize(QSize(256, 144));
    ui->mediaFrame->setMaximumWidth(300);
    ui->mediaFrame->setMinimumWidth(300);
    m_settings.setMainWindowVideoSize(Settings::Small);
    QTimer::singleShot(15, [&]() { autosizeViews(); });
}

void MainWindow::actionVideoMediumTriggered() {
    ui->videoPreview->setMinimumSize(QSize(384, 216));
    ui->videoPreview->setMaximumSize(QSize(384, 216));
    ui->videoPreviewBm->setMinimumSize(QSize(384, 216));
    ui->videoPreviewBm->setMaximumSize(QSize(384, 216));
    ui->mediaFrame->setMaximumWidth(430);
    ui->mediaFrame->setMinimumWidth(430);
    m_settings.setMainWindowVideoSize(Settings::Medium);
    QTimer::singleShot(15, [&]() { autosizeViews(); });
}

void MainWindow::actionVideoLargeTriggered() {
    ui->videoPreview->setMinimumSize(QSize(512, 288));
    ui->videoPreview->setMaximumSize(QSize(512, 288));
    ui->videoPreviewBm->setMinimumSize(QSize(512, 288));
    ui->videoPreviewBm->setMaximumSize(QSize(512, 288));
    ui->mediaFrame->setMaximumWidth(560);
    ui->mediaFrame->setMinimumWidth(560);
    m_settings.setMainWindowVideoSize(Settings::Large);
    QTimer::singleShot(15, [&]() { autosizeViews(); });
}

void MainWindow::actionKaraokeTorture() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static int runs = 0;
        m_logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        m_logger->info("{} num songs in db: {}", m_loggingPrefix, m_karaokeSongsModel.rowCount(QModelIndex()));
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
        int randomNum = QRandomGenerator::global()->bounded(0, m_karaokeSongsModel.rowCount(QModelIndex()) - 1);
        randomNum = 1;
        m_logger->info("{} randomNum: {}", m_loggingPrefix, randomNum);
        ui->tableViewDB->selectRow(randomNum);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        play(m_karaokeSongsModel.getPath(
                     ui->tableViewDB->selectionModel()->selectedRows(TableModelKaraokeSongs::COL_ID).at(0).data().toInt()),
             true);
        ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
    });
    m_timerTest.start(4000);
#endif
}

void MainWindow::actionKAndBTorture() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        if (playing) {
            buttonStopClicked();
            playing = false;
            ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
            return;
        }
        m_logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        m_logger->info("{} num songs in db: {}", m_loggingPrefix, m_karaokeSongsModel.rowCount(QModelIndex()));
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
        int randomNum = QRandomGenerator::global()->bounded(0, m_karaokeSongsModel.rowCount(QModelIndex()) - 1);
        m_logger->info("{} randomNum: {}", m_loggingPrefix, randomNum);
        ui->tableViewDB->selectRow(randomNum);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        play(ui->tableViewDB->selectionModel()->selectedRows(5).at(0).data().toString(), false);
        ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
        playing = true;
    });
    m_timerTest.start(2000);
#endif
}

void MainWindow::actionBurnIn() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    m_testMode = true;
    emit ui->buttonClearRotation->clicked();
    for (auto i = 0; i < 21; i++) {
        auto singerName = "Test Singer " + QString::number(i);
        m_rotModel.singerAdd(singerName);
    }
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        if (playing) {
            buttonStopClicked();
            playing = false;
            ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
            return;
        }

        m_rotModel.singerMove(QRandomGenerator::global()->bounded(0, 19), QRandomGenerator::global()->bounded(0, 19));
        ui->tableViewRotation->selectRow(QRandomGenerator::global()->bounded(0, 19));
        int randomNum{0};
        if (m_karaokeSongsModel.rowCount(QModelIndex()) > 1)
            randomNum = QRandomGenerator::global()->bounded(0, m_karaokeSongsModel.rowCount(QModelIndex()) - 1);
        m_logger->info("{} randomNum: {}", m_loggingPrefix, randomNum);
        ui->tableViewDB->selectRow(randomNum);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        emit ui->tableViewDB->doubleClicked(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        ui->tableViewQueue->selectRow(m_qModel.rowCount() - 1);
        ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        if (m_qModel.rowCount() > 2) {
            auto newPos = QRandomGenerator::global()->bounded(0, m_qModel.rowCount() - 1);
            m_qModel.move(m_qModel.rowCount() - 1, newPos);
            ui->tableViewQueue->selectRow(newPos);
            ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        }
        auto idx = ui->tableViewQueue->selectionModel()->selectedRows().at(0);
        emit ui->tableViewQueue->doubleClicked(idx);
        ui->tableViewQueue->selectRow(idx.row());
        playing = true;
        m_logger->info("{} Burn in test cycle: {}", m_loggingPrefix, ++runs);
    });
    m_timerTest.start(3000);
#endif
}

void MainWindow::actionPreviewBurnIn() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    m_testMode = true;
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        ui->labelSinger->setText("Preview burn-in run (" + QString::number(runs) + ")");
        int randomNum{0};
        if (m_karaokeSongsModel.rowCount(QModelIndex()) > 1)
            randomNum = QRandomGenerator::global()->bounded(0, m_karaokeSongsModel.rowCount(QModelIndex()) - 1);
        m_logger->info("{} randomNum: {}", m_loggingPrefix, randomNum);
        ui->tableViewDB->selectRow(randomNum);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        auto selRow = ui->tableViewDB->selectionModel()->selectedRows().at(0);
        int songId = selRow.sibling(selRow.row(), TableModelKaraokeSongs::COL_ID).data().toInt();
        previewKaraokeSong(m_karaokeSongsModel.getPath(songId));
        m_logger->info("{} Preview burn-in test cycle: {}", m_loggingPrefix, ++runs);
    });
    m_timerTest.start(3250);
#endif
}

void MainWindow::actionCdgDecodeTorture() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static int runs = 0;
        m_logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        m_logger->info("{} num songs in db: {}", m_loggingPrefix, m_karaokeSongsModel.rowCount(QModelIndex()));
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
        int randomNum = QRandomGenerator::global()->bounded(0, m_karaokeSongsModel.rowCount(QModelIndex()) - 1);
        m_logger->info("{} randomNum: {}", m_loggingPrefix, randomNum);
        ui->tableViewDB->selectRow(randomNum);
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
                    if (!archive.extractAudio(m_mediaTempDir->path(), "tmp" + archive.audioExtension())) {
                        return;
                    }
                    if (!archive.extractCdg(m_mediaTempDir->path(), "tmp.cdg")) {
                        return;
                    }
                    QString audioFile = m_mediaTempDir->path() + QDir::separator() + "tmp" + archive.audioExtension();
                    QString cdgFile = m_mediaTempDir->path() + QDir::separator() + "tmp.cdg";
                    m_logger->info("{} Extracted audio file size: {}", m_loggingPrefix, QFileInfo(audioFile).size());
                    m_logger->info("{} Setting karaoke backend source file to: {}", m_loggingPrefix,
                                   audioFile.toStdString());
                    m_mediaBackendKar.setMediaCdg(cdgFile, audioFile);
                    //m_mediaBackendKar.testCdgDecode(); // todo: andth
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
            QString audioFilename = findMatchingAudioFile(karaokeFilePath);
            if (audioFilename == "") {
                return;
            }
            QFile audioFile(audioFilename);
            if (audioFile.size() == 0) {
                return;
            }
            cdgFile.copy(m_mediaTempDir->path() + QDir::separator() + cdgTmpFile);
            QFile::copy(audioFilename, m_mediaTempDir->path() + QDir::separator() + audTmpFile);
            m_mediaBackendKar.setMediaCdg(m_mediaTempDir->path() + QDir::separator() + cdgTmpFile,
                                          m_mediaTempDir->path() + QDir::separator() + audTmpFile);
            // m_mediaBackendKar.testCdgDecode(); // todo: andth
        }
        ui->labelSinger->setText("Torture run (" + QString::number(++runs) + ")");
    });
    m_timerTest.start(2000);
#endif
}

void MainWindow::writeGstPipelineDiagramToDisk() {
    QString outputFolder = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0);
    m_mediaBackendKar.writePipelinesGraphToFile(outputFolder);
    m_mediaBackendBm.writePipelinesGraphToFile(outputFolder);
    m_mediaBackendSfx.writePipelinesGraphToFile(outputFolder);
}

void MainWindow::comboBoxSearchTypeIndexChanged(int index) {
    switch (index) {
        case 1:
            m_karaokeSongsModel.setSearchType(TableModelKaraokeSongs::SEARCH_TYPE_ARTIST);
            break;
        case 2:
            m_karaokeSongsModel.setSearchType(TableModelKaraokeSongs::SEARCH_TYPE_TITLE);
            break;
        default:
            m_karaokeSongsModel.setSearchType(TableModelKaraokeSongs::SEARCH_TYPE_ALL);
            break;
    }
}

void MainWindow::actionDocumentation() {
    QDesktopServices::openUrl(QUrl("https://docs.openkj.org"));
}

void MainWindow::buttonHistoryPlayClicked() {
    auto selRows = ui->tableViewHistory->selectionModel()->selectedRows();
    if (selRows.empty())
        return;
    auto index = selRows.at(0);
    m_k2kTransition = false;
    if (m_mediaBackendKar.state() == MediaBackend::PlayingState) {
        if (m_settings.showSongInterruptionWarning()) {
            QMessageBox msgBox(this);
            auto *cb = new QCheckBox("Show this warning in the future");
            cb->setChecked(m_settings.showSongInterruptionWarning());
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("Interrupt currently playing karaoke song?");
            msgBox.setInformativeText(
                    "There is currently a karaoke song playing.  If you continue, the current song will be stopped.  Are you sure?");
            QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setCheckBox(cb);
            connect(cb, &QCheckBox::toggled, &m_settings, &Settings::setShowSongInterruptionWarning);
            msgBox.exec();
            if (msgBox.clickedButton() != yesButton) {
                return;
            }
        }
        m_k2kTransition = true;
    }
    if (m_mediaBackendKar.state() == MediaBackend::PausedState) {
        if (m_settings.karaokeAutoAdvance()) {
            m_kAASkip = true;
            cdgWindow->showAlert(false);
        }
        audioRecorder.stop();
        m_mediaBackendKar.stop(true);
    }
    int curSingerId = m_rotModel.getSingerByName(m_historySongsModel.currentSingerName()).id;
    m_curSinger = m_rotModel.getSinger(curSingerId).name;
    m_curArtist = index.sibling(index.row(), 3).data().toString();
    m_curTitle = index.sibling(index.row(), 4).data().toString();
    QString curSongId = index.sibling(index.row(), 5).data().toString();
    QString filePath = index.sibling(index.row(), 2).data().toString();
    int curKeyChange = index.sibling(index.row(), 6).data().toInt();
    ui->labelSinger->setText(m_curSinger);
    ui->labelArtist->setText(m_curArtist);
    ui->labelTitle->setText(m_curTitle);
    m_karaokeSongsModel.updateSongHistory(m_karaokeSongsModel.getIdForPath(filePath));
    play(filePath, m_k2kTransition);
    if (m_settings.treatAllSingersAsRegs() || m_rotModel.getSinger(curSingerId).regular)
        m_historySongsModel.saveSong(m_curSinger, filePath, m_curArtist, m_curTitle, curSongId, curKeyChange);
    m_mediaBackendKar.setPitchShift(curKeyChange);
    m_rotModel.setCurrentSinger(curSingerId);
    m_rotDelegate.setCurrentSinger(curSingerId);
    if (m_settings.rotationAltSortOrder()) {
        auto curSingerPos = m_rotModel.getSinger(curSingerId).position;
        m_curSingerOriginalPosition = curSingerPos;
        if (curSingerPos != 0)
            m_rotModel.singerMove(curSingerPos, 0);
    }
}

void MainWindow::buttonHistoryToQueueClicked() {
    auto selRows = ui->tableViewHistory->selectionModel()->selectedRows();
    if (selRows.empty())
        return;

    std::for_each(selRows.begin(), selRows.end(), [&](auto index) {
        auto path = index.sibling(index.row(), 2).data().toString();
        int curSingerId = m_rotModel.getSingerByName(m_historySongsModel.currentSingerName()).id;
        int key = index.sibling(index.row(), 6).data().toInt();
        int dbSongId = m_karaokeSongsModel.getIdForPath(path);
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
        m_qModel.songAddSlot(dbSongId, curSingerId, key);
    });
    ui->tableViewHistory->clearSelection();
    ui->tabWidgetQueue->setCurrentIndex(0);
}

void MainWindow::tableViewHistoryDoubleClicked([[maybe_unused]]const QModelIndex &index) {
    switch (ui->comboBoxHistoryDblClick->currentIndex()) {
        case 0:
            buttonHistoryToQueueClicked();
            break;
        case 1:
            buttonHistoryPlayClicked();
            break;
    }
}

void MainWindow::tableViewHistoryContextMenu(const QPoint &pos) {
    int selCount = ui->tableViewHistory->selectionModel()->selectedRows().size();
    if (selCount == 1) {
        QModelIndex index = ui->tableViewHistory->indexAt(pos);
        if (index.isValid()) {
            QMenu contextMenu(this);
            contextMenu.addAction("Preview", [&]() {
                QString filename = index.sibling(index.row(), 2).data().toString();
                if (!QFile::exists(filename)) {
                    QMessageBox::warning(this, tr("Missing File!"),
                                         "Specified karaoke file missing, preview aborted!\n\n" + m_dbRtClickFile,
                                         QMessageBox::Ok);
                    return;
                }
                auto *videoPreview = new DlgVideoPreview(filename, this);
                videoPreview->setAttribute(Qt::WA_DeleteOnClose);
                videoPreview->show();
            });
            contextMenu.addAction("Play", this, &MainWindow::buttonHistoryPlayClicked);
            contextMenu.addAction("Add to queue", this, &MainWindow::buttonHistoryToQueueClicked);
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
                m_historySongsModel.deleteSong(songIndex);
                ui->tableViewHistory->clearSelection();
            });
            contextMenu.exec(QCursor::pos());
        }
    } else if (selCount > 1) {
        QMenu contextMenu(this);
        contextMenu.addAction("Add to queue", this, &MainWindow::buttonHistoryToQueueClicked);
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
                m_historySongsModel.deleteSong(index.data().toInt());
            });
            ui->tableViewHistory->clearSelection();
        });

        contextMenu.exec(QCursor::pos());
    }
}

void MainWindow::actionBreakMusicTorture() {
    m_tableModelPlaylists->select();
    bmAddPlaylist("torture");
    m_tableModelPlaylists->select();
    ui->comboBoxBmPlaylists->setCurrentText("torture");
    ui->tableViewBmDb->selectAll();
    auto mimeData = m_tableModelBreakSongs.mimeData(ui->tableViewBmDb->selectionModel()->selectedIndexes());
    auto throwaway = m_tableModelPlaylistSongs.dropMimeData(mimeData, Qt::CopyAction, 0, 3, QModelIndex());

    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static int runs = 0;
        m_logger->info("{} Karaoke torture test timer timeout", m_loggingPrefix);
        ui->tableViewBmPlaylist->selectRow(0);
        tableViewBmPlaylistDoubleClicked(ui->tableViewBmPlaylist->selectionModel()->selectedRows().at(0));
        m_logger->info("{} test runs: ", m_loggingPrefix, ++runs);
    });
    m_timerTest.start(2000);
}

void MainWindow::tableViewBmDbClicked([[maybe_unused]]const QModelIndex &index) {
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

void MainWindow::actionBurnInEosJump() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    m_testMode = true;
    emit ui->buttonClearRotation->clicked();
    for (auto i = 0; i < 21; i++) {
        auto singerName = "Test Singer " + QString::number(i);
        m_rotModel.singerAdd(singerName);
        // m_rotModel.regularDelete(singerName);
    }
    connect(&m_timerTest, &QTimer::timeout, [&]() {
        QApplication::beep();
        static bool playing = false;
        static int runs = 0;
        m_rotModel.singerMove(QRandomGenerator::global()->bounded(0, 19), QRandomGenerator::global()->bounded(0, 19));
        ui->tableViewRotation->selectRow(QRandomGenerator::global()->bounded(0, 19));
        int randomNum{0};
        if (m_karaokeSongsModel.rowCount(QModelIndex()) > 1)
            randomNum = QRandomGenerator::global()->bounded(0, m_karaokeSongsModel.rowCount(QModelIndex()) - 1);
        m_logger->info("{} randomNum: {}", m_loggingPrefix, randomNum);
        ui->tableViewDB->selectRow(randomNum);
        ui->tableViewDB->scrollTo(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        emit ui->tableViewDB->doubleClicked(ui->tableViewDB->selectionModel()->selectedRows().at(0));
        ui->tableViewQueue->selectRow(m_qModel.rowCount() - 1);
        ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        if (m_qModel.rowCount() > 2) {
            auto newPos = QRandomGenerator::global()->bounded(0, m_qModel.rowCount() - 1);
            m_qModel.move(m_qModel.rowCount() - 1, newPos);
            ui->tableViewQueue->selectRow(newPos);
            ui->tableViewQueue->scrollTo(ui->tableViewQueue->selectionModel()->selectedRows().at(0));
        }
        auto idx = ui->tableViewQueue->selectionModel()->selectedRows().at(0);
        emit ui->tableViewQueue->doubleClicked(idx);
        ui->tableViewQueue->selectRow(idx.row());
        playing = true;
        QTimer::singleShot(500, [&]() {
            auto duration = m_mediaBackendKar.duration();
            auto jumpPoint = duration - 10000;
            m_mediaBackendKar.setPosition(jumpPoint);
        });
        m_logger->info("{} Burn in test cycle: {}", m_loggingPrefix, ++runs);
        ui->labelSinger->setText("Torture run (" + QString::number(runs) + ")");
    });
    m_timerTest.start(13000);
#endif
}

void MainWindow::showAddSingerDialog() {
    auto dlgAddSinger = findChild<DlgAddSinger *>(QString(), Qt::FindDirectChildrenOnly);
    if (dlgAddSinger == nullptr)
    {
        dlgAddSinger = new DlgAddSinger(m_rotModel, this);
        connect(dlgAddSinger, &DlgAddSinger::finished, dlgAddSinger, &DlgAddSinger::deleteLater);
        connect(dlgAddSinger, &DlgAddSinger::newSingerAdded, [&](auto pos) {
            ui->tableViewRotation->selectRow(pos);
            ui->lineEdit->setFocus();
        });
        dlgAddSinger->show();
    } else
        dlgAddSinger->raise();
}





