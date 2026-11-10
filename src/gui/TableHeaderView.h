#pragma once

#include <QHeaderView>

namespace libster::gui {

// Kopfzeile mit feiner Trennlinie zum Tabelleninhalt. Der native Stil
// (macOS, Fusion) zeichnet unter der Kopfzeile sonst keinen Rand.
class TableHeaderView final : public QHeaderView {
    Q_OBJECT

public:
    explicit TableHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace libster::gui
