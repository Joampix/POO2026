#include "WgerClient.h"

#include <QJsonDocument>
#include <QHash>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

namespace {
const QString TranslationUrl = "https://wger.de/api/v2/exercise-translation/";
const QString ExerciseInfoUrl = "https://wger.de/api/v2/exerciseinfo/%1/";
const QString WgerBaseUrl = "https://wger.de";

QHash<QString, QString> broadMap()
{
    return {
        {"barbell full squat", "squat"},
        {"goblet squat", "squat"},
        {"cable seated row", "seated row"},
        {"dumbbell bent over row", "dumbbell row"},
        {"romanian deadlift", "deadlift"},
        {"cable lat pulldown", "lat pulldown"},
        {"incline dumbbell bench press", "bench press"},
        {"dumbbell fly", "fly"},
        {"walking lunge", "lunge"},
        {"glute bridge", "bridge"},
        {"dumbbell one arm shoulder press", "shoulder press"},
        {"dumbbell lateral raise", "lateral raise"},
        {"dumbbell rear lateral raise", "rear delt raise"},
        {"dumbbell arnold press", "arnold press"},
        {"front plank", "plank"},
        {"kneeling plank tap shoulder", "plank"},
        {"crunch hands overhead", "crunch"},
    };
}
}

WgerClient::WgerClient(QObject* parent)
    : QObject(parent)
{
}

void WgerClient::searchBestExercise(const QString& query)
{
    const QString cleanQuery = query.trimmed();
    if (cleanQuery.isEmpty()) {
        emit exerciseFailed(query, "Consulta vacia");
        return;
    }

    if (m_cacheLoaded) {
        resolveFromCache(cleanQuery);
        return;
    }

    if (!m_pendingQueries.contains(cleanQuery)) {
        m_pendingQueries.append(cleanQuery);
    }
    loadTranslationCache();
}

void WgerClient::loadTranslationCache()
{
    if (m_cacheLoaded || m_cacheLoading) {
        return;
    }
    m_cacheLoading = true;

    QUrl url(TranslationUrl);
    QUrlQuery query;
    query.addQueryItem("language", "2");
    query.addQueryItem("limit", "2500");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "ConcaGymCpp/0.1");

    auto* reply = m_manager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        m_cacheLoading = false;

        if (reply->error() != QNetworkReply::NoError) {
            const QStringList failed = m_pendingQueries;
            m_pendingQueries.clear();
            for (const auto& query : failed) {
                emit exerciseFailed(query, "wger no respondio: " + reply->errorString());
            }
            return;
        }

        const auto document = QJsonDocument::fromJson(body);
        m_translations = document.object().value("results").toArray();
        m_cacheLoaded = !m_translations.isEmpty();
        if (!m_cacheLoaded) {
            const QStringList failed = m_pendingQueries;
            m_pendingQueries.clear();
            for (const auto& query : failed) {
                emit exerciseFailed(query, "wger no devolvio ejercicios");
            }
            return;
        }
        processPendingQueries();
    });
}

void WgerClient::processPendingQueries()
{
    const QStringList queries = m_pendingQueries;
    m_pendingQueries.clear();
    for (const auto& query : queries) {
        resolveFromCache(query);
    }
}

void WgerClient::resolveFromCache(const QString& query)
{
    int bestScore = -1;
    QJsonObject best;
    for (const auto& candidateQuery : queryCandidates(query)) {
        for (const auto& value : m_translations) {
            const auto item = value.toObject();
            const int score = scoreTranslation(item, candidateQuery);
            if (score > bestScore) {
                bestScore = score;
                best = item;
            }
        }
    }

    if (bestScore < 40) {
        emit exerciseFailed(query, "wger no encontro una coincidencia confiable");
        return;
    }

    const int exerciseId = best.value("exercise").toInt();
    if (exerciseId <= 0) {
        emit exerciseFailed(query, "wger devolvio una traduccion sin ejercicio base");
        return;
    }
    fetchExerciseInfo(query, exerciseId);
}

void WgerClient::fetchExerciseInfo(const QString& originalQuery, int exerciseId)
{
    QNetworkRequest request{QUrl(ExerciseInfoUrl.arg(exerciseId))};
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "ConcaGymCpp/0.1");

    auto* reply = m_manager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, originalQuery]() {
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit exerciseFailed(originalQuery, "wger detalle no disponible: " + reply->errorString());
            return;
        }

        const auto document = QJsonDocument::fromJson(body);
        const ApiExercise exercise = toExercise(document.object());
        if (exercise.name.isEmpty()) {
            emit exerciseFailed(originalQuery, "wger devolvio un ejercicio sin nombre");
            return;
        }
        emit exerciseReady(originalQuery, exercise);
    });
}

QString WgerClient::normalize(const QString& text) const
{
    QString value = text.toLower();
    value.replace(QRegularExpression("[^a-z0-9]+"), " ");
    return value.simplified();
}

QStringList WgerClient::tokens(const QString& text) const
{
    QStringList result;
    for (const auto& token : normalize(text).split(" ", Qt::SkipEmptyParts)) {
        if (token.size() > 1) {
            result.append(token);
        }
    }
    return result;
}

QStringList WgerClient::queryCandidates(const QString& query) const
{
    const QString normalized = normalize(query);
    QStringList result{normalized};
    const QString broad = broadMap().value(normalized);
    if (!broad.isEmpty() && !result.contains(broad)) {
        result.append(broad);
    }
    return result;
}

int WgerClient::scoreTranslation(const QJsonObject& translation, const QString& query) const
{
    const QString normalizedQuery = normalize(query);
    const QString name = normalize(translation.value("name").toString());
    const QString description = normalize(translation.value("description_source").toString());
    if (name.isEmpty()) {
        return -1;
    }

    const auto queryTokens = tokens(normalizedQuery);
    int nameHits = 0;
    int score = 0;
    if (name == normalizedQuery) {
        score += 240;
    }
    if (name.contains(normalizedQuery)) {
        score += 140;
    }
    if (name.startsWith(normalizedQuery)) {
        score += 50;
    }
    for (const auto& token : queryTokens) {
        if (name.contains(token)) {
            score += 35;
            ++nameHits;
        } else if (description.contains(token)) {
            score += 6;
        }
    }
    if (!queryTokens.isEmpty() && nameHits == 0) {
        return -1;
    }
    const int extraWords = static_cast<int>(tokens(name).size()) - static_cast<int>(queryTokens.size());
    score -= std::max(0, extraWords) * 4;
    return score;
}

ApiExercise WgerClient::toExercise(const QJsonObject& item) const
{
    QJsonObject translation;
    for (const auto& value : item.value("translations").toArray()) {
        const auto candidate = value.toObject();
        if (candidate.value("language").toInt() == 2) {
            translation = candidate;
            break;
        }
        if (translation.isEmpty()) {
            translation = candidate;
        }
    }

    QString imageUrl;
    for (const auto& value : item.value("images").toArray()) {
        const auto image = value.toObject();
        if (image.value("is_main").toBool() || imageUrl.isEmpty()) {
            imageUrl = absoluteMediaUrl(image.value("image").toString());
        }
    }

    QString description = translation.value("description_source").toString().trimmed();
    if (description.isEmpty()) {
        description = stripHtml(translation.value("description").toString());
    }
    description = description.replace(QRegularExpression("\\n{3,}"), "\n\n").trimmed();

    const QString primaryMuscles = joinedObjectNames(item.value("muscles"));
    const QString secondaryMuscles = joinedObjectNames(item.value("muscles_secondary"));

    ApiExercise exercise;
    exercise.name = translation.value("name").toString();
    exercise.description = description;
    exercise.equipment = joinedObjectNames(item.value("equipment"), false);
    exercise.muscles = QStringList{primaryMuscles, secondaryMuscles}.join(", ").replace(QRegularExpression("^, |, $"), "");
    exercise.source = "wger API";
    exercise.gifUrl = imageUrl;
    if (exercise.equipment.isEmpty()) {
        exercise.equipment = "No especificado";
    }
    if (exercise.muscles.trimmed().isEmpty()) {
        exercise.muscles = item.value("category").toObject().value("name").toString("General");
    }
    return exercise;
}

QString WgerClient::stripHtml(const QString& text) const
{
    QString value = text;
    value.replace(QRegularExpression("<br\\s*/?>"), "\n");
    value.replace(QRegularExpression("</li>"), "\n");
    value.replace(QRegularExpression("<[^>]+>"), "");
    value.replace("&nbsp;", " ");
    value.replace("&amp;", "&");
    value.replace("&quot;", "\"");
    value.replace("&#39;", "'");
    return value.simplified();
}

QString WgerClient::joinedObjectNames(const QJsonValue& value, bool preferEnglish) const
{
    QStringList result;
    for (const auto& itemValue : value.toArray()) {
        const auto item = itemValue.toObject();
        QString name;
        if (preferEnglish) {
            name = item.value("name_en").toString().trimmed();
        }
        if (name.isEmpty()) {
            name = item.value("name").toString().trimmed();
        }
        if (!name.isEmpty() && !result.contains(name)) {
            result.append(name);
        }
    }
    return result.join(", ");
}

QString WgerClient::absoluteMediaUrl(const QString& value) const
{
    if (value.startsWith("http")) {
        return value;
    }
    if (value.startsWith("/")) {
        return WgerBaseUrl + value;
    }
    return value;
}
