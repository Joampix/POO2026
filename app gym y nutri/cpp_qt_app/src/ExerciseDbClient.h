#pragma once

#include "Models.h"

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>

class ExerciseDbClient : public QObject {
    Q_OBJECT

public:
    explicit ExerciseDbClient(QObject* parent = nullptr);

    void searchBestExercise(const QString& query);

signals:
    void exerciseReady(const QString& query, const ApiExercise& exercise);
    void exerciseFailed(const QString& query, const QString& message);

private:
    QNetworkAccessManager m_manager;

    QString normalize(const QString& text) const;
    QStringList tokens(const QString& text) const;
    QStringList queryCandidates(const QString& term) const;
    QStringList requirementsFor(const QString& term) const;
    int scoreItem(const QJsonObject& item, const QString& term) const;
    ApiExercise toExercise(const QJsonObject& item) const;
    ApiExercise fallback(const QString& query) const;
    QString mappedTerm(const QString& query) const;
};
