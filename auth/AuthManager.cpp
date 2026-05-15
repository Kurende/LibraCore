#include "AuthManager.h"
#include "DatabaseManager.h"
#include "Encryption.h"
#include <QRegularExpression>

AuthManager& AuthManager::instance() {
    static AuthManager instance;
    return instance;
}

AuthManager::AuthManager() : m_isLoggedIn(false) {
}

AuthManager::~AuthManager() {
}

bool AuthManager::login(const QString& email, const QString& password) {
    if (email.isEmpty() || password.isEmpty()) {
        setLastError("Email and password cannot be empty");
        return false;
    }

    User user = DatabaseManager::instance().getUserByEmail(email);

    if (user.getId() == -1) {
        setLastError("Invalid email or password");
        return false;
    }

    if (!Encryption::verifyPassword(password, user.getPasswordHash())) {
        setLastError("Invalid email or password");
        return false;
    }

    m_currentUser = user;
    m_isLoggedIn = true;
    m_lastError.clear();

    DatabaseManager::instance().getUserLastLogin(m_currentUser.getId());

    DatabaseManager::instance().logUserActivity(
        m_currentUser.getId(),
        "Login",
        "User logged in successfully"
    );

    return true;
}

void AuthManager::logout() {
    m_isLoggedIn = false;
    m_currentUser = User();
    m_lastError.clear();
}

bool AuthManager::hasAdminPermission() const {
    return m_isLoggedIn && m_currentUser.getRole() == User::Role::Admin;
}

bool AuthManager::hasLibrarianPermission() const {
    if (!m_isLoggedIn) return false;
    User::Role role = m_currentUser.getRole();
    return role == User::Role::Admin || role == User::Role::Librarian;
}

bool AuthManager::hasFinancePermission() const {
    if (!m_isLoggedIn) return false;
    User::Role role = m_currentUser.getRole();
    return role == User::Role::Admin || role == User::Role::Finance;
}

bool AuthManager::canManageUsers() const {
    return hasAdminPermission();
}

bool AuthManager::canManageLearners() const {
    return hasLibrarianPermission();
}

bool AuthManager::canManageBooks() const {
    return hasLibrarianPermission();
}

bool AuthManager::canManageTransactions() const {
    return hasLibrarianPermission();
}

bool AuthManager::registerUser(const User& user, const QString& password) {
    auto passwordResult = validatePassword(password);
    if (!passwordResult.isValid) {
        setLastError(passwordResult.message);
        return false;
    }

    auto emailResult = validateEmail(user.getEmail());
    if (!emailResult.isValid) {
        setLastError(emailResult.message);
        return false;
    }

    if (DatabaseManager::instance().userExists(user.getEmail())) {
        setLastError("Email already registered");
        return false;
    }

    User newUser = user;
    newUser.setPasswordHash(Encryption::hashPassword(password));
    newUser.setSecurityAnswer(Encryption::encryptText(user.getSecurityAnswer()));

    if (!DatabaseManager::instance().addUser(newUser)) {
        setLastError(DatabaseManager::instance().getLastError());
        return false;
    }

    m_lastError.clear();
    return true;
}

bool AuthManager::verifyEmail(const QString& email) {
    User user = DatabaseManager::instance().getUserByEmail(email);
    if (user.getId() == -1) {
        setLastError("Email not found");
        return false;
    }

    m_lastError.clear();
    return true;
}

QString AuthManager::getSecurityQuestion(const QString& email) {
    User user = DatabaseManager::instance().getUserByEmail(email);
    if (user.getId() == -1) {
        setLastError("Email not found");
        return "";
    }

    return user.getSecurityQuestion();
}

bool AuthManager::verifySecurityAnswer(const QString& email, const QString& answer) {
    User user = DatabaseManager::instance().getUserByEmail(email);
    if (user.getId() == -1) {
        setLastError("Email not found");
        return false;
    }

    QString decryptedAnswer = Encryption::decryptText(user.getSecurityAnswer());

    if (decryptedAnswer.toLower() != answer.toLower()) {
        setLastError("Incorrect security answer");
        return false;
    }

    m_lastError.clear();
    return true;
}

bool AuthManager::resetPassword(const QString& email, const QString& newPassword) {
    auto passwordResult = validatePassword(newPassword);
    if (!passwordResult.isValid) {
        setLastError(passwordResult.message);
        return false;
    }

    User user = DatabaseManager::instance().getUserByEmail(email);
    if (user.getId() == -1) {
        setLastError("Email not found");
        return false;
    }

    user.setPasswordHash(Encryption::hashPassword(newPassword));

    if (!DatabaseManager::instance().updateUser(user)) {
        setLastError(DatabaseManager::instance().getLastError());
        return false;
    }

    m_lastError.clear();
    return true;
}

bool AuthManager::changePassword(const QString& oldPassword, const QString& newPassword) {
    if (!m_isLoggedIn) {
        setLastError("No user logged in");
        return false;
    }

    if (!Encryption::verifyPassword(oldPassword, m_currentUser.getPasswordHash())) {
        setLastError("Current password is incorrect");
        return false;
    }

    auto passwordResult = validatePassword(newPassword);
    if (!passwordResult.isValid) {
        setLastError(passwordResult.message);
        return false;
    }

    m_currentUser.setPasswordHash(Encryption::hashPassword(newPassword));

    if (!DatabaseManager::instance().updateUser(m_currentUser)) {
        setLastError(DatabaseManager::instance().getLastError());
        return false;
    }

    m_lastError.clear();
    return true;
}

AuthManager::ValidationResult AuthManager::validatePassword(const QString& password) {
    ValidationResult result;

    if (password.isEmpty()) {
        result.isValid = false;
        result.message = "Password cannot be empty";
        return result;
    }

    if (password.length() < 6) {
        result.isValid = false;
        result.message = "Password must be at least 6 characters long";
        return result;
    }

    if (password.length() > 50) {
        result.isValid = false;
        result.message = "Password must be less than 50 characters";
        return result;
    }

    result.isValid = true;
    result.message = "Password is valid";
    return result;
}

AuthManager::ValidationResult AuthManager::validateEmail(const QString& email) {
    ValidationResult result;

    if (email.isEmpty()) {
        result.isValid = false;
        result.message = "Email cannot be empty";
        return result;
    }

    QRegularExpression regex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!regex.match(email).hasMatch()) {
        result.isValid = false;
        result.message = "Invalid email format";
        return result;
    }

    result.isValid = true;
    result.message = "Email is valid";
    return result;
}

void AuthManager::setLastError(const QString& error) {
    m_lastError = error;
}