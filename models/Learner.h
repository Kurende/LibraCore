#ifndef LEARNER_H
#define LEARNER_H

#include <QString>
#include <QDate>
#include <QDateTime>

class Learner {
public:
    
    Learner();
    Learner(int id, const QString& name, const QString& surname,
            const QString& grade, const QDate& dateOfBirth,
            const QString& contactNo, const QDateTime& createdAt);

    int getId() const { return m_id; }
    QString getName() const { return m_name; }
    QString getSurname() const { return m_surname; }
    QString getFullName() const { return m_name + " " + m_surname; }
    QString getGrade() const { return m_grade; }
    QDate getDateOfBirth() const { return m_dateOfBirth; }
    QString getContactNo() const { return m_contactNo; }
    QDateTime getCreatedAt() const { return m_createdAt; }

    void setId(int id) { m_id = id; }
    void setName(const QString& name) { m_name = name; }
    void setSurname(const QString& surname) { m_surname = surname; }
    void setGrade(const QString& grade) { m_grade = grade; }
    void setDateOfBirth(const QDate& dob) { m_dateOfBirth = dob; }
    void setContactNo(const QString& contact) { m_contactNo = contact; }
    void setCreatedAt(const QDateTime& dateTime) { m_createdAt = dateTime; }

    QString getInitialSurname() const;

private:
    int m_id;
    QString m_name;
    QString m_surname;
    QString m_grade;
    QDate m_dateOfBirth;
    QString m_contactNo;
    QDateTime m_createdAt;
};

#endif // LEARNER_H