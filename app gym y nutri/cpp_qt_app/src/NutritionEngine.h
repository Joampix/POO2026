#pragma once

#include "Models.h"

#include <QStringList>

class NutritionEngine {
public:
    static QStringList activities();
    static QStringList goals();
    static MacroPlan calculate(const UserProfile& profile);

private:
    static double activityFactor(const QString& activity);
};
