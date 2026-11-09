#include "gui/BookDialog.h"

#include <cmath>
#include <utility>

#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include "core/TimeUtil.h"
#include "core/Validation.h"
#include "gui/GuiUtil.h"

namespace libster::gui {

BookDialog::BookDialog(QString recordedBy, QWidget* parent) : QDialog(parent), recordedBy_(std::move(recordedBy)) {
    addedOn_ = toQString(epochToIsoDate(nowEpochSeconds()));
    setupUi();
    setWindowTitle(QStringLiteral("Neues Buch"));
    publishedOnEdit_->setDate(QDate::currentDate());
    setMetadata(addedOn_, recordedBy_);
}

BookDialog::BookDialog(const Book& book, QWidget* parent) : QDialog(parent), editMode_(true) {
    id_ = book.getId();
    addedOn_ = toQString(book.getAddedOn());
    recordedBy_ = toQString(book.getRecordedBy());
    setupUi();
    setWindowTitle(QStringLiteral("Buch bearbeiten"));
    loadBook(book);
    setMetadata(addedOn_, recordedBy_);
}

void BookDialog::setupUi() {
    titleEdit_ = new QLineEdit(this);
    titleEdit_->setMaxLength(127);
    titleEdit_->setPlaceholderText(QStringLiteral("Pflichtfeld"));

    authorsEdit_ = new QLineEdit(this);
    authorsEdit_->setMaxLength(255);
    authorsEdit_->setPlaceholderText(
        QStringLiteral("Mehrere Autoren mit ; trennen"));

    isbnEdit_ = new QLineEdit(this);
    isbnEdit_->setMaxLength(20);
    isbnEdit_->setPlaceholderText(
        QStringLiteral("ISBN-10 oder ISBN-13, Bindestriche erlaubt"));

    publisherEdit_ = new QLineEdit(this);
    publisherEdit_->setMaxLength(63);

    genreEdit_ = new QLineEdit(this);
    genreEdit_->setMaxLength(63);

    publishedOnEdit_ = new QDateEdit(this);
    publishedOnEdit_->setCalendarPopup(true);
    publishedOnEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    publishedOnEdit_->setDate(QDate::currentDate());

    locationEdit_ = new QLineEdit(this);
    locationEdit_->setMaxLength(63);

    priceEdit_ = new QDoubleSpinBox(this);
    priceEdit_->setDecimals(2);
    priceEdit_->setRange(0.0, 999999.99);
    priceEdit_->setSuffix(QStringLiteral(" €"));
    priceEdit_->setSingleStep(1.0);

    editionEdit_ = new QSpinBox(this);
    editionEdit_->setRange(0, 65535);

    pageCountEdit_ = new QSpinBox(this);
    pageCountEdit_->setRange(0, 65535);

    metadataLabel_ = new QLabel(this);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Titel:"), titleEdit_);
    form->addRow(QStringLiteral("Autor(en):"), authorsEdit_);
    form->addRow(QStringLiteral("ISBN:"), isbnEdit_);
    form->addRow(QStringLiteral("Verlag:"), publisherEdit_);
    form->addRow(QStringLiteral("Genre:"), genreEdit_);
    form->addRow(QStringLiteral("Erscheinungsdatum:"), publishedOnEdit_);
    form->addRow(QStringLiteral("Standort:"), locationEdit_);
    form->addRow(QStringLiteral("Preis:"), priceEdit_);
    form->addRow(QStringLiteral("Auflage:"), editionEdit_);
    form->addRow(QStringLiteral("Seitenzahl:"), pageCountEdit_);
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
            this, &BookDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    resize(560, 0);
}

void BookDialog::setMetadata(const QString& addedOn,
                             const QString& recordedBy) {
    metadataLabel_->setText(
        QStringLiteral("%1 von %2").arg(addedOn, recordedBy));
}

void BookDialog::loadBook(const Book& book) {
    titleEdit_->setText(toQString(book.getTitle()));
    authorsEdit_->setText(toQString(book.getAuthors()));
    isbnEdit_->setText(toQString(book.getIsbn()));
    publisherEdit_->setText(toQString(book.getPublisher()));
    genreEdit_->setText(toQString(book.getGenre()));

    const QDate published =
        QDate::fromString(toQString(book.getPublishedOn()), Qt::ISODate);
    if (published.isValid()) {
        publishedOnEdit_->setDate(published);
    }

    locationEdit_->setText(toQString(book.getLocation()));
    priceEdit_->setValue(static_cast<double>(book.getPrice()) / 100.0);
    editionEdit_->setValue(static_cast<int>(book.getEdition()));
    pageCountEdit_->setValue(static_cast<int>(book.getPageCount()));
}

bool BookDialog::validateInputs() {
    const QString title = titleEdit_->text().trimmed();
    if (title.isEmpty()) {
        warn(QStringLiteral("Bitte einen Titel angeben."), titleEdit_);
        return false;
    }
    if (!fitsField(toStdString(title), 127)) {
        warn(QStringLiteral("Der Titel ist zu lang (max. 127 Zeichen)."),
             titleEdit_);
        return false;
    }

    const QString authors = authorsEdit_->text().trimmed();
    if (authors.isEmpty()) {
        warn(QStringLiteral("Bitte mindestens einen Autor angeben."),
             authorsEdit_);
        return false;
    }
    if (!fitsField(toStdString(authors), 255)) {
        warn(QStringLiteral("Die Autorenliste ist zu lang (max. 255 Zeichen)."),
             authorsEdit_);
        return false;
    }

    const QString isbnText = isbnEdit_->text().trimmed();
    if (isbnText.isEmpty()) {
        warn(QStringLiteral("Bitte eine ISBN angeben."), isbnEdit_);
        return false;
    }

    const std::string isbn = normalizeIsbn(toStdString(isbnText));
    if (!fitsField(isbn, 13)) {
        warn(QStringLiteral("Die ISBN ist zu lang (max. 13 Zeichen)."),
             isbnEdit_);
        return false;
    }
    if (!isValidIsbn(isbn)) {
        warn(QStringLiteral(
                 "Die ISBN ist ungültig. Erwartet wird eine ISBN-10 oder "
                 "ISBN-13 mit gültiger Prüfziffer."),
             isbnEdit_);
        return false;
    }

    const QString publisher = publisherEdit_->text().trimmed();
    if (!fitsField(toStdString(publisher), 63)) {
        warn(QStringLiteral("Der Verlag ist zu lang (max. 63 Zeichen)."),
             publisherEdit_);
        return false;
    }

    const QString genre = genreEdit_->text().trimmed();
    if (!fitsField(toStdString(genre), 63)) {
        warn(QStringLiteral("Das Genre ist zu lang (max. 63 Zeichen)."),
             genreEdit_);
        return false;
    }

    const QString location = locationEdit_->text().trimmed();
    if (!fitsField(toStdString(location), 63)) {
        warn(QStringLiteral("Der Standort ist zu lang (max. 63 Zeichen)."),
             locationEdit_);
        return false;
    }

    return true;
}

void BookDialog::warn(const QString& message, QWidget* focus) {
    QMessageBox::warning(this, QStringLiteral("Eingabe prüfen"), message);
    if (focus != nullptr) {
        focus->setFocus();
    }
}

void BookDialog::validateAndAccept() {
    if (validateInputs()) {
        accept();
    }
}

Book BookDialog::book() const {
    const std::string isbn =
        normalizeIsbn(toStdString(isbnEdit_->text().trimmed()));

    return Book{
        id_,
        toStdString(titleEdit_->text().trimmed()),
        toStdString(publisherEdit_->text().trimmed()),
        toStdString(genreEdit_->text().trimmed()),
        toStdString(publishedOnEdit_->date().toString(Qt::ISODate)),
        toStdString(addedOn_),
        toStdString(locationEdit_->text().trimmed()),
        static_cast<std::uint32_t>(std::lround(priceEdit_->value() * 100.0)),
        toStdString(recordedBy_),
        isbn,
        static_cast<std::uint16_t>(editionEdit_->value()),
        toStdString(authorsEdit_->text().trimmed()),
        static_cast<std::uint16_t>(pageCountEdit_->value())
    };
}

} // namespace libster::gui
