// Uses an in-memory database and test-mode settings; never opens the host library.
#include "dlgaddsong.h"
#include <QApplication>
#include <QComboBox>
#include <QListWidget>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTest>
#include <spdlog/sinks/null_sink.h>
#include <cstdio>
#include <stdexcept>

static void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    spdlog::null_logger_mt("logger");
    auto database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(":memory:");
    try {
        require(database.open(), "Cannot open isolated database");
        QSqlQuery query;
        require(query.exec("CREATE TABLE historySingers (name TEXT)"), "Cannot create test history");
        TableModelKaraokeSongs library;
        TableModelQueueSongs queue(library);
        TableModelRotation rotation;
        int singleColumnWidth = 0;
        int twoColumnWidth = 0;
        int threeColumnWidth = 0;
        for (int count : {0, 1, 14, 15, 16, 30, 31, 45, 46, 61}) {
            QList<okj::RotationSinger> singers;
            // Deliberately reverse rotation order and use IDs unrelated to list rows.
            for (int row = count - 1; row >= 0; --row)
                singers.append(okj::RotationSinger(101 + row, QString("Singer %1").arg(row + 1, 2, 10, QLatin1Char('0')),
                                                   count - 1 - row, false, QDateTime::currentDateTime()));
            rotation.onRotationLoaded(singers);
            QPointer<DlgAddSong> dialog = new DlgAddSong(rotation, queue, 777);
            dialog->show();
            QTest::qWait(40);
            auto *list = dialog->findChild<QListWidget *>("listWidgetRotSingers");
            auto *existing = dialog->findChild<QRadioButton *>("radioButtonAddToExistingSinger");
            require(list && list->count() == count, "List must contain the rotation singers");
            auto *positionControl = dialog->findChild<QComboBox *>("comboBoxPosition");
            auto *keyControl = dialog->findChild<QSpinBox *>("spinBoxKeyChange");
            const auto positionRect = QRect(positionControl->mapTo(dialog, QPoint()), positionControl->size());
            const auto keyRect = QRect(keyControl->mapTo(dialog, QPoint()), keyControl->size());
            if (count <= 15)
                require(keyRect.top() > positionRect.bottom(), "One column must put Key Change below Singer Position");
            else
                require(keyRect.left() > positionRect.right() &&
                        keyRect.top() <= positionRect.bottom() && keyRect.bottom() >= positionRect.top(),
                        "Multiple columns must put Key Change beside Singer Position");
            if (count == 15)
                singleColumnWidth = dialog->width();
            if (count == 16) {
                twoColumnWidth = dialog->width();
                require(twoColumnWidth > singleColumnWidth, "Two singer columns must open wider than one");
            }
            if (count == 31) {
                threeColumnWidth = dialog->width();
                require(threeColumnWidth > twoColumnWidth, "Three singer columns must open wider than two");
            }
            if (count > 45)
                require(dialog->width() == threeColumnWidth, "Additional columns must not enlarge the default window");
            require(dialog->findChild<QRadioButton *>("radioButtonAddSinger")->isChecked(),
                    "Opening the list must not change the initial new-singer mode");
            if (!count) {
                require(!existing->isEnabled() && !list->isEnabled(), "Empty rotation must disable existing-singer mode");
                dialog->close();
                QTest::qWait(10);
                continue;
            }
            require(list->item(0)->text() == "Singer 01", "List must preserve alphabetical ordering");
            const auto first = list->visualItemRect(list->item(0));
            require(first.size() == list->gridSize(), "Item selection must fill the entire row cell");
            if (count >= 15) {
                const auto fifteenth = list->visualItemRect(list->item(14));
                require(fifteenth.left() == first.left() && fifteenth.bottom() < list->viewport()->height(),
                        "The first column must show fifteen complete rows");
            }
            if (count >= 16) {
                const auto sixteenth = list->visualItemRect(list->item(15));
                require(sixteenth.left() > first.left() && sixteenth.top() == first.top(),
                        "The sixteenth singer must start the second column");
            }
            if (count >= 31) {
                const auto thirdColumn = list->visualItemRect(list->item(30));
                require(thirdColumn.right() < list->viewport()->width() && thirdColumn.top() == first.top(),
                        "Three complete singer columns must be visible before scrolling");
            }
            if (count > 45)
                require(list->visualItemRect(list->item(45)).left() >= 3 * list->gridSize().width(),
                        "The forty-sixth singer must start a fourth column");
            if (count == 16) {
                auto font = dialog->font();
                font.setPointSize(20);
                dialog->setFont(font);
                dialog->resize(dialog->width() + 120, dialog->height());
                QTest::qWait(40);
                require(list->visualItemRect(list->item(14)).bottom() < list->viewport()->height() &&
                        list->visualItemRect(list->item(15)).top() == list->visualItemRect(list->item(0)).top(),
                        "Resizing and larger fonts must retain fifteen rows per column");
            }
            if (count > 45) {
                require(list->horizontalScrollBar()->maximum() > 0, "Large rotations must remain scrollable");
                list->scrollToItem(list->item(count - 1));
                QTest::qWait(10);
            }
            const int selected = count > 45 ? count - 1 : (count > 15 ? 15 : 0);
            list->scrollToItem(list->item(selected));
            QTest::qWait(10);
            QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                              list->visualItemRect(list->item(selected)).center());
            require(existing->isChecked(), "Clicking even the initially selected singer must activate existing mode");
            dialog->findChild<QSpinBox *>("spinBoxKeyChange")->setValue(-3);
            int singerId = -1, songId = -1, keyChange = 0, position = -1, requests = 0;
            const auto request = QObject::connect(&queue, &TableModelQueueSongs::requestAddSongToQueue,
                [&](int singer, int song, int key) { singerId = singer; songId = song; keyChange = key; ++requests; });
            QObject::connect(dialog, &DlgAddSong::newSingerAdded, [&](int value) { position = value; });
            if (app.arguments().size() > 1)
                dialog->grab().save(app.arguments().at(1) + QString("-%1.png").arg(count));
            if (count == 15)
                QTest::keyClick(list, Qt::Key_Return);
            else
                QTest::mouseClick(dialog->findChild<QPushButton *>("pushButtonAdd"), Qt::LeftButton);
            require(singerId == 101 + selected && songId == 777 && keyChange == -3 && requests == 1,
                    "Add must target the selected singer ID and preserve song/key");
            require(position == count - 1 - selected, "Add must preserve the rotation-position notification");
            QObject::disconnect(request);
            QTest::qWait(10);
        }
        std::puts("Add Song singer selection and column layout passed");
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
