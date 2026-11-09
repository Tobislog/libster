#pragma once

#include <vector>

#include <QAbstractTableModel>

#include "core/model/CD.h"

namespace libster::gui {

class CDTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Id = 0,
        Title,
        Performer,
        Publisher,
        Genre,
        Location,
        Price,
        Duration,
        StatusColumn,
        ColumnCount
    };

    explicit CDTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setCDs(std::vector<CD> cds);
    const CD& cdAt(int row) const;

private:
    std::vector<CD> cds_;
};

} // namespace libster::gui
