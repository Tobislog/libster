#pragma once

#include <vector>

#include <QAbstractTableModel>

#include "core/model/Borrower.h"

namespace libster::gui {
class BorrowerTableModel final : public QAbstractTableModel {
    Q_OBJECT
    
public:
    enum Column {
        Id = 0,
        LastName,
        FirstName,
        Email,
        Address,
        RecordedOn,
        ColumnCount
    };

    explicit BorrowerTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setBorrowers(std::vector<Borrower> borrowers);
    const Borrower& borrowerAt(int row) const;

private:
    std::vector<Borrower> borrowers_;
};

} // namespace libster::gui
