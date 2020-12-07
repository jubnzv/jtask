#include "tasksview.hpp"

#include <QApplication>
#include <QDebug>
#include <QModelIndex>
#include <QMouseEvent>
#include <QTableView>

#include "taskdescriptiondelegate.hpp"
#include "tasksmodel.hpp"

TasksView::TasksView(QWidget *parent)
    : QTableView(parent)
{
    // needed for the hover functionality
    setMouseTracking(true);
}

void TasksView::mousePressEvent(QMouseEvent *event)
{
    constexpr int project_column = 1;

    QModelIndex idx = indexAt(event->pos());

    // Enable "stop" button if the selected task is active
    if (idx.isValid() && event->buttons() & Qt::LeftButton) {
        const auto task_opt = qobject_cast<TasksModel *>(model())->getTask(idx);
        if (task_opt.isValid()) {
            const auto task = task_opt.value<Task>();
            emit selectedTaskIsActive(task.active);
        }
    }

    // Right click to the "project" column will push it to taskwarrior filter
    if (idx.isValid() && idx.column() == project_column &&
        event->buttons() & Qt::RightButton) {
        auto d = idx.data();
        if (!d.isNull())
            emit pushProjectFilter("pro:" + d.toString());
    }

    auto anchor = anchorAt(event->pos());
    m_mouse_press_anchor = anchor;

    QTableView::mousePressEvent(event);
}

void TasksView::mouseMoveEvent(QMouseEvent *event)
{
    auto anchor = anchorAt(event->pos());

    if (m_mouse_press_anchor != anchor) {
        m_mouse_press_anchor.clear();
    }

    if (m_last_hovered_anchor != anchor) {
        m_last_hovered_anchor = anchor;
        if (!m_last_hovered_anchor.isEmpty()) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            emit linkHovered(m_last_hovered_anchor);
        } else {
            QApplication::restoreOverrideCursor();
            emit linkUnhovered();
        }
    }
}

void TasksView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_mouse_press_anchor.isEmpty()) {
        auto anchor = anchorAt(event->pos());

        if (anchor == m_mouse_press_anchor) {
            emit linkActivated(m_mouse_press_anchor);
        }

        m_mouse_press_anchor.clear();
    }

    QTableView::mouseReleaseEvent(event);
}

QString TasksView::anchorAt(const QPoint &pos) const
{
    auto index = indexAt(pos);
    if (index.isValid()) {
        auto delegate = itemDelegate(index);
        auto task_delegate = qobject_cast<TaskDescriptionDelegate *>(delegate);
        if (task_delegate) {
            auto item_rect = visualRect(index);
            auto relative_click_position = pos - item_rect.topLeft();
            auto markdown = model()->data(index, Qt::DisplayRole).toString();
            return task_delegate->anchorAt(markdown, relative_click_position);
        }
    }

    return QString();
}
