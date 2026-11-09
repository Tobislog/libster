#include "gui/BookTableModel.h"

#include <utility>

#include <QVariant>

#include <gui/GuiUtil.h>

namespace libster::gui {

BookTableModel::BookTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int BookTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(books_.size());
}

int BookTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

QVariant BookTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const Book& book = books_[static_cast<std::size_t>(index.row())];

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Id || index.column() == Price) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (index.column()) {
        case Id:            return QVariant::fromValue(book.getId());
        case Title:         return toQString(book.getTitle());
        case Authors:       return toQString(book.getAuthors());
        case Isbn:          return toQString(book.getIsbn());
        case Publisher:     return toQString(book.getPublisher());
        case Genre:         return toQString(book.getGenre());
        case Location:      return toQString(book.getLocation());
        case Price:         return formatPrice(book.getPrice());
        case StatusColumn:  return statusText(book.getStatus());
        default:            return {};
    }
}

QVariant BookTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return {};
    }

    switch (section) {
        case Id:            return QStringLiteral("ID");
        case Title:         return QStringLiteral("Titel");
        case Authors:       return QStringLiteral("Autor(en)");
        case Isbn:          return QStringLiteral("ISBN");
        case Publisher:     return QStringLiteral("Verlag");
        case Genre:         return QStringLiteral("Genre");
        case Location:      return QStringLiteral("Standort");
        case Price:         return QStringLiteral("Preis");
        case StatusColumn:  return QStringLiteral("Status");
        default:            return {};
    }
}

void BookTableModel::setBooks(std::vector<Book> books) {
    beginResetModel();
    books_ = std::move(books);
    endResetModel();
}

const Book& BookTableModel::bookAt(int row) const {
    return books_.at(static_cast<std::size_t>(row));
}

} // namespace libster::gui
