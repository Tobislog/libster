#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
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

private:
    void addCD();
    void editSelected();
    void deleteSelected();
    void updateActions();
    int selectedRow() const;

    LibraryService& service_;
    CDTableModel* model_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QTimer* searchTimer_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
};

} // namespace libster::gui
