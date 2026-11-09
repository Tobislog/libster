#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

namespace libster::gui  {

class StartupDialog final : public QDialog {
    Q_OBJECT

public:
    explicit StartupDialog(QWidget* parent = nullptr);

    QString dataDirectory() const;
    QString recordedBy() const;

private slots:
    void chooseDirectory();
    void validateAndAccept();

private:
    QLineEdit* dataDirectoryEdit_ = nullptr;
    QLineEdit* recordedByEdit_ = nullptr;
};

} // namespace libster::gui
