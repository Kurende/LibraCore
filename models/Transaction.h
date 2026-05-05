#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <QString>
#include <QDate>
#include <QDateTime>

class Transaction {
public:
    enum class Status {
        Active,
        Returned,
        Lost,
        Paid
    };

    Transaction();
    Transaction(int learnerId, const QString& bookCode,
                const QDate& borrowDate, const QDate& dueDate,
                const QDate& returnDate, Status status,
                const QDateTime& createdAt);

    int getLearnerId() const { return m_learnerId; }
    QString getBookCode() const { return m_bookCode; }
    QDate getBorrowDate() const { return m_borrowDate; }
    QDate getDueDate() const { return m_dueDate; }
    QDate getReturnDate() const { return m_returnDate; }
    Status getStatus() const { return m_status; }
    QString getStatusString() const;
    QDateTime getCreatedAt() const { return m_createdAt; }

    void setLearnerId(int learnerId) { m_learnerId = learnerId; }
    void setBookCode(const QString& bookCode) { m_bookCode = bookCode; }
    void setBorrowDate(const QDate& date) { m_borrowDate = date; }
    void setDueDate(const QDate& date) { m_dueDate = date; }
    void setReturnDate(const QDate& date) { m_returnDate = date; }
    void setStatus(Status status) { m_status = status; }
    void setCreatedAt(const QDateTime& dateTime) { m_createdAt = dateTime; }

    bool isOverdue() const;
    int getDaysOverdue() const;
    bool isActive() const { return m_status == Status::Active; }
    bool isReturned() const { return m_status == Status::Returned; }
    bool isLost() const { return m_status == Status::Lost; }

    static Status stringToStatus(const QString& statusStr);
    static QString statusToString(Status status);
    static QDate calculateDueDate(const QDate& borrowDate);

private:
    int m_learnerId;
    QString m_bookCode;
    QDate m_borrowDate;
    QDate m_dueDate;
    QDate m_returnDate;
    Status m_status;
    QDateTime m_createdAt;
};

#endif // TRANSACTION_H