#pragma once

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

class BorrowerTableModel;

class BorrowerTab final : public QWidget {
    Q_OBJECT

public:
    explicit BorrowerTab(LibraryService& service, QWidget* parent = nullptr);

    void refresh();

protected:
    void showEvent(QShowEvent* event) override;    

private:
    void addBorrower();
    void editSelected();
    void deleteSelected();
    void updateActions();
    int selectedRow() const;

    LibraryService& service_;
    BorrowerTableModel* model_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QTimer* searchTimer_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
    bool loaded_ = false;
};

} // namespace libster::gui
