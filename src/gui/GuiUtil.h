#pragma once

#include <string>
#include <cstdint>

#include <QString>
#include <QMessageBox>

#include "core/services/ServiceError.h"
#include "core/model/Status.h"

class QWidget;

namespace libster::gui {

QString toQString(const std::string& value);

std::string toStdString(const QString& value);

QString serviceErrorMessage(ServiceError error);

void showServiceError(QWidget* parent, ServiceError error, const QString& context = {});

QString formatPrice(std::uint32_t cents);
QString statusText(Status status);

} // namespace libster::gui
