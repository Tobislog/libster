#include "gui/BorrowerTableModel.h"

#include <utility>

#include <QVariant>

#include "gui/GuiUtil.h"

namespace libster::gui {

BorrowerTableModel::BorrowerTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int BorrowerTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(borrowers_.size());
}

int BorrowerTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant BorrowerTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const Borrower& borrower =
        borrowers_[static_cast<std::size_t>(index.row())];

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Id) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role != Qt::DisplayRole) return {};

    switch (index.column()) {
        case Id:         return QVariant::fromValue(borrower.getId());
        case LastName:   return toQString(borrower.getLastName());
        case FirstName:  return toQString(borrower.getFirstName());
        case Email:      return toQString(borrower.getEmail());
        case Address:    return toQString(borrower.getAddress());
        case RecordedOn: return toQString(borrower.getRecordedOn());
        default:         return {};
    }
}

QVariant BorrowerTableModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return {};
    }

    switch (section) {
        case Id:         return QStringLiteral("ID");
        case LastName:   return QStringLiteral("Nachname");
        case FirstName:  return QStringLiteral("Vorname");
        case Email:      return QStringLiteral("E-Mail");
        case Address:    return QStringLiteral("Adresse");
        case RecordedOn: return QStringLiteral("Erfasst am");
        default:         return {};
    }
}

void BorrowerTableModel::setBorrowers(std::vector<Borrower> borrowers) {
    beginResetModel();
    borrowers_ = std::move(borrowers);
    endResetModel();
}

const Borrower& BorrowerTableModel::borrowerAt(int row) const {
    return borrowers_.at(static_cast<std::size_t>(row));
}


} // namespace libster::gui
