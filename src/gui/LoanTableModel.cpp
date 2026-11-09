#include "gui/LoanTableModel.h"

#include <utility>

#include <QVariant>

namespace libster::gui {

LoanTableModel::LoanTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int LoanTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(entries_.size());
}

int LoanTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant LoanTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const LoanEntry& entry = entries_[static_cast<std::size_t>(index.row())];

    if (role != Qt::DisplayRole) return {};

    switch (index.column()) {
        case Type:
            return entry.type == MediumType::BOOK
                ? QStringLiteral("Buch")
                : QStringLiteral("CD");
        case Title:    return entry.title;
        case LoanDate: return entry.loanDate;
        default:       return {};
    }
}

QVariant LoanTableModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return {};
    }

    switch (section) {
        case Type:     return QStringLiteral("Typ");
        case Title:    return QStringLiteral("Medium");
        case LoanDate: return QStringLiteral("Ausgeliehen am");
        default:       return {};
    }
}

void LoanTableModel::setEntries(std::vector<LoanEntry> entries) {
    beginResetModel();
    entries_ = std::move(entries);
    endResetModel();
}

const LoanEntry& LoanTableModel::entryAt(int row) const {
    return entries_.at(static_cast<std::size_t>(row));
}

} // namespace libster::gui
