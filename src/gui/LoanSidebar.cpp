#include "gui/LoanSidebar.h"

#include <utility>
#include <vector>

#include <QAbstractItemView>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "core/TimeUtil.h"
#include "core/model/Book.h"
#include "core/model/CD.h"
#include "core/services/LibraryService.h"
#include "gui/GuiUtil.h"
#include "gui/LoanTableModel.h"

namespace libster::gui {

LoanSidebar::LoanSidebar(LibraryService& service, QWidget* parent)
    : QWidget(parent), service_(service) {
    auto* header = new QLabel(QStringLiteral("Ausleihe"), this);
    QFont headerFont = header->font();
    headerFont.setPointSize(headerFont.pointSize() + 1);
    headerFont.setBold(true);
    header->setFont(headerFont);

    idEdit_ = new QLineEdit(this);
    idEdit_->setPlaceholderText(QStringLiteral("Ausleiher-ID"));
    idEdit_->setValidator(new QIntValidator(1, 999999999, this));

    loginButton_ = new QPushButton(QStringLiteral("Anmelden"), this);
    logoutButton_ = new QPushButton(QStringLiteral("Abmelden"), this);
    logoutButton_->setEnabled(false);

    auto* loginRow = new QHBoxLayout;
    loginRow->addWidget(idEdit_, 1);
    loginRow->addWidget(loginButton_);

    nameLabel_ = new QLabel(QStringLiteral("Nicht angemeldet"), this);
    nameLabel_->setWordWrap(true);

    auto* loansLabel = new QLabel(QStringLiteral("Ausgeliehene Medien"), this);

    model_ = new LoanTableModel(this);

    table_ = new QTableView(this);
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);

    returnButton_ = new QPushButton(QStringLiteral("Zurückgeben"), this);
    returnButton_->setEnabled(false);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(header);
    layout->addLayout(loginRow);
    layout->addWidget(nameLabel_);
    layout->addWidget(loansLabel);
    layout->addWidget(table_, 1);
    layout->addWidget(returnButton_);
    layout->addWidget(logoutButton_);

    connect(loginButton_, &QPushButton::clicked, this, &LoanSidebar::login);
    connect(idEdit_, &QLineEdit::returnPressed, this, &LoanSidebar::login);
    connect(logoutButton_, &QPushButton::clicked, this, &LoanSidebar::logout);
    connect(returnButton_, &QPushButton::clicked,
            this, &LoanSidebar::returnSelected);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] { updateActions(); });

    table_->horizontalHeader()->setSectionResizeMode(
        LoanTableModel::Title, QHeaderView::Stretch);
}

bool LoanSidebar::isLoggedIn() const noexcept {
    return borrower_.has_value();
}

std::uint32_t LoanSidebar::borrowerId() const noexcept {
    return borrowerId_;
}

void LoanSidebar::login() {
    bool ok = false;
    const std::uint32_t id = idEdit_->text().trimmed().toUInt(&ok);
    if (!ok || id == 0) {
        QMessageBox::warning(
            this, QStringLiteral("Anmeldung"),
            QStringLiteral("Bitte eine numerische Ausleiher-ID eingeben."));
        idEdit_->setFocus();
        return;
    }

    std::optional<Borrower> found;
    const ServiceError error = service_.getBorrower(id, found);

    if (error == ServiceError::NotFound) {
        QMessageBox::warning(
            this,
            QStringLiteral("Anmeldung"),
            QStringLiteral(
                "Keine Ausleiher-ID %1 gefunden.\n"
                "Bitte die Id prüfen oder im Tab \"Ausleiher\" suchen.")
                .arg(id)
            );
            idEdit_->selectAll();
            idEdit_->setFocus();
            return;
    }
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Anmeldung"));
        return;
    }

    borrower_ = std::move(found);
    borrowerId_ = id;
    nameLabel_->setText(
        QStringLiteral("Angemeldet: %1, %2 (ID %3)")
            .arg(toQString(borrower_->getLastName()),
                 toQString(borrower_->getFirstName()))
            .arg(borrowerId_));
    idEdit_->setEnabled(false);
    loginButton_->setEnabled(false);
    logoutButton_->setEnabled(true);

    refresh();
    emit borrowerChanged(borrowerId_);
}

void LoanSidebar::logout() {
    borrower_.reset();
    borrowerId_ = 0;
    model_->setEntries({});
    nameLabel_->setText(QStringLiteral("Nicht angemeldet"));
    idEdit_->clear();
    idEdit_->setEnabled(true);
    loginButton_->setEnabled(true);
    logoutButton_->setEnabled(false);
    updateActions();
    emit borrowerChanged(0);
}

void LoanSidebar::refresh() {
    if (!borrower_) {
        model_->setEntries({});
        updateActions();
        return;
    }

    std::vector<LoanTransaction> loans;
    const ServiceError error = service_.findOpenLoans(borrowerId_, loans);
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Ausleihen laden"));
        return;
    }

    std::vector<LoanEntry> entries;
    entries.reserve(loans.size());

    for (const LoanTransaction& loan : loans) {
        entries.push_back(LoanEntry{
            loan.getId(),
            loan.getMediumId(),
            loan.getType(),
            mediumTitle(loan),
            toQString(epochToIsoDate(loan.getLoanDate()))
        });
    }

    model_->setEntries(std::move(entries));
    updateActions();
}

void LoanSidebar::returnSelected() {
    const int row = selectedRow();
    if (row < 0) return;

    const LoanEntry& entry = model_->entryAt(row);

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Rückgabe"),
        QStringLiteral("Soll \"%1\" zurückgegeben werden?").arg(entry.title),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    const ServiceError error =
        service_.returnLoan(entry.loanId, nowEpochSeconds());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Rückgabe"));
        return;
    }

    refresh();
    emit loansChanged();
}

int LoanSidebar::selectedRow() const {
    const QModelIndexList rows = table_->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return rows.first().row();
}

void LoanSidebar::updateActions() {
    returnButton_->setEnabled(isLoggedIn() && selectedRow() >= 0);
}

QString LoanSidebar::mediumTitle(const LoanTransaction& loan) const {
    if (loan.getType() == MediumType::BOOK) {
        std::optional<Book> book;
        if (service_.getBook(loan.getMediumId(), book) == ServiceError::None
            && book.has_value()) {
            return toQString(book->getTitle());
        }
    } else {
        std::optional<CD> cd;
        if (service_.getCD(loan.getMediumId(), cd) == ServiceError::None
            && cd.has_value()) {
            return toQString(cd->getTitle());
        }
    }

    return QStringLiteral("Medium #%1").arg(loan.getMediumId());
}

} // namespace libster::gui
