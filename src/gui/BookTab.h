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

class BookTableModel;

class BookTab final : public QWidget {
    Q_OBJECT

public:
    explicit BookTab(LibraryService& service, QWidget* parent = nullptr);

    void refresh();

private:
    void addBook();
    void editSelected();
    void deleteSelected();
    void updateActions();
    int selectedRow() const;

    LibraryService& service_;
    BookTableModel* model_ = nullptr;
    QLineEdit* searchEdit_ = nullptr;
    QTableView* table_ = nullptr;
    QLabel* countLabel_ = nullptr;
    QTimer* searchTimer_ = nullptr;
    QPushButton* editButton_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
};

} // namespace libster::gui
