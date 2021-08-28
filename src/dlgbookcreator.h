#ifndef DLGBOOKCREATOR_H
#define DLGBOOKCREATOR_H

#include <QAbstractButton>
#include <QDialog>
#include <QTextDocument>
#include "settings.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/async_logger.h>
#include <spdlog/fmt/ostr.h>

std::ostream& operator<<(std::ostream& os, const QString& s);

namespace Ui {
class DlgBookCreator;
}

class DlgBookCreator : public QDialog
{
    Q_OBJECT

public:
    explicit DlgBookCreator(QWidget *parent = nullptr);
    ~DlgBookCreator() override;

private slots:
    void btnGenerateClicked();
    void saveFontSettings();

private:
    std::string m_loggingPrefix{"[BookCreator]"};
    std::shared_ptr<spdlog::logger> m_logger;
    std::unique_ptr<Ui::DlgBookCreator> ui;
    Settings m_settings;
    void writePdf(const QString& filename, int nCols = 2);
    static QStringList getArtists();
    static QStringList getTitles(const QString& artist);
    void setupConnections() const;
    void loadSettings();

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // DLGBOOKCREATOR_H
