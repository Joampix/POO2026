#pragma once

#include "Models.h"

#include <QVector>

class TrainingEngine {
public:
    static QVector<ExercisePlanItem> generate(const UserProfile& profile);
};
