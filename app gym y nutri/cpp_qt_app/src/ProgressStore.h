#pragma once

#include "Models.h"

#include <QVector>
#include <QString>

class ProgressStore {
public:
    explicit ProgressStore(QString filePath);

    QVector<ProgressEntry> load() const;
    bool save(const QVector<ProgressEntry>& entries) const;
    bool append(const ProgressEntry& entry) const;

private:
    QString m_filePath;
};
