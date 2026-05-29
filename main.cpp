#include "MainWindow.h"
#include "DatabaseManager.h"
#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("LibraCore");
    QApplication::setOrganizationName("Seagotle Secondary");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setWindowIcon(QIcon(":/icons/icons/utils/logo1.svg"));

    QFile file(":/theme/themes/dark.qss");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "Failed to open stylesheet!";
    } else {
        QString style = QTextStream(&file).readAll();
        a.setStyleSheet(style);
    }

    if (!DatabaseManager::instance().initialize("library_system.db")) {
        QMessageBox::critical(nullptr, "Database Error",
                              "Failed to initialize database:\n" +
                              DatabaseManager::instance().getLastError());
        return 1;
    }

    MainWindow w;
    w.show();

    return a.exec();
}
