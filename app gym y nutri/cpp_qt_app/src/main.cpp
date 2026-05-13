#include "MainWindow.h"
#include "DataManager.h"
#include "LocalAuditStore.h"
#include "LoginDialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QStringList>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    LocalAuditStore auditStore;
    auditStore.initialize();

    DataManager dataManager;
    if (QCoreApplication::arguments().contains("--smoke-test")) {
        return 0;
    }

    LoginDialog login(&dataManager, &auditStore);
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow window(&dataManager, &auditStore, login.username());
    window.show();
    return app.exec();
}
