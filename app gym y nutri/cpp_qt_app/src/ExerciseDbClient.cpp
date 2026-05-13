#include "ExerciseDbClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrlQuery>
#include <algorithm>

namespace {
const QString BaseUrl = "https://oss.exercisedb.dev/api/v1/exercises";

QHash<QString, QString> termMap()
{
    return {
        {"sentadilla", "squat"},
        {"pecho", "chest"},
        {"espalda", "back"},
        {"hombro", "shoulder"},
        {"hombros", "shoulder"},
        {"pierna", "leg"},
        {"piernas", "leg"},
        {"abdominales", "abs"},
        {"abdomen", "abs"},
        {"flexiones", "push"},
        {"remo", "row"},
        {"press banca", "bench press"},
        {"press banca con barra", "barbell bench press"},
        {"jalon", "lat pulldown"},
        {"remo sentado", "cable seated row"},
        {"peso muerto rumano", "romanian deadlift"},
        {"prensa", "leg press"},
        {"zancadas", "walking lunge"},
        {"plancha", "plank"},
    };
}

QHash<QString, QString> broadMap()
{
    return {
        {"barbell full squat", "squat"},
        {"goblet squat", "squat"},
        {"cable seated row", "row"},
        {"dumbbell bent over row", "row"},
        {"romanian deadlift", "deadlift"},
        {"cable lat pulldown", "pulldown"},
        {"incline dumbbell bench press", "bench press"},
        {"dumbbell fly", "fly"},
        {"walking lunge", "lunge"},
        {"glute bridge", "bridge"},
        {"dumbbell one arm shoulder press", "shoulder press"},
        {"dumbbell lateral raise", "lateral raise"},
        {"dumbbell rear lateral raise", "rear lateral raise"},
        {"dumbbell arnold press", "arnold press"},
        {"front plank", "plank"},
        {"kneeling plank tap shoulder", "plank"},
        {"crunch hands overhead", "crunch"},
    };
}

QHash<QString, QStringList> requirementMap()
{
    return {
        {"bench press", {"bench", "press"}},
        {"lat pulldown", {"pulldown"}},
        {"pulldown", {"pulldown"}},
        {"deadlift", {"deadlift"}},
        {"squat", {"squat"}},
        {"leg press", {"press"}},
        {"row", {"row"}},
        {"shoulder press", {"press"}},
        {"push up", {"push"}},
        {"fly", {"fly"}},
        {"lunge", {"lunge"}},
        {"plank", {"plank"}},
        {"dead bug", {"bug"}},
        {"crunch", {"crunch"}},
        {"lateral raise", {"raise"}},
        {"arnold press", {"arnold", "press"}},
        {"bridge", {"bridge"}},
        {"good morning", {"good", "morning"}},
    };
}

QStringList descriptorTokens()
{
    return {"barbell", "bench", "cable", "dumbbell", "incline", "lat", "leg", "machine", "romanian", "rear", "seated", "shoulder", "weighted", "wide"};
}

QString joinedArray(const QJsonValue& value)
{
    QStringList result;
    for (const auto& item : value.toArray()) {
        result.append(item.toString());
    }
    return result.join(", ");
}
}

ExerciseDbClient::ExerciseDbClient(QObject* parent)
    : QObject(parent)
{
}

void ExerciseDbClient::searchBestExercise(const QString& query)
{
    const QString term = mappedTerm(query);
    const QStringList candidates = queryCandidates(term);
    if (candidates.isEmpty()) {
        emit exerciseFailed(query, "Consulta vacia");
        return;
    }

    QUrl url(BaseUrl);
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("name", candidates.first());
    urlQuery.addQueryItem("limit", "60");
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "ConcaGymCpp/0.1");
    auto* reply = m_manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, query, term]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto item = fallback(term);
            if (!item.name.isEmpty()) {
                emit exerciseReady(query, item);
            } else {
                emit exerciseFailed(query, reply->errorString());
            }
            return;
        }

        const auto document = QJsonDocument::fromJson(reply->readAll());
        const auto data = document.object().value("data").toArray();
        int bestScore = -1;
        QJsonObject best;
        for (const auto& value : data) {
            const auto item = value.toObject();
            const int score = scoreItem(item, term);
            if (score > bestScore) {
                bestScore = score;
                best = item;
            }
        }

        if (bestScore >= 0) {
            emit exerciseReady(query, toExercise(best));
            return;
        }
        const auto item = fallback(term);
        if (!item.name.isEmpty()) {
            emit exerciseReady(query, item);
        } else {
            emit exerciseFailed(query, "No se encontro referencia confiable");
        }
    });
}

QString ExerciseDbClient::normalize(const QString& text) const
{
    QString value = text.toLower();
    value.replace(QRegularExpression("[^a-z0-9]+"), " ");
    return value.simplified();
}

QStringList ExerciseDbClient::tokens(const QString& text) const
{
    QStringList result;
    for (const auto& token : normalize(text).split(" ", Qt::SkipEmptyParts)) {
        if (token.size() > 1) {
            result.append(token);
        }
    }
    return result;
}

QStringList ExerciseDbClient::queryCandidates(const QString& term) const
{
    const QString normalized = normalize(term);
    QStringList result{normalized};
    const QString broad = broadMap().value(normalized);
    if (!broad.isEmpty() && !result.contains(broad)) {
        result.append(broad);
    }
    return result;
}

QStringList ExerciseDbClient::requirementsFor(const QString& term) const
{
    const QString normalized = normalize(term);
    const auto requirements = requirementMap();
    for (auto it = requirements.constBegin(); it != requirements.constEnd(); ++it) {
        if (normalized.contains(it.key())) {
            return it.value();
        }
    }
    const auto parts = tokens(normalized);
    return parts.isEmpty() ? QStringList{} : QStringList{parts.first()};
}

int ExerciseDbClient::scoreItem(const QJsonObject& item, const QString& term) const
{
    const QString normalizedTerm = normalize(term);
    const QString name = normalize(item.value("name").toString());
    const QString equipment = normalize(joinedArray(item.value("equipments")));
    const QString muscles = normalize(QStringList{
        joinedArray(item.value("bodyParts")),
        joinedArray(item.value("targetMuscles")),
        joinedArray(item.value("secondaryMuscles")),
    }.join(" "));

    const auto requirements = requirementsFor(normalizedTerm);
    for (const auto& required : requirements) {
        if (!name.contains(required)) {
            return -1;
        }
    }

    int score = 0;
    if (normalizedTerm == name) {
        score += 220;
    }
    if (name.contains(normalizedTerm)) {
        score += 120;
    }
    if (name.startsWith(normalizedTerm)) {
        score += 45;
    }
    for (const auto& token : tokens(normalizedTerm)) {
        if (name.contains(token)) {
            score += 18;
        } else if (equipment.contains(token)) {
            score += 12;
        } else if (muscles.contains(token)) {
            score += 8;
        }
    }
    for (const auto& token : descriptorTokens()) {
        if (normalizedTerm.contains(token) && name.contains(token)) {
            score += 14;
        } else if (normalizedTerm.contains(token) && equipment.contains(token)) {
            score += 10;
        }
    }
    score += static_cast<int>(requirements.size()) * 35;
    const int extraWords = static_cast<int>(tokens(name).size()) - static_cast<int>(tokens(normalizedTerm).size());
    score -= std::max(0, extraWords) * 3;
    return score;
}

ApiExercise ExerciseDbClient::toExercise(const QJsonObject& item) const
{
    QStringList instructions;
    for (const auto& step : item.value("instructions").toArray()) {
        instructions.append(step.toString());
    }
    const QString targets = joinedArray(item.value("targetMuscles"));
    const QString secondary = joinedArray(item.value("secondaryMuscles"));
    ApiExercise exercise;
    exercise.name = item.value("name").toString();
    if (!exercise.name.isEmpty()) {
        exercise.name[0] = exercise.name[0].toUpper();
    }
    exercise.description = instructions.join("\n");
    exercise.equipment = joinedArray(item.value("equipments"));
    exercise.muscles = QStringList{targets, secondary}.join(", ").replace(QRegularExpression("^, |, $"), "");
    if (exercise.equipment.isEmpty()) {
        exercise.equipment = "No especificado";
    }
    if (exercise.muscles.trimmed().isEmpty()) {
        exercise.muscles = joinedArray(item.value("bodyParts"));
    }
    exercise.source = "ExerciseDB";
    exercise.gifUrl = item.value("gifUrl").toString();
    return exercise;
}

ApiExercise ExerciseDbClient::fallback(const QString& query) const
{
    const QString normalized = normalize(query);
    const QHash<QString, ApiExercise> fallbacks = {
        {"dumbbell bent over row", {"Dumbbell Bent Over Row", "Inclina el torso con espalda neutra.\nLleva las mancuernas hacia las costillas.\nBaja controlado y repite.", "dumbbell", "espalda, dorsales, biceps", "ExerciseDB OSS referencia curada", "https://static.exercisedb.dev/media/BJ0Hz5L.gif"}},
        {"front plank", {"Front Plank With Twist", "Apoya antebrazos y pies manteniendo el cuerpo en linea.\nContrae abdomen y gluteos.\nMantene respiracion controlada.", "body weight", "core, abdominales", "ExerciseDB OSS referencia curada", "https://static.exercisedb.dev/media/CosupLu.gif"}},
        {"dumbbell lateral raise", {"Dumbbell Lateral Raise", "Sostene las mancuernas a los lados.\nEleva los brazos hasta la altura de hombros.\nBaja controlado sin balancear.", "dumbbell", "deltoides lateral", "ExerciseDB OSS referencia curada", "https://static.exercisedb.dev/media/DsgkuIt.gif"}},
    };
    return fallbacks.value(normalized);
}

QString ExerciseDbClient::mappedTerm(const QString& query) const
{
    const QString normalized = normalize(query);
    return termMap().value(normalized, normalized);
}
