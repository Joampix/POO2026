#include "DataManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextStream>
#include <QUrl>

namespace {
QString envValue(const QString& key)
{
    return QString::fromLocal8Bit(qgetenv(key.toLocal8Bit().constData())).trimmed();
}
}

DataManager::DataManager(QObject* parent)
    : QObject(parent)
    , m_baseUrl(loadBaseUrl())
{
}

void DataManager::setBaseUrl(const QString& value)
{
    QString url = value.trimmed();
    while (url.endsWith('/')) {
        url.chop(1);
    }
    m_baseUrl = url.isEmpty() ? "http://127.0.0.1:8000" : url;
}

void DataManager::login(const QString& username, const QString& password)
{
    QJsonObject payload;
    payload["username"] = username.trimmed().toLower();
    payload["password"] = password;

    auto* reply = m_manager.post(request("/auth/login"), QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();

        if (!ok) {
            emit loginFinished(false, extractError(body, "No se pudo iniciar sesion contra FastAPI."), {}, {});
            return;
        }

        const auto object = QJsonDocument::fromJson(body).object();
        m_token = object.value("token").toString();
        m_username = object.value("username").toString();
        m_role = object.value("role").toString();
        if (m_token.isEmpty()) {
            emit loginFinished(false, "FastAPI respondio sin token.", {}, {});
            return;
        }
        emit loginFinished(true, "Login correcto", m_username, m_role);
    });
}

void DataManager::registerEvent(const QString& eventType, const QString& severity, const QString& detail, const QString& source)
{
    if (!authenticated()) {
        emit eventFinished(false, "Sin token de login; evento no enviado al servidor.");
        return;
    }

    QJsonObject payload;
    payload["event_type"] = eventType;
    payload["severity"] = severity;
    payload["detail"] = detail.left(3800);
    payload["source"] = source;

    auto* reply = m_manager.post(request("/events", true), QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        emit eventFinished(ok, ok ? "Evento registrado" : extractError(body, "No se pudo registrar evento en FastAPI."));
    });
}

void DataManager::healthCheck()
{
    auto* reply = m_manager.get(request("/health"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        reply->deleteLater();
        emit healthFinished(ok, ok ? "FastAPI disponible" : extractError(body, "FastAPI no disponible."));
    });
}

QString DataManager::loadBaseUrl() const
{
    const QString fromEnv = envValue("CONCA_GYM_API_URL");
    if (!fromEnv.isEmpty()) {
        return fromEnv;
    }

    const QString projectDir = envValue("CONCA_GYM_PROJECT_DIR");
    const QStringList candidates = {
        projectDir.isEmpty() ? QString() : projectDir + "/.env",
        QCoreApplication::applicationDirPath() + "/.env",
        QDir::currentPath() + "/.env",
        QDir::currentPath() + "/../.env",
    };
    for (const auto& path : candidates) {
        if (path.isEmpty()) {
            continue;
        }
        QFile file(path);
        if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.startsWith('#') || !line.contains('=')) {
                continue;
            }
            const auto parts = line.split('=');
            QString key = parts.first().trimmed();
            key.remove(QChar(0xFEFF));
            if (key == "CONCA_GYM_API_URL") {
                return parts.mid(1).join("=").trimmed().remove('"').remove('\'');
            }
        }
    }
    return "http://127.0.0.1:8000";
}

QNetworkRequest DataManager::request(const QString& path, bool authenticated) const
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setRawHeader("User-Agent", "ConcaGymCpp/0.1");
    req.setTransferTimeout(20000);
    if (authenticated && !m_token.isEmpty()) {
        req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    }
    return req;
}

QString DataManager::extractError(const QByteArray& body, const QString& fallback) const
{
    const auto document = QJsonDocument::fromJson(body);
    const auto object = document.object();
    const QString detail = object.value("detail").toString();
    if (!detail.isEmpty()) {
        return detail;
    }
    return fallback;
}
