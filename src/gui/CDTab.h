#pragma once

#include <cstdint>

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QTableView;
class QTimer;

namespace libster {
class LibraryService;
}

namespace libster::gui {

class CDTableModel;

class CDTab final : public QWidget {
    Q_OBJECT

public:
    explicit CDTab(LibraryService& service, QWidget* parent = nullptr);

    void refresh();
    void setBorrowerId(std::uint32_t borrowerId);

protected:
    void showEvent(QShowEvent* event) override;

signals:
    void loansChanged();

private:
    void addCD();
    void editSelected();
    void deleteSelected();
    void loanSelected();
    void handleDoubleClick();
    void updateActions();
    int selectedRow() const;

    LibraryService& service_;
    CDTableModel* model_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QTimer* searchTimer_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    QPushButton* loanButton_ = nullptr;
    std::uint32_t borrowerId_ = 0;
    bool loaded_ = false;
};

} // namespace libster::gui
