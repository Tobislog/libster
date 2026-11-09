#pragma once

#include <cstdint>
#include <vector>

#include <QAbstractTableModel>
#include <QString>

#include "core/model/MediumType.h"

namespace libster::gui {

struct LoanEntry {
    std::uint32_t loanId = 0;
    std::uint32_t mediumId = 0;
    MediumType type = MediumType::BOOK;
    QString title;
    QString loanDate;
};

class LoanTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        Type = 0,
        Title,
        LoanDate,
        ColumnCount
    };

    explicit LoanTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setEntries(std::vector<LoanEntry> entries);
    const LoanEntry& entryAt(int row) const;

private:
    std::vector<LoanEntry> entries_;
};

} // namespace libster::gui
