#pragma once

#include <cstdint>

#include <QDialog>
#include <QString>

#include "core/model/CD.h"
#include "core/model/Ids.h"

class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace libster::gui {

class CDDialog final : public QDialog {
    Q_OBJECT

public:
    explicit CDDialog(QString recordedBy, QWidget* parent = nullptr);
    explicit CDDialog(const CD& cd, QWidget* parent = nullptr);

    CD cd() const;

private slots:
    void validateAndAccept();

private:
    void setupUi();
    void setMetadata(const QString& addedOn, const QString& recordedBy);
    void loadCD(const CD& cd);
    bool validateInputs();
    void warn(const QString& message, QWidget* focus);

    bool editMode_ = false;
    std::uint32_t id_ = kUnassignedId;
    QString addedOn_;
    QString recordedBy_;

    QLineEdit* titleEdit_ = nullptr;
    QLineEdit* performerEdit_ = nullptr;
    QLineEdit* publisherEdit_ = nullptr;
    QLineEdit* genreEdit_ = nullptr;
    QDateEdit* publishedOnEdit_ = nullptr;
    QLineEdit* locationEdit_ = nullptr;
    QDoubleSpinBox* priceEdit_ = nullptr;
    QSpinBox* durationEdit_ = nullptr;
    QLabel* metadataLabel_ = nullptr;
};

} // namespace libster::gui
