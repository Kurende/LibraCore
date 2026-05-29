#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QStackedWidget>
#include <QCompleter>
#include <QStringListModel>
#include "Learner.h"
#include "Book.h"
#include "Transaction.h"
#include "Payments.h"
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>
#include <QPropertyAnimation>
#include "User.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void applyGlow(QWidget *g);
    void glowEffect();
    void statisticsCardGlowEffect(QWidget *g);
    QString getLogoAsBase64();

private slots:
    void on_pushButton_login_clicked();
    void on_pushButton_register_clicked();
    void on_pushButton_quit_clicked();
    void on_pushButton_forgotPassword_clicked();
    void on_pushButton_regBackToLogin_clicked();
    void on_pushButton_registerSubmit_clicked();

    void on_pushButton_resetVerifyEmail_clicked();
    void on_pushButton_resetVerifyAnswer_clicked();
    void on_pushButton_resetSubmit_clicked();
    void on_pushButton_resetCancel_clicked();

    void on_pushButton_homeIconSection_clicked();
    void on_pushButton_settingsIconSection_clicked();
    void on_pushButton_usersIconSection_clicked();
    void on_pushButton_booksSidebar_clicked();
    void on_pushButton_learnersSidebar_clicked();
    void on_pushButton_transactSidebar_clicked();
    void on_pushButton_reportsSidebar_clicked();
    void on_pushButton_settingsSidebar_clicked();
    void on_pushButton_goToAdminLobby_clicked();
    void on_pushButton_AddUser_clicked();
    void on_pushButton_adminUserSearch_clicked();
    void on_pushButton_BackToLearnersList_4_clicked();
    void on_pushButton_BackToLearnersList_5_clicked();
    void on_pushButton_BackToLearnersList_6_clicked();
    void on_pushButton_BackToLearnersList_7_clicked();
    void on_pushButton_adminRegisterSubmit_clicked();
    void on_pushButton_adminRegBackToLogin_clicked();
    void on_tableWidget_users_cellClicked(int row, int column);
    void on_pushButton_logoutTopBar_clicked();
    void on_pushButton_profileTopBar_clicked();
    void on_pushButton_toogleMenu_clicked();
    void on_pushButton_makePaymentSidebar_clicked();



    void on_pushButton_booksIconSection_clicked();
    void on_pushButton_learnersIconSection_clicked();
    void on_pushButton_transactionIconSection_clicked();
    void on_pushButton_reportsIconSection_clicked();
    void on_pushButton_usersSidebar_clicked();

    void on_pushButton_addBookQuickButton_clicked();
    void on_pushButton_issueBookQuickButton_clicked();
    void on_pushButton_returnBookQuickButton_clicked();
    void setupLibraryChart();
    void displayUserTips();

    void on_pushButton_addBookSidebar_clicked();
    void on_pushButton_updateBookInfoSidebar_clicked();
    void on_pushButton_confirmAdd_clicked();
    void on_pushButton_clearForm_clicked();
    void on_pushButton_copyFromISBN_clicked();
    void on_pushButton_refreshBooks_clicked();
    void on_pushButton_bookDetails_clicked();
    void on_pushButton_markAsLost_clicked();
    void on_lineEdit_searchBooks_textChanged(const QString &text);
    void on_comboBox_sortBooks_currentIndexChanged(int index);
    void on_tableWidget_books_cellClicked(int row, int column);

    void on_pushButton_saveChanges_clicked();
    void on_pushButton_cancelEdit_clicked();
    void on_pushButton_deleteBook_clicked();

    void on_pushButton_editUserProfile_clicked();
    void on_pushButton_confirmEditProfile_clicked();
    void on_pushButton_cancelEditProfile_clicked();
    void on_pushButton_changeUserPassword_clicked();
    void on_pushButton_confirmChangeUserPassword_clicked();
    void on_pushButton_cancelBackToUserProfile_clicked();

    void on_pushButton_addLearnerSidebar_clicked();
    void on_pushButton_AddLearner_clicked();
    void on_pushButton_addLearnerClearForm_clicked();
    void on_pushButton_refreshLearnerList_clicked();
    void on_pushButton_viewLeanerProfile_clicked();
    void on_pushButton_viewLearnerHistory_clicked();
    void on_lineEdit_searchLearner_textChanged(const QString &text);
    void on_comboBox_filterLearnerGrade_currentIndexChanged(int index);
    void on_tableWidget_viewLearnersList_cellClicked(int row, int column);

    void on_pushButton_backToLearners_clicked();
    void on_pushButton_editLearnerProfile_clicked();
    void on_pushButton_viewTransactionRecord_clicked();
    void on_pushButton_editLearnerConfirm_clicked();
    void on_pushButton_editLearnerCancel_clicked();

    void on_pushButton_borrowBookSidebar_clicked();
    void on_pushButton_returnBookSidebar_clicked();
    void on_pushButton_viewTransactionsSidebar_clicked();

    void on_pushButton_findLearner_clicked();
    void on_pushButton_findBook_clicked();
    void on_pushButton_confirmBorrow_clicked();
    void on_pushButton_clearBorrowForm_clicked();
    void on_pushButton_goToReportsFromBorrow_clicked();

    void on_pushButton_searchReturn_clicked();
    void on_pushButton_processReturn_clicked();
    void on_pushButton_clearReturnForm_clicked();
    void on_pushButton_markBookAsLost_clicked();
    void on_pushButton_goToReportsFromReturn_clicked();
    void on_radioButton_searchByLearner_clicked();
    void on_radioButton_searchByBook_clicked();
    void on_tableWidget_returnBooks_cellClicked(int row, int column);

    void on_pushButton_BackToLearnersList_clicked();
    void on_pushButton_backToProfile_clicked();
    void on_pushButton_filterHistory_clicked();
    void on_pushButton_goToReports_clicked();

    void on_pushButton_printReportSidebar_clicked();
    void on_pushButton_loadLearnerData_clicked();
    void on_pushButton_generateReport_clicked();
    void on_pushButton_printReport_clicked();
    void on_pushButton_saveReportPDF_clicked();
    void on_pushButton_clearPreview_clicked();
    void on_radioButton_borrowReport_clicked();
    void on_radioButton_returnReport_clicked();

    void on_pushButton_findLearnerPayment_clicked();
    void on_pushButton_selectAllBooks_clicked();
    void on_pushButton_deselectAllBooks_clicked();
    void on_pushButton_processPayment_clicked();
    void on_pushButton_viewReceipt_clicked();
    void on_pushButton_clearPayment_clicked();
    void on_tableWidget_lostBooks_itemSelectionChanged();
    void on_lineEdit_quickSearch_textChanged(const QString &text);
    void onQuickSearchResultActivated(const QString &result);
    void on_pushButton_applyTheme_clicked();

private:
    Ui::MainWindow *ui;

    QString m_selectedBookCode;
    int m_selectedLearnerId;

    struct TransactionKey {
        int learnerId       = -1;
        QString bookCode;
        QDate borrowDate;
        bool isValid() const { return learnerId != -1 && !bookCode.isEmpty() && borrowDate.isValid(); }
    };
    TransactionKey m_selectedTransactionKey;

    QString m_currentEmail;
    int m_selectedAdminUserId;
    bool m_isAdminEditingUser;
    bool m_isAdminChangingUserPassword;

    QChartView *m_chartView;
    QCompleter *m_quickSearchCompleter;
    QStringListModel *m_quickSearchModel;
    bool m_isQuickSearchNavigating;

    void collapseAllSideTextOptions();
    void expandSection(QWidget *frameOptions, bool collapseOthers);
    bool m_sidebarExpanded = false;

    void initializeUI();
    void setupConnections();
    void loadDashboardData();
    void applyPermissions();

    void navigateToPage(QWidget* targetPage);
    void showLoginPage();
    void showRegisterPage();
    void showResetPasswordPage();
    void showHomePage();
    void showDashboardPage();
    void showBooksPage();
    void showAddBookPage();
    void showUpdateBookPage();
    void showLearnersPage();
    void showAddLearnerPage();
    void showLearnerProfilePage();
    void showUpdateLearnerPage();
    void showBorrowBookPage();
    void showReturnBookPage();
    void showTransactionHistoryPage();
    void showReportsPage();
    void showPaymentsPage();
    void showUsersPage();
    void showEditUserInfo();
    void showChangeUserPasswordPage();
    void showAdminLobby();
    void showAddUser();
    void showSettingsPage();

    void loadAllBooks();
    void loadAllLearners();
    void loadRecentTransactions();
    void loadLearnerProfile(int learnerId);
    void loadBookDetails(const QString& bookCode);
    void loadTransactionHistory(int learnerId);
    void loadActiveTransactionsForReturn(int learnerId);
    void loadTransactionByBookCode(const QString& bookCode);

    void populateBooksTable(const QVector<Book>& books);
    void populateLearnersTable(const QVector<Learner>& learners);
    void populateTransactionsTable(const QVector<Transaction>& transactions);
    void populateReturnBooksTable(const QVector<Transaction>& transactions);
    void populateDashboardTransactions(const QVector<Transaction>& transactions);
    void populateCurrentlyBorrowedBooks(int learnerId);

    void clearBookForm();
    void clearLearnerForm();
    void clearBorrowForm();
    void clearReturnForm();
    void clearRegistrationForm();

    void fillBookForm(const Book& book);
    void fillLearnerForm(const Learner& learner);
    void copyBookDataFromISBN();

    bool validateBookForm();
    bool validateBookDetails(const QString& isbn, const QString& title, const QString& author);
    bool validateLearnerForm();
    bool validateLearnerDetails(const QString& name,
                                const QString& surname,
                                const QString& contactNo,
                                const QDate& dateOfBirth);
    bool validateBorrowForm();
    bool validateRegistrationForm();
    bool validatePasswordResetForm();

    void performBorrow();
    void performReturn();
    void performMarkAsLost();
    void calculateAndDisplayAmount(int learnerId);

    void updateDashboardStats();
    void updateUserInfo();
    void loadAdminUsers(const QString& searchTerm = QString());
    void populateAdminUsersTable(const QVector<User>& users, int totalUserCount);
    User getSelectedAdminUser() const;
    void clearAdminAddUserForm();
    void toggleSidebar();
    void showSuccessMessage(const QString& message);
    void showErrorMessage(const QString& message);
    void showInfoMessage(const QString& message);

    void loadUserProfile();
    void updateProfileIcon();
    void updateSecurityInfo();
    void initializeProfilePage();

    void generateBorrowReport(int learnerId);
    void generateReturnReport(int learnerId);
    QString generateReportHTML(int learnerId, const QString& reportType);
    void printReport();
    void saveReportAsPDF();

    int m_currentPaymentId;
    QVector<QPair<int, QString>> m_selectedPaymentKeys;

    void searchBooks(const QString& searchTerm);
    void filterBooksByGrade(const QString& grade);
    void searchLearners(const QString& searchTerm);
    void filterLearnersByGrade(const QString& grade);

    QString getGradeText(int index);
    QString getSubjectText(int index);
    void setupComboBoxes();
    void setupTableHeaders();
    void loadLostBooksForPayment(int learnerId);
    void updatePaymentSummary();
    QString generateReceiptHTML(const Payments& payment);
    void updateQuickSearchResults(const QString& searchTerm);
    void applyTheme(const QString& resourcePath);
    void applySelectedTheme();
    void loadSavedTheme();
    void clearChangePasswordForm();
};

#endif // MAINWINDOW_H
