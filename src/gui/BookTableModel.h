#pragma once

#include <vector>

#include <QAbstractTableModel>

#include "core/model/Book.h"

namespace libster::gui {

class BookTableModel final : public QAbstractTableModel {
    Q_OBJECT

public: 
    enum Column {
        Id = 0,
        Title,
        Authors,
        Isbn,
        Publisher,
        Genre,
        Location,
        Price,
        StatusColumn,
        ColumnCount
    };

    explicit BookTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setBooks(std::vector<Book> books);
    const Book& bookAt(int row) const;

private:
    std::vector<Book> books_;
};

} // namespace libster::gui
