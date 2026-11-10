#pragma once

#include <cstdint>

#include <QMainWindow>
#include <QString>

class QAction;
class QCloseEvent;
class QDockWidget;
class QLabel;

namespace libster {
    class LibraryService;
}

namespace libster::gui {

class BookTab;
class CDTab;
class BorrowerTab;
class LoanSidebar;
class ReportTab;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(LibraryService& service, QString dataDirectory, QString recordedBy, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createTabs();
    void createLoanSidebar();
    void createMenus();
    void createStatusBar();
    void restoreWindowState();
    void saveWindowState();
    void updateBorrowerStatus(std::uint32_t borrowerId);
    void updateLoanDockAction();

    LibraryService& service_;
    QString dataDirectory_;
    QString recordedBy_;
    BookTab* bookTab_ = nullptr;
    CDTab* cdTab_ = nullptr;
    BorrowerTab* borrowerTab_ = nullptr;
    LoanSidebar* loanSidebar_ = nullptr;
    QDockWidget* loanDock_ = nullptr;
    ReportTab* reportTab_ = nullptr;
    QLabel* borrowerStatusLabel_ = nullptr;
    QAction* loanDockAction_ = nullptr;

};

} // namespace libster::gui
