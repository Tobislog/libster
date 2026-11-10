#pragma once

#include <QWidget>

class QDateEdit;
class QLabel;
class QShowEvent;

namespace libster {
class LibraryService;
}

namespace libster::gui {

class ReportTab final : public QWidget {
    Q_OBJECT

public:
    explicit ReportTab(LibraryService& service, QWidget* parent = nullptr);

    void refresh();

protected:
    void showEvent(QShowEvent* event) override;

private:
    LibraryService& service_;
    QDateEdit* dateEdit_ = nullptr;
    QLabel* loanCountLabel_ = nullptr;
    QLabel* returnCountLabel_ = nullptr;
};

}
