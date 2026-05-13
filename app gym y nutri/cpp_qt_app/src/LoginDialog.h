#pragma once

#include "DataManager.h"
#include "LocalAuditStore.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    LoginDialog(DataManager* dataManager, LocalAuditStore* auditStore, QWidget* parent = nullptr);

    QString username() const { return m_username; }

private:
    DataManager* m_dataManager = nullptr;
    LocalAuditStore* m_auditStore = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_lastLoginLabel = nullptr;
    QPushButton* m_loginButton = nullptr;
    QString m_username;

    void attemptLogin();
    void continueOffline();
};
