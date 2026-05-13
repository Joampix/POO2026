#pragma once

#include <QString>
#include <QStringList>
#include <QHash>
#include <QVector>

struct UserProfile {
    QString name;
    double weight = 75.0;
    int height = 175;
    int age = 25;
    QString gender = "Masculino";
    QString activity = "Moderado";
    QString goal = "Mantenimiento";
    QString level = "Principiante";
    int days = 4;
    QString equipment = "Gimnasio completo";
    QString notes;
};

struct MacroPlan {
    double weight = 0.0;
    int height = 0;
    int age = 0;
    QString gender;
    QString activity;
    QString objective;
    int tmb = 0;
    int total = 0;
    int calories = 0;
    int protein = 0;
    int carbs = 0;
    int fats = 0;
};

struct ExercisePlanItem {
    QString day;
    QString exercise;
    QString apiQuery;
    QString sets;
    QString reps;
    QString rest;
    QString focus;
    QString alternative;
    QString alternativeQuery;
    QString why;
};

struct ApiExercise {
    QString name;
    QString description;
    QString equipment = "No especificado";
    QString muscles = "General";
    QString source = "API";
    QString gifUrl;
};

struct FoodMacro {
    double kcal = 0.0;
    double protein = 0.0;
    double carbs = 0.0;
    double fat = 0.0;
};

struct RecipeIngredient {
    QString name;
    int grams = 0;
};

struct RecipeTemplate {
    QString name;
    QStringList required;
    QStringList optional;
    QHash<QString, int> baseGrams;
    QStringList steps;
    QString reason;
};

struct RecipeSuggestion {
    QString name;
    QVector<RecipeIngredient> ingredients;
    QStringList missingOptional;
    int kcal = 0;
    int protein = 0;
    int carbs = 0;
    int fat = 0;
    int score = 0;
    QString fitLabel;
    QStringList steps;
    QString reason;
};

struct ProgressEntry {
    QString week;
    double weight = 0.0;
    double waist = 0.0;
    int workouts = 0;
    int energy = 0;
    QString notes;
};
