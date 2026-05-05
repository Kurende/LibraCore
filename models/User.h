#ifndef USER_H
#define USER_H
#include <QString>
#include <QDateTime>

class User {
public:
    enum class Role {
        Admin,
        Librarian,
        Finance
    };

    User();
    User(int id, const QString& passwordHash,
         const QString& name, const QString& surname, const QString& email,
         const QString& contactNo, const QString& schoolName, Role role,
         const QString& securityQuestion, const QString& securityAnswer,
         const QDateTime& createdAt);

    int getId() const { return m_id; }
    QString getPasswordHash() const { return m_passwordHash; }
    QString getName() const { return m_name; }
    QString getSurname() const { return m_surname; }
    QString getFullName() const { return m_name + " " + m_surname; }
    QString getEmail() const { return m_email; }
    QString getContactNo() const { return m_contactNo; }
    QString getSchoolName() const { return m_schoolName; }
    Role getRole() const { return m_role; }
    QString getRoleString() const;
    QString getSecurityQuestion() const { return m_securityQuestion; }
    QString getSecurityAnswer() const { return m_securityAnswer; }
    QDateTime getCreatedAt() const { return m_createdAt; }

    void setId(int id) { m_id = id; }
    void setPasswordHash(const QString& hash) { m_passwordHash = hash; }
    void setName(const QString& name) { m_name = name; }
    void setSurname(const QString& surname) { m_surname = surname; }
    void setEmail(const QString& email) { m_email = email; }
    void setContactNo(const QString& contact) { m_contactNo = contact; }
    void setSchoolName(const QString& school) { m_schoolName = school; }
    void setRole(Role role) { m_role = role; }
    void setSecurityQuestion(const QString& question) { m_securityQuestion = question; }
    void setSecurityAnswer(const QString& answer) { m_securityAnswer = answer; }
    void setCreatedAt(const QDateTime& dateTime) { m_createdAt = dateTime; }

    static Role stringToRole(const QString& roleStr);
    static QString roleToString(Role role);

private:
    int m_id;
    QString m_passwordHash;
    QString m_name;
    QString m_surname;
    QString m_email;
    QString m_contactNo;
    QString m_schoolName;
    Role m_role;
    QString m_securityQuestion;
    QString m_securityAnswer;
    QDateTime m_createdAt;
};

struct UserActivityLog {
    int id;
    int userId;
    QString actionType;
    QString actionDetails;
    QDateTime createdAt;
    UserActivityLog() : id(-1), userId(-1) {}
};

#endif // USER_H