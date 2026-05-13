#include "LoginDialog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

LoginDialog::LoginDialog(DataManager* dataManager, LocalAuditStore* auditStore, QWidget* parent)
    : QDialog(parent)
    , m_dataManager(dataManager)
    , m_auditStore(auditStore)
{
    setWindowTitle("Conca Gym - Login");
    setModal(true);
    resize(430, 280);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 20);
    root->setSpacing(12);

    auto* title = new QLabel("Ingreso a Conca Gym");
    title->setObjectName("LoginTitle");
    root->addWidget(title);

    auto* subtitle = new QLabel("Qt consulta FastAPI para validar usuario. SQLite local guarda intentos y ultimo login.");
    subtitle->setWordWrap(true);
    subtitle->setObjectName("LoginSubtitle");
    root->addWidget(subtitle);

    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText("Usuario");
    m_userEdit->setText("admin");
    root->addWidget(m_userEdit);

    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText("Password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    root->addWidget(m_passwordEdit);

    m_lastLoginLabel = new QLabel;
    const QString lastLogin = m_auditStore ? m_auditStore->lastSuccessfulLogin() : QString();
    m_lastLoginLabel->setText(lastLogin.isEmpty() ? "Sin login local exitoso registrado." : "Ultimo login local: " + lastLogin);
    m_lastLoginLabel->setObjectName("LoginSubtitle");
    root->addWidget(m_lastLoginLabel);

    m_statusLabel = new QLabel("Servidor: " + (m_dataManager ? m_dataManager->baseUrl() : QString("sin configurar")));
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    auto* buttons = new QHBoxLayout;
    auto* offlineButton = new QPushButton("Continuar offline");
    offlineButton->setObjectName("SecondaryButton");
    m_loginButton = new QPushButton("Ingresar");
    m_loginButton->setObjectName("PrimaryButton");
    buttons->addWidget(offlineButton);
    buttons->addStretch(1);
    buttons->addWidget(m_loginButton);
    root->addLayout(buttons);

    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::attemptLogin);
    connect(offlineButton, &QPushButton::clicked, this, &LoginDialog::continueOffline);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::attemptLogin);

    if (m_dataManager) {
        connect(m_dataManager, &DataManager::loginFinished, this, [this](bool ok, const QString& message, const QString& username, const QString& role) {
            m_loginButton->setEnabled(true);
            const QString attemptedUser = m_userEdit->text().trimmed().toLower();
            if (m_auditStore) {
                m_auditStore->recordLoginAttempt(attemptedUser, ok, message);
            }
            if (!ok) {
                m_statusLabel->setText("Login fallido: " + message);
                return;
            }
            m_username = username;
            if (m_auditStore) {
                m_auditStore->setLastSuccessfulLogin(username);
                m_auditStore->recordLocalEvent("auth.login_success", "info", "Rol: " + role);
            }
            accept();
        });
    }

    setStyleSheet(qApp->styleSheet() + R"QSS(
#LoginTitle { font-size: 22px; font-weight: 850; color: #123267; }
#LoginSubtitle { color: #475b7c; }
)QSS");
}

void LoginDialog::attemptLogin()
{
    const QString username = m_userEdit->text().trimmed().toLower();
    const QString password = m_passwordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("Completa usuario y password.");
        return;
    }
    if (!m_dataManager) {
        m_statusLabel->setText("DataManager no esta disponible.");
        return;
    }
    m_loginButton->setEnabled(false);
    m_statusLabel->setText("Consultando FastAPI...");
    m_dataManager->login(username, password);
}

void LoginDialog::continueOffline()
{
    m_username = m_userEdit->text().trimmed();
    if (m_username.isEmpty()) {
        m_username = "offline";
    }
    if (m_auditStore) {
        m_auditStore->recordLocalEvent("auth.offline_mode", "warning", "El usuario entro sin FastAPI.");
    }
    accept();
}
