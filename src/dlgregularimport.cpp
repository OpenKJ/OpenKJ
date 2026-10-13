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

#include "dlgregularimport.h"
#include "ui_dlgregularimport.h"
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSqlQuery>
#include <QXmlStreamReader>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QInputDialog>
#include <QFileInfo>
#include <QPushButton>
#include "regularpathmapping.h"

namespace {
QString matchKey(const QString &songId, const QString &artist, const QString &title)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray{songId.toCaseFolded(),
        artist.toCaseFolded(), title.toCaseFolded()}).toJson(QJsonDocument::Compact));
}
}


DlgRegularImport::DlgRegularImport(TableModelKaraokeSongs &karaokeSongsModel, QWidget *parent) :
    m_karaokeSongsModel(karaokeSongsModel),
    QDialog(parent),
    ui(new Ui::DlgRegularImport)
{
    ui->setupUi(this);
    m_curImportFile = "";
    m_rootGroup = new QGroupBox(tr("Song folders"), this);
    auto *layout = new QVBoxLayout(m_rootGroup);
    auto *help = new QLabel(tr("Choose where each exported folder lives on this computer. "
                              "Unavailable songs keep their history and are reported after import."));
    help->setWordWrap(true);
    layout->addWidget(help);
    m_rootTable = new QTableWidget(0, 3, m_rootGroup);
    m_rootTable->setObjectName("tableRootMappings");
    m_rootTable->setHorizontalHeaderLabels({tr("Exported root / old folder"), tr("Folder on this computer"), tr("Choose")});
    m_rootTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_rootTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_rootTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    layout->addWidget(m_rootTable);
    auto *add = new QPushButton(tr("Add old folder mapping"), m_rootGroup);
    layout->addWidget(add);
    connect(add, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString source = QInputDialog::getText(this, tr("Old song folder"),
            tr("Folder prefix in the export (for example /Volumes/Karaoke Drive):"),
            QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok || source.isEmpty()) return;
        const QString clean = RegularPaths::normalized(source);
        if (!RegularPaths::absolute(clean)) {
            QMessageBox::warning(this, tr("Invalid folder"), tr("Enter an absolute folder path."));
            return;
        }
        for (int row = 0; row < m_rootTable->rowCount(); ++row)
            if (m_rootTable->item(row, 0)->text() == clean) return;
        addRootMapping(clean, clean);
    });
    ui->verticalLayout->insertWidget(1, m_rootGroup);
    m_rootGroup->hide();
    ui->pushButtonImport->setEnabled(false);
    ui->pushButtonImportAll->setEnabled(false);
    resize(780, 600);
}

DlgRegularImport::~DlgRegularImport()
{
    delete ui;
}

void DlgRegularImport::on_pushButtonSelectFile_clicked()
{
#ifdef Q_OS_LINUX
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select file to load regulars from"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "OpenKJ export files (*.xml *.json)",
                                                      nullptr, QFileDialog::DontUseNativeDialog);
#else
    QString importFile = QFileDialog::getOpenFileName(this,tr("Select file to load regulars from"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "OpenKJ export files (*.xml *.json)",
                                                      nullptr);
#endif
    if (importFile != "")
    {
        m_curImportFile = importFile;
        m_importSingers = {};
        m_mappings.clear();
        m_rootTable->setRowCount(0);
        m_rootGroup->hide();
        QStringList singers;
        if (m_curImportFile.endsWith("xml", Qt::CaseInsensitive))
            singers = legacyLoadSingerList(importFile);
        else
            singers = loadSingerList(importFile);
        ui->listWidgetRegulars->clear();
        ui->listWidgetRegulars->addItems(singers);
        ui->pushButtonImport->setEnabled(!singers.isEmpty());
        ui->pushButtonImportAll->setEnabled(!singers.isEmpty());
    }
}

void DlgRegularImport::on_pushButtonClose_clicked()
{
    close();
}

void DlgRegularImport::on_pushButtonImport_clicked()
{
    QStringList errors;
    if (ui->listWidgetRegulars->selectedItems().size() < 1)
        return;

    if (!m_curImportFile.endsWith("xml", Qt::CaseInsensitive) && !readMappings())
        return;
    m_historySingersModel.loadSingers();
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setStandardButtons(QFlags<QMessageBox::StandardButton>());
    msgBox->setText(tr("Importing regular singers, please wait..."));
    msgBox->show();
    for (int i=0; i < ui->listWidgetRegulars->selectedItems().size(); i++)
    {
        QString name = ui->listWidgetRegulars->selectedItems().at(i)->text();
        if (m_historySingersModel.exists(name))
        {
            QMessageBox msgBox;
            auto mergeBtn = msgBox.addButton("Merge", QMessageBox::ActionRole);
            auto replaceBtn = msgBox.addButton("Replace", QMessageBox::DestructiveRole);
            auto skipBtn = msgBox.addButton("Skip", QMessageBox::RejectRole);
            msgBox.setDefaultButton(mergeBtn);
            msgBox.setWindowTitle("Naming conflict");
            msgBox.setText("An existing singer named \"" + name + "\" already exists.\nHow would you like to proceed?");
            msgBox.exec();
            if (msgBox.clickedButton() == skipBtn)
                continue;
            if (msgBox.clickedButton() == replaceBtn)
            {
                m_historySingersModel.deleteHistory(m_historySingersModel.getId(name));
            }
        }
        msgBox->setInformativeText(tr("Importing singer: ") + name);
        if (m_curImportFile.endsWith("xml", Qt::CaseInsensitive))
            errors.append(legacyImportSinger(ui->listWidgetRegulars->selectedItems().at(i)->text()));
        else
            errors.append(importSinger(ui->listWidgetRegulars->selectedItems().at(i)->text()));
    }
    msgBox->close();
    delete msgBox;

    if (errors.size() > 0)
    {
        QMessageBox msgBox;
        msgBox.addButton(QMessageBox::StandardButton::Ok);
        msgBox.setDetailedText(errors.join("\n"));
        msgBox.setText("Some songs are unavailable or could not be matched. Their history was preserved where possible. See Details.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }

    QMessageBox::information(this, tr("Import complete"), tr("Regular singer import complete."));
    ui->listWidgetRegulars->clearSelection();
}

void DlgRegularImport::on_pushButtonImportAll_clicked()
{
    ui->listWidgetRegulars->selectAll();
    on_pushButtonImport_clicked();
}

QStringList DlgRegularImport::legacyLoadSingerList(const QString &fileName)
{
    QStringList singers;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Cannot load export"), file.errorString());
        return {};
    }
    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("singer"))
            singers << xml.attributes().value("name").toString();
    }
    if (xml.hasError()) {
        QMessageBox::warning(this, tr("Cannot load export"), xml.errorString());
        return {};
    }
    singers.sort();
    return singers;
}

QStringList DlgRegularImport::loadSingerList(const QString &filename)
{
    auto fail = [this](const QString &error) {
        QMessageBox::warning(this, tr("Cannot load export"), error);
        m_importSingers = {};
        m_rootTable->setRowCount(0);
        return QStringList{};
    };
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) return fail(file.errorString());
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) return fail(error.errorString());
    QJsonObject roots;
    if (document.isArray()) {
        m_importSingers = document.array();
    } else if (document.isObject()) {
        const auto object = document.object();
        if (object.value("formatVersion").toDouble() != 2 || !object.value("roots").isObject() ||
            !object.value("singers").isArray())
            return fail(tr("Unsupported regular-singer export format."));
        roots = object.value("roots").toObject();
        m_importSingers = object.value("singers").toArray();
        for (auto it = roots.begin(); it != roots.end(); ++it) {
            if (!QRegularExpression("^[A-Za-z0-9_]+$").match(it.key()).hasMatch() ||
                !it.value().isString() || !RegularPaths::absolute(RegularPaths::normalized(it.value().toString())))
                return fail(tr("The export contains an invalid root folder."));
        }
    } else return fail(tr("Expected a regular-singer JSON export."));

    QStringList singers;
    QStringList names;
    for (const auto &value : m_importSingers) {
        const auto singer = value.toObject();
        const QString name = singer.value("name").toString();
        if (name.trimmed().isEmpty() || names.contains(name.toCaseFolded()) || !singer.value("songs").isArray())
            return fail(tr("The export contains an invalid or duplicate singer."));
        for (const auto &song : singer.value("songs").toArray()) {
            const QString path = song.toObject().value("filepath").toString();
            if (path.isEmpty()) return fail(tr("A song has no file path."));
            if (path.startsWith('%')) {
                const auto token = QRegularExpression("^%([A-Za-z0-9_]+)%/").match(path);
                if (!token.hasMatch() || !roots.contains(token.captured(1)))
                    return fail(tr("A song refers to an unknown root folder."));
            } else if (!RegularPaths::absolute(RegularPaths::normalized(path)))
                return fail(tr("A song has an invalid file path."));
        }
        singers.append(name);
        names.append(name.toCaseFolded());
    }
    for (auto it = roots.begin(); it != roots.end(); ++it)
        addRootMapping('%' + it.key() + '%', it.value().toString(), it.value().toString());
    m_rootGroup->show();
    return singers;
}

void DlgRegularImport::addRootMapping(const QString &source, const QString &destination, const QString &original)
{
    const int row = m_rootTable->rowCount();
    m_rootTable->insertRow(row);
    auto *sourceItem = new QTableWidgetItem(source);
    sourceItem->setFlags(sourceItem->flags() & ~Qt::ItemIsEditable);
    sourceItem->setToolTip(original.isEmpty() ? source : original);
    m_rootTable->setItem(row, 0, sourceItem);
    m_rootTable->setItem(row, 1, new QTableWidgetItem(destination));
    auto *browse = new QPushButton(tr("Browse…"));
    m_rootTable->setCellWidget(row, 2, browse);
    connect(browse, &QPushButton::clicked, this, [this, row]() {
        const QString folder = QFileDialog::getExistingDirectory(this, tr("Choose song folder"),
            m_rootTable->item(row, 1)->text());
        if (!folder.isEmpty()) m_rootTable->item(row, 1)->setText(folder);
    });
}

bool DlgRegularImport::readMappings()
{
    m_mappings.clear();
    m_libraryMatches.clear();
    m_libraryIndexed = false;
    for (int row = 0; row < m_rootTable->rowCount(); ++row) {
        const QString destination = m_rootTable->item(row, 1)->text().trimmed();
        if (destination.isEmpty() || !RegularPaths::absolute(RegularPaths::normalized(destination))) {
            QMessageBox::warning(this, tr("Choose a folder"), tr("Each root needs an absolute destination folder."));
            return false;
        }
        m_mappings.insert(m_rootTable->item(row, 0)->text(), destination);
    }
    // Validate paths before the Replace option can delete existing history.
    for (const auto &singer : m_importSingers)
        for (const auto &song : singer.toObject().value("songs").toArray())
            if (RegularPaths::resolve(song.toObject().value("filepath").toString(), m_mappings).isEmpty()) {
                QMessageBox::warning(this, tr("Invalid song path"), tr("A song path escapes its root folder or cannot be mapped."));
                return false;
            }
    return true;
}

QString DlgRegularImport::matchingLibraryPath(const QJsonObject &song, QString &problem)
{
    if (song.value("songid").toString().isEmpty() || song.value("artist").toString().isEmpty() ||
        song.value("title").toString().isEmpty()) {
        problem = tr("File unavailable; insufficient metadata to match, history retained");
        return {};
    }
    if (!m_libraryIndexed) {
        QSqlQuery query("SELECT discid,artist,title,path FROM dbsongs");
        while (query.next())
            m_libraryMatches[matchKey(query.value(0).toString(), query.value(1).toString(), query.value(2).toString())]
                .append(query.value(3).toString());
        m_libraryIndexed = true;
    }
    QStringList available;
    const auto candidates = m_libraryMatches.value(matchKey(song.value("songid").toString(),
        song.value("artist").toString(), song.value("title").toString()));
    for (const auto &path : candidates)
        if (QFileInfo(path).isFile() && !available.contains(path)) available.append(path);
    if (available.size() == 1) return available.first();
    problem = available.isEmpty() ? tr("File unavailable; history retained")
                                  : tr("Multiple library matches; history retained without choosing a file");
    return {};
}

QStringList DlgRegularImport::legacyImportSinger(const QString &name)
{
    QStringList missingSongs;
    QFile *xmlFile = new QFile(m_curImportFile);
    xmlFile->open(QIODevice::ReadOnly);
    QXmlStreamReader xml(xmlFile);
    bool done = false;
    while ((!xml.atEnd()) && (!done))
    {
        QApplication::processEvents();
        xml.readNext();
        if ((xml.isStartElement()) && (xml.name() == QLatin1String("singer")) && (xml.attributes().value("name") == name))
        {
            xml.readNext();
            int position = 0;
            while ((xml.name() != QLatin1String("singer")) && (!xml.atEnd()))
            {
                if ((xml.isStartElement()) && (xml.name() == QLatin1String("song")))
                {
                    QApplication::processEvents();
                    QSqlQuery query;
                    QString songId = xml.attributes().value("discid").toString();
                    QString artist = xml.attributes().value("artist").toString();
                    QString title = xml.attributes().value("title").toString();
                    int keyChg = xml.attributes().value("key").toInt();

                    QString sql = "SELECT path FROM dbsongs WHERE artist == \"" + artist + "\" AND title == \"" + title + "\" AND discid == \"" + songId + "\" LIMIT 1";
                    query.exec(sql);
                    if (query.first())
                    {
                        QString path = query.value(0).toString();
                        m_historySongsModel.saveSong(name,path,artist,title,songId,keyChg);
                        position++;
                    }
                    else
                    {
                        QString vendorPart;
                        for (int i=0; i < songId.size(); i++)
                        {
                            QChar character = songId.at(i);
                            if (character.isLetter())
                                vendorPart.append(character);
                            else
                                break;
                        }
                       sql = "SELECT path FROM dbsongs WHERE artist == \"" + artist + "\" AND title == \"" + title + "\" AND discid LIKE \"%" + vendorPart + "%\" LIMIT 1";
                       query.exec(sql);
                       if (query.first())
                       {
                           QString path = query.value(0).toString();
                           m_historySongsModel.saveSong(name,path,artist,title,songId,keyChg);
                           position++;
                       }
                       else
                       {
                           query.prepare("SELECT path FROM dbsongs WHERE discid = :discid");
                           query.bindValue(":discid", songId);
                           query.exec();
                           if (query.first())
                           {
                               QString path = query.value(0).toString();
                               m_historySongsModel.saveSong(name,path,artist,title,songId,keyChg);
                           }
                           else
                               missingSongs.append("Song: \"" + songId + " - " + artist + " - " + title + "\" Missing for singer: " + name);
                       }
                    }
                }
                xml.readNext();
            }
            done = true;
        }
    }
    xmlFile->close();
    return missingSongs;
}

QStringList DlgRegularImport::importSinger(const QString &name)
{
    QStringList warnings;
    const auto match = std::find_if(m_importSingers.begin(), m_importSingers.end(), [&name](const QJsonValue &value) {
        return value.toObject().value("name").toString() == name;
    });
    if (match == m_importSingers.end()) return warnings;
    if (m_historySongsModel.getSingerId(name) == -1) {
        if (m_historySongsModel.addSinger(name) <= 0)
            return {tr("Unable to create singer: %1").arg(name)};
    }
    for (const auto &value : match->toObject().value("songs").toArray()) {
        const auto song = value.toObject();
        QString path = RegularPaths::resolve(song.value("filepath").toString(), m_mappings);
        if (!QFileInfo(path).isFile()) {
            QString problem;
            const QString found = matchingLibraryPath(song, problem);
            if (!found.isEmpty()) path = found;
            else warnings.append(tr("%1 — %2: %3 (%4)").arg(name, song.value("title").toString(), problem, path));
        }
        QDateTime lastPlay = QDateTime::fromString(song.value("lastplay").toString(), Qt::ISODateWithMs);
        if (!lastPlay.isValid()) lastPlay = QDateTime::fromString(song.value("lastplay").toString(), Qt::TextDate);
        m_historySongsModel.saveSong(name, path, song.value("artist").toString(), song.value("title").toString(),
            song.value("songid").toString(), song.value("keychange").toInt(), song.value("plays").toInt(), lastPlay);
    }
    m_historySingersModel.loadSingers();
    return warnings;
}


void DlgRegularImport::closeEvent([[maybe_unused]]QCloseEvent *event)
{
    deleteLater();
}


void DlgRegularImport::done(int)
{
    deleteLater();
}
