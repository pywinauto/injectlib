#pragma once

#include <QGraphicsItem>
#include <QHash>
#include <QObject>
#include <QPointer>

namespace QtBackend {

// Keeps temporary runtime IDs for Qt objects returned over the pipe.
class QtObjectStore {
public:
    int ensureIdFor(QObject* object);
    void dropIdFor(QObject* object);
    QObject* objectForId(int id) const;

    int ensureIdForGItem(QGraphicsItem* item);
    QGraphicsItem* gitemForId(int id) const;

private:
    // ID space for QObject* instances.
    QHash<int, QPointer<QObject>> m_id2obj;
    QHash<QObject*, int> m_obj2id;
    int m_nextId = 1;

    // ID space for QGraphicsItem* instances (not QObject-based).
    QHash<QGraphicsItem*, int> m_gitem2id;
    QHash<int, QGraphicsItem*> m_id2gitem;
    // Start graphics IDs far from QObject IDs.
    int m_nextGItemId = 1000000;
};

} // namespace QtBackend
