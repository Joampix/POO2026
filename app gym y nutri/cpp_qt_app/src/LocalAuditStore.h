#pragma once

#include <QString>

class LocalAuditStore {
public:
    explicit LocalAuditStore(const QString& databasePath = {});
    ~LocalAuditStore();

    bool initialize();
    bool recordLoginAttempt(const QString& username, bool success, const QString& message);
    bool setLastSuccessfulLogin(const QString& username);
    QString lastSuccessfulLogin() const;
    bool recordLocalEvent(const QString& eventType, const QString& severity, const QString& detail);
    QString databasePath() const { return m_databasePath; }

private:
    QString m_connectionName;
    QString m_databasePath;

    bool exec(const QString& sql) const;
    QString nowIso() const;
};
