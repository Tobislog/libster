#include "StartupDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace libster::gui {

StartupDialog::StartupDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("Libster - Start"));
    setModal(true);

    dataDirectoryEdit_ = new QLineEdit(this);
    dataDirectoryEdit_->setPlaceholderText(QStringLiteral("Verzeichnis mit den .bin-Dateien"));

    auto* browseButton = new QPushButton(QStringLiteral("Durchsuchen..."), this);

    recordedByEdit_ = new QLineEdit(this);
    recordedByEdit_->setPlaceholderText(QStringLiteral("Name oder Kürzel der erfassenden Person"));

    QSettings settings;
    dataDirectoryEdit_->setText(settings.value(QStringLiteral("startup/dataDirectory")).toString());
    recordedByEdit_->setText(settings.value(QStringLiteral("startup/recordedBy")).toString());

    auto* directoryRow = new QHBoxLayout;
    directoryRow->addWidget(dataDirectoryEdit_);
    directoryRow->addWidget(browseButton);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Datenverzeichnis:"), directoryRow);
    form->addRow(QStringLiteral("Erfassende Person:"), recordedByEdit_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Öffnen"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Abbrechen"));

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(browseButton, &QPushButton::clicked, this, &StartupDialog::chooseDirectory);
    connect(buttons, &QDialogButtonBox::accepted, this, &StartupDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}    

QString StartupDialog::dataDirectory() const {
    return dataDirectoryEdit_->text().trimmed();
}

QString StartupDialog::recordedBy() const {
    return recordedByEdit_->text().trimmed();
}

void StartupDialog::chooseDirectory() {
    const QString selected = QFileDialog::getExistingDirectory(this, QStringLiteral("Datenverzeichnis wählen"), dataDirectoryEdit_->text().trimmed());
    if (!selected.isEmpty()) dataDirectoryEdit_->setText(QDir::toNativeSeparators(selected));
}

void StartupDialog::validateAndAccept() {
    if (dataDirectory().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Eingabe fehlt"), QStringLiteral("Bitte ein Datenverzeichnis angeben."));
        dataDirectoryEdit_->setFocus();
        return;
    }

    if (recordedBy().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Eingabe fehlt"), QStringLiteral("Bitte die erfassende Person angeben."));
        recordedByEdit_->setFocus();
        return;
    }

    accept();
}

} // namespace libster::gui
