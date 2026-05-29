#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVector>
#include <QDate>
#include "User.h"
#include "Learner.h"
#include "Book.h"
#include "Transaction.h"
#include "Payments.h"
#include "PaymentItem.h"

class DatabaseManager {
public:
    static DatabaseManager& instance();
    QSqlDatabase& getDatabase() { return m_database; }

    bool initialize(const QString& dbPath = "library_system.db");
    bool createTables();
    bool createPaymentTables();
    void closeDatabase();

// ==================== Audit Logging ====================
    bool logUserActivity(int userId, const QString& actionType, const QString& actionDetails);
    QVector<UserActivityLog> getUserActivityLog(int userId, int limit = 100);
    void logLogin(int userId);
    void logBorrowBook(int userId, int learnerId, const QString& bookCode, const QDate& boroowDate);
    void logReturnBook(int userId, int learnerId, const QString& bookCode,const QDate& returnDate);
    void logMarkBookLost(int userId, int learnerId, const QString& bookCode, double amount,const QDate& borrowDate);
    void logDeleteBook(int userId, const QString& bookCode, const QString& title);
    void logPaymentSummary(int userId, int learnerId, double amount);
    void logPaymentItem(int userId, int learnerId, const QString& bookCode, double amount);

// ==================== User Operations ====================
    bool addUser(const User& user);
    bool updateUser(const User& user);
    bool deleteUser(int userId);
    User getUserById(int userId);
    User getUserByEmail(const QString& email);
    QVector<User> getAllUsers();
    bool userExists(const QString& email);
    QDateTime getUserLastLogin(int userId);
    QDateTime getPasswordChangedDate(int userId);
    bool changeUserPassword(int userId, const QString& newPassword);

// ==================== Learners ====================
    bool addLearner(const Learner& learner);
    bool updateLearner(const Learner& learner);
    bool deleteLearner(int learnerId);
    Learner getLearnerById(int learnerId);
    QVector<Learner> getAllLearners();
    QVector<Learner> getLearnersByGrade(const QString& grade);
    QVector<Learner> searchLearners(const QString& searchTerm);
    int getLearnerCount();
    int getActiveLearnerCount();

// ==================== Books ====================
    bool addBook(const Book& book);
    bool updateBook(const Book& book);
    bool deleteBook(int userId,const QString& bookCode);
    Book getBookByCode(const QString& bookCode);
    QVector<Book> getAllBooks();
    QVector<Book> getBooksByGrade(const QString& grade);
    QVector<Book> getBooksBySubject(const QString& subject);
    QVector<Book> getBooksByStatus(Book::Status status);
    QVector<Book> searchBooks(const QString& searchTerm);
    bool bookCodeExists(const QString& bookCode);
    int getBookCountByISBN(const QString& isbn);
    int getTotalBookCount();
    int getAvailableBookCount();
    int getBorrowedBookCount();

// ==================== Transactions ====================
    bool addTransaction(const Transaction& transaction);
    bool updateTransaction(const Transaction& transaction);
    Transaction getTransactionByKey(int learnerId, const QString& bookCode, const QDate& borrowDate);
    QVector<Transaction> getTransactionsByLearnerId(int learnerId);
    QVector<Transaction> getActiveTransactionsByLearnerId(int learnerId);
    QVector<Transaction> getTransactionsByBookCode(const QString& bookCode);
    QVector<Transaction> getAllTransactions();
    QVector<Transaction> getActiveTransactions();
    QVector<Transaction> getOverdueTransactions();
    QVector<Transaction> getTransactionsByDateRange(const QDate& startDate, const QDate& endDate);

    // ========================================
    bool borrowBook(int userId, int learnerId, const QString& bookCode, const QDate& borrowDate);
    bool returnBook(int learnerId, const QString& bookCode, const QDate& borrowDate, const QDate& returnDate);
    bool markBookAsLost(int learnerId, const QString& bookCode, const QDate& borrowDate);
    bool hasOverdueBooks(int learnerId);
    double calculateUnreturnedBooksAmount(int learnerId);

// ============== Dashboard==========================
    struct DashboardStats {
        int totalBooks;
        int availableBooks;
        int borrowedBooks;
        int totalLearners;
        int activeLearners;
        int totalUsers;
        int overdueBooks;
    };
    DashboardStats getDashboardStats();


// ==================== Payments ====================
    bool processPayment(Payments& payment, const QVector<QPair<int, QString>>& transactionKeys);
    Payments getPaymentById(int id);
    QVector<Payments> getPaymentsByLearnerId(int learnerId);
    QVector<PaymentItem> getPaymentItems(int paymentId);
    QVector<Transaction> getUnpaidLostTransactionsByLearnerId(int learnerId);
    double getTotalOutstandingFees(int learnerId);

    QVector<Transaction> getRecentTransactions(int limit = 10);

    QString getLastError() const { return m_lastError; }

private:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_database;
    QString m_lastError;

    void setLastError(const QString& error);
    bool executeQuery(QSqlQuery& query);
};

#endif // DATABASEMANAGER_H