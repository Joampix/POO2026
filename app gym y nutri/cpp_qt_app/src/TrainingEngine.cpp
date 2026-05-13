#include "TrainingEngine.h"

#include <QHash>
#include <QStringList>
#include <algorithm>

namespace {
struct ExerciseOption {
    QString exercise;
    QString query;
    QString alternative;
    QString alternativeQuery;
};

QHash<QString, QVector<ExerciseOption>> exerciseLibrary()
{
    return {
        {"pecho", {
            {"Press banca con barra", "barbell bench press", "Flexiones", "wide hand push up"},
            {"Press inclinado con mancuernas", "incline dumbbell bench press", "Flexiones inclinadas", "incline push up"},
            {"Aperturas con mancuernas", "dumbbell fly", "Flexiones abiertas", "wide hand push up"},
        }},
        {"espalda", {
            {"Jalon al pecho", "cable lat pulldown", "Remo invertido", "inverted row"},
            {"Remo sentado en cable", "cable seated row", "Remo con mochila", "dumbbell bent over row"},
            {"Peso muerto rumano", "romanian deadlift", "Buenos dias", "good morning"},
        }},
        {"piernas", {
            {"Sentadilla con barra", "barbell full squat", "Sentadilla goblet", "goblet squat"},
            {"Prensa de piernas", "leg press", "Zancadas caminando", "walking lunge"},
            {"Peso muerto rumano", "romanian deadlift", "Puente de gluteos", "glute bridge"},
        }},
        {"hombros", {
            {"Press militar con mancuernas", "dumbbell one arm shoulder press", "Flexiones declinadas", "decline push up"},
            {"Elevaciones laterales", "dumbbell lateral raise", "Elevaciones laterales con botellas", "dumbbell lateral raise"},
            {"Pajaros con mancuernas", "dumbbell rear lateral raise", "Press Arnold", "dumbbell arnold press"},
        }},
        {"core", {
            {"Plancha frontal", "front plank", "Plancha con rodillas", "kneeling plank tap shoulder"},
            {"Dead bug", "dead bug", "Crunch controlado", "crunch hands overhead"},
            {"Crunch abdominal", "decline crunch", "Crunch controlado", "crunch hands overhead"},
        }},
    };
}

QVector<QPair<QString, QStringList>> splitForGoal(const QString& goal)
{
    if (goal == "Fuerza") {
        return {
            {"Dia 1", {"pecho", "espalda", "piernas"}},
            {"Dia 2", {"piernas", "hombros", "core"}},
            {"Dia 3", {"pecho", "espalda", "piernas"}},
            {"Dia 4", {"piernas", "pecho", "espalda"}},
            {"Dia 5", {"hombros", "piernas", "core"}},
            {"Dia 6", {"pecho", "piernas", "espalda"}},
        };
    }
    if (goal == "Ganar Musculo") {
        return {
            {"Dia 1", {"pecho", "espalda", "hombros", "core"}},
            {"Dia 2", {"piernas", "espalda", "core"}},
            {"Dia 3", {"pecho", "hombros", "espalda", "core"}},
            {"Dia 4", {"piernas", "pecho", "espalda", "core"}},
            {"Dia 5", {"hombros", "piernas", "pecho", "core"}},
            {"Dia 6", {"espalda", "piernas", "core"}},
        };
    }
    if (goal == "Perder Grasa") {
        return {
            {"Dia 1", {"pecho", "espalda", "piernas", "core"}},
            {"Dia 2", {"piernas", "hombros", "espalda", "core"}},
            {"Dia 3", {"pecho", "piernas", "core"}},
            {"Dia 4", {"espalda", "hombros", "piernas", "core"}},
            {"Dia 5", {"pecho", "espalda", "core"}},
            {"Dia 6", {"piernas", "hombros", "core"}},
        };
    }
    return {
        {"Dia 1", {"pecho", "espalda", "core"}},
        {"Dia 2", {"piernas", "hombros", "core"}},
        {"Dia 3", {"pecho", "espalda", "piernas"}},
        {"Dia 4", {"piernas", "hombros", "core"}},
        {"Dia 5", {"pecho", "espalda", "core"}},
        {"Dia 6", {"piernas", "core"}},
    };
}

QString titleCaseFocus(const QString& focus)
{
    QString value = focus;
    if (!value.isEmpty()) {
        value[0] = value[0].toUpper();
    }
    return value;
}
}

QVector<ExercisePlanItem> TrainingEngine::generate(const UserProfile& profile)
{
    const int days = std::max(2, std::min(profile.days, 6));
    QString sets;
    QString reps;
    QString rest;
    QString why;

    if (profile.goal == "Perder Grasa") {
        sets = "3";
        reps = "10-15";
        rest = "45-75 s";
        why = "Volumen moderado, descansos cortos y ejercicios multiarticulares para elevar gasto energetico.";
    } else if (profile.goal == "Ganar Musculo") {
        sets = "3-4";
        reps = "8-12";
        rest = "60-120 s";
        why = "Rango util para hipertrofia: volumen suficiente, tecnica estable y progresion semanal.";
    } else if (profile.goal == "Fuerza") {
        sets = "4-5";
        reps = "3-6";
        rest = "2-3 min";
        why = "Menos repeticiones y mas descanso para priorizar tension mecanica y rendimiento.";
    } else {
        sets = "3";
        reps = "8-12";
        rest = "60-90 s";
        why = "Plan equilibrado para adherencia, salud y mejora general.";
    }

    if (profile.level == "Principiante") {
        sets = sets == "4-5" ? "3-4" : "2-3";
    }

    const auto library = exerciseLibrary();
    const auto split = splitForGoal(profile.goal);
    QVector<ExercisePlanItem> plan;

    for (int dayIndex = 0; dayIndex < days && dayIndex < split.size(); ++dayIndex) {
        const auto& day = split[dayIndex].first;
        const auto& muscles = split[dayIndex].second;
        for (const auto& muscle : muscles) {
            const auto options = library.value(muscle);
            if (options.isEmpty()) {
                continue;
            }
            ExerciseOption option = options.at(plan.size() % options.size());
            if (profile.equipment == "Casa / minimo equipo") {
                std::swap(option.exercise, option.alternative);
                std::swap(option.query, option.alternativeQuery);
            }
            plan.push_back({
                day,
                option.exercise,
                option.query,
                sets,
                reps,
                rest,
                titleCaseFocus(muscle),
                option.alternative,
                option.alternativeQuery,
                why,
            });
        }
    }
    return plan;
}
