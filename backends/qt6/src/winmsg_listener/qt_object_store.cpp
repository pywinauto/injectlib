#include "qt_object_store.h"

namespace QtBackend {

int QtObjectStore::ensureIdFor(QObject* object) {
    if (!object) return 0;
    if (m_obj2id.contains(object)) return m_obj2id.value(object);

    const int id = m_nextId++;
    m_obj2id.insert(object, id);
    m_id2obj.insert(id, QPointer<QObject>(object));

    // Auto-drop when the QObject is destroyed.
    QObject::connect(object, &QObject::destroyed, [this, object]() { dropIdFor(object); });
    return id;
}

void QtObjectStore::dropIdFor(QObject* object) {
    if (!object) return;
    auto it = m_obj2id.find(object);
    if (it == m_obj2id.end()) return;
    const int id = it.value();
    m_obj2id.erase(it);
    m_id2obj.remove(id);
}

QObject* QtObjectStore::objectForId(int id) const {
    if (id <= 0) return nullptr;
    QPointer<QObject> ptr = m_id2obj.value(id);
    return ptr ? ptr.data() : nullptr;
}

int QtObjectStore::ensureIdForGItem(QGraphicsItem* item) {
    if (!item) return 0;
    auto it = m_gitem2id.find(item);
    if (it != m_gitem2id.end())
        return it.value();

    const int id = m_nextGItemId++;
    m_gitem2id.insert(item, id);
    m_id2gitem.insert(id, item);
    return id;
}

QGraphicsItem* QtObjectStore::gitemForId(int id) const {
    auto it = m_id2gitem.find(id);
    return it == m_id2gitem.end() ? nullptr : it.value();
}

} // namespace QtBackend
