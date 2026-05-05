#include "User.h"

User::User()
    : m_id(-1), m_role(Role::Librarian) {
}

User::User(int id, const QString& passwordHash,
           const QString& name, const QString& surname, const QString& email,
           const QString& contactNo, const QString& schoolName, Role role,
           const QString& securityQuestion, const QString& securityAnswer,
           const QDateTime& createdAt)
    : m_id(id), m_passwordHash(passwordHash), m_name(name), m_surname(surname),
      m_email(email), m_contactNo(contactNo), m_schoolName(schoolName), m_role(role),
      m_securityQuestion(securityQuestion), m_securityAnswer(securityAnswer),
      m_createdAt(createdAt) {
}

QString User::getRoleString() const {
    return roleToString(m_role);
}

User::Role User::stringToRole(const QString& roleStr) {
    QString lower = roleStr.toLower();
    if (lower == "admin") return Role::Admin;
    if (lower == "finance") return Role::Finance;
    return Role::Librarian;
}

QString User::roleToString(Role role) {
    switch (role) {
        case Role::Admin: return "Admin";
        case Role::Librarian: return "Librarian";
        case Role::Finance: return "Finance";
        default: return "Librarian";
    }
}