#ifndef BOOK_H
#define BOOK_H
#include <QString>
#include <QDateTime>

class Book {
public:
    enum class Status {
        Available,
        Borrowed,
        Lost
    };

    Book();
    Book(const QString& bookCode, const QString& isbn,
         const QString& title, const QString& author, const QString& subject,
         const QString& grade, double price, Status status,
         const QDateTime& createdAt);

    QString getBookCode() const { return m_bookCode; }
    QString getIsbn() const { return m_isbn; }
    QString getTitle() const { return m_title; }
    QString getAuthor() const { return m_author; }
    QString getSubject() const { return m_subject; }
    QString getGrade() const { return m_grade; }
    double getPrice() const { return m_price; }
    Status getStatus() const { return m_status; }
    QString getStatusString() const;
    QDateTime getCreatedAt() const { return m_createdAt; }

    void setBookCode(const QString& code) { m_bookCode = code; }
    void setIsbn(const QString& isbn) { m_isbn = isbn; }
    void setTitle(const QString& title) { m_title = title; }
    void setAuthor(const QString& author) { m_author = author; }
    void setSubject(const QString& subject) { m_subject = subject; }
    void setGrade(const QString& grade) { m_grade = grade; }
    void setPrice(double price) { m_price = price; }
    void setStatus(Status status) { m_status = status; }
    void setCreatedAt(const QDateTime& dateTime) { m_createdAt = dateTime; }

    bool isAvailable() const { return m_status == Status::Available; }
    bool isBorrowed() const { return m_status == Status::Borrowed; }
    bool isLost() const { return m_status == Status::Lost; }

    static Status stringToStatus(const QString& statusStr);
    static QString statusToString(Status status);

private:
    QString m_bookCode;
    QString m_isbn;
    QString m_title;
    QString m_author;
    QString m_subject;
    QString m_grade;
    double m_price;
    Status m_status;
    QDateTime m_createdAt;
};

#endif // BOOK_H