#include "gui/CDTableModel.h"

#include <utility>

#include <QVariant>

#include "gui/GuiUtil.h"

namespace libster::gui {

CDTableModel::CDTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int CDTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(cds_.size());
}

int CDTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant CDTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const CD& cd = cds_[static_cast<std::size_t>(index.row())];

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Id || index.column() == Price
            || index.column() == Duration) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (index.column()) {
        case Id:            return QVariant::fromValue(cd.getId());
        case Title:         return toQString(cd.getTitle());
        case Performer:     return toQString(cd.getPerformer());
        case Publisher:     return toQString(cd.getPublisher());
        case Genre:         return toQString(cd.getGenre());
        case Location:      return toQString(cd.getLocation());
        case Price:         return formatPrice(cd.getPrice());
        case Duration:      return QStringLiteral("%1 min").arg(cd.getDuration());
        case StatusColumn:  return statusText(cd.getStatus());
        default:            return {};
    }
}

QVariant CDTableModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return {};
    }

    switch (section) {
        case Id:            return QStringLiteral("ID");
        case Title:         return QStringLiteral("Titel");
        case Performer:     return QStringLiteral("Interpret");
        case Publisher:     return QStringLiteral("Label");
        case Genre:         return QStringLiteral("Genre");
        case Location:      return QStringLiteral("Standort");
        case Price:         return QStringLiteral("Preis");
        case Duration:      return QStringLiteral("Dauer");
        case StatusColumn:  return QStringLiteral("Status");
        default:            return {};
    }
}

void CDTableModel::setCDs(std::vector<CD> cds) {
    beginResetModel();
    cds_ = std::move(cds);
    endResetModel();
}

const CD& CDTableModel::cdAt(int row) const {
    return cds_.at(static_cast<std::size_t>(row));
}

} // namespace libster::gui
