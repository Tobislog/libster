#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QSettings>

#include "core/persistence/IoError.h"
#include "core/services/LibraryService.h"
#include "gui/GuiUtil.h"
#include "gui/MainWindow.h"
#include "gui/StartupDialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Libster"));
    QCoreApplication::setApplicationName(QStringLiteral("Libster"));

    libster::gui::StartupDialog startup;
    if (startup.exec() != QDialog::Accepted) {
        return 0;
    }

    const QString dataDirectory = QDir(startup.dataDirectory()).absolutePath();
    const QString recordedBy = startup.recordedBy();

    QSettings settings;
    settings.setValue(QStringLiteral("startup/dataDirectory"), dataDirectory);
    settings.setValue(QStringLiteral("startup/recordedBy"), recordedBy);

    libster::LibraryService service;
    service.setRecordedBy(libster::gui::toStdString(recordedBy));
    
    const libster::IoError error = service.open(libster::gui::toStdString(dataDirectory));

    if (error != libster::IoError::None) {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("Datenverzeichnis"),
            QStringLiteral("Das Daenverzeichnis konnte nicht geöffnet werden: \n%1").arg(libster::toString(error)));
        return 1;
    }

    libster::gui::MainWindow window(service, dataDirectory, recordedBy);
    window.show();

    return app.exec();
}
