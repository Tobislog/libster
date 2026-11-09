#include "gui/CDTab.h"

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

#include "core/model/CD.h"
#include "core/services/LibraryService.h"
#include "gui/CDDialog.h"
#include "gui/CDTableModel.h"
#include "gui/GuiUtil.h"

namespace libster::gui {

CDTab::CDTab(LibraryService& service, QWidget* parent)
    : QWidget(parent), service_(service) {
    auto* searchLabel = new QLabel(QStringLiteral("Suche:"), this);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(
        QStringLiteral("Titel, Interpret, Label oder Standort – leer zeigt alle"));
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

    model_ = new CDTableModel(this);

    table_ = new QTableView(this);
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
    connect(searchTimer_, &QTimer::timeout, this, &CDTab::refresh);

    connect(addButton, &QPushButton::clicked, this, &CDTab::addCD);
    connect(editButton_, &QPushButton::clicked,
            this, &CDTab::editSelected);
    connect(deleteButton_, &QPushButton::clicked,
            this, &CDTab::deleteSelected);
    connect(table_, &QTableView::doubleClicked,
            this, &CDTab::editSelected);
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] { updateActions(); });

    refresh();

    table_->resizeColumnsToContents();
    table_->horizontalHeader()->setSectionResizeMode(
        CDTableModel::Title, QHeaderView::Stretch);
}

void CDTab::refresh() {
    std::vector<CD> cds;
    const ServiceError error =
        service_.findCDs(toStdString(searchEdit_->text()), cds);

    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("CDs suchen"));
        countLabel_->clear();
        return;
    }

    const int count = static_cast<int>(cds.size());
    model_->setCDs(std::move(cds));

    countLabel_->setText(
        count == 1
            ? QStringLiteral("1 CD")
            : QStringLiteral("%1 CDs").arg(count));

    updateActions();
}

int CDTab::selectedRow() const {
    const QModelIndexList rows = table_->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return rows.first().row();
}

void CDTab::updateActions() {
    const bool hasSelection = selectedRow() >= 0;
    editButton_->setEnabled(hasSelection);
    deleteButton_->setEnabled(hasSelection);
}

void CDTab::addCD() {
    CDDialog dialog(toQString(service_.getRecordedBy()), this);
    if (dialog.exec() != QDialog::Accepted) return;

    CD cd = dialog.cd();
    const ServiceError error = service_.addCD(cd);
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("CD anlegen"));
        return;
    }

    refresh();
}

void CDTab::editSelected() {
    const int row = selectedRow();
    if (row < 0) return;

    CDDialog dialog(model_->cdAt(row), this);
    if (dialog.exec() != QDialog::Accepted) return;

    const ServiceError error = service_.updateCD(dialog.cd());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("CD bearbeiten"));
        return;
    }

    refresh();
}

void CDTab::deleteSelected() {
    const int row = selectedRow();
    if (row < 0) return;

    const CD& cd = model_->cdAt(row);

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("CD löschen"),
        QStringLiteral("Soll \"%1\" wirklich gelöscht werden?")
            .arg(toQString(cd.getTitle())),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (answer != QMessageBox::Yes) return;

    const ServiceError error = service_.deleteCD(cd.getId());
    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("CD löschen"));
        return;
    }

    refresh();
}

} // namespace libster::gui
