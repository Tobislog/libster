#include "gui/BookTab.h"

#include <utility>
#include <vector>

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

#include "core/model/Book.h"
#include "core/services/LibraryService.h"
#include "gui/BookTableModel.h"
#include "gui/GuiUtil.h"

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

    model_ = new BookTableModel(this);

    table_ = new QTableView(this);
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(searchRow);
    layout->addWidget(table_, 1);

    searchTimer_ = new QTimer(this);
    searchTimer_->setSingleShot(true);
    searchTimer_->setInterval(200);

    connect(searchEdit_, &QLineEdit::textChanged, this, [this] {
        searchTimer_->start();
    });
    connect(searchTimer_, &QTimer::timeout, this, &BookTab::refresh);

    refresh();

    table_->resizeColumnsToContents();
    table_->horizontalHeader()->setSectionResizeMode(BookTableModel::Title, QHeaderView::Stretch);
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
}

} // namespace libster::gui
