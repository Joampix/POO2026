#include "GeminiClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTextStream>
#include <QUrl>

namespace {
const QString GeminiEndpoint = "https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent";
const QStringList GeminiModels = {
    "gemini-2.5-flash",
    "gemini-2.5-flash-lite",
    "gemini-2.0-flash",
    "gemini-1.5-flash",
};

QString envValue(const QString& key)
{
    QString value = QString::fromLocal8Bit(qgetenv(key.toLocal8Bit().constData())).trimmed();
    if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith('\'') && value.endsWith('\''))) {
        value = value.mid(1, value.size() - 2).trimmed();
    }
    QString clean;
    for (const QChar& ch : value) {
        const ushort code = ch.unicode();
        if (code <= 32 || code == 127 || code == 0xFEFF) {
            continue;
        }
        clean.append(ch);
    }
    return clean;
}
}

GeminiClient::GeminiClient(QObject* parent)
    : QObject(parent)
    , m_apiKey(loadApiKey())
{
}

bool GeminiClient::configured() const
{
    return m_apiKey.size() >= 20 && m_apiKey != "PEGAR_TU_CLAVE_ACA";
}

void GeminiClient::askCoach(const UserProfile* profile, const MacroPlan* plan, const QString& question)
{
    if (!configured()) {
        emit answerReady(false,
            "Gemini no esta configurado en esta PC. Ejecuta configurar_gemini.ps1 una sola vez o crea un archivo .env con GEMINI_API_KEY. "
            "La app ya esta preparada para usar la API automaticamente cuando encuentre la clave.",
            "Sistema");
        return;
    }

    sendPrompt(buildPrompt(profile, plan, question), 0, {});
}

void GeminiClient::sendPrompt(const QString& prompt, int modelIndex, const QStringList& failures)
{
    if (modelIndex >= GeminiModels.size()) {
        const QString details = failures.isEmpty()
            ? "Google rechazo la solicitud, pero no devolvio detalle tecnico."
            : failures.join("\n");
        emit answerReady(false,
            "Gemini no pudo responder con los modelos disponibles.\n\n"
            "Que revisar:\n"
            "1. Que la API key sea de Google AI Studio y este activa.\n"
            "2. Que no tenga restricciones que bloqueen Generative Language API.\n"
            "3. Que el proyecto tenga habilitado el acceso necesario o facturacion si tu cuenta/region lo exige.\n\n"
            "Detalle tecnico:\n" + details,
            "Sistema");
        return;
    }

    const QString model = GeminiModels.at(modelIndex);
    QJsonObject payload;
    QJsonObject part;
    part["text"] = prompt;
    QJsonObject content;
    content["role"] = "user";
    content["parts"] = QJsonArray{part};
    payload["contents"] = QJsonArray{content};
    payload["generationConfig"] = QJsonObject{
        {"temperature", 0.35},
        {"topP", 0.9},
        {"maxOutputTokens", 1200},
    };

    QNetworkRequest request{QUrl(GeminiEndpoint.arg(model))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    request.setTransferTimeout(30000);

    auto* reply = m_manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, prompt, modelIndex, failures, model]() {
        const QByteArray body = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString networkError = reply->errorString();
        const auto document = QJsonDocument::fromJson(body);
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QStringList nextFailures = failures;
            nextFailures.append(extractErrorText(document, httpStatus, networkError, model));
            sendPrompt(prompt, modelIndex + 1, nextFailures);
            return;
        }

        const QString text = extractText(document).trimmed();
        if (text.isEmpty() || wasTruncated(document) || looksIncomplete(text)) {
            QStringList nextFailures = failures;
            nextFailures.append(QString("%1: respuesta vacia, truncada o incompleta.").arg(model));
            sendPrompt(prompt, modelIndex + 1, nextFailures);
            return;
        }
        emit answerReady(true, text.left(2200), "Gemini");
    });
}

QString GeminiClient::loadApiKey() const
{
    const QString fromEnv = envValue("GEMINI_API_KEY");
    if (!fromEnv.isEmpty()) {
        return fromEnv;
    }
    const QString fromGoogleEnv = envValue("GOOGLE_API_KEY");
    if (!fromGoogleEnv.isEmpty()) {
        return fromGoogleEnv;
    }

    const QString projectDir = envValue("CONCA_GYM_PROJECT_DIR");
    QStringList candidates = {
        projectDir.isEmpty() ? QString() : projectDir + "/.env",
        QCoreApplication::applicationDirPath() + "/.env",
        QCoreApplication::applicationDirPath() + "/../.env",
        QCoreApplication::applicationDirPath() + "/../../.env",
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
            if (line.startsWith("#") || !line.contains("=")) {
                continue;
            }
            const auto parts = line.split("=");
            QString key = parts.first().trimmed();
            key.remove(QChar(0xFEFF));
            if (key == "GEMINI_API_KEY" || key == "GOOGLE_API_KEY") {
                QString value = parts.mid(1).join("=").trimmed();
                value.remove('"').remove('\'');
                QString clean;
                for (const QChar& ch : value) {
                    const ushort code = ch.unicode();
                    if (code <= 32 || code == 127 || code == 0xFEFF) {
                        continue;
                    }
                    clean.append(ch);
                }
                return clean;
            }
        }
    }
    return {};
}

QString GeminiClient::buildPrompt(const UserProfile* profile, const MacroPlan* plan, const QString& question) const
{
    const UserProfile emptyProfile;
    const UserProfile& p = profile ? *profile : emptyProfile;
    const QString planText = plan
        ? QString("Meta: %1 kcal/dia; proteina %2g; carbohidratos %3g; grasas %4g.")
              .arg(plan->calories)
              .arg(plan->protein)
              .arg(plan->carbs)
              .arg(plan->fats)
        : "Todavia no hay plan nutricional activo.";

    return QString(R"PROMPT(
Sos el Coach IA de una app educativa de entrenamiento y alimentacion.
Responde en espanol rioplatense neutral, profesional y claro.
No reemplaces a un medico, nutricionista o entrenador. No indiques tratamientos.
No inventes citas exactas, autores o porcentajes si no fueron dados por la app.

Perfil:
- Nombre: %1
- Edad: %2
- Objetivo: %3
- Nivel: %4
- Dias de entrenamiento: %5
- Equipamiento: %6
- Notas: %7

Plan de macros:
%8

Pregunta del usuario:
%9

Responde con:
- recomendacion concreta;
- por que encaja con el objetivo;
- 2 acciones practicas para esta semana;
- aviso educativo breve.
Limite: maximo 180 palabras.
)PROMPT")
        .arg(p.name.isEmpty() ? "Usuario" : p.name)
        .arg(p.age)
        .arg(p.goal)
        .arg(p.level)
        .arg(p.days)
        .arg(p.equipment)
        .arg(p.notes.isEmpty() ? "Sin notas" : p.notes)
        .arg(planText)
        .arg(question);
}

QString GeminiClient::extractText(const QJsonDocument& document) const
{
    QStringList chunks;
    for (const auto& candidateValue : document.object().value("candidates").toArray()) {
        const auto content = candidateValue.toObject().value("content").toObject();
        for (const auto& partValue : content.value("parts").toArray()) {
            const QString text = partValue.toObject().value("text").toString();
            if (!text.isEmpty()) {
                chunks.append(text);
            }
        }
    }
    return chunks.join("\n");
}

QString GeminiClient::extractErrorText(const QJsonDocument& document, int httpStatus, const QString& networkError, const QString& model) const
{
    const auto error = document.object().value("error").toObject();
    const QString status = error.value("status").toString();
    const QString message = error.value("message").toString();

    QString text = QString("%1: HTTP %2").arg(model).arg(httpStatus > 0 ? QString::number(httpStatus) : "sin codigo");
    if (!status.isEmpty()) {
        text += " - " + status;
    }
    if (!message.isEmpty()) {
        text += " - " + message;
    } else if (!networkError.isEmpty()) {
        text += " - " + networkError;
    }
    return text;
}

bool GeminiClient::wasTruncated(const QJsonDocument& document) const
{
    for (const auto& candidateValue : document.object().value("candidates").toArray()) {
        if (candidateValue.toObject().value("finishReason").toString() == "MAX_TOKENS") {
            return true;
        }
    }
    return false;
}

bool GeminiClient::looksIncomplete(const QString& text) const
{
    const QString trimmed = text.trimmed();
    if (trimmed.size() < 20) {
        return true;
    }
    const QChar last = trimmed.back();
    return !(last == '.' || last == '!' || last == '?' || last == ':' || last == ')' || last == '"');
}
