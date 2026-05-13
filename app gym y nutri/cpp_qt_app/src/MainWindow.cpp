#include "MainWindow.h"

#include "NutritionEngine.h"
#include "RecipeEngine.h"
#include "TrainingEngine.h"

#include <QApplication>
#include <QDate>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPixmap>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTextStream>
#include <QVBoxLayout>

MainWindow::MainWindow(DataManager* dataManager, LocalAuditStore* auditStore, const QString& activeUser, QWidget* parent)
    : QMainWindow(parent)
    , m_progressStore(QCoreApplication::applicationDirPath() + "/progreso_semanal_cpp.json")
    , m_activeUser(activeUser.isEmpty() ? "offline" : activeUser)
    , m_dataManager(dataManager)
    , m_auditStore(auditStore)
{
    m_exerciseClient = new ExerciseDbClient(this);
    m_wgerClient = new WgerClient(this);
    m_geminiClient = new GeminiClient(this);
    setWindowTitle("Conca Gym C++ - Entrenamiento y nutricion");
    resize(1260, 800);
    applyAppStyle();
    buildUi();

    connect(m_exerciseClient, &ExerciseDbClient::exerciseReady, this, [this](const QString& query, const ApiExercise& exercise) {
        if (query != m_pendingExerciseQuery) {
            return;
        }
        const int row = m_trainingTable->currentRow();
        if (row >= 0 && row < m_trainingPlan.size()) {
            m_exerciseDbExercise = exercise;
            m_hasExerciseDbExercise = true;
            m_exerciseDbError.clear();
            renderExerciseSources();
        }
    });
    connect(m_exerciseClient, &ExerciseDbClient::exerciseFailed, this, [this](const QString& query, const QString& message) {
        if (query != m_pendingExerciseQuery) {
            return;
        }
        const int row = m_trainingTable->currentRow();
        if (row >= 0 && row < m_trainingPlan.size()) {
            m_exerciseDbError = message;
            if (!m_hasWgerExercise && !m_wgerError.isEmpty()) {
                showMissingExercise(m_trainingPlan.at(row), "ExerciseDB: " + m_exerciseDbError + "\nwger: " + m_wgerError);
            }
        }
    });
    connect(m_wgerClient, &WgerClient::exerciseReady, this, [this](const QString& query, const ApiExercise& exercise) {
        if (query != m_pendingExerciseQuery) {
            return;
        }
        const int row = m_trainingTable->currentRow();
        if (row >= 0 && row < m_trainingPlan.size()) {
            m_wgerExercise = exercise;
            m_hasWgerExercise = true;
            m_wgerError.clear();
            renderExerciseSources();
        }
    });
    connect(m_wgerClient, &WgerClient::exerciseFailed, this, [this](const QString& query, const QString& message) {
        if (query != m_pendingExerciseQuery) {
            return;
        }
        const int row = m_trainingTable->currentRow();
        if (row >= 0 && row < m_trainingPlan.size()) {
            m_wgerError = message;
            if (!m_hasExerciseDbExercise && !m_exerciseDbError.isEmpty()) {
                showMissingExercise(m_trainingPlan.at(row), "ExerciseDB: " + m_exerciseDbError + "\nwger: " + m_wgerError);
            }
        }
    });
    connect(m_geminiClient, &GeminiClient::answerReady, this, [this](bool ok, const QString& text, const QString& source) {
        appendChat(source == "Gemini" ? "Coach IA" : "Sistema", text);
        m_askButton->setEnabled(true);
        if (ok) {
            m_aiStatus->setText("Gemini activo");
        } else {
            m_aiStatus->setText(m_geminiClient->configured() ? "Gemini con error: revisar clave/permisos" : "Gemini sin clave: falta GEMINI_API_KEY");
        }
    });
    if (m_auditStore) {
        m_auditStore->recordLocalEvent("app.open", "info", "Usuario activo: " + m_activeUser);
    }
    if (m_dataManager && m_dataManager->authenticated()) {
        m_dataManager->registerEvent("app.open", "info", "App Qt iniciada por " + m_activeUser);
    }
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(createSidebar());
    root->addWidget(createHeaderAndStack(), 1);
    setCentralWidget(central);
    switchPage(0, "Perfil");
}

QWidget* MainWindow::createSidebar()
{
    auto* sidebar = new QFrame(this);
    sidebar->setObjectName("Sidebar");
    sidebar->setFixedWidth(250);
    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(20, 24, 20, 18);
    layout->setSpacing(10);
    layout->addWidget(makeLabel("Conca Gym", "Brand"));
    layout->addWidget(makeLabel("C++ / Qt Widgets", "SidebarSub"));
    layout->addSpacing(18);

    const QStringList titles = {"Perfil", "Plan nutricional", "Entrenamiento", "Recetas", "Progreso", "Coach IA", "Info cientifica"};
    for (int i = 0; i < titles.size(); ++i) {
        layout->addWidget(makeNavButton(titles.at(i), i));
    }
    layout->addStretch(1);
    return sidebar;
}

QWidget* MainWindow::createHeaderAndStack()
{
    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QFrame(content);
    header->setObjectName("Header");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(24, 16, 24, 16);
    m_headerTitle = makeLabel("Perfil", "HeaderTitle");
    m_status = makeLabel(QString("Usuario: %1 | FastAPI %2").arg(m_activeUser, m_dataManager && m_dataManager->authenticated() ? "conectado" : "offline"), "Muted");
    headerLayout->addWidget(m_headerTitle);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_status);
    layout->addWidget(header);

    m_stack = new QStackedWidget(content);
    m_stack->addWidget(createProfilePage());
    m_stack->addWidget(createNutritionPage());
    m_stack->addWidget(createTrainingPage());
    m_stack->addWidget(createRecipesPage());
    m_stack->addWidget(createProgressPage());
    m_stack->addWidget(createCoachPage());
    m_stack->addWidget(createEvidencePage());
    layout->addWidget(m_stack, 1);
    return content;
}

QWidget* MainWindow::createProfilePage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    root->addWidget(makeLabel("Carga el perfil y genera la base de dieta, macros, rutina y recetas.", "Muted", true));

    auto* panel = new QFrame;
    panel->setObjectName("Panel");
    auto* grid = new QGridLayout(panel);
    grid->setContentsMargins(18, 16, 18, 16);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(10);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("Nombre del usuario");
    m_weightSpin = new QDoubleSpinBox;
    m_weightSpin->setRange(40, 180);
    m_weightSpin->setDecimals(1);
    m_weightSpin->setValue(75);
    m_weightSpin->setSuffix(" kg");
    m_heightSpin = new QSpinBox;
    m_heightSpin->setRange(120, 230);
    m_heightSpin->setValue(175);
    m_heightSpin->setSuffix(" cm");
    m_ageSpin = new QSpinBox;
    m_ageSpin->setRange(14, 90);
    m_ageSpin->setValue(25);
    m_genderCombo = new QComboBox;
    m_genderCombo->addItems({"Masculino", "Femenino"});
    m_activityCombo = new QComboBox;
    m_activityCombo->addItems(NutritionEngine::activities());
    m_activityCombo->setCurrentText("Moderado");
    m_goalCombo = new QComboBox;
    m_goalCombo->addItems(NutritionEngine::goals());
    m_levelCombo = new QComboBox;
    m_levelCombo->addItems({"Principiante", "Intermedio", "Avanzado"});
    m_daysSpin = new QSpinBox;
    m_daysSpin->setRange(2, 6);
    m_daysSpin->setValue(4);
    m_daysSpin->setSuffix(" dias");
    m_equipmentCombo = new QComboBox;
    m_equipmentCombo->addItems({"Gimnasio completo", "Casa / minimo equipo"});
    m_notesEdit = new QTextEdit;
    m_notesEdit->setPlaceholderText("Lesiones, horarios, preferencias o limitaciones");
    m_notesEdit->setMaximumHeight(88);

    QVector<QPair<QString, QWidget*>> fields = {
        {"Nombre", m_nameEdit}, {"Peso", m_weightSpin}, {"Altura", m_heightSpin}, {"Edad", m_ageSpin},
        {"Genero", m_genderCombo}, {"Actividad", m_activityCombo}, {"Objetivo", m_goalCombo}, {"Nivel", m_levelCombo},
        {"Dias/semana", m_daysSpin}, {"Equipamiento", m_equipmentCombo},
    };
    for (int i = 0; i < fields.size(); ++i) {
        const int row = i / 2;
        const int col = (i % 2) * 2;
        grid->addWidget(makeLabel(fields[i].first), row, col);
        grid->addWidget(fields[i].second, row, col + 1);
    }
    grid->addWidget(makeLabel("Notas"), 5, 0);
    grid->addWidget(m_notesEdit, 5, 1, 1, 3);

    auto* button = new QPushButton("Generar base del plan");
    button->setObjectName("PrimaryButton");
    connect(button, &QPushButton::clicked, this, [this]() { applyProfile(readProfile()); });
    grid->addWidget(button, 6, 0, 1, 4);
    root->addWidget(panel);

    m_profileSummary = new QTextBrowser;
    m_profileSummary->setObjectName("TextPanel");
    m_profileSummary->setHtml("<p>Completa el perfil y genera el plan base.</p>");
    root->addWidget(m_profileSummary, 1);
    return page;
}

QWidget* MainWindow::createNutritionPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    auto* metrics = new QHBoxLayout;
    metrics->addWidget(makeMetricCard("Calorias", &m_caloriesValue, "energia diaria"));
    metrics->addWidget(makeMetricCard("Proteinas", &m_proteinValue, "masa muscular"));
    metrics->addWidget(makeMetricCard("Carbohidratos", &m_carbsValue, "rendimiento"));
    metrics->addWidget(makeMetricCard("Grasas", &m_fatsValue, "salud hormonal"));
    root->addLayout(metrics);

    auto* actions = new QHBoxLayout;
    auto* recalc = new QPushButton("Recalcular con perfil");
    recalc->setObjectName("PrimaryButton");
    connect(recalc, &QPushButton::clicked, this, [this]() {
        if (!m_hasProfile) {
            QMessageBox::information(this, "Primero el perfil", "Carga el perfil del usuario antes de recalcular.");
            switchPage(0, "Perfil");
            return;
        }
        applyProfile(m_profile);
    });
    auto* exportButton = new QPushButton("Exportar plan HTML");
    exportButton->setObjectName("SecondaryButton");
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        if (!m_hasProfile) {
            return;
        }
        const QString path = QFileDialog::getSaveFileName(this, "Guardar plan", "Plan_Nutricional.html", "HTML (*.html)");
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "No se pudo guardar el archivo.");
            return;
        }
        QTextStream out(&file);
        out << m_nutritionExplanation->toHtml();
    });
    actions->addWidget(recalc);
    actions->addWidget(exportButton);
    actions->addStretch(1);
    root->addLayout(actions);

    m_nutritionExplanation = new QTextBrowser;
    m_nutritionExplanation->setObjectName("TextPanel");
    m_nutritionExplanation->setHtml("<h3>Por que esta dieta</h3><p>Primero carga un perfil para calcular macros.</p>");
    root->addWidget(m_nutritionExplanation, 1);
    return page;
}

QWidget* MainWindow::createTrainingPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    auto* hero = new QFrame;
    hero->setObjectName("HeroPanel");
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->addWidget(makeLabel("Biblioteca visual de ejercicios", "HeroTitle"));
    heroLayout->addWidget(makeLabel("Selecciona un ejercicio de la rutina para ver GIFs e instrucciones cruzando ExerciseDB y wger.", "HeroText", true));
    root->addWidget(hero);

    auto* content = new QHBoxLayout;
    content->setSpacing(16);
    auto* left = new QFrame;
    left->setObjectName("Panel");
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->addWidget(makeLabel("Rutina generada", "SectionLabel"));
    m_trainingTable = new QTableWidget(0, 7);
    m_trainingTable->setObjectName("DataTable");
    m_trainingTable->setHorizontalHeaderLabels({"Dia", "Ejercicio", "Series", "Reps", "Descanso", "Foco", "Alternativa"});
    m_trainingTable->verticalHeader()->setVisible(false);
    m_trainingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trainingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_trainingTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_trainingTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_trainingTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    connect(m_trainingTable, &QTableWidget::itemSelectionChanged, this, [this]() { showTrainingReference(m_trainingTable->currentRow()); });
    leftLayout->addWidget(m_trainingTable, 1);
    content->addWidget(left, 3);

    auto* right = new QFrame;
    right->setObjectName("Panel");
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->addWidget(makeLabel("Referencia del ejercicio", "SectionLabel"));
    m_gifPreview = new QLabel("Selecciona un ejercicio para ver la referencia visual");
    m_gifPreview->setObjectName("GifPreview");
    m_gifPreview->setAlignment(Qt::AlignCenter);
    m_gifPreview->setMinimumSize(360, 260);
    rightLayout->addWidget(m_gifPreview);
    m_exerciseDetail = new QTextBrowser;
    m_exerciseDetail->setObjectName("TextPanel");
    m_exerciseDetail->setOpenExternalLinks(true);
    m_exerciseDetail->setHtml("<p>Selecciona un ejercicio de la rutina.</p>");
    rightLayout->addWidget(m_exerciseDetail, 1);
    content->addWidget(right, 2);
    root->addLayout(content, 1);
    return page;
}

QWidget* MainWindow::createRecipesPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    auto* controls = new QFrame;
    controls->setObjectName("Panel");
    auto* row = new QHBoxLayout(controls);
    row->addWidget(makeLabel("Ingredientes en casa"));
    m_ingredientsEdit = new QLineEdit;
    m_ingredientsEdit->setPlaceholderText("Ej: pollo, arroz, huevo, avena");
    auto* button = new QPushButton("Generar recetas");
    button->setObjectName("PrimaryButton");
    connect(button, &QPushButton::clicked, this, &MainWindow::generateRecipes);
    row->addWidget(m_ingredientsEdit, 1);
    row->addWidget(button);
    root->addWidget(controls);

    m_recipesOutput = new QTextBrowser;
    m_recipesOutput->setObjectName("TextPanel");
    m_recipesOutput->setHtml("<p>Carga ingredientes para sugerir recetas ajustadas al plan.</p>");
    root->addWidget(m_recipesOutput, 1);
    return page;
}

QWidget* MainWindow::createProgressPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    auto* panel = new QFrame;
    panel->setObjectName("Panel");
    auto* grid = new QGridLayout(panel);
    m_weekEdit = new QLineEdit(QDate::currentDate().toString("yyyy-MM-dd"));
    m_progressWeightSpin = new QDoubleSpinBox;
    m_progressWeightSpin->setRange(40, 180);
    m_progressWeightSpin->setDecimals(1);
    m_progressWeightSpin->setValue(75);
    m_progressWeightSpin->setSuffix(" kg");
    m_waistSpin = new QDoubleSpinBox;
    m_waistSpin->setRange(40, 180);
    m_waistSpin->setDecimals(1);
    m_waistSpin->setValue(85);
    m_waistSpin->setSuffix(" cm");
    m_workoutsSpin = new QSpinBox;
    m_workoutsSpin->setRange(0, 14);
    m_energySpin = new QSpinBox;
    m_energySpin->setRange(1, 10);
    m_energySpin->setValue(7);
    m_progressNotesEdit = new QTextEdit;
    m_progressNotesEdit->setMaximumHeight(70);
    m_progressNotesEdit->setPlaceholderText("Notas de la semana");
    QVector<QPair<QString, QWidget*>> fields = {{"Semana", m_weekEdit}, {"Peso", m_progressWeightSpin}, {"Cintura", m_waistSpin}, {"Entrenos", m_workoutsSpin}, {"Energia", m_energySpin}};
    for (int i = 0; i < fields.size(); ++i) {
        grid->addWidget(makeLabel(fields[i].first), i / 2, (i % 2) * 2);
        grid->addWidget(fields[i].second, i / 2, (i % 2) * 2 + 1);
    }
    grid->addWidget(makeLabel("Notas"), 3, 0);
    grid->addWidget(m_progressNotesEdit, 3, 1, 1, 3);
    auto* addButton = new QPushButton("Guardar progreso");
    addButton->setObjectName("PrimaryButton");
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addProgressEntry);
    grid->addWidget(addButton, 4, 0, 1, 4);
    root->addWidget(panel);

    m_progressTable = new QTableWidget(0, 6);
    m_progressTable->setObjectName("DataTable");
    m_progressTable->setHorizontalHeaderLabels({"Semana", "Peso", "Cintura", "Entrenos", "Energia", "Notas"});
    m_progressTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_progressTable->verticalHeader()->setVisible(false);
    root->addWidget(m_progressTable, 1);
    renderProgress();
    return page;
}

QWidget* MainWindow::createCoachPage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    auto* hero = new QFrame;
    hero->setObjectName("HeroPanel");
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->addWidget(makeLabel("Coach IA", "HeroTitle"));
    heroLayout->addWidget(makeLabel("Chatbot contextual: usa perfil, macros, objetivo y progreso. Gemini se detecta desde GEMINI_API_KEY o .env.", "HeroText", true));
    m_aiStatus = makeLabel(m_geminiClient->configured() ? "Gemini activo" : "Gemini sin clave: falta GEMINI_API_KEY", "Muted");
    heroLayout->addWidget(m_aiStatus);
    root->addWidget(hero);

    auto* quick = new QHBoxLayout;
    for (const auto& item : QVector<QPair<QString, QString>>{
             {"Por que mi dieta?", "Explicame por que mi dieta y macros encajan con mi objetivo actual."},
             {"Por que mi rutina?", "Explicame por que mi rutina encaja con mi objetivo y nivel."},
             {"Ajustar semana", "Que deberia ajustar esta semana para progresar sin perder adherencia?"},
             {"FAQ cientifica", "Armame una FAQ cientifica breve sobre alimentacion, entrenamiento y progreso."},
         }) {
        auto* button = new QPushButton(item.first);
        button->setObjectName("SecondaryButton");
        connect(button, &QPushButton::clicked, this, [this, q = item.second]() { askCoach(q); });
        quick->addWidget(button);
    }
    root->addLayout(quick);

    m_chatBrowser = new QTextBrowser;
    m_chatBrowser->setObjectName("TextPanel");
    m_chatBrowser->setHtml("<p>Hace una consulta para recibir una respuesta contextual.</p>");
    root->addWidget(m_chatBrowser, 1);
    auto* bottom = new QHBoxLayout;
    m_questionEdit = new QTextEdit;
    m_questionEdit->setMaximumHeight(92);
    m_questionEdit->setPlaceholderText("Ej: Que deberia ajustar esta semana para perder grasa sin bajar rendimiento?");
    m_askButton = new QPushButton("Enviar");
    m_askButton->setObjectName("PrimaryButton");
    connect(m_askButton, &QPushButton::clicked, this, [this]() { askCoach(); });
    bottom->addWidget(m_questionEdit, 1);
    bottom->addWidget(m_askButton);
    root->addLayout(bottom);
    return page;
}

QWidget* MainWindow::createEvidencePage()
{
    auto* page = new QWidget;
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(24, 22, 24, 22);
    auto* browser = new QTextBrowser;
    browser->setObjectName("TextPanel");
    browser->setOpenExternalLinks(true);
    browser->setHtml(R"HTML(
<h2>Informacion util y criterio cientifico</h2>
<p>La app es educativa y no reemplaza a medico, nutricionista ni entrenador certificado.</p>
<h3>Nutricion</h3>
<ul>
<li>La dieta se basa en energia estimada, objetivo y adherencia, no en restricciones extremas.</li>
<li>Se priorizan alimentos poco procesados, proteinas suficientes, verduras, frutas, grasas saludables y carbohidratos utiles para entrenar.</li>
</ul>
<h3>Entrenamiento</h3>
<ul>
<li>La rutina usa ejercicios multiarticulares, referencias visuales y progresion gradual.</li>
<li>Para hipertrofia se prioriza volumen recuperable; para fuerza, carga y descanso; para perdida de grasa, constancia y gasto semanal.</li>
</ul>
<h3>Fuentes para documentar</h3>
<ul>
<li>USDA/HHS Dietary Guidelines for Americans.</li>
<li>CDC Nutrition Guidelines and Nutrition Facts Label.</li>
<li>NIH Office of Dietary Supplements.</li>
<li>ExerciseDB OSS API para biblioteca visual de ejercicios.</li>
<li>wger API como segunda fuente abierta de ejercicios, musculos, equipo e imagenes.</li>
<li>Gemini API para Coach IA contextual.</li>
</ul>
)HTML");
    root->addWidget(browser, 1);
    return page;
}

QLabel* MainWindow::makeLabel(const QString& text, const QString& objectName, bool wrap)
{
    auto* label = new QLabel(text);
    if (!objectName.isEmpty()) {
        label->setObjectName(objectName);
    }
    label->setWordWrap(wrap);
    return label;
}

QWidget* MainWindow::makeMetricCard(const QString& title, QLabel** valueLabel, const QString& detail)
{
    auto* card = new QFrame;
    card->setObjectName("MetricCard");
    auto* layout = new QVBoxLayout(card);
    layout->addWidget(makeLabel(title, "MetricTitle"));
    *valueLabel = makeLabel("-", "MetricValue");
    layout->addWidget(*valueLabel);
    layout->addWidget(makeLabel(detail, "Muted"));
    return card;
}

QPushButton* MainWindow::makeNavButton(const QString& title, int index)
{
    auto* button = new QPushButton(title);
    button->setObjectName("NavButton");
    button->setCheckable(true);
    connect(button, &QPushButton::clicked, this, [this, index, title]() { switchPage(index, title); });
    m_navButtons.append(button);
    return button;
}

void MainWindow::switchPage(int index, const QString& title)
{
    if (m_stack) {
        m_stack->setCurrentIndex(index);
    }
    if (m_headerTitle) {
        m_headerTitle->setText(title);
    }
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == index);
    }
}

UserProfile MainWindow::readProfile() const
{
    UserProfile profile;
    profile.name = m_nameEdit->text().trimmed();
    profile.weight = m_weightSpin->value();
    profile.height = m_heightSpin->value();
    profile.age = m_ageSpin->value();
    profile.gender = m_genderCombo->currentText();
    profile.activity = m_activityCombo->currentText();
    profile.goal = m_goalCombo->currentText();
    profile.level = m_levelCombo->currentText();
    profile.days = m_daysSpin->value();
    profile.equipment = m_equipmentCombo->currentText();
    profile.notes = m_notesEdit->toPlainText().trimmed();
    return profile;
}

void MainWindow::applyProfile(const UserProfile& profile)
{
    m_profile = profile;
    m_hasProfile = true;
    m_macroPlan = NutritionEngine::calculate(profile);
    m_trainingPlan = TrainingEngine::generate(profile);
    renderProfileSummary();
    renderNutrition();
    renderTraining();
    renderRecipes();
    renderCoachContext();
    m_status->setText(QString("Perfil activo: %1 | %2").arg(profile.name.isEmpty() ? "Usuario" : profile.name, profile.goal));
    if (m_auditStore) {
        m_auditStore->recordLocalEvent("profile.plan_generated", "info", QString("Objetivo: %1").arg(profile.goal));
    }
    if (m_dataManager && m_dataManager->authenticated()) {
        m_dataManager->registerEvent("profile.plan_generated", "info", QString("Objetivo: %1 | Dias: %2").arg(profile.goal).arg(profile.days));
    }
    switchPage(1, "Plan nutricional");
}

void MainWindow::renderProfileSummary()
{
    m_profileSummary->setHtml(QString("<h3>Perfil cargado</h3><p><b>%1</b> | %2 anios | %3 kg | %4 cm | %5</p><p>La app ya genero macros, dieta base y rutina segun objetivo, dias disponibles y equipamiento.</p>")
        .arg(htmlEscape(m_profile.name.isEmpty() ? "Usuario" : m_profile.name))
        .arg(m_profile.age)
        .arg(m_profile.weight, 0, 'f', 1)
        .arg(m_profile.height)
        .arg(htmlEscape(m_profile.goal)));
}

void MainWindow::renderNutrition()
{
    m_caloriesValue->setText(QString("%1 kcal").arg(m_macroPlan.calories));
    m_proteinValue->setText(QString("%1 g").arg(m_macroPlan.protein));
    m_carbsValue->setText(QString("%1 g").arg(m_macroPlan.carbs));
    m_fatsValue->setText(QString("%1 g").arg(m_macroPlan.fats));
    m_nutritionExplanation->setHtml(QString(R"HTML(
<h3>Por que esta dieta para %1</h3>
<p>La meta diaria sale de una estimacion del metabolismo basal y del nivel de actividad.
Para perdida de grasa se usa deficit moderado; para ganancia muscular se usa superavit moderado;
para mantenimiento se conserva el gasto estimado.</p>
<ul>
<li><b>Calorias:</b> %2 kcal diarias. TMB %3 | GET %4.</li>
<li><b>Proteinas:</b> %5 g para apoyar reparacion muscular y saciedad.</li>
<li><b>Carbohidratos:</b> %6 g para sostener energia de entrenamiento.</li>
<li><b>Grasas:</b> %7 g como base razonable para salud y adherencia.</li>
</ul>
<p><b>Aviso:</b> guia educativa. No reemplaza consulta profesional.</p>
)HTML")
        .arg(htmlEscape(m_profile.goal))
        .arg(m_macroPlan.calories)
        .arg(m_macroPlan.tmb)
        .arg(m_macroPlan.total)
        .arg(m_macroPlan.protein)
        .arg(m_macroPlan.carbs)
        .arg(m_macroPlan.fats));
}

void MainWindow::renderTraining()
{
    m_trainingTable->setRowCount(m_trainingPlan.size());
    for (int row = 0; row < m_trainingPlan.size(); ++row) {
        const auto& item = m_trainingPlan[row];
        const QStringList values = {item.day, item.exercise, item.sets, item.reps, item.rest, item.focus, item.alternative};
        for (int col = 0; col < values.size(); ++col) {
            m_trainingTable->setItem(row, col, new QTableWidgetItem(values.at(col)));
        }
    }
    if (!m_trainingPlan.isEmpty()) {
        m_trainingTable->selectRow(0);
        showTrainingReference(0);
    }
}

void MainWindow::renderRecipes()
{
    if (!m_hasProfile) {
        return;
    }
    m_recipesOutput->setHtml("<p>Carga ingredientes para sugerir recetas ajustadas al plan nutricional.</p>");
}

void MainWindow::renderProgress()
{
    const auto entries = m_progressStore.load();
    m_progressTable->setRowCount(entries.size());
    for (int row = 0; row < entries.size(); ++row) {
        const auto& entry = entries[row];
        const QStringList values = {
            entry.week,
            QString::number(entry.weight, 'f', 1),
            QString::number(entry.waist, 'f', 1),
            QString::number(entry.workouts),
            QString::number(entry.energy),
            entry.notes,
        };
        for (int col = 0; col < values.size(); ++col) {
            m_progressTable->setItem(row, col, new QTableWidgetItem(values.at(col)));
        }
    }
}

void MainWindow::renderCoachContext()
{
    m_aiStatus->setText(m_geminiClient->configured() ? "Gemini activo" : "Gemini sin clave: falta GEMINI_API_KEY");
}

void MainWindow::showTrainingReference(int row)
{
    if (row < 0 || row >= m_trainingPlan.size()) {
        return;
    }
    const auto& item = m_trainingPlan.at(row);
    m_pendingExerciseQuery = item.apiQuery;
    m_currentPlanItem = item;
    m_hasExerciseDbExercise = false;
    m_hasWgerExercise = false;
    m_exerciseDbError.clear();
    m_wgerError.clear();
    m_gifPreview->setMovie(nullptr);
    m_gifPreview->setText("Cargando referencia visual...");
    m_exerciseDetail->setHtml(QString("<h2>%1</h2><p><b>Dia:</b> %2<br><b>Series:</b> %3 | <b>Reps:</b> %4 | <b>Descanso:</b> %5<br><b>Foco:</b> %6<br><b>Alternativa:</b> %7</p><p>Buscando referencias en ExerciseDB y wger.</p>")
        .arg(htmlEscape(item.exercise), htmlEscape(item.day), htmlEscape(item.sets), htmlEscape(item.reps), htmlEscape(item.rest), htmlEscape(item.focus), htmlEscape(item.alternative)));
    m_exerciseClient->searchBestExercise(item.apiQuery);
    m_wgerClient->searchBestExercise(item.apiQuery);
}

void MainWindow::showExercise(const ApiExercise& exercise, const ExercisePlanItem& planItem)
{
    loadGif(exercise.gifUrl);
    const QString link = exercise.gifUrl.isEmpty() ? "" : QString("<p><a href='%1'>Abrir GIF en navegador</a></p>").arg(htmlEscape(exercise.gifUrl));
    QString html = QString(R"HTML(
<h2>%1</h2>
<p><b>En tu plan:</b> %2 | %3 series | %4 reps | descanso %5<br><b>Alternativa:</b> %6</p>
<p><b>Fuente:</b> %7<br><b>Musculos:</b> %8<br><b>Equipo:</b> %9</p>
<h3>Como realizarlo</h3>
%10
%11
<p><small>Usa la referencia visual para entender tecnica. Ajusta carga, rango y dificultad segun tu nivel.</small></p>
)HTML")
        .arg(htmlEscape(exercise.name))
        .arg(htmlEscape(planItem.day))
        .arg(htmlEscape(planItem.sets))
        .arg(htmlEscape(planItem.reps))
        .arg(htmlEscape(planItem.rest))
        .arg(htmlEscape(planItem.alternative))
        .arg(htmlEscape(exercise.source))
        .arg(htmlEscape(exercise.muscles))
        .arg(htmlEscape(exercise.equipment))
        .arg(instructionsHtml(exercise.description))
        .arg(link);
    m_exerciseDetail->setHtml(html);
}

void MainWindow::renderExerciseSources()
{
    if (!m_hasExerciseDbExercise && !m_hasWgerExercise) {
        return;
    }

    const ApiExercise visual = m_hasExerciseDbExercise ? m_exerciseDbExercise : m_wgerExercise;
    loadGif(visual.gifUrl);

    QString html = QString(R"HTML(
<h2>%1</h2>
<p><b>En tu plan:</b> %2 | %3 series | %4 reps | descanso %5<br><b>Alternativa:</b> %6</p>
<p><small>ExerciseDB se usa principalmente para GIF/referencia visual. wger se usa como fuente abierta adicional de descripcion, musculos, equipo e imagenes.</small></p>
)HTML")
        .arg(htmlEscape(m_currentPlanItem.exercise))
        .arg(htmlEscape(m_currentPlanItem.day))
        .arg(htmlEscape(m_currentPlanItem.sets))
        .arg(htmlEscape(m_currentPlanItem.reps))
        .arg(htmlEscape(m_currentPlanItem.rest))
        .arg(htmlEscape(m_currentPlanItem.alternative));

    if (m_hasExerciseDbExercise) {
        html += exerciseSourceHtml(m_exerciseDbExercise, "Referencia visual principal");
    }
    if (m_hasWgerExercise) {
        html += exerciseSourceHtml(m_wgerExercise, "Ficha complementaria wger");
    }
    if (!m_exerciseDbError.isEmpty() || !m_wgerError.isEmpty()) {
        html += "<h3>Estado de APIs</h3><ul>";
        if (!m_exerciseDbError.isEmpty()) {
            html += QString("<li>ExerciseDB: %1</li>").arg(htmlEscape(m_exerciseDbError));
        }
        if (!m_wgerError.isEmpty()) {
            html += QString("<li>wger: %1</li>").arg(htmlEscape(m_wgerError));
        }
        html += "</ul>";
    }
    html += "<p><small>Usa la referencia visual para entender tecnica. Ajusta carga, rango y dificultad segun tu nivel.</small></p>";
    m_exerciseDetail->setHtml(html);
}

QString MainWindow::exerciseSourceHtml(const ApiExercise& exercise, const QString& title) const
{
    const QString link = exercise.gifUrl.isEmpty() ? "" : QString("<p><a href='%1'>Abrir recurso visual</a></p>").arg(htmlEscape(exercise.gifUrl));
    return QString(R"HTML(
<h3>%1</h3>
<p><b>Ejercicio:</b> %2<br><b>Fuente:</b> %3<br><b>Musculos:</b> %4<br><b>Equipo:</b> %5</p>
%6
%7
)HTML")
        .arg(htmlEscape(title))
        .arg(htmlEscape(exercise.name))
        .arg(htmlEscape(exercise.source))
        .arg(htmlEscape(exercise.muscles))
        .arg(htmlEscape(exercise.equipment))
        .arg(instructionsHtml(exercise.description))
        .arg(link);
}

void MainWindow::showMissingExercise(const ExercisePlanItem& planItem, const QString& message)
{
    m_gifPreview->setText("Referencia visual no disponible");
    m_exerciseDetail->setHtml(QString("<h2>%1</h2><p>No encontre GIF confiable ahora. La rutina queda valida y se puede usar la alternativa.</p><p><b>Detalle:</b> %2</p>")
        .arg(htmlEscape(planItem.exercise), htmlEscape(message)));
}

void MainWindow::loadGif(const QString& url)
{
    if (m_gifMovie) {
        m_gifMovie->stop();
        m_gifMovie->deleteLater();
        m_gifMovie = nullptr;
    }
    if (m_gifBuffer) {
        m_gifBuffer->deleteLater();
        m_gifBuffer = nullptr;
    }
    if (!url.startsWith("http")) {
        m_gifPreview->setText("Este ejercicio no trae GIF");
        return;
    }
    m_gifPreview->setText("Cargando GIF...");
    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", "ConcaGymCpp/0.1");
    auto* reply = m_gifManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_gifPreview->setText("No se pudo cargar el GIF");
            return;
        }
        m_gifBuffer = new QBuffer(this);
        m_gifBuffer->setData(data);
        m_gifBuffer->open(QIODevice::ReadOnly);
        m_gifMovie = new QMovie(m_gifBuffer, QByteArray(), this);
        m_gifMovie->setScaledSize(QSize(350, 250));
        if (!m_gifMovie->isValid()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(data)) {
                m_gifPreview->setText("");
                m_gifPreview->setPixmap(pixmap.scaled(QSize(350, 250), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                return;
            }
            m_gifPreview->setText("Formato de imagen no disponible");
            return;
        }
        m_gifPreview->setText("");
        m_gifPreview->setMovie(m_gifMovie);
        m_gifMovie->start();
    });
}

QString MainWindow::instructionsHtml(const QString& description) const
{
    const auto lines = description.split("\n", Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        return "<p>Sin instrucciones disponibles.</p>";
    }
    QString html = "<ol>";
    for (const auto& line : lines) {
        html += "<li>" + htmlEscape(line.trimmed()) + "</li>";
    }
    html += "</ol>";
    return html;
}

void MainWindow::generateRecipes()
{
    if (!m_hasProfile) {
        QMessageBox::information(this, "Primero el perfil", "Carga el perfil para ajustar recetas a tu plan.");
        switchPage(0, "Perfil");
        return;
    }
    const auto ingredients = RecipeEngine::parseIngredients(m_ingredientsEdit->text());
    const auto suggestions = RecipeEngine::generate(ingredients, &m_macroPlan, m_profile.goal);
    if (suggestions.isEmpty()) {
        m_recipesOutput->setHtml("<p>No encontre una receta suficiente con esos ingredientes. Proba con pollo, arroz, huevo, avena, atun, papa o lentejas.</p>");
        return;
    }
    const auto targets = RecipeEngine::mealTargets(&m_macroPlan);
    QString html = QString("<h2>Recetas ajustadas a tu plan</h2><p><b>Objetivo por comida aprox:</b> %1 kcal | P %2 g | C %3 g | G %4 g</p>")
        .arg(targets[0]).arg(targets[1]).arg(targets[2]).arg(targets[3]);
    for (const auto& recipe : suggestions) {
        QString ingredientsHtml;
        for (const auto& ingredient : recipe.ingredients) {
            ingredientsHtml += QString("%1 g %2, ").arg(ingredient.grams).arg(htmlEscape(ingredient.name));
        }
        ingredientsHtml.chop(2);
        QString stepsHtml = "<ol>";
        for (const auto& step : recipe.steps) {
            stepsHtml += "<li>" + htmlEscape(step) + "</li>";
        }
        stepsHtml += "</ol>";
        html += QString("<h3>%1 - %2 (%3/100)</h3><p><b>Porcion sugerida:</b> %4<br><b>Macros aprox:</b> %5 kcal | P %6 g | C %7 g | G %8 g<br><b>Por que sirve:</b> %9</p>%10")
            .arg(htmlEscape(recipe.name), htmlEscape(recipe.fitLabel))
            .arg(recipe.score)
            .arg(ingredientsHtml)
            .arg(recipe.kcal)
            .arg(recipe.protein)
            .arg(recipe.carbs)
            .arg(recipe.fat)
            .arg(htmlEscape(recipe.reason), stepsHtml);
    }
    m_recipesOutput->setHtml(html);
}

void MainWindow::addProgressEntry()
{
    ProgressEntry entry;
    entry.week = m_weekEdit->text().trimmed();
    entry.weight = m_progressWeightSpin->value();
    entry.waist = m_waistSpin->value();
    entry.workouts = m_workoutsSpin->value();
    entry.energy = m_energySpin->value();
    entry.notes = m_progressNotesEdit->toPlainText().trimmed();
    if (!m_progressStore.append(entry)) {
        QMessageBox::warning(this, "Error", "No se pudo guardar el progreso.");
        return;
    }
    if (m_auditStore) {
        m_auditStore->recordLocalEvent("progress.saved", "info", "Semana: " + entry.week);
    }
    if (m_dataManager && m_dataManager->authenticated()) {
        m_dataManager->registerEvent("progress.saved", "info", QString("Semana %1 | peso %2 | entrenos %3").arg(entry.week).arg(entry.weight).arg(entry.workouts));
    }
    renderProgress();
}

void MainWindow::askCoach(const QString& question)
{
    QString text = question.trimmed();
    if (text.isEmpty()) {
        text = m_questionEdit->toPlainText().trimmed();
    }
    if (text.isEmpty()) {
        return;
    }
    appendChat("Vos", text);
    if (m_auditStore) {
        m_auditStore->recordLocalEvent("ai.question", "info", text.left(240));
    }
    if (m_dataManager && m_dataManager->authenticated()) {
        m_dataManager->registerEvent("ai.question", "info", text.left(500));
    }
    m_questionEdit->clear();
    m_askButton->setEnabled(false);
    m_aiStatus->setText(m_geminiClient->configured() ? "Gemini procesando..." : "Gemini sin clave");
    m_geminiClient->askCoach(m_hasProfile ? &m_profile : nullptr, m_hasProfile ? &m_macroPlan : nullptr, text);
}

void MainWindow::appendChat(const QString& role, const QString& text)
{
    const QString current = m_chatBrowser->toHtml();
    const QString background = role == "Vos" ? "#eaf1ff" : "#f6f8fb";
    const QString block = QString("<div style='background:%1; border:1px solid #d9e2ec; border-radius:8px; padding:10px; margin:8px 0;'><b>%2:</b><br>%3</div>")
        .arg(background, htmlEscape(role), htmlEscape(text).replace("\n", "<br>"));
    m_chatBrowser->setHtml(current + block);
}

QString MainWindow::htmlEscape(const QString& text) const
{
    QString value = text.toHtmlEscaped();
    return value;
}

void MainWindow::applyAppStyle()
{
    qApp->setStyleSheet(R"QSS(
QWidget {
    color: #172033;
    font-family: "Segoe UI", Arial, sans-serif;
    font-size: 14px;
}
QMainWindow, QWidget { background: #f3f6fa; }
#Sidebar { background: #101623; border-right: 1px solid #233049; }
#Brand { color: #ffffff; font-size: 24px; font-weight: 850; background: transparent; }
#SidebarSub { color: #9ca3af; font-size: 12px; background: transparent; }
#Header { background: #ffffff; border-bottom: 1px solid #d7e1ee; }
#HeaderTitle { font-size: 24px; font-weight: 850; color: #0b1730; background: transparent; }
#Muted, #HeroText { color: #475b7c; background: transparent; }
#NavButton {
    text-align: left; padding: 14px 16px; border: none; border-radius: 8px;
    color: #f8fafc; background: transparent; font-weight: 700;
}
#NavButton:checked, #NavButton:hover { background: #2f6bed; color: #ffffff; }
#Panel, #HeroPanel, #MetricCard {
    background: #ffffff; border: 1px solid #d7e1ee; border-radius: 8px;
}
#HeroTitle { font-size: 22px; font-weight: 850; color: #123267; background: transparent; }
#SectionLabel { font-size: 16px; font-weight: 800; color: #0b1730; background: transparent; }
#MetricTitle { color: #6b7b93; font-size: 12px; background: transparent; }
#MetricValue { font-size: 22px; font-weight: 850; color: #0b1730; background: transparent; }
#TextPanel {
    background: #ffffff; border: 1px solid #d7e1ee; border-radius: 8px;
    padding: 10px;
}
#DataTable {
    background: #ffffff; border: 1px solid #d7e1ee; gridline-color: #e4ebf4;
    selection-background-color: #dbe8ff; selection-color: #0b1730;
}
QHeaderView::section {
    background: #edf2f8; border: none; border-right: 1px solid #d7e1ee;
    padding: 8px; font-weight: 800;
}
QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background: #ffffff; border: 1px solid #c8d5e6; border-radius: 7px; padding: 8px;
}
QPushButton {
    border: none; border-radius: 7px; padding: 10px 14px; font-weight: 800;
    background: #e8eef7; color: #0b1730;
}
#PrimaryButton { background: #2f6bed; color: #ffffff; }
#SecondaryButton { background: #e8eef7; color: #0b1730; }
QPushButton:hover { background: #dbe5f2; }
#PrimaryButton:hover { background: #2459ce; }
#GifPreview {
    background: #f8fafc; border: 1px dashed #b8c7da; border-radius: 8px; color: #607089;
}
)QSS");
}
