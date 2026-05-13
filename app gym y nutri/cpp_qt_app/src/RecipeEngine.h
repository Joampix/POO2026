#pragma once

#include "Models.h"

#include <QSet>
#include <QVector>

class RecipeEngine {
public:
    static QSet<QString> parseIngredients(const QString& text);
    static QVector<RecipeSuggestion> generate(const QSet<QString>& available, const MacroPlan* plan, const QString& goal, int limit = 4);
    static RecipeSuggestion fallbackRecipe(const MacroPlan* plan);
    static QVector<int> mealTargets(const MacroPlan* plan);
};
