#include "gui/BookTab.h"

#include <utility>
#include <vector>

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include "core/TimeUtil.h"
#include "core/model/Book.h"
#include "core/model/Status.h"
#include "core/services/LibraryService.h"
#include "gui/BookDialog.h"
#include "gui/BookTableModel.h"
#include "gui/GuiUtil.h"
#include "gui/TableHeaderView.h"

namespace libster::gui {

BookTab::BookTab(LibraryService& service, QWidget* parent) : QWidget(parent), service_(service) {
    auto* searchLabel = new QLabel(QStringLiteral("Suche:"), this);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(QStringLiteral("Titel, Autor, ISBN oder Verlag - leer zeigt alle"));
    searchEdit_->setClearButtonEnabled(true);

    countLabel_ = new QLabel(this);

    auto* searchRow = new QHBoxLayout;
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(searchEdit_, 1);
    searchRow->addWidget(countLabel_);

    addButton_ = new QPushButton(QStringLiteral("Neu"), this);
    editButton_ = new QPushButton(QStringLiteral("Bearbeiten"), this);
    deleteButton_ = new QPushButton(QStringLiteral("Löschen"), this);
    loanButton_ = new QPushButton(QStringLiteral("Ausleihen"), this);

    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
    loanButton_->setVisible(false);
    loanButton_->setEnabled(false);

    auto* actionRow = new QHBoxLayout;
    actionRow->addWidget(addButton_);
    actionRow->addWidget(editButton_);
    actionRow->addWidget(deleteButton_);
    actionRow->addWidget(loanButton_);
    actionRow->addStretch(1);

    model_ = new BookTableModel(this);

    table_ = new QTableView(this);
    table_->setHorizontalHeader(new TableHeaderView(Qt::Horizontal, table_));
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(searchRow);
    layout->addLayout(actionRow);
    layout->addWidget(table_, 1);

    searchTimer_ = new QTimer(this);
    searchTimer_->setSingleShot(true);
    searchTimer_->setInterval(200);

    connect(searchEdit_, &QLineEdit::textChanged, this, [this] {
        searchTimer_->start();
    });
    connect(searchTimer_, &QTimer::timeout, this, &BookTab::refresh);
    connect(addButton_, &QPushButton::clicked, this, &BookTab::addBook);
    connect(editButton_, &QPushButton::clicked, this, &BookTab::editSelected);
    connect(deleteButton_, &QPushButton::clicked, this, &BookTab::deleteSelected);
    connect(loanButton_, &QPushButton::clicked, this, &BookTab::loanSelected);
    connect(table_, &QTableView::doubleClicked, this, &BookTab::handleDoubleClick);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { updateActions(); });

    table_->horizontalHeader()->setSectionResizeMode(BookTableModel::Title, QHeaderView::Stretch);
}

void BookTab::setBorrowerId(std::uint32_t borrowerId) {
    borrowerId_ = borrowerId;
    const bool lending = borrowerId_ != 0;

    addButton_->setVisible(!lending);
    editButton_->setVisible(!lending);
    deleteButton_->setVisible(!lending);
    loanButton_->setVisible(lending);

    updateActions();
}

void BookTab::showEvent (QShowEvent* event) {
    QWidget::showEvent(event);
    if (loaded_) return;

    loaded_ = true;
    refresh();
    table_->resizeColumnsToContents();
}
void BookTab::refresh() {
    std::vector<Book> books;
    const ServiceError error = service_.findBooks(toStdString(searchEdit_->text()), books);

    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Bücher suchen"));
        countLabel_->clear();
        return;
    }

    const int count = static_cast<int>(books.size());
    model_->setBooks(std::move(books));

    countLabel_->setText(count == 1 ? QStringLiteral("1 Buch") : QStringLiteral("%1 Bücher").arg(count));
    updateActions();
}
int BookTab::selectedRow() const {
    const QModelIndexList rows = table_->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return rows.first().row();
}

void BookTab::updateActions() {
    const bool hasSelection = selectedRow() >= 0;
    const bool lending = borrowerId_ != 0;

    editButton_->setEnabled(hasSelection && !lending);
    deleteButton_->setEnabled(hasSelection && !lending);
    loanButton_->setEnabled(hasSelection && lending);
}

void BookTab::handleDoubleClick() {
    if (borrowerId_ == 0) {
        editSelected();
    } else {
        loanSelected();
    }
}

void BookTab::addBook() {
    BookDialog dialog(toQString(service_.getRecordedBy()), this);
    if (dialog.exec() != QDialog::Accepted) return;

    Book book = dialog.book();
    const ServiceError error = service_.addBook(book);
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Buch anlegen"));
        return;
    }
    refresh();
}

void BookTab::editSelected() {
    if (borrowerId_ != 0) return;

    const int row = selectedRow();
    if (row < 0) return;

    BookDialog dialog(model_->bookAt(row), this);
    if (dialog.exec() != QDialog::Accepted) return;

    const ServiceError error = service_.updateBook(dialog.book());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Buch bearbeiten"));
        return;
    }
    
    refresh();
}

void BookTab::deleteSelected() {
    if (borrowerId_ != 0) return;

    const int row = selectedRow();
    if (row < 0) return;

    const Book& book = model_->bookAt(row);

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Buch löschen"),
        QStringLiteral("Soll \"%1\" wirklich gelöscht werden?").arg(toQString(book.getTitle())),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (answer != QMessageBox::Yes) return;

    const ServiceError error = service_.deleteBook(book.getId());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Buch löschen"));
        return;    
    }
    refresh();
}

void BookTab::loanSelected() {
    if (borrowerId_ == 0) return;

    const int row = selectedRow();
    if (row < 0) return;

    const Book& book = model_->bookAt(row);

    if (book.getStatus() != Status::AVAILABLE) {
        QMessageBox::information(this, QStringLiteral("Ausleihen"), QStringLiteral("Dieses Buch ist derzeit nicht verfügbar."));
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Buch ausleihen"),
        QStringLiteral("Soll \"%1\" an Ausleiher %2 ausgeliehen werden?")
            .arg(toQString(book.getTitle()))
            .arg(borrowerId_),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    if (answer != QMessageBox::Yes) return;

    std::uint32_t loanId = 0;
    const ServiceError error = service_.loanBook(borrowerId_, book.getId(), nowEpochSeconds(), loanId);

    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Buch ausleihen"));
        refresh();
        emit loansChanged();
        return;
    }

    refresh();
    emit loansChanged();

}

} // namespace libster::gui
