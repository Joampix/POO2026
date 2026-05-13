#include "NutritionEngine.h"

#include <algorithm>

QStringList NutritionEngine::activities()
{
    return {"Sedentario", "Ligero", "Moderado", "Intenso"};
}

QStringList NutritionEngine::goals()
{
    return {"Mantenimiento", "Perder Grasa", "Ganar Musculo", "Fuerza"};
}

MacroPlan NutritionEngine::calculate(const UserProfile& profile)
{
    const int adjustment = profile.gender == "Masculino" ? 5 : -161;
    const int tmb = static_cast<int>((10.0 * profile.weight) + (6.25 * profile.height) - (5.0 * profile.age) + adjustment);
    const int total = static_cast<int>(tmb * activityFactor(profile.activity));

    int calories = total;
    if (profile.goal == "Perder Grasa") {
        calories -= 400;
    } else if (profile.goal == "Ganar Musculo") {
        calories += 400;
    }
    calories = std::max(900, calories);

    const int protein = static_cast<int>(profile.weight * 2.2);
    const int fats = static_cast<int>(profile.weight * 0.9);
    const int carbs = std::max(0, static_cast<int>((calories - (protein * 4) - (fats * 9)) / 4.0));

    MacroPlan plan;
    plan.weight = profile.weight;
    plan.height = profile.height;
    plan.age = profile.age;
    plan.gender = profile.gender;
    plan.activity = profile.activity;
    plan.objective = profile.goal;
    plan.tmb = tmb;
    plan.total = total;
    plan.calories = calories;
    plan.protein = protein;
    plan.carbs = carbs;
    plan.fats = fats;
    return plan;
}

double NutritionEngine::activityFactor(const QString& activity)
{
    if (activity == "Ligero") {
        return 1.375;
    }
    if (activity == "Moderado") {
        return 1.55;
    }
    if (activity == "Intenso") {
        return 1.725;
    }
    return 1.2;
}
