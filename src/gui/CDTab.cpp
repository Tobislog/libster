#include "gui/CDTab.h"

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

#include "core/model/CD.h"
#include "core/services/LibraryService.h"
#include "gui/CDTableModel.h"
#include "gui/GuiUtil.h"

namespace libster::gui {

CDTab::CDTab(LibraryService& service, QWidget* parent)
    : QWidget(parent), service_(service) {
    auto* searchLabel = new QLabel(QStringLiteral("Suche:"), this);

    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(
        QStringLiteral("Titel, Interpret, Verlag oder Standort – leer zeigt alle"));
    searchEdit_->setClearButtonEnabled(true);

    countLabel_ = new QLabel(this);

    auto* searchRow = new QHBoxLayout;
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(searchEdit_, 1);
    searchRow->addWidget(countLabel_);

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
    layout->addWidget(table_, 1);

    searchTimer_ = new QTimer(this);
    searchTimer_->setSingleShot(true);
    searchTimer_->setInterval(200);

    connect(searchEdit_, &QLineEdit::textChanged, this, [this] {
        searchTimer_->start();
    });
    connect(searchTimer_, &QTimer::timeout, this, &CDTab::refresh);

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
}

} // namespace libster::gui
