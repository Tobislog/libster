#pragma once

#include <cstdint>

#include <QDialog>
#include <QString>

#include "core/model/Borrower.h"
#include "core/model/Ids.h"

class QLabel;
class QLineEdit;

namespace libster::gui {

class BorrowerDialog final : public QDialog {
    Q_OBJECT

public:
    explicit BorrowerDialog(QString recordedBy, QWidget* parent = nullptr);
    explicit BorrowerDialog(const Borrower& borrower, QWidget* parent = nullptr);

    Borrower borrower() const;

private slots:
    void validateAndAccept();

private:
    void setupUi();
    void setMetadata(const QString& recordedOn, const QString& recordedBy);
    void loadBorrower(const Borrower& borrower);
    bool validateInputs();
    void warn(const QString& message, QWidget* focus);

    bool editMode_ = false;
    std::uint32_t id_ = kUnassignedId;
    QString recordedOn_;
    QString recordedBy_;

    QLineEdit* lastNameEdit_ = nullptr;
    QLineEdit* firstNameEdit_ = nullptr;
    QLineEdit* emailEdit_ = nullptr;
    QLineEdit* addressEdit_ = nullptr;
    QLabel* metadataLabel_ = nullptr;
};

} // namespace libster::gui
