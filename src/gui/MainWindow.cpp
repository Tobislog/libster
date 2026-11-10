#include "gui/MainWindow.h"

#include <utility>

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QWidget>

#include "core/services/LibraryService.h"
#include "gui/BookTab.h"
#include "gui/CDTab.h"
#include "gui/BorrowerTab.h"
#include "gui/LoanSidebar.h"
#include "gui/ReportTab.h"

namespace libster::gui {

MainWindow::MainWindow(LibraryService& service, QString dataDirectory, QString recordedBy, QWidget* parent) 
    :   QMainWindow(parent),
        service_(service),
        dataDirectory_(std::move(dataDirectory)),
        recordedBy_(std::move(recordedBy)) {
    setWindowTitle(QStringLiteral("Libster - Bibliotheksverwaltung"));
    resize(1100, 700);

    createTabs();
    createLoanSidebar();
    createMenus();
    createStatusBar();
}

void MainWindow::createTabs() {
    auto* tabs = new QTabWidget(this);

    bookTab_ = new BookTab(service_, tabs);
    cdTab_ = new CDTab(service_, tabs);
    borrowerTab_ = new BorrowerTab(service_, tabs);
    reportTab_ = new ReportTab(service_, tabs);
    
    tabs->addTab(bookTab_, QStringLiteral("Bücher"));
    tabs->addTab(cdTab_, QStringLiteral("CDs"));
    tabs->addTab(borrowerTab_, QStringLiteral("Ausleiher"));
    tabs->addTab(reportTab_, QStringLiteral("Bericht"));

    setCentralWidget(tabs);
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&Datei"));
    auto* quitAction = fileMenu->addAction(QStringLiteral("&Beenden"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::createStatusBar() {
    const QString state = service_.isOpen() ? QStringLiteral("geöffnet") : QStringLiteral("nicht geöffnet");

    auto* dataLabel = new QLabel(this);
    dataLabel->setText(QStringLiteral("Daten: %1 (%2)").arg(QDir::toNativeSeparators(dataDirectory_), state));
    dataLabel->setToolTip(dataDirectory_);
    dataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusBar()->addPermanentWidget(dataLabel);

    auto* userLabel = new QLabel(
        QStringLiteral("Erfasst von: %1").arg(recordedBy_), this);
    statusBar()->addPermanentWidget(userLabel);
}

void MainWindow::createLoanSidebar() {
    loanSidebar_ = new LoanSidebar(service_, this);
    loanSidebar_->setMinimumWidth(340);

    auto* dock = new QDockWidget(QStringLiteral("Ausleihe"), this);
    dock->setObjectName(QStringLiteral("loanDock"));
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dock->setWidget(loanSidebar_);

    addDockWidget(Qt::LeftDockWidgetArea, dock);

    connect(loanSidebar_, &LoanSidebar::borrowerChanged, bookTab_, &BookTab::setBorrowerId);
    connect(loanSidebar_, &LoanSidebar::borrowerChanged, cdTab_, &CDTab::setBorrowerId);

    connect(loanSidebar_, &LoanSidebar::loansChanged,bookTab_, &BookTab::refresh);
    connect(loanSidebar_, &LoanSidebar::loansChanged,cdTab_, &CDTab::refresh);

    connect(bookTab_, &BookTab::loansChanged, loanSidebar_, &LoanSidebar::refresh);
    connect(cdTab_, &CDTab::loansChanged, loanSidebar_, &LoanSidebar::refresh);
}

} // namespace libster::gui
