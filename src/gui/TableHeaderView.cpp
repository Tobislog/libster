#include "gui/TableHeaderView.h"

#include <QPainter>
#include <QPaintEvent>

namespace libster::gui {

TableHeaderView::TableHeaderView(Qt::Orientation orientation, QWidget* parent)
    : QHeaderView(orientation, parent) {}

void TableHeaderView::paintEvent(QPaintEvent* event) {
    QHeaderView::paintEvent(event);

    if (orientation() != Qt::Horizontal) return;

    QPainter painter(viewport());
    painter.setPen(palette().color(QPalette::Mid));

    const int y = viewport()->height() - 1;
    painter.drawLine(0, y, viewport()->width() - 1, y);
}

} // namespace libster::gui
