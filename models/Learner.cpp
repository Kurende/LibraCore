#include "Learner.h"

Learner::Learner()
    : m_id(-1) {
}

Learner::Learner(int id, const QString& name, const QString& surname,
                 const QString& grade, const QDate& dateOfBirth,
                 const QString& contactNo, const QDateTime& createdAt)
    : m_id(id), m_name(name), m_surname(surname), m_grade(grade),
      m_dateOfBirth(dateOfBirth), m_contactNo(contactNo),
      m_createdAt(createdAt) {
}

QString Learner::getInitialSurname() const {
    if (m_name.isEmpty() || m_surname.isEmpty()) {
        return "";
    }
    return QString("%1. %2").arg(m_name[0].toUpper()).arg(m_surname);
}