#include "gui/MainWindow.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <QAction>
#include <QByteArray>
#include <QCloseEvent>
#include <QDir>
#include <QEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
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

    createStatusBar();
    createLoanSidebar();
    createTabs();
    createMenus();

    restoreWindowState();
}

void MainWindow::createLoanSidebar() {
    loanSidebar_ = new LoanSidebar(service_, this);
    loanSidebar_->setMinimumWidth(340);
}

void MainWindow::createTabs() {
    splitter_ = new QSplitter(Qt::Horizontal, this);
    splitter_->setChildrenCollapsible(false);
    splitter_->addWidget(loanSidebar_);

    auto* tabs = new QTabWidget(splitter_);

    bookTab_ = new BookTab(service_, tabs);
    cdTab_ = new CDTab(service_, tabs);
    borrowerTab_ = new BorrowerTab(service_, tabs);
    reportTab_ = new ReportTab(service_, tabs);

    tabs->addTab(bookTab_, QStringLiteral("Bücher"));
    tabs->addTab(cdTab_, QStringLiteral("CDs"));
    tabs->addTab(borrowerTab_, QStringLiteral("Ausleiher"));
    tabs->addTab(reportTab_, QStringLiteral("Bericht"));

    splitter_->addWidget(tabs);
    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({340, 760});

    setCentralWidget(splitter_);

    connect(loanSidebar_, &LoanSidebar::borrowerChanged,
            this, &MainWindow::updateBorrowerStatus);

    connect(loanSidebar_, &LoanSidebar::borrowerChanged,
            bookTab_, &BookTab::setBorrowerId);
    connect(loanSidebar_, &LoanSidebar::borrowerChanged,
            cdTab_, &CDTab::setBorrowerId);

    connect(loanSidebar_, &LoanSidebar::loansChanged,
            bookTab_, &BookTab::refresh);
    connect(loanSidebar_, &LoanSidebar::loansChanged,
            cdTab_, &CDTab::refresh);

    connect(bookTab_, &BookTab::loansChanged,
            loanSidebar_, &LoanSidebar::refresh);
    connect(cdTab_, &CDTab::loansChanged,
            loanSidebar_, &LoanSidebar::refresh);
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&Datei"));
    auto* quitAction = fileMenu->addAction(QStringLiteral("&Beenden"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto* viewMenu = menuBar()->addMenu(QStringLiteral("&Ansicht"));

    auto* showAction = viewMenu->addAction(QStringLiteral("Seitenleiste anzeigen"));
    showAction->setCheckable(true);
    showAction->setChecked(true);
    connect(showAction, &QAction::toggled, this, [this](bool visible) {
        if (loanWindow_ != nullptr && loanSidebar_->parentWidget() == loanWindow_)
            loanWindow_->setVisible(visible);
        else
            loanSidebar_->setVisible(visible);
    });

    viewMenu->addSeparator();

    floatLoanAction_ = viewMenu->addAction(QStringLiteral("Ausleihe in eigenem Fenster"));
    connect(floatLoanAction_, &QAction::triggered, this, &MainWindow::floatLoanSidebar);

    dockLoanAction_ = viewMenu->addAction(QStringLiteral("Ausleihe andocken"));
    connect(dockLoanAction_, &QAction::triggered, this, &MainWindow::dockLoanSidebar);

    updateLoanSidebarActions();

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

void MainWindow::createStatusBar() {
    borrowerStatusLabel_ = new QLabel(QStringLiteral("Nicht angemeldet"), this);
    // Abstand zur linken Kante: sonst liegt der erste Buchstabe im
    // abgerundeten Eckbereich der Statusleiste.
    borrowerStatusLabel_->setContentsMargins(12, 0, 8, 0);
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

void MainWindow::floatLoanSidebar() {
    if (loanWindow_ == nullptr) {
        loanWindow_ = new QWidget(this, Qt::Window);
        loanWindow_->setWindowTitle(QStringLiteral("Ausleihe"));
        auto* layout = new QVBoxLayout(loanWindow_);
        layout->setContentsMargins(0, 0, 0, 0);
        loanWindow_->installEventFilter(this);
    }

    loanWindow_->layout()->addWidget(loanSidebar_);
    loanWindow_->resize(380, 620);
    loanWindow_->show();
    loanWindow_->raise();
    loanWindow_->activateWindow();

    updateLoanSidebarActions();
}

void MainWindow::dockLoanSidebar() {
    if (loanWindow_ != nullptr)
        loanWindow_->hide();

    splitter_->insertWidget(0, loanSidebar_);
    splitter_->setSizes({340, std::max(0, splitter_->width() - 340)});
    loanSidebar_->show();

    updateLoanSidebarActions();
}

void MainWindow::updateLoanSidebarActions() {
    const bool poppedOut = loanWindow_ != nullptr
        && loanSidebar_->parentWidget() == loanWindow_;

    if (floatLoanAction_ != nullptr) floatLoanAction_->setEnabled(!poppedOut);
    if (dockLoanAction_ != nullptr) dockLoanAction_->setEnabled(poppedOut);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == loanWindow_ && event->type() == QEvent::Close) {
        dockLoanSidebar();
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
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

    const QByteArray splitterState = settings.value(QStringLiteral("window/splitter")).toByteArray();
    if (!splitterState.isEmpty()) splitter_->restoreState(splitterState);
}

void MainWindow::saveWindowState() {
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());

    // Nur speichern, wenn die Seitenleiste angedockt ist – sonst würde der
    // Splitter-Zustand mit nur einem Widget überschrieben.
    if (loanWindow_ == nullptr || loanSidebar_->parentWidget() != loanWindow_) {
        settings.setValue(QStringLiteral("window/splitter"), splitter_->saveState());
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveWindowState();
    QMainWindow::closeEvent(event);
}

} // namespace libster::gui
