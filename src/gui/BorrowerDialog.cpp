#include "gui/BorrowerDialog.h"

#include <utility>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "core/TimeUtil.h"
#include "core/Validation.h"
#include "gui/GuiUtil.h"

namespace libster::gui {

BorrowerDialog::BorrowerDialog(QString recordedBy, QWidget* parent)
    : QDialog(parent), recordedBy_(std::move(recordedBy)) {
    recordedOn_ = toQString(epochToIsoDate(nowEpochSeconds()));
    setupUi();
    setWindowTitle(QStringLiteral("Neuer Ausleiher"));
    setMetadata(recordedOn_, recordedBy_);
}

BorrowerDialog::BorrowerDialog(const Borrower& borrower, QWidget* parent)
    : QDialog(parent), editMode_(true) {
    id_ = borrower.getId();
    recordedOn_ = toQString(borrower.getRecordedOn());
    recordedBy_ = toQString(borrower.getRecordedBy());
    setupUi();
    setWindowTitle(QStringLiteral("Ausleiher bearbeiten"));
    loadBorrower(borrower);
    setMetadata(recordedOn_, recordedBy_);
}

void BorrowerDialog::setupUi() {
    lastNameEdit_ = new QLineEdit(this);
    lastNameEdit_->setMaxLength(63);
    lastNameEdit_->setPlaceholderText(QStringLiteral("Pflichtfeld"));

    firstNameEdit_ = new QLineEdit(this);
    firstNameEdit_->setMaxLength(63);
    firstNameEdit_->setPlaceholderText(QStringLiteral("Pflichtfeld"));

    emailEdit_ = new QLineEdit(this);
    emailEdit_->setMaxLength(127);
    emailEdit_->setPlaceholderText(QStringLiteral("optional"));

    addressEdit_ = new QLineEdit(this);
    addressEdit_->setMaxLength(127);

    metadataLabel_ = new QLabel(this);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Nachname:"), lastNameEdit_);
    form->addRow(QStringLiteral("Vorname:"), firstNameEdit_);
    form->addRow(QStringLiteral("E-Mail:"), emailEdit_);
    form->addRow(QStringLiteral("Adresse:"), addressEdit_);
    form->addRow(QStringLiteral("Erfasst:"), metadataLabel_);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(
        QStringLiteral("Speichern"));
    buttons->button(QDialogButtonBox::Cancel)->setText(
        QStringLiteral("Abbrechen"));

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted,
            this, &BorrowerDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    resize(520, 0);
}

void BorrowerDialog::setMetadata(const QString& recordedOn,
                                 const QString& recordedBy) {
    metadataLabel_->setText(
        QStringLiteral("%1 von %2").arg(recordedOn, recordedBy));
}

void BorrowerDialog::loadBorrower(const Borrower& borrower) {
    lastNameEdit_->setText(toQString(borrower.getLastName()));
    firstNameEdit_->setText(toQString(borrower.getFirstName()));
    emailEdit_->setText(toQString(borrower.getEmail()));
    addressEdit_->setText(toQString(borrower.getAddress()));
}

bool BorrowerDialog::validateInputs() {
    const QString lastName = lastNameEdit_->text().trimmed();
    if (lastName.isEmpty()) {
        warn(QStringLiteral("Bitte einen Nachnamen angeben."), lastNameEdit_);
        return false;
    }
    if (!fitsField(toStdString(lastName), 63)) {
        warn(QStringLiteral("Der Nachname ist zu lang (max. 63 Zeichen)."),
             lastNameEdit_);
        return false;
    }

    const QString firstName = firstNameEdit_->text().trimmed();
    if (firstName.isEmpty()) {
        warn(QStringLiteral("Bitte einen Vornamen angeben."), firstNameEdit_);
        return false;
    }
    if (!fitsField(toStdString(firstName), 63)) {
        warn(QStringLiteral("Der Vorname ist zu lang (max. 63 Zeichen)."),
             firstNameEdit_);
        return false;
    }

    const QString email = emailEdit_->text().trimmed();
    if (!email.isEmpty()) {
        if (!fitsField(toStdString(email), 127)) {
            warn(QStringLiteral("Die E-Mail ist zu lang (max. 127 Zeichen)."),
                 emailEdit_);
            return false;
        }
        if (!isValidEmail(toStdString(email))) {
            warn(QStringLiteral("Die E-Mail-Adresse ist ungültig."), emailEdit_);
            return false;
        }
    }

    const QString address = addressEdit_->text().trimmed();
    if (!fitsField(toStdString(address), 127)) {
        warn(QStringLiteral("Die Adresse ist zu lang (max. 127 Zeichen)."),
             addressEdit_);
        return false;
    }

    return true;
}

void BorrowerDialog::warn(const QString& message, QWidget* focus) {
    QMessageBox::warning(this, QStringLiteral("Eingabe prüfen"), message);
    if (focus != nullptr) {
        focus->setFocus();
    }
}

void BorrowerDialog::validateAndAccept() {
    if (validateInputs()) {
        accept();
    }
}

Borrower BorrowerDialog::borrower() const {
    return Borrower{
        id_,
        toStdString(lastNameEdit_->text().trimmed()),
        toStdString(firstNameEdit_->text().trimmed()),
        toStdString(emailEdit_->text().trimmed()),
        toStdString(addressEdit_->text().trimmed()),
        toStdString(recordedOn_),
        toStdString(recordedBy_)
    };
}

} // namespace libster::gui
