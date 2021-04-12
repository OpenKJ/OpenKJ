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


#include "okarchive.h"
#include <QDebug>
#include <QFile>
#include <QBuffer>
#include <QTemporaryDir>
#ifdef Q_OS_WIN
#include <io.h>
#else
#include <QProcess>
#include <unistd.h>
#endif

QString infoZipPath;

OkArchive::OkArchive(QString ArchiveFile, QObject *parent) : QObject(parent)
{
    process = new QProcess();
    archiveFile = ArchiveFile;
    qInfo() << "OkArchive opening file: " << archiveFile;
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    goodArchive = false;
    audioFileName = "";
    cdgFileName = "";
    audioExt = "";
    m_entriesProcessed = false;
    lastError = "";
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
    audioExtensions.append(".flac");
#ifdef Q_OS_WIN
    infoZipPath = "unzip.exe";
#else
    infoZipPath = "/usr/bin/unzip";
#endif

}

OkArchive::OkArchive(QObject *parent) : QObject(parent)
{
    process = new QProcess();
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    goodArchive = false;
    audioFileName = "";
    cdgFileName = "";
    audioExt = "";
    m_entriesProcessed = false;
    lastError = "";
    audioExtensions.append(".mp3");
    audioExtensions.append(".wav");
    audioExtensions.append(".ogg");
    audioExtensions.append(".mov");
    audioExtensions.append(".flac");
#ifdef Q_OS_WIN
    infoZipPath = "unzip.exe";
#else
    infoZipPath = "/usr/bin/unzip";
#endif
}

OkArchive::~OkArchive()
{
    delete process;
}

unsigned int OkArchive::getSongDuration()
{
    if ((m_cdgFound) && (m_cdgSize > 0))
        return ((m_cdgSize / 96) / 75) * 1000;
    else if (findCDG())
        return ((m_cdgSize / 96) / 75) * 1000;
    else
        return 0;
}

QByteArray OkArchive::getCDGData()
{
    if (findCDG())
    {
        QTemporaryDir dir;
        if (!extractFile(cdgFileName, dir.path(), "tmp.cdg"))
            return QByteArray();
        QByteArray data;
        QFile cdg(dir.path() + QDir::separator() + "tmp.cdg");
        cdg.open(QFile::ReadOnly);
        data = cdg.readAll();
        cdg.close();
        return data;
    }
    return QByteArray();
}

QString OkArchive::getArchiveFile() const
{
    return archiveFile;
}

void OkArchive::setArchiveFile(const QString &value)
{
//    qInfo() << "OkArchive opening archive file: " << value;
    archiveFile = value;
    m_cdgFound = false;
    m_audioFound = false;
    m_cdgSize = 0;
    m_audioSize = 0;
    m_entries.clear();
    audioFileName = "";
    cdgFileName = "";
    audioExt = "";
    m_entriesProcessed = false;
    lastError = "";
}

bool OkArchive::checkCDG()
{
    if (!findCDG())
        return false;
    if (m_cdgSize <= 0)
        return false;
    return true;
}

bool OkArchive::checkAudio()
{
    if (!findAudio())
        return false;
    if (m_audioSize <= 0)
        return false;
    return true;
}

QString OkArchive::audioExtension()
{
    return audioExt;
}

bool OkArchive::extractAudio(QString destPath, QString destFile)
{
    if (findAudio())
    {
        if (extractFile(audioFileName, destPath, destFile))
            return true;
    }
    return false;
}

bool OkArchive::extractCdg(QString destPath, QString destFile)
{
    if (findCDG())
    {
        if (extractFile(cdgFileName, destPath, destFile))
            return true;
    }
    return false;
}

bool OkArchive::isValidKaraokeFile()
{
//    if (!zipIsValid())
//    {
//        qInfo() << archiveFile << " - Archive is corrupt or invalid";
//        lastError = "Corrupt or invalid archive";
//        return false;
//    }
    if (!findEntries())
    {
        if (!goodArchive)
        {
            qInfo() << archiveFile << " - Invalid or corrupt zip file";
            lastError = "Invalid or corrupt zip file";
            return false;
        }
        if (!m_cdgFound)
        {
            qInfo() << archiveFile << " - Missing CDG file";
            lastError = "CDG not found in zip file";
        }
        if (!m_audioFound)
        {
            qInfo() << archiveFile << " - Missing audio file";
            lastError = "Audio file not found in zip file";
        }
        return false;
    }
    if (m_audioSize <= 0)
    {
        qInfo() << archiveFile << " - Zero byte audio file";
        lastError = "Zero byte audio file";
        return false;
    }
    if (m_cdgSize <= 0)
    {
        qInfo() << archiveFile << " - Zero byte CDG file";
        lastError = "Zero byte CDG file";
        return false;
    }
    return true;
}

QString OkArchive::getLastError()
{
    return lastError;
}

bool OkArchive::findCDG()
{
    findEntries();
    if (m_cdgFound)
        return true;
    return false;
}

bool OkArchive::findAudio()
{
    findEntries();
    if (m_audioFound)
        return true;
    return false;
}

bool OkArchive::findEntries()
{
    if (m_entriesProcessed && (!m_audioFound || !m_cdgFound))
        return false;
    if (m_audioFound && m_cdgFound)
        return true;
    getZipContents();
    for (int i=0; i < m_entries.size(); i++)
    {

        QString fileName = m_entries.at(i).fileName;
        if (fileName.endsWith(".cdg",Qt::CaseInsensitive))
        {
            cdgFileName = fileName;
            m_cdgSize = m_entries.at(i).fileSize;
            m_cdgFound = true;
        }
        else
        {
            for (int e=0; e < audioExtensions.size(); e++)
            {
                if (fileName.endsWith(audioExtensions.at(e), Qt::CaseInsensitive))
                {
                    audioFileName = fileName;
                    audioExt = audioExtensions.at(e);
                    m_audioSize = m_entries.at(i).fileSize;
                    m_audioFound = true;
                }
            }
        }
        if (m_audioFound && m_cdgFound)
        {
            return true;
        }

    }
    return false;
}

zipEntries OkArchive::getZipContents()
{
    if (m_entriesProcessed)
        return m_entries;
    QStringList arguments;
    arguments << "-l";
    arguments << archiveFile;
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->setProgram(infoZipPath);
    process->setArguments(arguments);
    process->start(QProcess::ReadOnly);
    process->waitForFinished();
    if (process->exitCode() == 0)
    {
        // no error
        goodArchive = true;
    }
    else if (process->exitCode() <= 2)
    {
        qInfo() << "Non-fatal error while processing zip: " << archiveFile;
        qInfo() << "infozip returned error code: " << process->exitCode();
        goodArchive = true;
    }
    else if (process->exitCode() >= 3)
    {
        qInfo() << "Fatal error while processing zip: " << archiveFile;
        qInfo() << "infozip returned error code: " << process->exitCode();
        goodArchive = false;
        return zipEntries();
    }
    QString output = process->readAll();
    if (output.contains("zipfile is empty", Qt::CaseInsensitive))
    {
        qInfo() << "Zip file is empty, skipping";
        goodArchive = false;
        return zipEntries();
    }
    qInfo() << "getZipContents() - infozip output: " << output;
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList data = output.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
#else
    QStringList data = output.split(QRegExp("[\r\n]"),Qt::SkipEmptyParts);
#endif
    int fnStart = 0;
    int listStart = 0;
    for (int l=0; l < data.size(); l++)
    {
        if (data.at(l).contains("Name", Qt::CaseInsensitive) && data.at(l).contains("Length", Qt::CaseInsensitive) && data.at(l).contains("Date", Qt::CaseInsensitive))
        {
            fnStart = data.at(l).indexOf("Name");
            listStart = l;
            break;
        }
    }
    for (int l=0; l < listStart; l++)
        data.removeFirst();
    if (data.size() < 6)
    {
        qInfo() << "Error processing infozip output, bailing out";
        goodArchive = false;
        return zipEntries();
    }
    data.removeFirst();
    data.removeFirst();
    data.removeLast();
    data.removeLast();
    for (int i=0; i < data.size(); i++)
    {
        zipEntry entry;
        int fnOffset = data.at(i).size() - fnStart;
        entry.fileName = data.at(i).right(fnOffset);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        entry.fileSize = data.at(i).split(" ", QString::SkipEmptyParts).at(0).toInt();
#else
        entry.fileSize = data.at(i).split(' ', Qt::SkipEmptyParts).at(0).toInt();
#endif
        m_entries.append(entry);
    }
    m_entriesProcessed = true;
    return m_entries;
}

bool OkArchive::extractFile(QString fileName, QString destDir, QString destFile)
{
    qInfo() << "OkArchive(" << fileName << ", " << destDir << ", " << destFile << ") called";
    QTemporaryDir tmpDir;
    QString tmpZipPath = tmpDir.path() + QDir::separator() + "tmp.zip";
    if (!QFile::copy(archiveFile, tmpZipPath))
    {
        qInfo() << "error copying zip";
        return false;
    }
    QString ext = "*" + fileName.right(4);
    qInfo() << "Finding first file with extension: " << ext;
    QStringList arguments;
    arguments << "-j";
    arguments << tmpZipPath;
    arguments << ext;
    arguments << "-d";
    arguments << destDir;
    process->start(infoZipPath, arguments, QProcess::ReadOnly);
    process->waitForFinished();
    if (process->exitCode() == 0)
    {
        // no error
    }
    else if (process->exitCode() <= 2)
    {
        qInfo() << "Non-fatal error while processing zip: " << archiveFile;
        qInfo() << "infozip returned error code: " << process->exitCode();
    }
    else if (process->exitCode() >= 3)
    {
        qInfo() << "Fatal error while processing zip: " << archiveFile;
        qInfo() << "infozip returned error code: " << process->exitCode();
        return false;
    }
    if (!QFile::rename(destDir + QDir::separator() + fileName, destDir + QDir::separator() + destFile))
    {
        qInfo() << "infozip didn't report fatal error, but file was not unzipped successfully";
        return false;
    }
    return true;

}

bool OkArchive::zipIsValid()
{
    QProcess *process = new QProcess(this);
    QStringList arguments;
    arguments << "-t";
    arguments << archiveFile;
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->start(infoZipPath, arguments, QProcess::ReadOnly);
    process->waitForFinished();
    //qInfo() << process->readAll();
    qInfo() << process->state();
    qInfo() << process->error();
    if (process->exitCode() != 0)
    {
        process->close();
        delete process;
        return false;
    }
    else
    {
        process->close();
        delete process;
        return true;
    }
}


