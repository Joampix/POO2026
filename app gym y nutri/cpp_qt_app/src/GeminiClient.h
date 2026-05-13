#pragma once

#include "Models.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>

class GeminiClient : public QObject {
    Q_OBJECT

public:
    explicit GeminiClient(QObject* parent = nullptr);

    bool configured() const;
    void askCoach(const UserProfile* profile, const MacroPlan* plan, const QString& question);
    QString apiKey() const { return m_apiKey; }

signals:
    void answerReady(bool ok, const QString& text, const QString& source);

private:
    QNetworkAccessManager m_manager;
    QString m_apiKey;

    QString loadApiKey() const;
    QString buildPrompt(const UserProfile* profile, const MacroPlan* plan, const QString& question) const;
    void sendPrompt(const QString& prompt, int modelIndex, const QStringList& failures);
    QString extractText(const QJsonDocument& document) const;
    QString extractErrorText(const QJsonDocument& document, int httpStatus, const QString& networkError, const QString& model) const;
    bool wasTruncated(const QJsonDocument& document) const;
    bool looksIncomplete(const QString& text) const;
};
