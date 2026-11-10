#include "gui/BorrowerTab.h"

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
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include "core/model/Borrower.h"
#include "core/services/LibraryService.h"
#include "gui/BorrowerDialog.h"
#include "gui/BorrowerTableModel.h"
#include "gui/GuiUtil.h"
#include "gui/TableHeaderView.h"

namespace libster::gui {

BorrowerTab::BorrowerTab(LibraryService& service, QWidget* parent)
    : QWidget(parent), service_(service) {
    auto* searchLabel = new QLabel(QStringLiteral("Suche:"), this);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(
        QStringLiteral("Nachname, Vorname, E-Mail oder ID – leer zeigt alle"));
    searchEdit_->setClearButtonEnabled(true);

    countLabel_ = new QLabel(this);

    auto* searchRow = new QHBoxLayout;
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(searchEdit_, 1);
    searchRow->addWidget(countLabel_);

    auto* addButton = new QPushButton(QStringLiteral("Neu"), this);
    editButton_ = new QPushButton(QStringLiteral("Bearbeiten"), this);
    deleteButton_ = new QPushButton(QStringLiteral("Löschen"), this);

    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);

    auto* actionRow = new QHBoxLayout;
    actionRow->addWidget(addButton);
    actionRow->addWidget(editButton_);
    actionRow->addWidget(deleteButton_);
    actionRow->addStretch(1);

    model_ = new BorrowerTableModel(this);

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
    connect(searchTimer_, &QTimer::timeout, this, &BorrowerTab::refresh);

    connect(addButton, &QPushButton::clicked,
            this, &BorrowerTab::addBorrower);
    connect(editButton_, &QPushButton::clicked,
            this, &BorrowerTab::editSelected);
    connect(deleteButton_, &QPushButton::clicked,
            this, &BorrowerTab::deleteSelected);
    connect(table_, &QTableView::doubleClicked,
            this, &BorrowerTab::editSelected);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] { updateActions(); });

    table_->horizontalHeader()->setSectionResizeMode(
        BorrowerTableModel::LastName, QHeaderView::Stretch);
}

void BorrowerTab::refresh() {
    std::vector<Borrower> borrowers;
    const ServiceError error =
        service_.findBorrowers(toStdString(searchEdit_->text()), borrowers);

    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Ausleiher suchen"));
        countLabel_->clear();
        return;
    }

    const int count = static_cast<int>(borrowers.size());
    model_->setBorrowers(std::move(borrowers));

    countLabel_->setText(
        count == 1
            ? QStringLiteral("1 Ausleiher")
            : QStringLiteral("%1 Ausleiher").arg(count));

    updateActions();
}

void BorrowerTab::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (loaded_) return;

    loaded_ = true;
    refresh();
    table_->resizeColumnsToContents();
}

int BorrowerTab::selectedRow() const {
    const QModelIndexList rows = table_->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return rows.first().row();
}

void BorrowerTab::updateActions() {
    const bool hasSelection = selectedRow() >= 0;
    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
}

void BorrowerTab::addBorrower() {
    BorrowerDialog dialog(toQString(service_.getRecordedBy()), this);
    if (dialog.exec() != QDialog::Accepted) return;

    Borrower borrower = dialog.borrower();
    const ServiceError error = service_.addBorrower(borrower);
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Ausleiher anlegen"));
        return;
    }

    refresh();
}

void BorrowerTab::editSelected() {
    const int row = selectedRow();
    if (row < 0) return;

    BorrowerDialog dialog(model_->borrowerAt(row), this);
    if (dialog.exec() != QDialog::Accepted) return;

    const ServiceError error = service_.updateBorrower(dialog.borrower());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Ausleiher bearbeiten"));
        return;
    }

    refresh();
}

void BorrowerTab::deleteSelected() {
    const int row = selectedRow();
    if (row < 0) return;

    const Borrower& borrower = model_->borrowerAt(row);

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("Ausleiher löschen"),
        QStringLiteral("Soll \"%1, %2\" wirklich gelöscht werden?")
            .arg(toQString(borrower.getLastName()),
                 toQString(borrower.getFirstName())),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    const ServiceError error = service_.deleteBorrower(borrower.getId());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Ausleiher löschen"));
        return;
    }

    refresh();
}

} // namespace libster::gui
