#ifndef TABLEVIEWTOOLTIPFILTER_H
#define TABLEVIEWTOOLTIPFILTER_H
#include <QObject>
#include <QAbstractItemView>
#include <QHelpEvent>
#include <QToolTip>

class TableViewToolTipFilter : public QObject
{
 Q_OBJECT
public:
 explicit TableViewToolTipFilter(QObject* parent = NULL)
 : QObject(parent)
 {
 }

protected:
 bool eventFilter(QObject* obj, QEvent* event)
 {
 if (event->type() == QEvent::ToolTip)
 {
 QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj->parent());
 if (!view)
 {
 return false;
 }

QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
 QPoint pos = helpEvent->pos();
 QModelIndex index = view->indexAt(pos);
 if (!index.isValid())
 return false;

QString itemText = view->model()->data(index).toString();
 QString itemTooltip = view->model()->data(index, Qt::ToolTipRole).toString();

 QFontMetrics fm(view->font());
#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
 int itemTextWidth = fm.horizontalAdvance(itemText);
#else
 int itemTextWidth = fm.width(itemText);
#endif
 QRect rect = view->visualRect(index);
 int rectWidth = rect.width();

if ((itemTextWidth > rectWidth) && !itemTooltip.isEmpty())
 {
 QToolTip::showText(helpEvent->globalPos(), itemTooltip, view, rect);
 }
 else
 {
 QToolTip::hideText();
 }
 return true;
 }
 return false;
 }
};







#endif // TABLEVIEWTOOLTIPFILTER_H
