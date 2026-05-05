#include "Book.h"

Book::Book()
    : m_price(0.0), m_status(Status::Available) {
}

Book::Book(const QString& bookCode, const QString& isbn,
           const QString& title, const QString& author, const QString& subject,
           const QString& grade, double price, Status status,
           const QDateTime& createdAt)
    : m_bookCode(bookCode), m_isbn(isbn), m_title(title),
      m_author(author), m_subject(subject), m_grade(grade), m_price(price),
      m_status(status), m_createdAt(createdAt) {
}

QString Book::getStatusString() const {
    return statusToString(m_status);
}

Book::Status Book::stringToStatus(const QString& statusStr) {
    QString lower = statusStr.toLower();
    if (lower == "borrowed") return Status::Borrowed;
    if (lower == "lost") return Status::Lost;
    return Status::Available;
}

QString Book::statusToString(Status status) {
    switch (status) {
        case Status::Available: return "Available";
        case Status::Borrowed: return "Borrowed";
        case Status::Lost: return "Lost";
        default: return "Available";
    }
}