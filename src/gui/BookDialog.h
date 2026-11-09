#pragma once

#include <cstdint>

#include <QDialog>
#include <QString>

#include "core/model/Book.h"
#include "core/model/Ids.h"

class QDateEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSpinBox;

namespace libster::gui {

class BookDialog final : public QDialog {
    Q_OBJECT

public:
    explicit BookDialog(QString recordedBy, QWidget* parent = nullptr);
    explicit BookDialog(const Book& book, QWidget* parent = nullptr);

    Book book() const;

private slots:
    void validateAndAccept();

private:
    void setupUi();
    void setMetadata(const QString& addedOn, const QString& recordedBy);
    void loadBook(const Book& book);
    bool validateInputs();
    void warn(const QString& message, QWidget* focus);

    bool editMode_ = false;
    std::uint32_t id_ = kUnassignedId;
    QString addedOn_;
    QString recordedBy_;

    QLineEdit* titleEdit_ = nullptr;
    QLineEdit* authorsEdit_ = nullptr;
    QLineEdit* isbnEdit_ = nullptr;
    QLineEdit* publisherEdit_ = nullptr;
    QLineEdit* genreEdit_ = nullptr;
    QDateEdit* publishedOnEdit_ = nullptr;
    QLineEdit* locationEdit_ = nullptr;
    QDoubleSpinBox* priceEdit_ = nullptr;
    QSpinBox* editionEdit_ = nullptr;
    QSpinBox* pageCountEdit_ = nullptr;
    QLabel* metadataLabel_ = nullptr;

}; 

} // namespace libster::gui
