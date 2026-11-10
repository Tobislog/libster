#pragma once

#include <cstdint>

#include <QMainWindow>
#include <QString>

class QAction;
class QCloseEvent;
class QEvent;
class QLabel;
class QSplitter;

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
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void createTabs();
    void createLoanSidebar();
    void createMenus();
    void createStatusBar();
    void dockLoanSidebar();
    void floatLoanSidebar();
    void updateLoanSidebarActions();
    void restoreWindowState();
    void saveWindowState();
    void updateBorrowerStatus(std::uint32_t borrowerId);

    LibraryService& service_;
    QString dataDirectory_;
    QString recordedBy_;
    BookTab* bookTab_ = nullptr;
    CDTab* cdTab_ = nullptr;
    BorrowerTab* borrowerTab_ = nullptr;
    ReportTab* reportTab_ = nullptr;
    LoanSidebar* loanSidebar_ = nullptr;
    QSplitter* splitter_ = nullptr;
    QWidget* loanWindow_ = nullptr;
    QLabel* borrowerStatusLabel_ = nullptr;
    QAction* floatLoanAction_ = nullptr;
    QAction* dockLoanAction_ = nullptr;

};

} // namespace libster::gui
