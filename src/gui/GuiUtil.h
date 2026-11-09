#pragma once

#include <string>

#include <QString>
#include <QMessageBox>

#include "core/services/ServiceError.h"

class QWidget;

namespace libster::gui {

QString toQString(const std::string& value);

std::string toStdString(const QString& value);

QString serviceErrorMessage(ServiceError error);

void showServiceError(QWidget* parent, ServiceError error, const QString& context = {});

} // namespace libster::gui
