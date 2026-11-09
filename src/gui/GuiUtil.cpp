#include "gui/GuiUtil.h"

#include <cstddef>

#include <QByteArray>
#include <QMessageBox>
#include <QWidget>

namespace libster::gui {

QString toQString(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
}

std::string toStdString(const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return std::string(utf8.constData(), static_cast<std::size_t>(utf8.size()));
}

QString serviceErrorMessage(ServiceError error) {
    switch (error) {
        case ServiceError::None: return {};
        case ServiceError::NotOpen: return QStringLiteral("Der Datenbestand ist nicht geöffnet.");
        case ServiceError::NotFound: return QStringLiteral("Der Datensatz wurde nicht gefunden.");
        case ServiceError::AlreadyDeleted: return QStringLiteral("Der Datensatz ist bereits gelöscht.");
        case ServiceError::HasOpenLoans: return QStringLiteral("Der Datensatz hat noch offene Ausleihen.");
        case ServiceError::NotAvailable: return QStringLiteral("Das Medium ist nicht verfügbar.");
        case ServiceError::AlreadyReturned: return QStringLiteral("Diese Ausleihe wurde bereits zurückgegeben.");
        case ServiceError::InvalidData: return QStringLiteral("Die Eingaben sind ungültig.");
        case ServiceError::PartialWrite: return QStringLiteral("Der Vorgang wurde nur teilweise gespeichert.");
        case ServiceError::Persistence: return QStringLiteral("Fehler beim Zugriff auf den Datenbestand.");
    }
    return QStringLiteral("Unbekannter Fehler.");
}

void showServiceError(QWidget* parent, ServiceError error, const QString& context) {
    if (error == ServiceError::None) return;

    QMessageBox::warning(parent, context.isEmpty() ? QStringLiteral("Fehler") : context, serviceErrorMessage(error));
}

} // namespace libster::gui
