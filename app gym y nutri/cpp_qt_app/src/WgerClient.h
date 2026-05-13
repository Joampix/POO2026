#pragma once

#include "Models.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>

class WgerClient : public QObject {
    Q_OBJECT

public:
    explicit WgerClient(QObject* parent = nullptr);

    void searchBestExercise(const QString& query);

signals:
    void exerciseReady(const QString& query, const ApiExercise& exercise);
    void exerciseFailed(const QString& query, const QString& message);

private:
    QNetworkAccessManager m_manager;
    QJsonArray m_translations;
    bool m_cacheLoaded = false;
    bool m_cacheLoading = false;
    QStringList m_pendingQueries;

    void loadTranslationCache();
    void processPendingQueries();
    void resolveFromCache(const QString& query);
    void fetchExerciseInfo(const QString& originalQuery, int exerciseId);

    QString normalize(const QString& text) const;
    QStringList tokens(const QString& text) const;
    QStringList queryCandidates(const QString& query) const;
    int scoreTranslation(const QJsonObject& translation, const QString& query) const;
    ApiExercise toExercise(const QJsonObject& item) const;
    QString stripHtml(const QString& text) const;
    QString joinedObjectNames(const QJsonValue& value, bool preferEnglish = true) const;
    QString absoluteMediaUrl(const QString& value) const;
};
