#include "LocalAuditStore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

LocalAuditStore::LocalAuditStore(const QString& databasePath)
    : m_connectionName("local_audit_" + QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_databasePath(databasePath)
{
    if (m_databasePath.isEmpty()) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (dir.isEmpty()) {
            dir = QCoreApplication::applicationDirPath();
        }
        QDir().mkpath(dir);
        m_databasePath = dir + "/conca_gym_local.sqlite";
    }
}

LocalAuditStore::~LocalAuditStore()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::database(m_connectionName).close();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool LocalAuditStore::initialize()
{
    auto db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_databasePath);
    if (!db.open()) {
        return false;
    }

    return exec("CREATE TABLE IF NOT EXISTS local_login_attempts ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "username TEXT NOT NULL,"
                "success INTEGER NOT NULL,"
                "message TEXT,"
                "created_at TEXT NOT NULL)")
        && exec("CREATE TABLE IF NOT EXISTS local_events ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "event_type TEXT NOT NULL,"
                "severity TEXT NOT NULL,"
                "detail TEXT,"
                "sync_status TEXT DEFAULT 'local',"
                "created_at TEXT NOT NULL)")
        && exec("CREATE TABLE IF NOT EXISTS local_state ("
                "key TEXT PRIMARY KEY,"
                "value TEXT NOT NULL,"
                "updated_at TEXT NOT NULL)");
}

bool LocalAuditStore::recordLoginAttempt(const QString& username, bool success, const QString& message)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("INSERT INTO local_login_attempts(username, success, message, created_at) VALUES(?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(success ? 1 : 0);
    query.addBindValue(message);
    query.addBindValue(nowIso());
    return query.exec();
}

bool LocalAuditStore::setLastSuccessfulLogin(const QString& username)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("INSERT INTO local_state(key, value, updated_at) VALUES('last_successful_login', ?, ?) "
                  "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = excluded.updated_at");
    query.addBindValue(username + " | " + nowIso());
    query.addBindValue(nowIso());
    return query.exec();
}

QString LocalAuditStore::lastSuccessfulLogin() const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("SELECT value FROM local_state WHERE key = 'last_successful_login'");
    if (!query.exec() || !query.next()) {
        return {};
    }
    return query.value(0).toString();
}

bool LocalAuditStore::recordLocalEvent(const QString& eventType, const QString& severity, const QString& detail)
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    query.prepare("INSERT INTO local_events(event_type, severity, detail, sync_status, created_at) VALUES(?, ?, ?, 'local', ?)");
    query.addBindValue(eventType);
    query.addBindValue(severity);
    query.addBindValue(detail);
    query.addBindValue(nowIso());
    return query.exec();
}

bool LocalAuditStore::exec(const QString& sql) const
{
    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    return query.exec(sql);
}

QString LocalAuditStore::nowIso() const
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}
