#include "ProgressStore.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <utility>

ProgressStore::ProgressStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

QVector<ProgressEntry> ProgressStore::load() const
{
    QFile file(m_filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    QVector<ProgressEntry> entries;
    for (const auto& value : document.array()) {
        const auto item = value.toObject();
        entries.push_back({
            item.value("week").toString(),
            item.value("weight").toDouble(),
            item.value("waist").toDouble(),
            item.value("workouts").toInt(),
            item.value("energy").toInt(),
            item.value("notes").toString(),
        });
    }
    return entries;
}

bool ProgressStore::save(const QVector<ProgressEntry>& entries) const
{
    QJsonArray array;
    for (const auto& entry : entries) {
        QJsonObject item;
        item["week"] = entry.week;
        item["weight"] = entry.weight;
        item["waist"] = entry.waist;
        item["workouts"] = entry.workouts;
        item["energy"] = entry.energy;
        item["notes"] = entry.notes;
        array.append(item);
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

bool ProgressStore::append(const ProgressEntry& entry) const
{
    auto entries = load();
    entries.push_back(entry);
    return save(entries);
}
