#include "gui/ReportTab.h"

#include <cstdint>
#include <string>

#include <QDate>
#include <QDateEdit>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

#include "core/TimeUtil.h"
#include "core/services/DailyReport.h"
#include "core/services/LibraryService.h"
#include "gui/GuiUtil.h"

namespace libster::gui {

ReportTab::ReportTab(LibraryService& service, QWidget* parent) : QWidget(parent), service_(service) {
    auto* dateLabel = new QLabel(QStringLiteral("Tag:"), this);

    dateEdit_ = new QDateEdit(this);
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    dateEdit_->setDate(QDate::currentDate());

    auto* showButton = new QPushButton(QStringLiteral("Bericht anzeigen"), this);

    auto* dateRow = new QHBoxLayout;
    dateRow->addWidget(dateLabel);
    dateRow->addWidget(dateEdit_);
    dateRow->addWidget(showButton);
    dateRow->addStretch(1);

    loanCountLabel_ = new QLabel(this);
    returnCountLabel_ = new QLabel(this);

    QFont countFont = loanCountLabel_->font();
    countFont.setPointSize(countFont.pointSize() + 8);
    countFont.setBold(true);

    loanCountLabel_->setFont(countFont);
    returnCountLabel_->setFont(countFont);
    loanCountLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    returnCountLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Ausleihen:"), loanCountLabel_);
    form->addRow(QStringLiteral("Rückgaben:"), returnCountLabel_);

    auto* box = new QGroupBox(QStringLiteral("Tagesbericht"), this);
    box->setLayout(form);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(dateRow);
    layout->addWidget(box);
    layout->addStretch(1);

    connect(showButton, &QPushButton::clicked, this, &ReportTab::refresh);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &ReportTab::refresh);

    refresh();
}

void ReportTab::refresh() {
    const std::string isoDate = toStdString(dateEdit_->date().toString(Qt::ISODate));

    std::int64_t dayStart = 0;
    std::int64_t dayEnd = 0;

    if (!isoDateToDayRange(isoDate, dayStart, dayEnd)) {
        loanCountLabel_->setText(QStringLiteral("-"));
        returnCountLabel_->setText(QStringLiteral("-"));
        return;
    }

    DailyReport report;
    const ServiceError error = service_.dailyReport(dayStart, dayEnd, report);

    if (error != ServiceError::None) {
        showServiceError(this, error, QStringLiteral("Bericht laden"));
        loanCountLabel_->setText(QStringLiteral("-"));
        returnCountLabel_->setText(QStringLiteral("-"));
        return;
    }

    loanCountLabel_->setText(QString::number(report.loanCount));
    returnCountLabel_->setText(QString::number(report.returnCount));
}

void ReportTab::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refresh();
}


}
