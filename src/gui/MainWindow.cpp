#include "gui/MainWindow.h"

#include <utility>

#include <QAction>
#include <QDir>
#include <QFont>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "core/services/LibraryService.h"

namespace {

QWidget* makePlaceholderTab(const QString& text) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);

    auto* label = new QLabel(text, page);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);

    QFont font = label->font();
    font.setPointSize(font.pointSize() +2);
    label->setFont(font);

    layout->addWidget(label);
    return page;
} 
    
} // namespace

namespace libster::gui {

MainWindow::MainWindow(LibraryService& service, QString dataDirectory, QString recordedBy, QWidget* parent) 
    :   QMainWindow(parent),
        service_(service),
        dataDirectory_(std::move(dataDirectory)),
        recordedBy_(std::move(recordedBy)) {
    setWindowTitle(QStringLiteral("Libster - Bibliotheksverwaltung"));
    resize(1100, 700);

    createTabs();
    createMenus();
    createStatusBar();
}

void MainWindow::createTabs() {
    auto* tabs = new QTabWidget(this);
    
    tabs->addTab(makePlaceholderTab(QStringLiteral("Die Buchverwaltung folgt in M5.2.")), QStringLiteral("Bücher"));
    tabs->addTab(makePlaceholderTab(QStringLiteral("Die CD-Verwaltung folgt in M5.2.")), QStringLiteral("CDs"));
    tabs->addTab(makePlaceholderTab(QStringLiteral("Die Ausleiherverwaltung folgt in M5.3.")), QStringLiteral("Ausleiher"));
    tabs->addTab(makePlaceholderTab(QStringLiteral("Das Berichtsmodul folgt in M5.5.")), QStringLiteral("Bericht"));

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

} // namespace libster::gui
