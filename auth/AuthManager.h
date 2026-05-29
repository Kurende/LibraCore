#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QString>
#include "User.h"

class AuthManager {
public:
    static AuthManager& instance();

    bool login(const QString& email, const QString& password);
    void logout();
    bool isLoggedIn() const { return m_isLoggedIn; }

    User getCurrentUser() const { return m_currentUser; }
    int getCurrentUserId() const { return m_currentUser.getId(); }
    User::Role getCurrentUserRole() const { return m_currentUser.getRole(); }

    bool hasAdminPermission() const;
    bool hasLibrarianPermission() const;
    bool hasFinancePermission() const;
    bool canManageUsers() const;
    bool canManageLearners() const;
    bool canManageBooks() const;
    bool canManageTransactions() const;

    bool registerUser(const User& user, const QString& password);

    bool verifyEmail(const QString& email);
    QString getSecurityQuestion(const QString& email);
    bool verifySecurityAnswer(const QString& email, const QString& answer);
    bool resetPassword(const QString& email, const QString& newPassword);

    bool changePassword(const QString& oldPassword, const QString& newPassword);

    QString validatePassword(const QString& password);
    QString validateEmail(const QString& email);

    QString getLastError() const { return m_lastError; }

private:
    AuthManager();
    ~AuthManager();
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    bool m_isLoggedIn;
    User m_currentUser;
    QString m_lastError;

    void setLastError(const QString& error);
};

#endif // AUTHMANAGER_H
