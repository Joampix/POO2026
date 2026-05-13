#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

class DataManager : public QObject {
    Q_OBJECT

public:
    explicit DataManager(QObject* parent = nullptr);

    QString baseUrl() const { return m_baseUrl; }
    QString token() const { return m_token; }
    QString username() const { return m_username; }
    QString role() const { return m_role; }
    bool authenticated() const { return !m_token.isEmpty(); }

    void setBaseUrl(const QString& value);
    void login(const QString& username, const QString& password);
    void registerEvent(const QString& eventType, const QString& severity, const QString& detail, const QString& source = "qt");
    void healthCheck();

signals:
    void loginFinished(bool ok, const QString& message, const QString& username, const QString& role);
    void eventFinished(bool ok, const QString& message);
    void healthFinished(bool ok, const QString& message);

private:
    QNetworkAccessManager m_manager;
    QString m_baseUrl;
    QString m_token;
    QString m_username;
    QString m_role;

    QString loadBaseUrl() const;
    QNetworkRequest request(const QString& path, bool authenticated = false) const;
    QString extractError(const QByteArray& body, const QString& fallback) const;
};
