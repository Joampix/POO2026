#pragma once

#include "DataManager.h"
#include "ExerciseDbClient.h"
#include "GeminiClient.h"
#include "LocalAuditStore.h"
#include "Models.h"
#include "ProgressStore.h"
#include "WgerClient.h"

#include <QBuffer>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMovie>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QComboBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(DataManager* dataManager = nullptr, LocalAuditStore* auditStore = nullptr, const QString& activeUser = {}, QWidget* parent = nullptr);

private:
    UserProfile m_profile;
    MacroPlan m_macroPlan;
    bool m_hasProfile = false;
    QVector<ExercisePlanItem> m_trainingPlan;
    QString m_pendingExerciseQuery;
    QString m_activeUser = "offline";
    ExercisePlanItem m_currentPlanItem;
    ApiExercise m_exerciseDbExercise;
    ApiExercise m_wgerExercise;
    bool m_hasExerciseDbExercise = false;
    bool m_hasWgerExercise = false;
    QString m_exerciseDbError;
    QString m_wgerError;

    QStackedWidget* m_stack = nullptr;
    QLabel* m_headerTitle = nullptr;
    QLabel* m_status = nullptr;
    QVector<QPushButton*> m_navButtons;

    QLineEdit* m_nameEdit = nullptr;
    QDoubleSpinBox* m_weightSpin = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QSpinBox* m_ageSpin = nullptr;
    QComboBox* m_genderCombo = nullptr;
    QComboBox* m_activityCombo = nullptr;
    QComboBox* m_goalCombo = nullptr;
    QComboBox* m_levelCombo = nullptr;
    QSpinBox* m_daysSpin = nullptr;
    QComboBox* m_equipmentCombo = nullptr;
    QTextEdit* m_notesEdit = nullptr;
    QTextBrowser* m_profileSummary = nullptr;

    QLabel* m_caloriesValue = nullptr;
    QLabel* m_proteinValue = nullptr;
    QLabel* m_carbsValue = nullptr;
    QLabel* m_fatsValue = nullptr;
    QTextBrowser* m_nutritionExplanation = nullptr;

    QTableWidget* m_trainingTable = nullptr;
    QLabel* m_gifPreview = nullptr;
    QTextBrowser* m_exerciseDetail = nullptr;
    QNetworkAccessManager m_gifManager;
    QBuffer* m_gifBuffer = nullptr;
    QMovie* m_gifMovie = nullptr;

    QLineEdit* m_ingredientsEdit = nullptr;
    QTextBrowser* m_recipesOutput = nullptr;

    QLineEdit* m_weekEdit = nullptr;
    QDoubleSpinBox* m_progressWeightSpin = nullptr;
    QDoubleSpinBox* m_waistSpin = nullptr;
    QSpinBox* m_workoutsSpin = nullptr;
    QSpinBox* m_energySpin = nullptr;
    QTextEdit* m_progressNotesEdit = nullptr;
    QTableWidget* m_progressTable = nullptr;

    QTextBrowser* m_chatBrowser = nullptr;
    QTextEdit* m_questionEdit = nullptr;
    QLabel* m_aiStatus = nullptr;
    QPushButton* m_askButton = nullptr;

    ProgressStore m_progressStore;
    DataManager* m_dataManager = nullptr;
    LocalAuditStore* m_auditStore = nullptr;
    ExerciseDbClient* m_exerciseClient = nullptr;
    WgerClient* m_wgerClient = nullptr;
    GeminiClient* m_geminiClient = nullptr;

    void buildUi();
    QWidget* createSidebar();
    QWidget* createHeaderAndStack();
    QWidget* createProfilePage();
    QWidget* createNutritionPage();
    QWidget* createTrainingPage();
    QWidget* createRecipesPage();
    QWidget* createProgressPage();
    QWidget* createCoachPage();
    QWidget* createEvidencePage();

    QLabel* makeLabel(const QString& text, const QString& objectName = {}, bool wrap = false);
    QWidget* makeMetricCard(const QString& title, QLabel** valueLabel, const QString& detail);
    QPushButton* makeNavButton(const QString& title, int index);
    void switchPage(int index, const QString& title);
    void applyAppStyle();

    UserProfile readProfile() const;
    void applyProfile(const UserProfile& profile);
    void renderProfileSummary();
    void renderNutrition();
    void renderTraining();
    void renderRecipes();
    void renderProgress();
    void renderCoachContext();

    void showTrainingReference(int row);
    void showExercise(const ApiExercise& exercise, const ExercisePlanItem& planItem);
    void renderExerciseSources();
    QString exerciseSourceHtml(const ApiExercise& exercise, const QString& title) const;
    void showMissingExercise(const ExercisePlanItem& planItem, const QString& message);
    void loadGif(const QString& url);
    QString instructionsHtml(const QString& description) const;

    void generateRecipes();
    void addProgressEntry();
    void askCoach(const QString& question = {});
    void appendChat(const QString& role, const QString& text);
    QString htmlEscape(const QString& text) const;
};
