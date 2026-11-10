#include "gui/MainWindow.h"

#include <optional>
#include <utility>

#include <QAction>
#include <QByteArray>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QWidget>

#include "core/Version.h"
#include "core/model/Borrower.h"
#include "core/services/LibraryService.h"
#include "gui/BookTab.h"
#include "gui/BorrowerTab.h"
#include "gui/CDTab.h"
#include "gui/GuiUtil.h"
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
    createStatusBar();
    createLoanSidebar();
    createMenus();

    restoreWindowState();
    updateLoanDockAction();
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

    auto* viewMenu = menuBar()->addMenu(QStringLiteral("&Ansicht"));
    if (loanDock_ != nullptr) {
        viewMenu->addAction(loanDock_->toggleViewAction());
        viewMenu->addSeparator();

        loanDockAction_ = viewMenu->addAction(QStringLiteral("Ausleihe lösen"));

        connect(loanDockAction_, &QAction::triggered, this, [this] {
    if (loanDock_->isFloating()) {
        loanDock_->setFloating(false);
        removeDockWidget(loanDock_);
        addDockWidget(Qt::LeftDockWidgetArea, loanDock_);
        loanDock_->show();
    } else {
        loanDock_->setFloating(true);
        loanDock_->show();
        loanDock_->raise();
        loanDock_->activateWindow();
    }
    updateLoanDockAction();
});

        connect(loanDock_, &QDockWidget::topLevelChanged, this, [this](bool) { updateLoanDockAction(); });
    }

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Hilfe"));
    auto* aboutAction = helpMenu->addAction(QStringLiteral("Über Libster"));

    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this,
            QStringLiteral("Über Libster"),
            QStringLiteral(
                "<h3>Libster %1</h3>"
                "<p>Bibliotheksverwaltung mit Qt %2.</p>"
                "<p>Bücher, CDs und Ausleiher werden in Binärdateien "
                "gespeichert.</p>")
                .arg(toQString(version()), QString::fromLatin1(qVersion())));
    });
}

void MainWindow::updateLoanDockAction() {
    if (loanDockAction_ == nullptr || loanDock_ == nullptr) return;

    loanDockAction_->setText(
        loanDock_->isFloating() ? QStringLiteral("Ausleihe andocken") : QStringLiteral("Ausleihe lösen")
    );
}

void MainWindow::createStatusBar() {
    borrowerStatusLabel_ = new QLabel(QStringLiteral("Nicht angemeldet"), this);
    statusBar()->addWidget(borrowerStatusLabel_);

    const QString state = service_.isOpen() ? QStringLiteral("geöffnet") : QStringLiteral("nicht geöffnet");

    auto* dataLabel = new QLabel(this);
    dataLabel->setText(QStringLiteral("Daten: %1 (%2)").arg(QDir::toNativeSeparators(dataDirectory_), state));
    dataLabel->setToolTip(dataDirectory_);
    dataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusBar()->addPermanentWidget(dataLabel);

    auto* userLabel = new QLabel(
        QStringLiteral("Erfasser: %1").arg(recordedBy_), this);
    statusBar()->addPermanentWidget(userLabel);
}

void MainWindow::createLoanSidebar() {
    loanSidebar_ = new LoanSidebar(service_, this);
    loanSidebar_->setMinimumWidth(340);

    loanDock_ = new QDockWidget(QStringLiteral("Ausleihe"), this);
    loanDock_->setObjectName(QStringLiteral("loanDock"));
    loanDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    loanDock_->setWidget(loanSidebar_);

    addDockWidget(Qt::LeftDockWidgetArea, loanDock_);

    connect(loanSidebar_, &LoanSidebar::borrowerChanged, this, &MainWindow::updateBorrowerStatus);

    connect(loanSidebar_, &LoanSidebar::borrowerChanged, bookTab_, &BookTab::setBorrowerId);
    connect(loanSidebar_, &LoanSidebar::borrowerChanged, cdTab_, &CDTab::setBorrowerId);

    connect(loanSidebar_, &LoanSidebar::loansChanged,bookTab_, &BookTab::refresh);
    connect(loanSidebar_, &LoanSidebar::loansChanged,cdTab_, &CDTab::refresh);

    connect(bookTab_, &BookTab::loansChanged, loanSidebar_, &LoanSidebar::refresh);
    connect(cdTab_, &CDTab::loansChanged, loanSidebar_, &LoanSidebar::refresh);
}

void MainWindow::updateBorrowerStatus(std::uint32_t borrowerId) {
    if (borrowerStatusLabel_ == nullptr) return;

    if (borrowerId == 0) {
        borrowerStatusLabel_->setText(QStringLiteral("Nicht angemeldet"));
        return;
    }

    std::optional<Borrower> borrower;
    if (service_.getBorrower(borrowerId, borrower) != ServiceError::None || !borrower.has_value()) {
        borrowerStatusLabel_->setText(QStringLiteral("Angemeldet: ID %1").arg(borrowerId));
        return;
    }

    borrowerStatusLabel_->setText(
        QStringLiteral("Angemeldet: %1, %2 (ID %3)")
            .arg(toQString(borrower->getLastName()), toQString(borrower->getFirstName()))
            .arg(borrowerId)
    );
}     

void MainWindow::restoreWindowState() {
    QSettings settings;

    const QByteArray geometry = settings.value(QStringLiteral("window/geometry")).toByteArray();
    if (!geometry.isEmpty()) restoreGeometry(geometry);

    const QByteArray state = settings.value(QStringLiteral("window/state")).toByteArray();
    if (!state.isEmpty()) restoreState(state);

    if (loanDock_ != nullptr && loanDock_->isFloating()) {
        loanDock_->setFloating(false);
        removeDockWidget(loanDock_);
        addDockWidget(Qt::LeftDockWidgetArea, loanDock_);
        loanDock_->show();
    }
}

void MainWindow::saveWindowState() {
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveWindowState();
    QMainWindow::closeEvent(event);
}

} // namespace libster::gui
