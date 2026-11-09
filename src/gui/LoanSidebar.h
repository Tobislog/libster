#pragma once

#include <cstdint>
#include <optional>

#include <QWidget>

#include "core/model/Borrower.h"
#include "core/model/LoanTransaction.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;

namespace libster {
class LibraryService;
}

namespace libster::gui {

class LoanTableModel;

class LoanSidebar final : public QWidget {
    Q_OBJECT

public:
    explicit LoanSidebar(LibraryService& service, QWidget* parent = nullptr);

    bool isLoggedIn() const noexcept;
    std::uint32_t borrowerId() const noexcept;

    void refresh();

signals:
    void borrowerChanged(std::uint32_t borrowerId); // 0 = abgemeldet
    void loansChanged();

private slots:
    void login();
    void logout();
    void returnSelected();

private:
    void updateActions();
    int selectedRow() const;
    QString mediumTitle(const LoanTransaction& loan) const;

    LibraryService& service_;
    std::optional<Borrower> borrower_;
    std::uint32_t borrowerId_ = 0;

    QLineEdit* idEdit_ = nullptr;
    QPushButton* loginButton_ = nullptr;
    QPushButton* logoutButton_ = nullptr;
    QLabel* nameLabel_ = nullptr;
    QTableView* table_ = nullptr;
    LoanTableModel* model_ = nullptr;
    QPushButton* returnButton_ = nullptr;
};

} // namespace libster::gui
