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
#include <QScreen>
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
        for (const auto config : {QPair<int, int>{13, 0}, {13, 12}, {20, 12}}) {
            auto font = app.font();
            font.setPointSize(config.first);
            app.setFont(font);
            for (int count : {0, 1, 9, 10, 11, 20, 21, 40, 61}) {
                QList<okj::RotationSinger> singers;
                // Deliberately reverse rotation order and use IDs unrelated to list rows.
                for (int row = count - 1; row >= 0; --row)
                    singers.append(okj::RotationSinger(101 + row, QString("Singer %1").arg(row + 1, 2, 10, QLatin1Char('0')) + QString(row == 0 ? config.second : 0, QLatin1Char('W')),
                                                       count - 1 - row, false, QDateTime::currentDateTime()));
                rotation.onRotationLoaded(singers);
                QPointer<DlgAddSong> dialog = new DlgAddSong(rotation, queue, 777);
                dialog->show();
                QTest::qWait(40);
                auto *list = dialog->findChild<QListWidget *>("listWidgetRotSingers");
                auto *existing = dialog->findChild<QRadioButton *>("radioButtonAddToExistingSinger");
                require(list && list->count() == count, "List must contain the rotation singers");
                const auto checkFixedHeight = [&] {
                    require(dialog->minimumHeight() == dialog->maximumHeight() &&
                            dialog->height() == dialog->minimumHeight(),
                            "Dialog height must be fixed to the current content");
                    const int contentHeight = dialog->height();
                    dialog->resize(dialog->width(), contentHeight + 100);
                    require(dialog->height() == contentHeight, "Users must not be able to resize the dialog vertically");
                };
                checkFixedHeight();
                auto *positionControl = dialog->findChild<QComboBox *>("comboBoxPosition");
                auto *keyControl = dialog->findChild<QSpinBox *>("spinBoxKeyChange");
                const auto checkControlLayout = [&](bool stacked) {
                    const auto positionRect = QRect(positionControl->mapTo(dialog, QPoint()), positionControl->size());
                    const auto keyRect = QRect(keyControl->mapTo(dialog, QPoint()), keyControl->size());
                    if (stacked)
                        require(keyRect.top() > positionRect.bottom(), "Narrow dialogs must stack the control sections");
                    else
                        require(keyRect.left() > positionRect.right() &&
                                keyRect.top() <= positionRect.bottom() && keyRect.bottom() >= positionRect.top(),
                                "Wide dialogs must show the control sections side by side");
                };
                require(dialog->maximumWidth() == std::max(dialog->minimumWidth(),
                            dialog->screen()->availableGeometry().width() * 45 / 100),
                        "Maximum dialog width must follow the available screen width and control minimum");
                require(dialog->width() <= dialog->maximumWidth(), "Opening width must respect the screen budget");
                if (count == 1 || count == 11) {
                    const QSize openingSize = dialog->size();
                    dialog->resize(dialog->maximumWidth(), dialog->height());
                    QTest::qWait(40);
                    checkFixedHeight();
                    require(dialog->rect().contains(QRect(keyControl->mapTo(dialog, QPoint()), keyControl->size())),
                            "Key controls must fit at the widest allowed size");
                    dialog->resize(dialog->minimumWidth(), dialog->height());
                    QTest::qWait(40);
                    checkControlLayout(true);
                    checkFixedHeight();
                    dialog->resize(openingSize);
                    QTest::qWait(40);
                }
                require(dialog->findChild<QRadioButton *>("radioButtonAddSinger")->isChecked(),
                        "Opening the list must not change the initial new-singer mode");
                if (!count) {
                    require(!existing->isEnabled() && !list->isEnabled(), "Empty rotation must disable existing-singer mode");
                    dialog->close();
                    QTest::qWait(10);
                    continue;
                }
                require(list->item(0)->text().startsWith("Singer 01"), "List must preserve alphabetical ordering");
                const auto first = list->visualItemRect(list->item(0));
                require(first.size() == list->item(0)->sizeHint(), "Item selection must fill the entire row cell");
                const int visibleColumns = std::min((count + 9) / 10, std::max(1, list->viewport()->width() / first.width()));
                require(list->viewport()->width() - visibleColumns * first.width() < visibleColumns,
                        "Columns must fill the available width without an empty strip");
                // Different name lengths must share the width of the longest name,
                // including columns that contain only shorter names.
                for (int row = 0; row < count; ++row) {
                    const auto rect = list->visualItemRect(list->item(row));
                    require(rect.width() == list->item(row)->sizeHint().width(), "Selection must use the column's measured width");
                    require(rect.width() >= list->fontMetrics().horizontalAdvance(list->item(row)->text()),
                            "Every singer name must fit its column");
                    require(rect.width() == first.width(), "All columns must share the longest name width");
                }
                if (count >= 10) {
                    const auto tenth = list->visualItemRect(list->item(9));
                    require(tenth.left() == first.left() && tenth.bottom() < list->viewport()->height(),
                            "The first column must show ten complete rows");
                }
                if (count >= 11) {
                    const auto eleventh = list->visualItemRect(list->item(10));
                    require(eleventh.left() > first.left() && eleventh.top() == first.top(),
                            "The eleventh singer must start the second column");
                }
                if (count == 11) {
                    dialog->resize(dialog->maximumWidth(), dialog->height());
                    QTest::qWait(40);
                    const int cellWidth = list->visualItemRect(list->item(0)).width();
                    const int visible = std::min(2, std::max(1, list->viewport()->width() / cellWidth));
                    require(list->viewport()->width() - visible * cellWidth < visible,
                            "Columns must fill their section at the maximum dialog width");
                }
                const auto last = list->visualItemRect(list->item(count - 1));
                if (last.right() >= list->viewport()->width())
                    require(list->horizontalScrollBar()->maximum() > 0,
                            "Rotations wider than the available space must remain scrollable");
                const int selected = count - 1;
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
                    dialog->grab().save(app.arguments().at(1) + QString("-%1-%2-%3.png").arg(config.first).arg(config.second).arg(count));
                if (count == 10)
                    QTest::keyClick(list, Qt::Key_Return);
                else
                    QTest::mouseClick(dialog->findChild<QPushButton *>("pushButtonAdd"), Qt::LeftButton);
                require(singerId == 101 + selected && songId == 777 && keyChange == -3 && requests == 1,
                        "Add must target the selected singer ID and preserve song/key");
                require(position == count - 1 - selected, "Add must preserve the rotation-position notification");
                QObject::disconnect(request);
                QTest::qWait(10);
            }
        }
        std::puts("Add Song singer selection and column layout passed");
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
