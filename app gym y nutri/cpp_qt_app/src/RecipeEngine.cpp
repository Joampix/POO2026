#include "RecipeEngine.h"

#include <QHash>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace {
QHash<QString, FoodMacro> foods()
{
    return {
        {"pollo", {165, 31, 0, 3.6}},
        {"arroz", {130, 2.7, 28, 0.3}},
        {"limon", {29, 1.1, 9.3, 0.3}},
        {"queso", {280, 18, 3, 22}},
        {"huevo", {143, 12.6, 0.7, 9.5}},
        {"avena", {389, 16.9, 66.3, 6.9}},
        {"banana", {89, 1.1, 22.8, 0.3}},
        {"brocoli", {35, 2.4, 7.2, 0.4}},
        {"atun", {116, 25.5, 0, 0.8}},
        {"papa", {87, 1.9, 20.1, 0.1}},
        {"tomate", {18, 0.9, 3.9, 0.2}},
        {"lentejas", {116, 9, 20, 0.4}},
        {"aceite", {884, 0, 0, 100}},
        {"yogur", {62, 3.5, 4.7, 3.3}},
    };
}

QHash<QString, QString> aliases()
{
    return {
        {"pechuga", "pollo"},
        {"chicken", "pollo"},
        {"rice", "arroz"},
        {"lemon", "limon"},
        {"cheese", "queso"},
        {"egg", "huevo"},
        {"oats", "avena"},
        {"platano", "banana"},
        {"atun al natural", "atun"},
        {"tuna", "atun"},
        {"patata", "papa"},
        {"potato", "papa"},
        {"lentils", "lentejas"},
    };
}

QVector<RecipeTemplate> templates()
{
    return {
        {
            "Bowl de pollo al limon con arroz",
            {"pollo", "arroz"},
            {"limon", "queso", "brocoli", "tomate"},
            {{"pollo", 170}, {"arroz", 180}, {"limon", 15}, {"queso", 25}, {"brocoli", 100}, {"tomate", 80}},
            {"Cocinar o calentar el arroz y reservar.", "Dorar el pollo con sal, pimienta y jugo de limon.", "Servir en bowl y sumar queso en cantidad controlada."},
            "Combina proteina magra con carbohidrato util para energia y se puede ajustar facil segun objetivo.",
        },
        {
            "Arroz cremoso con pollo y queso",
            {"pollo", "arroz", "queso"},
            {"limon", "tomate"},
            {{"pollo", 160}, {"arroz", 170}, {"queso", 30}, {"limon", 10}, {"tomate", 80}},
            {"Mezclar arroz caliente con queso para dar textura cremosa.", "Agregar pollo cocido en cubos.", "Terminar con limon para levantar sabor sin sumar muchas calorias."},
            "Buena opcion de adherencia cuando el plan necesita una comida saciante y alta en proteina.",
        },
        {
            "Tortilla de huevo y queso",
            {"huevo", "queso"},
            {"tomate", "brocoli"},
            {{"huevo", 150}, {"queso", 25}, {"tomate", 100}, {"brocoli", 80}},
            {"Batir los huevos y cocinar a fuego medio.", "Agregar queso y vegetales picados.", "Doblar la tortilla cuando este firme."},
            "Alta en proteina y practica para comidas rapidas con pocos ingredientes.",
        },
        {
            "Pancakes de avena y banana",
            {"avena", "banana", "huevo"},
            {"yogur"},
            {{"avena", 55}, {"banana", 100}, {"huevo", 100}, {"yogur", 80}},
            {"Procesar avena, banana y huevo.", "Cocinar porciones chicas en sarten antiadherente.", "Servir con yogur si esta disponible."},
            "Aporta carbohidratos de digestion moderada y proteina para desayuno o merienda.",
        },
        {
            "Ensalada de atun, papa y tomate",
            {"atun", "papa"},
            {"tomate", "limon", "aceite"},
            {{"atun", 120}, {"papa", 220}, {"tomate", 120}, {"limon", 10}, {"aceite", 5}},
            {"Hervir papa en cubos y enfriar unos minutos.", "Mezclar con atun, tomate y limon.", "Agregar aceite medido si el plan permite mas grasas."},
            "Comida simple con proteina alta, carbohidrato saciante y grasas controlables.",
        },
        {
            "Guiso simple de lentejas y arroz",
            {"lentejas", "arroz"},
            {"tomate", "queso"},
            {{"lentejas", 220}, {"arroz", 120}, {"tomate", 100}, {"queso", 15}},
            {"Calentar lentejas con tomate y condimentos.", "Sumar arroz cocido para completar carbohidratos.", "Agregar queso al final si se busca mas sabor y calorias."},
            "Opcion economica, rica en fibra y util para sostener energia.",
        },
    };
}

QString normalize(QString value)
{
    value = value.trimmed().toLower();
    value.replace("á", "a").replace("é", "e").replace("í", "i").replace("ó", "o").replace("ú", "u");
    return aliases().value(value, value);
}

QVector<RecipeIngredient> buildAmounts(const RecipeTemplate& recipe, const QSet<QString>& available, const QString& goal)
{
    auto amounts = recipe.baseGrams;
    if (goal == "Perder Grasa") {
        for (const QString& carb : {"arroz", "papa", "avena", "banana"}) {
            if (amounts.contains(carb)) {
                amounts[carb] = static_cast<int>(amounts[carb] * 0.75);
            }
        }
        for (const QString& protein : {"pollo", "atun", "huevo", "lentejas"}) {
            if (amounts.contains(protein)) {
                amounts[protein] = static_cast<int>(amounts[protein] * 1.1);
            }
        }
        if (amounts.contains("queso")) {
            amounts["queso"] = std::min(amounts["queso"], 15);
        }
        if (amounts.contains("aceite")) {
            amounts["aceite"] = std::min(amounts["aceite"], 3);
        }
    } else if (goal == "Ganar Musculo") {
        for (const QString& carb : {"arroz", "papa", "avena", "banana"}) {
            if (amounts.contains(carb)) {
                amounts[carb] = static_cast<int>(amounts[carb] * 1.3);
            }
        }
        for (const QString& protein : {"pollo", "atun", "huevo", "lentejas"}) {
            if (amounts.contains(protein)) {
                amounts[protein] = static_cast<int>(amounts[protein] * 1.15);
            }
        }
    }

    QVector<RecipeIngredient> result;
    QStringList selected = recipe.required;
    for (const auto& item : recipe.optional) {
        if (available.contains(item)) {
            selected.append(item);
        }
    }
    for (const auto& item : selected) {
        if (amounts.contains(item)) {
            result.push_back({item, amounts.value(item)});
        }
    }
    return result;
}

QVector<int> calculateMacros(const QVector<RecipeIngredient>& ingredients)
{
    const auto data = foods();
    double kcal = 0;
    double protein = 0;
    double carbs = 0;
    double fat = 0;
    for (const auto& ingredient : ingredients) {
        if (!data.contains(ingredient.name)) {
            continue;
        }
        const auto macro = data.value(ingredient.name);
        const double factor = ingredient.grams / 100.0;
        kcal += macro.kcal * factor;
        protein += macro.protein * factor;
        carbs += macro.carbs * factor;
        fat += macro.fat * factor;
    }
    return {static_cast<int>(kcal), static_cast<int>(protein), static_cast<int>(carbs), static_cast<int>(fat)};
}

int scoreRecipe(const RecipeTemplate& recipe, const QSet<QString>& available, const MacroPlan* plan, int kcal, int protein)
{
    int requiredScore = 0;
    for (const auto& item : recipe.required) {
        if (available.contains(item)) {
            requiredScore += 35;
        }
    }
    int optionalScore = 0;
    for (const auto& item : recipe.optional) {
        if (available.contains(item)) {
            optionalScore += 8;
        }
    }
    if (!plan) {
        return std::min(100, requiredScore + optionalScore);
    }

    const double targetKcal = plan->calories * 0.28;
    const double targetProtein = plan->protein * 0.30;
    const int kcalPenalty = std::min(25, static_cast<int>(std::abs(kcal - targetKcal) / std::max(targetKcal, 1.0) * 25.0));
    const int proteinPenalty = protein >= targetProtein * 0.75 ? 0 : 15;
    return std::max(0, std::min(100, requiredScore + optionalScore + 20 - kcalPenalty - proteinPenalty));
}

QString fitLabel(int score)
{
    if (score >= 85) {
        return "Muy buena para tu plan";
    }
    if (score >= 65) {
        return "Compatible con ajustes";
    }
    return "Usable, pero incompleta";
}
}

QSet<QString> RecipeEngine::parseIngredients(const QString& text)
{
    QSet<QString> result;
    for (const auto& part : text.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts)) {
        const auto item = normalize(part);
        if (!item.isEmpty()) {
            result.insert(item);
        }
    }
    return result;
}

QVector<RecipeSuggestion> RecipeEngine::generate(const QSet<QString>& available, const MacroPlan* plan, const QString& goal, int limit)
{
    QVector<RecipeSuggestion> suggestions;
    for (const auto& recipe : templates()) {
        int requiredMatches = 0;
        int optionalMatches = 0;
        for (const auto& item : recipe.required) {
            if (available.contains(item)) {
                ++requiredMatches;
            }
        }
        for (const auto& item : recipe.optional) {
            if (available.contains(item)) {
                ++optionalMatches;
            }
        }
        if (requiredMatches == 0 || (requiredMatches < recipe.required.size() && optionalMatches == 0)) {
            continue;
        }

        const auto ingredients = buildAmounts(recipe, available, goal);
        const auto macros = calculateMacros(ingredients);
        const int score = scoreRecipe(recipe, available, plan, macros[0], macros[1]);

        QStringList missing;
        for (const auto& item : recipe.optional) {
            if (!available.contains(item)) {
                missing.append(item);
            }
        }
        suggestions.push_back({
            recipe.name,
            ingredients,
            missing,
            macros[0],
            macros[1],
            macros[2],
            macros[3],
            score,
            fitLabel(score),
            recipe.steps,
            recipe.reason,
        });
    }

    std::sort(suggestions.begin(), suggestions.end(), [](const RecipeSuggestion& left, const RecipeSuggestion& right) {
        return left.score > right.score;
    });
    while (suggestions.size() > limit) {
        suggestions.pop_back();
    }
    return suggestions;
}

RecipeSuggestion RecipeEngine::fallbackRecipe(const MacroPlan* plan)
{
    QSet<QString> available = {"pollo", "arroz", "huevo"};
    const auto suggestions = generate(available, plan, plan ? plan->objective : "Mantenimiento", 1);
    return suggestions.isEmpty() ? RecipeSuggestion{} : suggestions.first();
}

QVector<int> RecipeEngine::mealTargets(const MacroPlan* plan)
{
    if (!plan) {
        return {0, 0, 0, 0};
    }
    return {
        static_cast<int>(plan->calories * 0.28),
        static_cast<int>(plan->protein * 0.30),
        static_cast<int>(plan->carbs * 0.28),
        static_cast<int>(plan->fats * 0.28),
    };
}
