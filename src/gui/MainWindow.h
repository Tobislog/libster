#pragma once

#include <QMainWindow>
#include <QString>

namespace libster {
    class LibraryService;
}

namespace libster::gui {

class BookTab;
class CDTab;
class BorrowerTab;
class LoanSidebar;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(LibraryService& service, QString dataDirectory, QString recordedBy, QWidget* parent = nullptr);

private:
    void createTabs();
    void createLoanSidebar();
    void createMenus();
    void createStatusBar();

    LibraryService& service_;
    QString dataDirectory_;
    QString recordedBy_;
    BookTab* bookTab_ = nullptr;
    CDTab* cdTab_ = nullptr;
    BorrowerTab* borrowerTab_ = nullptr;
    LoanSidebar* loanSidebar_ = nullptr;

};

} // namespace libster::gui
