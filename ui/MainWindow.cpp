#include "MainWindow.h"
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QHeaderView>
#include <QLegendMarker>
#include <QLabel>
#include <QMessageBox>
#include <QPieSlice>
#include <QPrintDialog>
#include <QPrinter>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QSettings>
#include <QSignalBlocker>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringListModel>
#include <QTextDocument>
#include <QtCharts/QPieLegendMarker>
#include "AuthManager.h"
#include "DatabaseManager.h"
#include "ui/ui_MainWindow.h"
#include <User.h>

namespace {
void configureResizableTable(QTableWidget *table)
{
    QHeaderView *header = table->horizontalHeader();
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setMinimumSectionSize(70);
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(false);

    table->setWordWrap(false);
    table->setTextElideMode(Qt::ElideRight);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void justifyTableContents(QTableWidget *table, const QList<int> &leftAlignedColumns)
{
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int column = 0; column < table->columnCount(); ++column) {
            QTableWidgetItem *item = table->item(row, column);
            if (!item)
                continue;

            item->setTextAlignment(leftAlignedColumns.contains(column)
                                       ? Qt::AlignLeft | Qt::AlignVCenter
                                       : Qt::AlignCenter);
        }
    }

    table->resizeColumnsToContents();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
}

bool isPersonNameValid(const QString &name)
{
    static const QRegularExpression nameRegex("^[A-Za-z]+(?: [A-Za-z]+)*$");
    return nameRegex.match(name).hasMatch();
}

bool isContactNumberValid(const QString &contactNo)
{
    static const QRegularExpression contactRegex("^0\\d{9}$");
    return contactRegex.match(contactNo).hasMatch();
}

QString passwordRequirementLine(bool isMet, const QString &text)
{
    const QString color = isMet ? "#1f8f45" : "#c62828";
    return QString("<span style=\"color:%1;\">%2 %3</span>")
        .arg(color, isMet ? "&#10003;" : "&#10007;", text.toHtmlEscaped());
}

void updatePasswordRequirements(QLabel *label, const QString &password)
{
    const bool hasLength = password.length() >= 8;
    static const QRegularExpression specialRegex("[^A-Za-z0-9]");
    static const QRegularExpression uppercaseRegex("[A-Z]");
    static const QRegularExpression lowercaseRegex("[a-z]");

    label->setText("<b>Password Must Contain:</b><br>" +
                   QStringList{
                       passwordRequirementLine(hasLength, "At least 8 characters"),
                       passwordRequirementLine(specialRegex.match(password).hasMatch(), "A special character"),
                       passwordRequirementLine(uppercaseRegex.match(password).hasMatch(), "An uppercase letter"),
                       passwordRequirementLine(lowercaseRegex.match(password).hasMatch(), "A lowercase letter")
                   }.join("<br>"));
}

bool isIsbnValid(const QString &isbn)
{
    static const QRegularExpression isbnRegex(
        "^(?:(?:97[89](?:[- ]?\\d){10})|(?:\\d(?:[- ]?\\d){8}[- ]?[\\dX]))$");
    return isbnRegex.match(isbn.trimmed().toUpper()).hasMatch();
}

QString cleanIsbnInput(const QString &text)
{
    QString cleaned;
    for (const QChar &ch : text.trimmed().toUpper()) {
        if (ch.isDigit()) {
            cleaned.append(ch);
        } else if (ch == 'X' && cleaned.length() >= 9) {
            cleaned.append(ch);
            break;
        }

        if ((cleaned.contains('X') && cleaned.length() == 10) || cleaned.length() == 13) {
            break;
        }
    }
    return cleaned;
}

QString formatIsbn(const QString &text)
{
    const QString cleaned = cleanIsbnInput(text);
    const QList<int> groups = cleaned.startsWith("978") || cleaned.startsWith("979")
                                  || cleaned.length() > 10
                                  ? QList<int>{3, 1, 3, 5, 1}
                                  : QList<int>{1, 3, 5, 1};

    QStringList parts;
    int index = 0;
    for (int size : groups) {
        if (index >= cleaned.length()) {
            break;
        }
        parts.append(cleaned.mid(index, size));
        index += size;
    }
    return parts.join("-");
}

int formattedIsbnCursorPosition(const QString &formatted, int cleanedCharactersBeforeCursor)
{
    if (cleanedCharactersBeforeCursor <= 0) {
        return 0;
    }

    int seen = 0;
    for (int i = 0; i < formatted.length(); ++i) {
        if (formatted.at(i).isDigit() || formatted.at(i) == 'X') {
            ++seen;
        }
        if (seen == cleanedCharactersBeforeCursor) {
            return i + 1;
        }
    }
    return formatted.length();
}

void updateIsbnLineEdit(QLineEdit *lineEdit, const QString &text)
{
    const int cursor = lineEdit->cursorPosition();
    const int cleanedBeforeCursor = cleanIsbnInput(text.left(cursor)).length();
    const QString formatted = formatIsbn(text);

    if (formatted == text) {
        return;
    }

    QSignalBlocker blocker(lineEdit);
    lineEdit->setText(formatted);
    lineEdit->setCursorPosition(formattedIsbnCursorPosition(formatted, cleanedBeforeCursor));
}

int extractIntValue(const QString &text, const QString &label)
{
    QRegularExpression regex(label + "\\s+(\\d+)");
    QRegularExpressionMatch match = regex.match(text);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

QString extractTextValue(const QString &text, const QString &label)
{
    QRegularExpression regex(label + "\\s+([^|]+)");
    QRegularExpressionMatch match = regex.match(text);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_selectedBookCode("")
    , m_selectedLearnerId(-1)
    , m_selectedAdminUserId(-1)
    , m_isAdminEditingUser(false)
    , m_isAdminChangingUserPassword(false)
    , m_sidebarExpanded(false)
    , m_chartView(nullptr)
    , m_quickSearchCompleter(new QCompleter(this))
    , m_quickSearchModel(new QStringListModel(this))
    , m_isQuickSearchNavigating(false)
    , m_currentPaymentId(-1)
{
    ui->setupUi(this);
    initializeUI();
    setupConnections();
    showLoginPage();

    glowEffect();
    ui->frame_iconOnly->show();
    ui->frame_sideText->hide();
    applyGlow(ui->frame_loginCard);
    applyGlow(ui->frame_registrationCard);
    applyGlow(ui->frame_resetPasswordCard);
    collapseAllSideTextOptions();

    ui->pushButton_homeIconSection->setToolTip("Home");
    ui->pushButton_transactionIconSection->setToolTip("Transact");
    ui->pushButton_booksIconSection->setToolTip("Books");
    ui->pushButton_learnersIconSection->setToolTip("Learners");
    ui->pushButton_reportsIconSection->setToolTip("Reports");
    ui->pushButton_usersIconSection->setToolTip("Users");
    ui->pushButton_settingsIconSection->setToolTip("Settings");
    ui->pushButton_logoutTopBar->setToolTip("Logout");
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_chartView) {
        delete m_chartView;
    }
}

void MainWindow::applyGlow(QWidget *g)
{
    QGraphicsDropShadowEffect *glow = new QGraphicsDropShadowEffect(g);
    glow->setBlurRadius(60);
    glow->setOffset(0, 0);
    glow->setColor(QColor(0xBF3952));
    g->setGraphicsEffect(glow);
}

void MainWindow::statisticsCardGlowEffect(QWidget *g)
{
    QGraphicsDropShadowEffect *glow = new QGraphicsDropShadowEffect(g);
    glow->setBlurRadius(60);
    glow->setOffset(0, 0);
    glow->setColor(QColor(Qt::white));
    g->setGraphicsEffect(glow);
}

void MainWindow::glowEffect()
{
    statisticsCardGlowEffect(ui->frame_overdueBooksCard);
    statisticsCardGlowEffect(ui->frame_totalBooksCard);
    statisticsCardGlowEffect(ui->frame_totalUsersCard);
    statisticsCardGlowEffect(ui->frame_BooksCurrentlyBorrowedCard);
    statisticsCardGlowEffect(ui->frame_totalActiveLearnersCard);
    statisticsCardGlowEffect(ui->frame_BooksAvailableCard);
}

void MainWindow::navigateToPage(QWidget *targetPage)
{
    if (!targetPage)
        return;
    QWidget *current = targetPage;
    while (current != nullptr) {
        QWidget *parent = current->parentWidget();
        QStackedWidget *stack = qobject_cast<QStackedWidget *>(parent);
        if (stack) {
            stack->setCurrentWidget(current);
        }
        current = parent;
    }
}

// ==================== Initialization ====================

void MainWindow::initializeUI()
{
    setWindowTitle("LibraCore - Library Management System");

    setupComboBoxes();
    setupTableHeaders();

    QDate today = QDate::currentDate();
    int currentYear = today.year();
    QDate latestLearnerDateOfBirth = today.addYears(-12);

    ui->dateEdit_borrowDate->setDate(today);
    ui->dateEdit_returnDate->setDate(today);
    ui->dateEdit_dueDate->setDate(QDate(currentYear, 11, 28));
    ui->dateEdit_addLearnerDOB->setMaximumDate(latestLearnerDateOfBirth);
    ui->dateEdit_editLearnerDOB->setMaximumDate(latestLearnerDateOfBirth);
    ui->dateEdit_addLearnerDOB->setDate(latestLearnerDateOfBirth);
    ui->dateEdit_editLearnerDOB->setDate(latestLearnerDateOfBirth);

    ui->dateEdit_borrowDate->setReadOnly(true);
    ui->dateEdit_dueDate->setReadOnly(true);
    ui->dateEdit_returnDate->setReadOnly(true);

    m_quickSearchCompleter->setModel(m_quickSearchModel);
    m_quickSearchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_quickSearchCompleter->setFilterMode(Qt::MatchContains);
    m_quickSearchCompleter->setCompletionMode(QCompleter::PopupCompletion);
    ui->lineEdit_quickSearch->setCompleter(m_quickSearchCompleter);

    ui->lineEdit_loginPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_regPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_regRepeatPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_resetNewPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_resetConfirmPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_currentPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_newPassword->setEchoMode(QLineEdit::Password);
    ui->lineEdit_repeatPassword->setEchoMode(QLineEdit::Password);
    ui->label_passwordRx->setTextFormat(Qt::RichText);
    updatePasswordRequirements(ui->label_passwordRx, ui->lineEdit_regPassword->text());
    loadSavedTheme();

    ui->frame_resetSecurityQuestionCard->setVisible(false);
    ui->frame_newPassword->setVisible(false);

    m_currentPaymentId = -1;
    ui->pushButton_viewReceipt->setEnabled(false);

    ui->tableWidget_lostBooks->setColumnCount(6);
    ui->tableWidget_lostBooks->setHorizontalHeaderLabels(
        {"Learner ID", "Book Code", "Book Title", "Lost Date", "Amount", "Select"});
    ui->tableWidget_lostBooks->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_lostBooks->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::setupConnections()
{
    connect(ui->tableWidget_lostBooks,
            &QTableWidget::itemChanged,
            this,
            &MainWindow::updatePaymentSummary);
    connect(m_quickSearchCompleter,
            qOverload<const QString &>(&QCompleter::activated),
            this,
            &MainWindow::onQuickSearchResultActivated);
    connect(ui->lineEdit_regPassword,
            &QLineEdit::textChanged,
            this,
            [this](const QString &password) {
                updatePasswordRequirements(ui->label_passwordRx, password);
            });
    connect(ui->lineEdit_bookISBN,
            &QLineEdit::textChanged,
            this,
            [this](const QString &text) {
                updateIsbnLineEdit(ui->lineEdit_bookISBN, text);
            });
    connect(ui->lineEdit_editBookISBN,
            &QLineEdit::textChanged,
            this,
            [this](const QString &text) {
                updateIsbnLineEdit(ui->lineEdit_editBookISBN, text);
            });
}

void MainWindow::setupComboBoxes()
{
    QStringList grades = {"8", "9", "10", "11", "12"};
    ui->comboBox_bookGrade->addItems(grades);
    ui->comboBox_editBookGrade->addItems(grades);
    ui->comboBox_addLearnerGrade->addItems(grades);
    ui->comboBox_editLearnerGrade->addItems(grades);
    ui->comboBox_filterLearnerGrade->addItem("All Grades");
    ui->comboBox_filterLearnerGrade->addItems(grades);

    QStringList subjects = {"Mathematics",
                            "Physical Sciences",
                            "Life Sciences",
                            "Geography",
                            "History",
                            "Tourism",
                            "Agriculture",
                            "Accounting",
                            "Business Studies",
                            "Economics",
                            "Life Orientation",
                            "Mathematical Literacy",
                            "Economic Management Sciences",
                            "English",
                            "Sepedi",
                            "Creative Arts"};
    ui->comboBox_bookSubject->addItems(subjects);
    ui->comboBox_editBookSubject->addItems(subjects);

    QStringList statuses = {"Available", "Borrowed", "Lost"};
    ui->comboBox_editBookStatus->addItems(statuses);

    ui->comboBox_sortBooks->addItem("Sort by Title");
    ui->comboBox_sortBooks->addItem("Sort by Author");
    ui->comboBox_sortBooks->addItem("Sort by Grade");
    ui->comboBox_sortBooks->addItem("Sort by Subject");

    ui->comboBox_filterStatus->addItem("All");
    ui->comboBox_filterStatus->addItem("Active");
    ui->comboBox_filterStatus->addItem("Returned");
    ui->comboBox_filterStatus->addItem("Lost");

    ui->QComboBox_regRole->addItem("Librarian");
    ui->QComboBox_regRole->addItem("Admin");
    ui->QComboBox_regRole->addItem("Finance");
    ui->QComboBox_adminRegRole->addItem("Librarian");
    ui->QComboBox_adminRegRole->addItem("Admin");
    ui->QComboBox_adminRegRole->addItem("Finance");

    QStringList securityQuestions = {"What is your mother's maiden name?",
                                     "What was the name of your first pet?",
                                     "What city were you born in?",
                                     "What is your favorite book?",
                                     "What was your first car?"};

    ui->QComboBox_securityQuestion->addItems(securityQuestions);
    ui->QComboBox_adminSecurityQuestion->addItems(securityQuestions);
}

void MainWindow::setupTableHeaders()
{
    ui->tableWidget_books->setColumnCount(7);
    ui->tableWidget_books->setHorizontalHeaderLabels(
        {"Book Code", "Title", "Author", "Subject", "Grade", "Price", "Status"});
    ui->tableWidget_books->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_books->setSelectionMode(QAbstractItemView::SingleSelection);
    configureResizableTable(ui->tableWidget_books);

    ui->tableWidget_viewLearnersList->setColumnCount(6);
    ui->tableWidget_viewLearnersList->setHorizontalHeaderLabels(
        {"ID", "Name", "Surname", "Grade", "DOB", "Contact"});
    ui->tableWidget_viewLearnersList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_viewLearnersList->setSelectionMode(QAbstractItemView::SingleSelection);
    configureResizableTable(ui->tableWidget_viewLearnersList);

    ui->tableWidget_transactionHistory->setColumnCount(7);
    ui->tableWidget_transactionHistory->setHorizontalHeaderLabels({"Book Code",
                                                                   "Book Title",
                                                                   "Borrow Date",
                                                                   "Due Date",
                                                                   "Return Date",
                                                                   "Status",
                                                                   "Days Overdue"});
    ui->tableWidget_transactionHistory->setSelectionBehavior(QAbstractItemView::SelectRows);
    configureResizableTable(ui->tableWidget_transactionHistory);

    ui->tableWidget_returnBooks->setColumnCount(6);
    ui->tableWidget_returnBooks->setHorizontalHeaderLabels(
        {"Learner ID", "Book Code", "Book Title", "Borrow Date", "Due Date", "Status"});
    ui->tableWidget_returnBooks->setSelectionBehavior(QAbstractItemView::SelectRows);
    configureResizableTable(ui->tableWidget_returnBooks);

    ui->tableWidget_homeRecentTransactions->setColumnCount(5);
    ui->tableWidget_homeRecentTransactions->setHorizontalHeaderLabels(
        {"Learner", "Book", "Date", "Type", "Status"});
    configureResizableTable(ui->tableWidget_homeRecentTransactions);

    ui->tableWidget_currentlyBorrowedBooks->setColumnCount(5);
    ui->tableWidget_currentlyBorrowedBooks->setHorizontalHeaderLabels(
        {"Book Title", "Borrow Date", "Due Date", "Days Left", "Status"});
    configureResizableTable(ui->tableWidget_currentlyBorrowedBooks);

    ui->tableWidget_users->setColumnCount(7);
    ui->tableWidget_users->setHorizontalHeaderLabels(
        {"ID", "Name", "Surname", "Email", "Contact", "School", "Role"});
    ui->tableWidget_users->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_users->setSelectionMode(QAbstractItemView::SingleSelection);
    configureResizableTable(ui->tableWidget_users);
}

void MainWindow::applyPermissions()
{
    bool isAdmin = AuthManager::instance().hasAdminPermission();
    bool isLibrarian = AuthManager::instance().hasLibrarianPermission();
    bool isFinance = AuthManager::instance().hasFinancePermission();

    ui->pushButton_goToAdminLobby->setVisible(isAdmin);
    ui->pushButton_booksSidebar->setEnabled(isLibrarian);
    ui->pushButton_learnersSidebar->setEnabled(isLibrarian);
    ui->pushButton_transactSidebar->setEnabled(isLibrarian);
    ui->pushButton_reportsSidebar->setEnabled(isLibrarian || isFinance);
}

// ==================== Login & Registration ====================

void MainWindow::on_pushButton_login_clicked()
{
    QString email = ui->lineEdit_loginUsername->text().trimmed();
    QString password = ui->lineEdit_loginPassword->text();

    if (email.isEmpty() || password.isEmpty()) {
        showErrorMessage("Please enter both email and password");
        return;
    }

    if (AuthManager::instance().login(email, password)) {
        showHomePage();
        updateUserInfo();
        applyPermissions();
        loadDashboardData();

        ui->lineEdit_loginUsername->clear();
        ui->lineEdit_loginPassword->clear();

        showSuccessMessage("Welcome, " + AuthManager::instance().getCurrentUser().getFullName());
    } else {
        showErrorMessage(AuthManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_register_clicked()
{
    showRegisterPage();
}

void MainWindow::on_pushButton_regBackToLogin_clicked()
{
    showLoginPage();
    clearRegistrationForm();
}

void MainWindow::on_pushButton_registerSubmit_clicked()
{
    if (!validateRegistrationForm()) {
        return;
    }

    User newUser;
    QString email = ui->lineEdit_regEmail->text().trimmed();
    newUser.setName(ui->lineEdit_regName->text().trimmed());
    newUser.setSurname(ui->lineEdit_regSurname->text().trimmed());
    newUser.setEmail(email);
    newUser.setContactNo(ui->lineEdit_regContactNo->text().trimmed());
    newUser.setSchoolName(ui->lineEdit_regSchoolName->text().trimmed());
    newUser.setRole(User::stringToRole(ui->QComboBox_regRole->currentText()));
    newUser.setSecurityQuestion(ui->QComboBox_securityQuestion->currentText());
    newUser.setSecurityAnswer(ui->lineEdit_regSecurityAnswer->text().trimmed());

    QString password = ui->lineEdit_regPassword->text();

    if (AuthManager::instance().registerUser(newUser, password)) {
        if (AuthManager::instance().login(email, password)) {
            showSuccessMessage("Registration successful! Welcome, " + newUser.getName() + "!");
            clearRegistrationForm();
            showHomePage();
        }
    } else {
        showErrorMessage(AuthManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_quit_clicked()
{
    QApplication::quit();
}

void MainWindow::on_pushButton_forgotPassword_clicked()
{
    showResetPasswordPage();
}

void MainWindow::on_pushButton_resetVerifyEmail_clicked()
{
    QString email = ui->lineEdit_resetEmail->text().trimmed();
    if (email.isEmpty()) {
        showErrorMessage("Please enter your email address");
        return;
    }
    if (AuthManager::instance().verifyEmail(email)) {
        m_currentEmail = email;
        ui->label_resetSecurityQuestion->setText(AuthManager::instance().getSecurityQuestion(email));
        ui->frame_resetSecurityQuestionCard->setVisible(true);
        ui->lineEdit_resetEmail->setEnabled(false);
        ui->pushButton_resetVerifyEmail->setEnabled(false);
    } else {
        showErrorMessage(AuthManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_resetVerifyAnswer_clicked()
{
    QString answer = ui->lineEdit_resetSecurityAnswer->text().trimmed();
    if (answer.isEmpty()) {
        showErrorMessage("Please enter your security answer");
        return;
    }
    if (AuthManager::instance().verifySecurityAnswer(m_currentEmail, answer)) {
        ui->frame_newPassword->setVisible(true);
        ui->frame_resetSecurityQuestionCard->setEnabled(false);
        ui->pushButton_resetVerifyAnswer->setEnabled(false);
    } else {
        showErrorMessage(AuthManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_resetSubmit_clicked()
{
    if (!validatePasswordResetForm())
        return;

    if (AuthManager::instance().resetPassword(m_currentEmail,
                                              ui->lineEdit_resetNewPassword->text())) {
        showSuccessMessage("Password reset successful! You can now login with your new password.");
        showLoginPage();
        ui->lineEdit_resetEmail->clear();
        ui->lineEdit_resetSecurityAnswer->clear();
        ui->lineEdit_resetNewPassword->clear();
        ui->lineEdit_resetConfirmPassword->clear();
        ui->frame_resetSecurityQuestionCard->setVisible(false);
        ui->frame_newPassword->setVisible(false);
        ui->lineEdit_resetEmail->setEnabled(true);
        ui->pushButton_resetVerifyEmail->setEnabled(true);
        ui->pushButton_resetVerifyAnswer->setEnabled(true);
    } else {
        showErrorMessage(AuthManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_resetCancel_clicked()
{
    showLoginPage();
    ui->lineEdit_resetEmail->clear();
    ui->lineEdit_resetSecurityAnswer->clear();
    ui->lineEdit_resetNewPassword->clear();
    ui->lineEdit_resetConfirmPassword->clear();
    ui->frame_resetSecurityQuestionCard->setVisible(false);
    ui->frame_newPassword->setVisible(false);
    ui->lineEdit_resetEmail->setEnabled(true);
    ui->pushButton_resetVerifyEmail->setEnabled(true);
    ui->pushButton_resetVerifyAnswer->setEnabled(true);
}

// ==================== Navigation ====================

void MainWindow::on_pushButton_homeIconSection_clicked()
{
    showDashboardPage();
    loadDashboardData();
}

void MainWindow::on_pushButton_logoutTopBar_clicked()
{
    AuthManager::instance().logout();
    showLoginPage();
}

void MainWindow::on_pushButton_toogleMenu_clicked()
{
    toggleSidebar();
}

void MainWindow::on_pushButton_transactionIconSection_clicked()
{
    m_sidebarExpanded = true;
    ui->frame_iconOnly->hide();
    ui->frame_sideText->show();
}

void MainWindow::on_pushButton_booksIconSection_clicked()
{
    m_sidebarExpanded = true;
    ui->frame_iconOnly->hide();
    ui->frame_sideText->show();
}

void MainWindow::on_pushButton_learnersIconSection_clicked()
{
    m_sidebarExpanded = true;
    ui->frame_iconOnly->hide();
    ui->frame_sideText->show();
}

void MainWindow::on_pushButton_reportsIconSection_clicked()
{
    m_sidebarExpanded = true;
    ui->frame_iconOnly->hide();
    ui->frame_sideText->show();
}

void MainWindow::on_pushButton_transactSidebar_clicked()
{
    expandSection(ui->frame_sidebarTransactOptions, false);
}

void MainWindow::on_pushButton_booksSidebar_clicked()
{
    expandSection(ui->frame_sidebarBooksOptions, false);
}

void MainWindow::on_pushButton_learnersSidebar_clicked()
{
    expandSection(ui->frame_sidebarLearnersOptions, false);
}

void MainWindow::on_pushButton_reportsSidebar_clicked()
{
    expandSection(ui->frame_sidebarReportOptions, false);
}

void MainWindow::on_pushButton_makePaymentSidebar_clicked()
{
    showPaymentsPage();
}

void MainWindow::on_pushButton_usersIconSection_clicked()
{
    showUsersPage();
}

void MainWindow::on_pushButton_usersSidebar_clicked()
{
    showUsersPage();
}

void MainWindow::on_pushButton_settingsIconSection_clicked()
{
    showSettingsPage();
}

void MainWindow::on_pushButton_settingsSidebar_clicked()
{
    showSettingsPage();
}

void MainWindow::on_pushButton_AddUser_clicked()
{
    clearAdminAddUserForm();
    showAddUser();
}

void MainWindow::on_pushButton_goToAdminLobby_clicked()
{
    if (!AuthManager::instance().hasAdminPermission()) {
        ui->pushButton_goToAdminLobby->setVisible(false);
        showErrorMessage("Only administrators can access the admin lobby");
        return;
    }

    showAdminLobby();
    loadAdminUsers();
}

void MainWindow::on_pushButton_profileTopBar_clicked()
{
    showUsersPage();
}

void MainWindow::collapseAllSideTextOptions()
{
    ui->frame_sidebarTransactOptions->setMaximumHeight(0);
    ui->frame_sidebarBooksOptions->setMaximumHeight(0);
    ui->frame_sidebarLearnersOptions->setMaximumHeight(0);
    ui->frame_sidebarReportOptions->setMaximumHeight(0);
}

void MainWindow::expandSection(QWidget *frameOptions, bool collapseOthers)
{
    int contentHeight = frameOptions->sizeHint().height();
    bool isCollapsed = frameOptions->maximumHeight() == 0;

    if (collapseOthers)
        collapseAllSideTextOptions();

    QPropertyAnimation *anim = new QPropertyAnimation(frameOptions, "maximumHeight");
    anim->setDuration(250);
    anim->setStartValue(isCollapsed ? 0 : contentHeight);
    anim->setEndValue(isCollapsed ? contentHeight : 0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::toggleSidebar()
{
    m_sidebarExpanded = !m_sidebarExpanded;
    if (m_sidebarExpanded) {
        ui->frame_iconOnly->hide();
        ui->frame_sideText->show();
    } else {
        ui->frame_sideText->hide();
        ui->frame_iconOnly->show();
    }
}

// ==================== Page Navigation ====================

void MainWindow::showLoginPage()
{
    navigateToPage(ui->page_login);
}

void MainWindow::showRegisterPage()
{
    navigateToPage(ui->page_register);
}

void MainWindow::showResetPasswordPage()
{
    navigateToPage(ui->page_resetPassword);
}

void MainWindow::showHomePage()
{
    navigateToPage(ui->page_home);
    showDashboardPage();
}

void MainWindow::showDashboardPage()
{
    navigateToPage(ui->page_dashboard);
}

void MainWindow::showBooksPage()
{
    navigateToPage(ui->page_addViewBooks);
}

void MainWindow::showAddBookPage()
{
    navigateToPage(ui->page_addViewBooks);
}

void MainWindow::showUpdateBookPage()
{
    navigateToPage(ui->page_updateBookInfo);
}

void MainWindow::showLearnersPage()
{
    navigateToPage(ui->page_addViewLearnerInfo);
}

void MainWindow::showAddLearnerPage()
{
    navigateToPage(ui->page_addViewLearnerInfo);
}

void MainWindow::showLearnerProfilePage()
{
    navigateToPage(ui->page_profile);
}

void MainWindow::showUpdateLearnerPage()
{
    navigateToPage(ui->page_updateLearnerInfo);
}

void MainWindow::showBorrowBookPage()
{
    navigateToPage(ui->page_borrowBook);
}

void MainWindow::showReturnBookPage()
{
    navigateToPage(ui->page_returnBook);
}

void MainWindow::showTransactionHistoryPage()
{
    navigateToPage(ui->page_transactionHistory);
}

void MainWindow::showUsersPage()
{
    loadUserProfile();
    ui->pushButton_goToAdminLobby->setVisible(AuthManager::instance().hasAdminPermission());
    navigateToPage(ui->page_userProfile);
}

void MainWindow::showEditUserInfo()
{
    navigateToPage(ui->page_profileUpdateUserInfo);
}

void MainWindow::showChangeUserPasswordPage()
{
    navigateToPage(ui->page_profileChangePassword);
}

void MainWindow::showAdminLobby()
{
    navigateToPage(ui->page_adminLobby);
    loadAdminUsers(ui->lineEdit_adminSearchUser->text().trimmed());
}

void MainWindow::showAddUser()
{
    navigateToPage(ui->page_usersAddUser);
}

void MainWindow::showReportsPage()
{
    navigateToPage(ui->page_reports);
}

void MainWindow::showPaymentsPage()
{
    navigateToPage(ui->page_payments);
}

void MainWindow::showSettingsPage()
{
    navigateToPage(ui->page_userSettings);
}

void MainWindow::loadAdminUsers(const QString& searchTerm)
{
    QVector<User> allUsers = DatabaseManager::instance().getAllUsers();
    QVector<User> visibleUsers;
    const QString term = searchTerm.trimmed().toLower();

    for (const User& user : allUsers) {
        const bool matches = term.isEmpty()
                             || QString::number(user.getId()).contains(term)
                             || user.getName().toLower().contains(term)
                             || user.getSurname().toLower().contains(term)
                             || user.getFullName().toLower().contains(term)
                             || user.getEmail().toLower().contains(term)
                             || user.getContactNo().contains(term)
                             || user.getSchoolName().toLower().contains(term)
                             || user.getRoleString().toLower().contains(term);
        if (matches) {
            visibleUsers.append(user);
        }
    }

    populateAdminUsersTable(visibleUsers, allUsers.size());
}

void MainWindow::populateAdminUsersTable(const QVector<User>& users, int totalUserCount)
{
    ui->tableWidget_users->setRowCount(0);
    m_selectedAdminUserId = -1;

    for (const User& user : users) {
        int row = ui->tableWidget_users->rowCount();
        ui->tableWidget_users->insertRow(row);

        QStringList values = {QString::number(user.getId()),
                              user.getName(),
                              user.getSurname(),
                              user.getEmail(),
                              user.getContactNo(),
                              user.getSchoolName(),
                              user.getRoleString()};

        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            item->setData(Qt::UserRole, user.getId());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setTextAlignment(column == 1 || column == 2 || column == 3 || column == 5
                                       ? Qt::AlignLeft | Qt::AlignVCenter
                                       : Qt::AlignCenter);
            ui->tableWidget_users->setItem(row, column, item);
        }
    }

    ui->label_summaryTitle_5->setText(
        QString("Showing %1 of %2 Users").arg(users.size()).arg(totalUserCount));
    ui->tableWidget_users->resizeColumnsToContents();
}

User MainWindow::getSelectedAdminUser() const
{
    if (m_selectedAdminUserId == -1) {
        return User();
    }
    return DatabaseManager::instance().getUserById(m_selectedAdminUserId);
}

void MainWindow::clearAdminAddUserForm()
{
    ui->lineEdit_adminRegName->clear();
    ui->lineEdit_adminRegSurname->clear();
    ui->lineEdit_adminRegEmail->clear();
    ui->lineEdit_adminRegPassword->clear();
    ui->lineEdit_adminRegRepeatPassword->clear();
    ui->lineEdit_adminRegContactNo->clear();
    ui->lineEdit_adminRegSchoolName->clear();
    ui->lineEdit_adminRegSecurityAnswer->clear();
    ui->QComboBox_adminRegRole->setCurrentIndex(0);
    ui->QComboBox_adminSecurityQuestion->setCurrentIndex(0);
}

void MainWindow::on_pushButton_adminUserSearch_clicked()
{
    loadAdminUsers(ui->lineEdit_adminSearchUser->text());
}

void MainWindow::on_pushButton_BackToLearnersList_7_clicked()
{
    ui->lineEdit_adminSearchUser->clear();
    loadAdminUsers();
}

void MainWindow::on_tableWidget_users_cellClicked(int row, int column)
{
    QTableWidgetItem *item = ui->tableWidget_users->item(row, column);
    if (!item) {
        return;
    }

    m_selectedAdminUserId = item->data(Qt::UserRole).toInt();
}

void MainWindow::on_pushButton_adminRegisterSubmit_clicked()
{
    QString name = ui->lineEdit_adminRegName->text().trimmed();
    QString surname = ui->lineEdit_adminRegSurname->text().trimmed();
    QString email = ui->lineEdit_adminRegEmail->text().trimmed();
    QString contactNo = ui->lineEdit_adminRegContactNo->text().trimmed();
    QString password = ui->lineEdit_adminRegPassword->text();
    QString repeatPassword = ui->lineEdit_adminRegRepeatPassword->text();

    if (name.isEmpty() || !isPersonNameValid(name)) {
        showErrorMessage("Name must contain letters only");
        return;
    }
    if (surname.isEmpty() || !isPersonNameValid(surname)) {
        showErrorMessage("Surname must contain letters only");
        return;
    }
    if (password != repeatPassword) {
        showErrorMessage("Passwords do not match");
        return;
    }
    QString passwordError = AuthManager::instance().validatePassword(password);
    if (!passwordError.isEmpty()) {
        showErrorMessage(passwordError);
        return;
    }
    QString emailError = AuthManager::instance().validateEmail(email);
    if (!emailError.isEmpty()) {
        showErrorMessage(emailError);
        return;
    }
    if (!isContactNumberValid(contactNo)) {
        showErrorMessage("Contact number must be 10 digits and start with 0");
        return;
    }
    if (ui->lineEdit_adminRegSecurityAnswer->text().trimmed().isEmpty()) {
        showErrorMessage("Security answer is required");
        return;
    }

    User newUser;
    newUser.setName(name);
    newUser.setSurname(surname);
    newUser.setEmail(email);
    newUser.setContactNo(contactNo);
    newUser.setSchoolName(ui->lineEdit_adminRegSchoolName->text().trimmed());
    newUser.setRole(User::stringToRole(ui->QComboBox_adminRegRole->currentText()));
    newUser.setSecurityQuestion(ui->QComboBox_adminSecurityQuestion->currentText());
    newUser.setSecurityAnswer(ui->lineEdit_adminRegSecurityAnswer->text().trimmed());

    if (!AuthManager::instance().registerUser(newUser, password)) {
        showErrorMessage(AuthManager::instance().getLastError());
        return;
    }

    int adminId = AuthManager::instance().getCurrentUserId();
    DatabaseManager::instance().logUserActivity(
        adminId,
        "Add User",
        QString("Added user %1 <%2> as %3").arg(newUser.getFullName(), email, newUser.getRoleString()));

    clearAdminAddUserForm();
    showAdminLobby();
    showSuccessMessage("User added successfully");
}

void MainWindow::on_pushButton_adminRegBackToLogin_clicked()
{
    clearAdminAddUserForm();
    showAdminLobby();
}

void MainWindow::on_pushButton_BackToLearnersList_4_clicked()
{
    User selectedUser = getSelectedAdminUser();
    if (selectedUser.getId() == -1) {
        showErrorMessage("Please select a user to edit");
        return;
    }

    m_isAdminEditingUser = true;
    m_isAdminChangingUserPassword = false;
    showEditUserInfo();
    ui->lineEdit_editProfile_name->setText(selectedUser.getName());
    ui->lineEdit_editProfile_surname->setText(selectedUser.getSurname());
    ui->lineEdit_editProfile_contact->setText(selectedUser.getContactNo());
    ui->lineEdit_editProfile_email->setText(selectedUser.getEmail());
    ui->lineEdit_editProfile_name->setFocus();
}

void MainWindow::on_pushButton_BackToLearnersList_5_clicked()
{
    User selectedUser = getSelectedAdminUser();
    if (selectedUser.getId() == -1) {
        showErrorMessage("Please select a user to delete");
        return;
    }
    if (selectedUser.getId() == AuthManager::instance().getCurrentUserId()) {
        showErrorMessage("You cannot delete your own account while logged in");
        return;
    }

    auto reply = QMessageBox::question(
        this,
        "Delete User",
        "Delete " + selectedUser.getFullName() + "? This cannot be undone.");
    if (reply != QMessageBox::Yes) {
        return;
    }

    if (!DatabaseManager::instance().deleteUser(selectedUser.getId())) {
        showErrorMessage(DatabaseManager::instance().getLastError());
        return;
    }

    DatabaseManager::instance().logUserActivity(
        AuthManager::instance().getCurrentUserId(),
        "Delete User",
        QString("Deleted user ID %1: %2 <%3>").arg(selectedUser.getId()).arg(selectedUser.getFullName(), selectedUser.getEmail()));
    loadAdminUsers(ui->lineEdit_adminSearchUser->text());
    showSuccessMessage("User deleted successfully");
}

void MainWindow::on_pushButton_BackToLearnersList_6_clicked()
{
    User selectedUser = getSelectedAdminUser();
    if (selectedUser.getId() == -1) {
        showErrorMessage("Please select a user");
        return;
    }

    m_isAdminEditingUser = false;
    m_isAdminChangingUserPassword = true;
    showChangeUserPasswordPage();
    clearChangePasswordForm();
    ui->lineEdit_currentPassword->setEnabled(false);
    ui->lineEdit_currentPassword->setPlaceholderText("Admin reset for " + selectedUser.getFullName());
    ui->lineEdit_newPassword->setFocus();
}

void MainWindow::applyTheme(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        showErrorMessage("Failed to load theme: " + resourcePath);
        return;
    }

    qApp->setStyleSheet(QTextStream(&file).readAll());
}

void MainWindow::applySelectedTheme()
{
    if (ui->radioButton_lightMode->isChecked()) {
        applyTheme(":/theme/themes/light.qss");
        QSettings().setValue("appearance/theme", "light");
        return;
    }

    applyTheme(":/theme/themes/dark.qss");
    QSettings().setValue("appearance/theme", "dark");
}

void MainWindow::on_pushButton_applyTheme_clicked()
{
    applySelectedTheme();
}

void MainWindow::loadSavedTheme()
{
    const QString theme = QSettings().value("appearance/theme", "dark").toString();
    const bool useLightTheme = theme == "light";

    ui->radioButton_lightMode->setChecked(useLightTheme);
    ui->radioButton_darkMode->setChecked(!useLightTheme);
    applyTheme(useLightTheme ? ":/theme/themes/light.qss" : ":/theme/themes/dark.qss");
}

// ==================== Dashboard ====================

void MainWindow::loadDashboardData()
{
    updateDashboardStats();
    loadRecentTransactions();
    setupLibraryChart();
}

void MainWindow::updateDashboardStats()
{
    auto stats = DatabaseManager::instance().getDashboardStats();
    ui->label_totalBooksDisplay->setText(QString::number(stats.totalBooks));
    ui->label_booksAvailableDisplay->setText(QString::number(stats.availableBooks));
    ui->label_BooksCurrentlyBorrowedDisplay->setText(QString::number(stats.borrowedBooks));
    ui->label_totalActiveLearnersDisplay->setText(QString::number(stats.activeLearners));
    ui->label_totalUsersDisplay->setText(QString::number(stats.totalUsers));
    ui->label_unreturnedBooksDisplay->setText(QString::number(stats.overdueBooks));
}

void MainWindow::loadRecentTransactions()
{
    QVector<Transaction> transactions = DatabaseManager::instance().getRecentTransactions(10);
    populateDashboardTransactions(transactions);
}

void MainWindow::on_pushButton_addBookQuickButton_clicked()
{
    showAddBookPage();
    loadAllBooks();
}

void MainWindow::on_pushButton_issueBookQuickButton_clicked()
{
    showBorrowBookPage();
}

void MainWindow::on_pushButton_returnBookQuickButton_clicked()
{
    showReturnBookPage();
}

void MainWindow::setupLibraryChart()
{
    auto stats = DatabaseManager::instance().getDashboardStats();
    int available = stats.availableBooks;
    int borrowed = stats.borrowedBooks;
    int lost = stats.totalBooks - available - borrowed;
    int total = stats.totalBooks;

    QPieSeries *series = new QPieSeries();
    auto lostSlice = series->append("Lost", lost);
    auto borrowedSlice = series->append("Borrowed", borrowed);
    auto availableSlice = series->append("Available", available);

    lostSlice->setBrush(QColor(0xe74c3c));
    borrowedSlice->setBrush(QColor(0xf39c12));
    availableSlice->setBrush(QColor(0x27ae60));

    series->setHoleSize(0.5);

    for (QPieSlice *slice : series->slices()) {
        slice->setLabel(QString("%1\n%2").arg(slice->label()).arg((int) slice->value()));
        slice->setLabelVisible(true);
        slice->setLabelBrush(QBrush(Qt::white));
        connect(slice, &QPieSlice::hovered, this, [slice](bool state) {
            slice->setExploded(state);
        });
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Library Stats");
    chart->setTitleBrush(QBrush(Qt::white));
    chart->setBackgroundVisible(false);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(Qt::white);

    for (QLegendMarker *marker : chart->legend()->markers(series)) {
        QPieLegendMarker *pieMarker = qobject_cast<QPieLegendMarker *>(marker);
        if (!pieMarker)
            continue;
        double percentage = (total > 0) ? (pieMarker->slice()->value() / total) * 100 : 0;
        marker->setLabel(QString("%1%").arg(QString::number(percentage, 'f', 1)));
    }

    if (m_chartView) {
        m_chartView->setChart(chart);
    } else {
        m_chartView = new QChartView(chart);
        m_chartView->setRenderHint(QPainter::Antialiasing);
        m_chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(ui->frame_chartContainer->layout());
        if (!layout) {
            layout = new QVBoxLayout(ui->frame_chartContainer);
            layout->setContentsMargins(0, 0, 0, 0);
        }
        layout->addWidget(m_chartView);
    }
}

void MainWindow::displayUserTips()
{
    QStringList tips = {"", "", "", "", "", "", "", "", ""};
}

// ==================== Book Management ====================

void MainWindow::on_pushButton_addBookSidebar_clicked()
{
    showAddBookPage();
    loadAllBooks();
}

void MainWindow::on_pushButton_updateBookInfoSidebar_clicked()
{
    if (m_selectedBookCode.isEmpty()) {
        showErrorMessage("Please select a book first");
        return;
    }
    showUpdateBookPage();
    loadBookDetails(m_selectedBookCode);
}

void MainWindow::on_pushButton_confirmAdd_clicked()
{
    if (!validateBookForm())
        return;

    Book book;
    book.setBookCode(ui->lineEdit_bookCode->text().trimmed());
    book.setIsbn(ui->lineEdit_bookISBN->text().trimmed());
    book.setTitle(ui->lineEdit_bookTitle->text().trimmed());
    book.setAuthor(ui->lineEdit_bookAuthor->text().trimmed());
    book.setSubject(ui->comboBox_bookSubject->currentText());
    book.setGrade(ui->comboBox_bookGrade->currentText());
    book.setPrice(ui->doubleSpinBox_bookPrice->value());
    book.setStatus(Book::Status::Available);

    if (DatabaseManager::instance().addBook(book)) {
        showSuccessMessage("Book added successfully!");
        clearBookForm();
        loadAllBooks();
    } else {
        showErrorMessage(DatabaseManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_clearForm_clicked()
{
    clearBookForm();
}

void MainWindow::on_pushButton_copyFromISBN_clicked()
{
    copyBookDataFromISBN();
}

void MainWindow::on_pushButton_refreshBooks_clicked()
{
    loadAllBooks();
    ui->lineEdit_searchBooks->clear();
    ui->comboBox_sortBooks->setCurrentIndex(0);
}

void MainWindow::on_pushButton_bookDetails_clicked()
{
    if (m_selectedBookCode.isEmpty()) {
        showErrorMessage("Please select a book first");
        return;
    }

    Book book = DatabaseManager::instance().getBookByCode(m_selectedBookCode);

    QString details = QString("Book Details:\n\n"
                              "Book Code: %1\n"
                              "ISBN: %2\n"
                              "Title: %3\n"
                              "Author: %4\n"
                              "Subject: %5\n"
                              "Grade: %6\n"
                              "Price: R%7\n"
                              "Status: %8\n")
                          .arg(book.getBookCode(),
                               book.getIsbn(),
                               book.getTitle(),
                               book.getAuthor(),
                               book.getSubject(),
                               book.getGrade())
                          .arg(book.getPrice())
                          .arg(book.getStatusString());

    QMessageBox::information(this, "Book Details", details);
}

void MainWindow::on_pushButton_markAsLost_clicked()
{
    if (m_selectedBookCode.isEmpty()) {
        showErrorMessage("Please select a book first");
        return;
    }

    Book book = DatabaseManager::instance().getBookByCode(m_selectedBookCode);

    if (book.getStatus() != Book::Status::Borrowed) {
        showErrorMessage("Only borrowed books can be marked as lost");
        return;
    }

    QMessageBox::StandardButton reply
        = QMessageBox::question(this,
                                "Mark Book as Lost",
                                "Are you sure you want to mark this book as lost?\n\n"
                                    + book.getTitle(),
                                QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QVector<Transaction> transactions = DatabaseManager::instance().getTransactionsByBookCode(
            m_selectedBookCode);
        for (const Transaction &trans : std::as_const(transactions)) {
            if (trans.isActive()) {
                if (DatabaseManager::instance().markBookAsLost(trans.getLearnerId(),
                                                               trans.getBookCode(),
                                                               trans.getBorrowDate())) {
                    showSuccessMessage("Book marked as lost");
                    loadAllBooks();
                } else {
                    showErrorMessage(DatabaseManager::instance().getLastError());
                }
                break;
            }
        }
    }
}

void MainWindow::on_lineEdit_searchBooks_textChanged(const QString &text)
{
    searchBooks(text);
}

void MainWindow::on_comboBox_sortBooks_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    loadAllBooks();
}

void MainWindow::on_tableWidget_books_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    QTableWidgetItem *item = ui->tableWidget_books->item(row, 0);
    if (item) {
        m_selectedBookCode = item->text();
    }
}

// Update Book

void MainWindow::on_pushButton_saveChanges_clicked()
{
    if (m_selectedBookCode.isEmpty()) {
        showErrorMessage("No book selected for update");
        return;
    }

    if (!validateBookDetails(ui->lineEdit_editBookISBN->text().trimmed(),
                             ui->lineEdit_editBookTitle->text().trimmed(),
                             ui->lineEdit_editBookAuthor->text().trimmed())) {
        return;
    }

    Book book = DatabaseManager::instance().getBookByCode(m_selectedBookCode);

    book.setIsbn(ui->lineEdit_editBookISBN->text().trimmed());
    book.setTitle(ui->lineEdit_editBookTitle->text().trimmed());
    book.setAuthor(ui->lineEdit_editBookAuthor->text().trimmed());
    book.setSubject(ui->comboBox_editBookSubject->currentText());
    book.setGrade(ui->comboBox_editBookGrade->currentText());
    book.setPrice(ui->doubleSpinBox_editBookPrice->value());
    book.setStatus(Book::stringToStatus(ui->comboBox_editBookStatus->currentText()));

    if (DatabaseManager::instance().updateBook(book)) {
        showSuccessMessage("Book updated successfully!");
        showAddBookPage();
        loadAllBooks();
        m_selectedBookCode.clear();
    } else {
        showErrorMessage(DatabaseManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_cancelEdit_clicked()
{
    showAddBookPage();
    m_selectedBookCode.clear();
}

void MainWindow::on_pushButton_deleteBook_clicked()
{
    if (m_selectedBookCode.isEmpty()) {
        showErrorMessage("No book selected for deletion");
        return;
    }

    User currentUser = AuthManager::instance().getCurrentUser();
    if (currentUser.getRole() != User::Role::Admin) {
        showErrorMessage("Only administrators can delete books.");
        return;
    }

    Book book = DatabaseManager::instance().getBookByCode(m_selectedBookCode);

    QMessageBox::StandardButton reply
        = QMessageBox::question(this,
                                "Delete Book",
                                "Are you sure you want to delete this book?\n\n" + book.getTitle()
                                    + "\n\nThis action cannot be undone!",
                                QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        int userId = currentUser.getId();
        if (DatabaseManager::instance().deleteBook(userId, m_selectedBookCode)) {
            showSuccessMessage("Book deleted successfully!");
            showAddBookPage();
            loadAllBooks();

            m_selectedBookCode.clear();
        } else {
            showErrorMessage(DatabaseManager::instance().getLastError());
        }
    }
}

// ==================== Learner Management ====================

void MainWindow::on_pushButton_addLearnerSidebar_clicked()
{
    showAddLearnerPage();
    loadAllLearners();
}

void MainWindow::on_pushButton_AddLearner_clicked()
{
    if (!validateLearnerForm())
        return;

    Learner learner;
    learner.setName(ui->lineEdit_addLearnerName->text().trimmed());
    learner.setSurname(ui->lineEdit_addLearnerSurname->text().trimmed());
    learner.setGrade(ui->comboBox_addLearnerGrade->currentText());
    learner.setDateOfBirth(ui->dateEdit_addLearnerDOB->date());
    learner.setContactNo(ui->lineEdit_addLearnerContact->text().trimmed());

    if (DatabaseManager::instance().addLearner(learner)) {
        showSuccessMessage("Learner added successfully!");
        clearLearnerForm();
        loadAllLearners();
    } else {
        showErrorMessage(DatabaseManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_addLearnerClearForm_clicked()
{
    clearLearnerForm();
}

void MainWindow::on_pushButton_refreshLearnerList_clicked()
{
    loadAllLearners();
    ui->lineEdit_searchLearner->clear();
    ui->comboBox_filterLearnerGrade->setCurrentIndex(0);
}

void MainWindow::on_pushButton_viewLeanerProfile_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("Please select a learner first");
        return;
    }
    loadLearnerProfile(m_selectedLearnerId);
    showLearnerProfilePage();
}

void MainWindow::on_pushButton_viewLearnerHistory_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("Please select a learner first");
        return;
    }
    loadTransactionHistory(m_selectedLearnerId);
    showTransactionHistoryPage();
}

void MainWindow::on_lineEdit_searchLearner_textChanged(const QString &text)
{
    searchLearners(text);
}

void MainWindow::on_lineEdit_quickSearch_textChanged(const QString &text)
{
    if (m_isQuickSearchNavigating)
        return;

    if (text.startsWith("Learner: ") || text.startsWith("Book: ")
        || text.startsWith("Transaction: ")) {
        onQuickSearchResultActivated(text);
        return;
    }

    updateQuickSearchResults(text);
}

void MainWindow::onQuickSearchResultActivated(const QString &result)
{
    if (m_isQuickSearchNavigating)
        return;

    QScopedValueRollback<bool> navigationGuard(m_isQuickSearchNavigating, true);

    if (result.startsWith("Learner: ")) {
        int learnerId = extractIntValue(result, "ID");
        if (learnerId == -1) {
            showErrorMessage("Could not open the selected learner");
            return;
        }

        m_selectedLearnerId = learnerId;
        loadLearnerProfile(learnerId);
        showLearnerProfilePage();
        ui->lineEdit_quickSearch->clear();
        return;
    }

    if (result.startsWith("Book: ")) {
        QString bookCode = extractTextValue(result, "Code");
        if (bookCode.isEmpty()) {
            showErrorMessage("Could not open the selected book");
            return;
        }

        m_selectedBookCode = bookCode;
        loadBookDetails(bookCode);
        showUpdateBookPage();
        ui->lineEdit_quickSearch->clear();
        return;
    }

    if (result.startsWith("Transaction: ")) {
        int learnerId = extractIntValue(result, "Learner");
        if (learnerId == -1) {
            showErrorMessage("Could not open the selected transaction");
            return;
        }

        m_selectedLearnerId = learnerId;
        loadTransactionHistory(learnerId);
        showTransactionHistoryPage();
        ui->lineEdit_quickSearch->clear();
        return;
    }
}

void MainWindow::on_comboBox_filterLearnerGrade_currentIndexChanged(int index)
{
    if (index == 0) {
        loadAllLearners();
    } else {
        filterLearnersByGrade(ui->comboBox_filterLearnerGrade->currentText());
    }
}

void MainWindow::on_tableWidget_viewLearnersList_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    QTableWidgetItem *item = ui->tableWidget_viewLearnersList->item(row, 0);
    if (item) {
        m_selectedLearnerId = item->text().toInt();
    }
}

void MainWindow::on_pushButton_backToLearners_clicked()
{
    showAddLearnerPage();
    m_selectedLearnerId = -1;
}

void MainWindow::on_pushButton_editLearnerProfile_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("No learner selected");
        return;
    }
    showUpdateLearnerPage();
    fillLearnerForm(DatabaseManager::instance().getLearnerById(m_selectedLearnerId));
}

void MainWindow::on_pushButton_viewTransactionRecord_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("No learner selected");
        return;
    }
    loadTransactionHistory(m_selectedLearnerId);
    showTransactionHistoryPage();
}

void MainWindow::on_pushButton_editLearnerConfirm_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("No learner selected for update");
        return;
    }

    if (!validateLearnerDetails(ui->lineEdit_editLearnerFirstName->text().trimmed(),
                                ui->lineEdit_editLearnerSurname->text().trimmed(),
                                ui->lineEdit_editLearnerContact->text().trimmed(),
                                ui->dateEdit_editLearnerDOB->date())) {
        return;
    }

    Learner learner = DatabaseManager::instance().getLearnerById(m_selectedLearnerId);
    learner.setName(ui->lineEdit_editLearnerFirstName->text().trimmed());
    learner.setSurname(ui->lineEdit_editLearnerSurname->text().trimmed());
    learner.setGrade(ui->comboBox_editLearnerGrade->currentText());
    learner.setDateOfBirth(ui->dateEdit_editLearnerDOB->date());
    learner.setContactNo(ui->lineEdit_editLearnerContact->text().trimmed());

    if (DatabaseManager::instance().updateLearner(learner)) {
        showSuccessMessage("Learner updated successfully!");
        showAddLearnerPage();
        loadAllLearners();
        m_selectedLearnerId = -1;
    } else {
        showErrorMessage(DatabaseManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_editLearnerCancel_clicked()
{
    showAddLearnerPage();
    m_selectedLearnerId = -1;
}

// ==================== Transaction Management ====================

void MainWindow::on_pushButton_borrowBookSidebar_clicked()
{
    showBorrowBookPage();
    clearBorrowForm();
}

void MainWindow::on_pushButton_returnBookSidebar_clicked()
{
    showReturnBookPage();
    clearReturnForm();
}

void MainWindow::on_pushButton_viewTransactionsSidebar_clicked()
{
    on_pushButton_viewLearnerHistory_clicked();
}

void MainWindow::on_pushButton_findLearner_clicked()
{
    QString learnerIdText = ui->lineEdit_borrowLearnerID->text().trimmed();
    if (learnerIdText.isEmpty()) {
        showErrorMessage("Please enter a learner ID");
        return;
    }

    bool ok;
    int learnerId = learnerIdText.toInt(&ok);
    if (!ok) {
        showErrorMessage("Invalid learner ID");
        return;
    }

    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    if (learner.getId() == -1) {
        showErrorMessage("Learner not found");
        ui->label_borrowLearnerName->setText("Not Found");
        ui->label_borrowLearnerGrade->setText("-");
        return;
    }

    if (DatabaseManager::instance().hasOverdueBooks(learnerId)) {
        showErrorMessage("This learner has overdue books and cannot borrow more books");
        return;
    }

    m_selectedLearnerId = learnerId;
    ui->label_borrowLearnerName->setText(learner.getFullName());
    ui->label_borrowLearnerGrade->setText(learner.getGrade());
}

void MainWindow::on_pushButton_findBook_clicked()
{
    QString bookCode = ui->lineEdit_borrowBookCode->text().trimmed();
    if (bookCode.isEmpty()) {
        showErrorMessage("Please enter a book code");
        return;
    }

    Book book = DatabaseManager::instance().getBookByCode(bookCode);
    if (book.getBookCode().isEmpty()) {
        showErrorMessage("Book not found");
        return;
    }

    if (!book.isAvailable()) {
        showErrorMessage("Book is not available (Status: " + book.getStatusString() + ")");
        return;
    }

    m_selectedBookCode = book.getBookCode();
    ui->label_borrowBookDisplay->setText(book.getTitle());
    ui->label_borrowBookAuthorDisplay->setText(book.getAuthor());
    ui->label_borrowBookISBNDisplay->setText(book.getIsbn());
    ui->label_borrowBookStatusDisplay->setText(book.getStatusString());
    ui->label_borrowBookCount->setText(
        QString::number(DatabaseManager::instance().getBookCountByISBN(book.getIsbn())));
}

void MainWindow::on_pushButton_confirmBorrow_clicked()
{
    if (!validateBorrowForm())
        return;
    performBorrow();
}

void MainWindow::on_pushButton_clearBorrowForm_clicked()
{
    clearBorrowForm();
}

void MainWindow::on_pushButton_goToReportsFromBorrow_clicked()
{
    showReportsPage();
}

void MainWindow::on_radioButton_searchByLearner_clicked()
{
    ui->label_searchBy->setText("Enter Learner ID:");
}

void MainWindow::on_radioButton_searchByBook_clicked()
{
    ui->label_searchBy->setText("Enter Book Code:");
}

void MainWindow::on_pushButton_searchReturn_clicked()
{
    if (!ui->radioButton_searchByLearner->isChecked()
        && !ui->radioButton_searchByBook->isChecked()) {
        showErrorMessage("Please select whether to search by learner ID or book code");
        return;
    }

    QString searchText = ui->lineEdit_returnSearch->text().trimmed();
    if (searchText.isEmpty()) {
        showErrorMessage("Please enter a search term");
        return;
    }

    if (ui->radioButton_searchByLearner->isChecked()) {
        bool ok;
        int learnerId = searchText.toInt(&ok);
        if (!ok) {
            showErrorMessage("Invalid learner ID");
            return;
        }
        loadActiveTransactionsForReturn(learnerId);
    } else if (ui->radioButton_searchByBook->isChecked()) {
        loadTransactionByBookCode(searchText);
    }
}

void MainWindow::on_tableWidget_returnBooks_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    QTableWidgetItem *learnerItem = ui->tableWidget_returnBooks->item(row, 0);
    QTableWidgetItem *bookItem = ui->tableWidget_returnBooks->item(row, 1);
    QTableWidgetItem *dateItem = ui->tableWidget_returnBooks->item(row, 3);

    if (!learnerItem || !bookItem || !dateItem)
        return;

    m_selectedTransactionKey.learnerId = learnerItem->text().toInt();
    m_selectedTransactionKey.bookCode = bookItem->text();
    m_selectedTransactionKey.borrowDate = QDate::fromString(dateItem->text(), "dd/MM/yyyy");

    Transaction trans = DatabaseManager::instance()
                            .getTransactionByKey(m_selectedTransactionKey.learnerId,
                                                 m_selectedTransactionKey.bookCode,
                                                 m_selectedTransactionKey.borrowDate);

    Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
    ui->label_returnSelectedBook->setText(book.getTitle());
}

void MainWindow::on_pushButton_processReturn_clicked()
{
    if (!m_selectedTransactionKey.isValid()) {
        showErrorMessage("Please select a transaction to return");
        return;
    }
    performReturn();
}

void MainWindow::on_pushButton_clearReturnForm_clicked()
{
    clearReturnForm();
}

void MainWindow::on_pushButton_markBookAsLost_clicked()
{
    if (!m_selectedTransactionKey.isValid()) {
        showErrorMessage("Please select a transaction first");
        return;
    }
    performMarkAsLost();
}

void MainWindow::on_pushButton_goToReportsFromReturn_clicked()
{
    showReportsPage();
}

void MainWindow::on_pushButton_BackToLearnersList_clicked()
{
    showAddLearnerPage();
}

void MainWindow::on_pushButton_backToProfile_clicked()
{
    loadLearnerProfile(m_selectedLearnerId);
    showLearnerProfilePage();
}

void MainWindow::on_pushButton_filterHistory_clicked()
{
    QDate startDate = ui->dateEdit_filterFrom->date();
    QDate endDate = ui->dateEdit_filterTo->date();
    QString statusFilter = ui->comboBox_filterStatus->currentText();

    QVector<Transaction> transactions = DatabaseManager::instance().getTransactionsByLearnerId(
        m_selectedLearnerId);
    QVector<Transaction> filtered;

    for (const Transaction &trans : std::as_const(transactions)) {
        bool dateMatch = trans.getBorrowDate() >= startDate && trans.getBorrowDate() <= endDate;
        bool statusMatch = (statusFilter == "All") || (trans.getStatusString() == statusFilter);
        if (dateMatch && statusMatch) {
            filtered.append(trans);
        }
    }
    populateTransactionsTable(filtered);
}

void MainWindow::on_pushButton_goToReports_clicked()
{
    showReportsPage();
}

// ==================== Reports ====================

void MainWindow::on_pushButton_printReportSidebar_clicked()
{
    showReportsPage();
}

void MainWindow::on_pushButton_loadLearnerData_clicked()
{
    QString learnerIdText = ui->lineEdit_reportLearnerID->text().trimmed();
    if (learnerIdText.isEmpty()) {
        showErrorMessage("Please enter a learner ID");
        return;
    }

    bool ok;
    int learnerId = learnerIdText.toInt(&ok);
    if (!ok) {
        showErrorMessage("Invalid learner ID");
        return;
    }

    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    if (learner.getId() == -1) {
        showErrorMessage("Learner not found");
        return;
    }

    m_selectedLearnerId = learnerId;
    ui->label_reportLearnerName->setText(learner.getFullName());
    ui->label_reportLearnerGrade->setText(learner.getGrade());
    showSuccessMessage("Learner loaded: " + learner.getFullName());
}

void MainWindow::on_pushButton_generateReport_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("Please load learner data first");
        return;
    }
    if (ui->radioButton_borrowReport->isChecked()) {
        generateBorrowReport(m_selectedLearnerId);
    } else if (ui->radioButton_returnReport->isChecked()) {
        generateReturnReport(m_selectedLearnerId);
    } else {
        showErrorMessage("Please select a report type");
    }
}

void MainWindow::on_pushButton_printReport_clicked()
{
    printReport();
}
void MainWindow::on_pushButton_saveReportPDF_clicked()
{
    saveReportAsPDF();
}

void MainWindow::on_pushButton_clearPreview_clicked()
{
    ui->textEdit_reportPreview->clear();
    ui->lineEdit_reportLearnerID->clear();
    ui->label_reportLearnerName->clear();
    ui->label_reportLearnerGrade->clear();
    m_selectedLearnerId = -1;
}

void MainWindow::on_radioButton_borrowReport_clicked() {}
void MainWindow::on_radioButton_returnReport_clicked() {}

// ==================== Payments ====================

void MainWindow::on_pushButton_findLearnerPayment_clicked()
{
    QString learnerIdText = ui->lineEdit_paymentLearnerId->text().trimmed();
    if (learnerIdText.isEmpty()) {
        showErrorMessage("Please enter a learner ID");
        return;
    }

    bool ok;
    int learnerId = learnerIdText.toInt(&ok);
    if (!ok) {
        showErrorMessage("Invalid learner ID");
        return;
    }

    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    if (learner.getId() == -1) {
        showErrorMessage("Learner not found");
        ui->label_learnerInfo->setText("Not Found");
        ui->label_totalOutstanding->setText("R0.00");
        ui->tableWidget_lostBooks->setRowCount(0);
        return;
    }

    m_selectedLearnerId = learnerId;
    ui->label_learnerInfo->setText(
        QString("%1 %2 - Grade %3").arg(learner.getName(), learner.getSurname(), learner.getGrade()));

    double totalOutstanding = DatabaseManager::instance().getTotalOutstandingFees(learnerId);
    ui->label_totalOutstanding->setText("R" + QString::number(totalOutstanding, 'f', 2));

    loadLostBooksForPayment(learnerId);

    ui->label_selectedItems->setText("0");
    ui->label_amountToPay->setText("R0.00");
    m_selectedPaymentKeys.clear();
}

void MainWindow::loadLostBooksForPayment(int learnerId)
{
    QVector<Transaction> lostTransactions = DatabaseManager::instance()
                                                .getUnpaidLostTransactionsByLearnerId(learnerId);

    ui->tableWidget_lostBooks->setRowCount(0);

    for (const Transaction &trans : std::as_const(lostTransactions)) {
        Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());

        int row = ui->tableWidget_lostBooks->rowCount();
        ui->tableWidget_lostBooks->insertRow(row);

        ui->tableWidget_lostBooks
            ->setItem(row, 0, new QTableWidgetItem(QString::number(trans.getLearnerId())));
        ui->tableWidget_lostBooks->setItem(row, 1, new QTableWidgetItem(trans.getBookCode()));
        ui->tableWidget_lostBooks->setItem(row, 2, new QTableWidgetItem(book.getTitle()));

        QString lostDate = trans.getReturnDate().isValid()
                               ? trans.getReturnDate().toString("dd/MM/yyyy")
                               : "N/A";
        ui->tableWidget_lostBooks->setItem(row, 3, new QTableWidgetItem(lostDate));
        ui->tableWidget_lostBooks
            ->setItem(row, 4, new QTableWidgetItem("R" + QString::number(book.getPrice(), 'f', 2)));

        QTableWidgetItem *checkboxItem = new QTableWidgetItem();
        checkboxItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkboxItem->setCheckState(Qt::Unchecked);
        ui->tableWidget_lostBooks->setItem(row, 5, checkboxItem);
    }
}

void MainWindow::on_tableWidget_lostBooks_itemSelectionChanged()
{
    updatePaymentSummary();
}

void MainWindow::updatePaymentSummary()
{
    int selectedCount = 0;
    double totalAmount = 0.0;
    m_selectedPaymentKeys.clear();

    for (int row = 0; row < ui->tableWidget_lostBooks->rowCount(); ++row) {
        QTableWidgetItem *checkboxItem = ui->tableWidget_lostBooks->item(row, 5);
        if (checkboxItem && checkboxItem->checkState() == Qt::Checked) {
            selectedCount++;

            int learnerId = ui->tableWidget_lostBooks->item(row, 0)->text().toInt();
            QString bookCode = ui->tableWidget_lostBooks->item(row, 1)->text();
            m_selectedPaymentKeys.append(qMakePair(learnerId, bookCode));

            QString amountStr = ui->tableWidget_lostBooks->item(row, 4)->text();
            amountStr.remove("R");
            totalAmount += amountStr.toDouble();
        }
    }

    ui->label_selectedItems->setText(QString::number(selectedCount));
    ui->label_amountToPay->setText("R" + QString::number(totalAmount, 'f', 2));
}

void MainWindow::on_pushButton_selectAllBooks_clicked()
{
    for (int row = 0; row < ui->tableWidget_lostBooks->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidget_lostBooks->item(row, 5);
        if (item)
            item->setCheckState(Qt::Checked);
    }
    updatePaymentSummary();
}

void MainWindow::on_pushButton_deselectAllBooks_clicked()
{
    for (int row = 0; row < ui->tableWidget_lostBooks->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidget_lostBooks->item(row, 5);
        if (item)
            item->setCheckState(Qt::Unchecked);
    }
    updatePaymentSummary();
}

void MainWindow::on_pushButton_processPayment_clicked()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("Please find a learner first");
        return;
    }
    if (m_selectedPaymentKeys.isEmpty()) {
        showErrorMessage("Please select at least one book to process payment");
        return;
    }

    QString amountStr = ui->label_amountToPay->text();
    amountStr.remove("R");
    double amount = amountStr.toDouble();

    if (amount <= 0) {
        showErrorMessage("Invalid payment amount");
        return;
    }

    Learner learner = DatabaseManager::instance().getLearnerById(m_selectedLearnerId);
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Payment",
        QString(
            "Process payment for %1?\n\nBooks: %2\nAmount: R%3\n\nThis action cannot be undone.")
            .arg(learner.getFullName())
            .arg(m_selectedPaymentKeys.size())
            .arg(QString::number(amount, 'f', 2)),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    Payments payment;
    payment.setLearnerId(m_selectedLearnerId);
    payment.setAmount(amount);
    payment.setProcessedBy(AuthManager::instance().getCurrentUser().getId());
    payment.setPaymentDate(QDateTime::currentDateTime());
    payment.setNotes("Cash payment for lost books");

    if (DatabaseManager::instance().processPayment(payment, m_selectedPaymentKeys)) {
        m_currentPaymentId = payment.getId();
        showSuccessMessage("Payment processed successfully!\nReceipt No: " + payment.getReceiptNo());
        ui->pushButton_viewReceipt->setEnabled(true);

    } else {
        showErrorMessage("Failed to process payment: " + DatabaseManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_viewReceipt_clicked()
{
    if (m_currentPaymentId == -1) {
        showErrorMessage("No payment receipt available");
        return;
    }

    Payments payment = DatabaseManager::instance().getPaymentById(m_currentPaymentId);
    if (payment.getId() == -1) {
        showErrorMessage("Payment not found");
        return;
    }

    QString receiptHtml = generateReceiptHTML(payment);

    QDialog *receiptDialog = new QDialog(this);
    receiptDialog->setWindowTitle("Payment Receipt");
    receiptDialog->resize(600, 800);

    QVBoxLayout *layout = new QVBoxLayout(receiptDialog);
    QTextEdit *textEdit = new QTextEdit(receiptDialog);
    textEdit->setHtml(receiptHtml);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *printBtn = new QPushButton("Print", receiptDialog);
    QPushButton *closeBtn = new QPushButton("Close", receiptDialog);
    buttonLayout->addWidget(printBtn);
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);

    connect(printBtn, &QPushButton::clicked, [textEdit]() {
        QPrinter printer;
        QPrintDialog dialog(&printer);
        if (dialog.exec() == QDialog::Accepted) {
            textEdit->document()->print(&printer);
        }
    });
    connect(closeBtn, &QPushButton::clicked, receiptDialog, &QDialog::accept);

    receiptDialog->exec();
    delete receiptDialog;
}

void MainWindow::on_pushButton_clearPayment_clicked()
{
    ui->lineEdit_paymentLearnerId->clear();
    ui->label_learnerInfo->clear();
    ui->label_totalOutstanding->setText("R0.00");
    ui->tableWidget_lostBooks->setRowCount(0);
    ui->label_selectedItems->setText("0");
    ui->label_amountToPay->setText("R0.00");
    m_selectedLearnerId = -1;
    m_currentPaymentId = -1;
    m_selectedPaymentKeys.clear();
    ui->pushButton_viewReceipt->setEnabled(false);
}

// ==================== Helper Methods ====================

void MainWindow::updateUserInfo()
{
    User currentUser = AuthManager::instance().getCurrentUser();
    ui->label_fullNameCurrentUser->setText(currentUser.getFullName() + "\n"
                                           + currentUser.getRoleString());
    ui->label_greetingsDateTopBar->setText("Hello, " + currentUser.getName() + "\n"
                                           + QDate::currentDate().toString("dddd, MMMM d, yyyy"));
}

void MainWindow::showSuccessMessage(const QString &message)
{
    QMessageBox::information(this, "Success", message);
}

void MainWindow::showErrorMessage(const QString &message)
{
    QMessageBox::critical(this, "Error", message);
}

void MainWindow::showInfoMessage(const QString &message)
{
    QMessageBox::information(this, "Information", message);
}

// ==================== Validation ====================

bool MainWindow::validateBookForm()
{
    if (ui->lineEdit_bookCode->text().trimmed().isEmpty()) {
        showErrorMessage("Book code is required");
        return false;
    }
    if (DatabaseManager::instance().bookCodeExists(ui->lineEdit_bookCode->text().trimmed())) {
        showErrorMessage("Book code already exists");
        return false;
    }
    if (ui->lineEdit_bookISBN->text().trimmed().isEmpty()) {
        showErrorMessage("ISBN is required");
        return false;
    }
    return validateBookDetails(ui->lineEdit_bookISBN->text().trimmed(),
                               ui->lineEdit_bookTitle->text().trimmed(),
                               ui->lineEdit_bookAuthor->text().trimmed());
}

bool MainWindow::validateBookDetails(const QString &isbn,
                                     const QString &title,
                                     const QString &author)
{
    if (isbn.isEmpty()) {
        showErrorMessage("ISBN is required");
        return false;
    }
    if (!isIsbnValid(isbn)) {
        showErrorMessage(
            "ISBN must be a valid ISBN-10 or ISBN-13. Use digits with optional spaces or hyphens; "
            "ISBN-13 must start with 978 or 979, and ISBN-10 may end with a capital X.");
        return false;
    }
    if (title.isEmpty()) {
        showErrorMessage("Book title is required");
        return false;
    }
    if (author.isEmpty()) {
        showErrorMessage("Author is required");
        return false;
    }
    return true;
}

bool MainWindow::validateLearnerForm()
{
    return validateLearnerDetails(ui->lineEdit_addLearnerName->text().trimmed(),
                                  ui->lineEdit_addLearnerSurname->text().trimmed(),
                                  ui->lineEdit_addLearnerContact->text().trimmed(),
                                  ui->dateEdit_addLearnerDOB->date());
}

bool MainWindow::validateLearnerDetails(const QString &name,
                                        const QString &surname,
                                        const QString &contactNo,
                                        const QDate &dateOfBirth)
{
    if (name.isEmpty()) {
        showErrorMessage("Learner name is required");
        return false;
    }
    if (!isPersonNameValid(name)) {
        showErrorMessage("Learner name must contain letters only");
        return false;
    }
    if (surname.isEmpty()) {
        showErrorMessage("Learner surname is required");
        return false;
    }
    if (!isPersonNameValid(surname)) {
        showErrorMessage("Learner surname must contain letters only");
        return false;
    }
    if (!isContactNumberValid(contactNo)) {
        showErrorMessage("Contact number must be 10 digits and start with 0");
        return false;
    }
    if (!dateOfBirth.isValid()) {
        showErrorMessage("Valid date of birth is required");
        return false;
    }
    if (dateOfBirth > QDate::currentDate()) {
        showErrorMessage("Date of birth cannot be in the future");
        return false;
    }
    if (dateOfBirth > QDate::currentDate().addYears(-12)) {
        showErrorMessage("Learner must be at least 12 years old");
        return false;
    }
    return true;
}

bool MainWindow::validateBorrowForm()
{
    if (m_selectedLearnerId == -1) {
        showErrorMessage("Please find and select a learner first");
        return false;
    }
    if (m_selectedBookCode.isEmpty()) {
        showErrorMessage("Please find and select a book first");
        return false;
    }

    Learner learner = DatabaseManager::instance().getLearnerById(m_selectedLearnerId);
    Book book = DatabaseManager::instance().getBookByCode(m_selectedBookCode);
    bool learnerGradeOk = false;
    bool bookGradeOk = false;
    int learnerGrade = learner.getGrade().toInt(&learnerGradeOk);
    int bookGrade = book.getGrade().toInt(&bookGradeOk);

    if (!learnerGradeOk || !bookGradeOk) {
        showErrorMessage("Unable to validate learner and book grades");
        return false;
    }
    if (bookGrade > learnerGrade) {
        showErrorMessage("Learner cannot borrow a book for a higher grade");
        return false;
    }
    return true;
}

bool MainWindow::validateRegistrationForm()
{
    QString name = ui->lineEdit_regName->text().trimmed();
    QString surname = ui->lineEdit_regSurname->text().trimmed();
    QString contactNo = ui->lineEdit_regContactNo->text().trimmed();

    if (name.isEmpty()) {
        showErrorMessage("Name is required");
        return false;
    }
    if (!isPersonNameValid(name)) {
        showErrorMessage("Name must contain letters only");
        return false;
    }
    if (surname.isEmpty()) {
        showErrorMessage("Surname is required");
        return false;
    }
    if (!isPersonNameValid(surname)) {
        showErrorMessage("Surname must contain letters only");
        return false;
    }
    if (!isContactNumberValid(contactNo)) {
        showErrorMessage("Contact number must be 10 digits and start with 0");
        return false;
    }
    if (ui->lineEdit_regPassword->text() != ui->lineEdit_regRepeatPassword->text()) {
        showErrorMessage("Passwords do not match");
        return false;
    }
    QString passwordError = AuthManager::instance().validatePassword(ui->lineEdit_regPassword->text());
    if (!passwordError.isEmpty()) {
        showErrorMessage(passwordError);
        return false;
    }
    QString emailError = AuthManager::instance().validateEmail(ui->lineEdit_regEmail->text());
    if (!emailError.isEmpty()) {
        showErrorMessage(emailError);
        return false;
    }
    if (ui->lineEdit_regSecurityAnswer->text().trimmed().isEmpty()) {
        showErrorMessage("Security answer is required");
        return false;
    }
    return true;
}

bool MainWindow::validatePasswordResetForm()
{
    if (ui->lineEdit_resetNewPassword->text() != ui->lineEdit_resetConfirmPassword->text()) {
        showErrorMessage("Passwords do not match");
        return false;
    }
    QString passwordError = AuthManager::instance().validatePassword(ui->lineEdit_resetNewPassword->text());
    if (!passwordError.isEmpty()) {
        showErrorMessage(passwordError);
        return false;
    }
    return true;
}

// ==================== Business Logic ====================

void MainWindow::performBorrow()
{
    QDate borrowDate = ui->dateEdit_borrowDate->date();

    int userId = AuthManager::instance().getCurrentUser().getId();

    if (DatabaseManager::instance().borrowBook(userId,
                                               m_selectedLearnerId,
                                               m_selectedBookCode,
                                               borrowDate)) {
        showSuccessMessage("Book borrowed successfully!");
        loadDashboardData();
        clearBorrowForm();

    } else {
        showErrorMessage(DatabaseManager::instance().getLastError());
    }
}

void MainWindow::performReturn()
{
    QDate returnDate = ui->dateEdit_returnDate->date();

    if (DatabaseManager::instance().returnBook(m_selectedTransactionKey.learnerId,
                                               m_selectedTransactionKey.bookCode,
                                               m_selectedTransactionKey.borrowDate,
                                               returnDate)) {
        showSuccessMessage("Book returned successfully!");
        loadDashboardData();
        clearReturnForm();
    } else {
        showErrorMessage(DatabaseManager::instance().getLastError());
    }
}

void MainWindow::performMarkAsLost()
{
    Transaction trans = DatabaseManager::instance()
                            .getTransactionByKey(m_selectedTransactionKey.learnerId,
                                                 m_selectedTransactionKey.bookCode,
                                                 m_selectedTransactionKey.borrowDate);

    Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());

    QMessageBox::StandardButton reply
        = QMessageBox::question(this,
                                "Mark Book as Lost",
                                "Are you sure you want to mark this book as lost?\n\n"
                                    + book.getTitle() + "\n\nThe learner will be charged R"
                                    + QString::number(book.getPrice(), 'f', 2),
                                QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (DatabaseManager::instance().markBookAsLost(m_selectedTransactionKey.learnerId,
                                                       m_selectedTransactionKey.bookCode,
                                                       m_selectedTransactionKey.borrowDate)) {
            showSuccessMessage("Book marked as lost. Learner will be charged R"
                               + QString::number(book.getPrice(), 'f', 2));
            loadDashboardData();
            clearReturnForm();
        } else {
            showErrorMessage(DatabaseManager::instance().getLastError());
        }
    }
}

void MainWindow::calculateAndDisplayAmount(int learnerId)
{
    double amount = DatabaseManager::instance().calculateUnreturnedBooksAmount(learnerId);
    showInfoMessage("Total amount due: R" + QString::number(amount, 'f', 2));
}

// ==================== Data Loading ====================

void MainWindow::loadAllBooks()
{
    populateBooksTable(DatabaseManager::instance().getAllBooks());
}

void MainWindow::loadAllLearners()
{
    populateLearnersTable(DatabaseManager::instance().getAllLearners());
}

void MainWindow::loadLearnerProfile(int learnerId)
{
    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    if (learner.getId() == -1) {
        showErrorMessage("Learner not found");
        return;
    }

    ui->label_profileFullName->setText(learner.getFullName());
    ui->label_profileInitialSurname->setText(learner.getInitialSurname());
    ui->label_profileGrade->setText(learner.getGrade());
    ui->label_profileDOB->setText(learner.getDateOfBirth().toString("dd MMMM yyyy"));
    ui->label_profileContact->setText(learner.getContactNo());

    populateCurrentlyBorrowedBooks(learnerId);

    double totalDue = DatabaseManager::instance().calculateUnreturnedBooksAmount(learnerId);
    ui->label_totalOutstanding->setText("R" + QString::number(totalDue, 'f', 2));
}

void MainWindow::loadBookDetails(const QString &bookCode)
{
    Book book = DatabaseManager::instance().getBookByCode(bookCode);
    if (book.getBookCode().isEmpty()) {
        showErrorMessage("Book not found");
        return;
    }

    ui->label_editBookCode->setText(book.getBookCode());
    ui->lineEdit_editBookISBN->setText(book.getIsbn());
    ui->lineEdit_editBookTitle->setText(book.getTitle());
    ui->lineEdit_editBookAuthor->setText(book.getAuthor());
    ui->comboBox_editBookSubject->setCurrentText(book.getSubject());
    ui->comboBox_editBookGrade->setCurrentText(book.getGrade());
    ui->doubleSpinBox_editBookPrice->setValue(book.getPrice());
    ui->comboBox_editBookStatus->setCurrentText(book.getStatusString());

    int copies = DatabaseManager::instance().getBookCountByISBN(book.getIsbn());
    ui->label_copiesInfoDisplay->setText(QString::number(copies) + " copies with this ISBN");
}

void MainWindow::loadTransactionHistory(int learnerId)
{
    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    ui->label_historyLearnerName->setText(learner.getFullName());
    ui->label_historyLearnerId->setText(QString::number(learnerId));
    populateTransactionsTable(DatabaseManager::instance().getTransactionsByLearnerId(learnerId));
}

void MainWindow::loadActiveTransactionsForReturn(int learnerId)
{
    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    if (learner.getId() == -1) {
        populateReturnBooksTable({});
        showErrorMessage("Learner not found");
        return;
    }

    QVector<Transaction> activeTransactions = DatabaseManager::instance()
                                                  .getActiveTransactionsByLearnerId(learnerId);
    populateReturnBooksTable(activeTransactions);
    if (activeTransactions.isEmpty()) {
        showErrorMessage("No active borrowed books found for this learner");
    }
}

void MainWindow::loadTransactionByBookCode(const QString &bookCode)
{
    Book book = DatabaseManager::instance().getBookByCode(bookCode);
    if (book.getBookCode().isEmpty()) {
        populateReturnBooksTable({});
        showErrorMessage("Book not found");
        return;
    }

    QVector<Transaction> all = DatabaseManager::instance().getTransactionsByBookCode(bookCode);
    QVector<Transaction> active;
    for (const Transaction &trans : std::as_const(all)) {
        if (trans.isActive())
            active.append(trans);
    }
    populateReturnBooksTable(active);
    if (active.isEmpty()) {
        showErrorMessage("No active borrow transaction found for this book");
    }
}

// ==================== Table Population ====================

void MainWindow::populateBooksTable(const QVector<Book> &books)
{
    ui->tableWidget_books->setRowCount(0);
    for (const Book &book : std::as_const(books)) {
        int row = ui->tableWidget_books->rowCount();
        ui->tableWidget_books->insertRow(row);
        ui->tableWidget_books->setItem(row, 0, new QTableWidgetItem(book.getBookCode()));
        ui->tableWidget_books->setItem(row, 1, new QTableWidgetItem(book.getTitle()));
        ui->tableWidget_books->setItem(row, 2, new QTableWidgetItem(book.getAuthor()));
        ui->tableWidget_books->setItem(row, 3, new QTableWidgetItem(book.getSubject()));
        ui->tableWidget_books->setItem(row, 4, new QTableWidgetItem(book.getGrade()));
        ui->tableWidget_books
            ->setItem(row, 5, new QTableWidgetItem("R" + QString::number(book.getPrice(), 'f', 2)));
        ui->tableWidget_books->setItem(row, 6, new QTableWidgetItem(book.getStatusString()));
    }
    justifyTableContents(ui->tableWidget_books, {1, 2, 3});
}

void MainWindow::populateLearnersTable(const QVector<Learner> &learners)
{
    ui->tableWidget_viewLearnersList->setRowCount(0);
    for (const Learner &learner : std::as_const(learners)) {
        int row = ui->tableWidget_viewLearnersList->rowCount();
        ui->tableWidget_viewLearnersList->insertRow(row);
        ui->tableWidget_viewLearnersList
            ->setItem(row, 0, new QTableWidgetItem(QString::number(learner.getId())));
        ui->tableWidget_viewLearnersList->setItem(row, 1, new QTableWidgetItem(learner.getName()));
        ui->tableWidget_viewLearnersList->setItem(row,
                                                  2,
                                                  new QTableWidgetItem(learner.getSurname()));
        ui->tableWidget_viewLearnersList->setItem(row, 3, new QTableWidgetItem(learner.getGrade()));
        ui->tableWidget_viewLearnersList
            ->setItem(row, 4, new QTableWidgetItem(learner.getDateOfBirth().toString("dd/MM/yyyy")));
        ui->tableWidget_viewLearnersList->setItem(row,
                                                  5,
                                                  new QTableWidgetItem(learner.getContactNo()));
    }
    justifyTableContents(ui->tableWidget_viewLearnersList, {1, 2});
}

void MainWindow::populateTransactionsTable(const QVector<Transaction> &transactions)
{
    ui->tableWidget_transactionHistory->setRowCount(0);
    for (const Transaction &trans : std::as_const(transactions)) {
        Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
        int row = ui->tableWidget_transactionHistory->rowCount();
        ui->tableWidget_transactionHistory->insertRow(row);
        ui->tableWidget_transactionHistory->setItem(row,
                                                    0,
                                                    new QTableWidgetItem(trans.getBookCode()));
        ui->tableWidget_transactionHistory->setItem(row, 1, new QTableWidgetItem(book.getTitle()));
        ui->tableWidget_transactionHistory
            ->setItem(row, 2, new QTableWidgetItem(trans.getBorrowDate().toString("dd/MM/yyyy")));
        ui->tableWidget_transactionHistory
            ->setItem(row, 3, new QTableWidgetItem(trans.getDueDate().toString("dd/MM/yyyy")));
        ui->tableWidget_transactionHistory
            ->setItem(row,
                      4,
                      new QTableWidgetItem(trans.getReturnDate().isValid()
                                               ? trans.getReturnDate().toString("dd/MM/yyyy")
                                               : "Not Returned"));
        ui->tableWidget_transactionHistory->setItem(row,
                                                    5,
                                                    new QTableWidgetItem(trans.getStatusString()));
        ui->tableWidget_transactionHistory
            ->setItem(row,
                      6,
                      new QTableWidgetItem(
                          trans.isOverdue() ? QString::number(trans.getDaysOverdue()) : "-"));
    }
    justifyTableContents(ui->tableWidget_transactionHistory, {1});
}

void MainWindow::populateReturnBooksTable(const QVector<Transaction> &transactions)
{
    ui->tableWidget_returnBooks->setRowCount(0);
    for (const Transaction &trans : std::as_const(transactions)) {
        Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
        int row = ui->tableWidget_returnBooks->rowCount();
        ui->tableWidget_returnBooks->insertRow(row);
        ui->tableWidget_returnBooks
            ->setItem(row, 0, new QTableWidgetItem(QString::number(trans.getLearnerId())));
        ui->tableWidget_returnBooks->setItem(row, 1, new QTableWidgetItem(trans.getBookCode()));
        ui->tableWidget_returnBooks->setItem(row, 2, new QTableWidgetItem(book.getTitle()));
        ui->tableWidget_returnBooks
            ->setItem(row, 3, new QTableWidgetItem(trans.getBorrowDate().toString("dd/MM/yyyy")));
        ui->tableWidget_returnBooks
            ->setItem(row, 4, new QTableWidgetItem(trans.getDueDate().toString("dd/MM/yyyy")));
        ui->tableWidget_returnBooks->setItem(row,
                                             5,
                                             new QTableWidgetItem(trans.isOverdue() ? "OVERDUE"
                                                                                    : "Active"));
    }
    justifyTableContents(ui->tableWidget_returnBooks, {2});
}

void MainWindow::populateDashboardTransactions(const QVector<Transaction> &transactions)
{
    ui->tableWidget_homeRecentTransactions->setRowCount(0);
    for (const Transaction &trans : std::as_const(transactions)) {
        Learner learner = DatabaseManager::instance().getLearnerById(trans.getLearnerId());
        Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
        int row = ui->tableWidget_homeRecentTransactions->rowCount();
        ui->tableWidget_homeRecentTransactions->insertRow(row);
        ui->tableWidget_homeRecentTransactions->setItem(row,
                                                        0,
                                                        new QTableWidgetItem(learner.getFullName()));
        ui->tableWidget_homeRecentTransactions->setItem(row,
                                                        1,
                                                        new QTableWidgetItem(book.getTitle()));
        ui->tableWidget_homeRecentTransactions
            ->setItem(row, 2, new QTableWidgetItem(trans.getBorrowDate().toString("dd/MM/yyyy")));
        ui->tableWidget_homeRecentTransactions
            ->setItem(row,
                      3,
                      new QTableWidgetItem(trans.getReturnDate().isValid() ? "Return" : "Borrow"));
        ui->tableWidget_homeRecentTransactions
            ->setItem(row, 4, new QTableWidgetItem(trans.getStatusString()));
    }
    justifyTableContents(ui->tableWidget_homeRecentTransactions, {0, 1});
}

void MainWindow::populateCurrentlyBorrowedBooks(int learnerId)
{
    ui->tableWidget_currentlyBorrowedBooks->setRowCount(0);
    QVector<Transaction> activeTransactions = DatabaseManager::instance()
                                                  .getActiveTransactionsByLearnerId(learnerId);

    for (const Transaction &trans : std::as_const(activeTransactions)) {
        Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
        int row = ui->tableWidget_currentlyBorrowedBooks->rowCount();
        ui->tableWidget_currentlyBorrowedBooks->insertRow(row);
        ui->tableWidget_currentlyBorrowedBooks->setItem(row,
                                                        0,
                                                        new QTableWidgetItem(book.getTitle()));
        ui->tableWidget_currentlyBorrowedBooks
            ->setItem(row, 1, new QTableWidgetItem(trans.getBorrowDate().toString("dd/MM/yyyy")));
        ui->tableWidget_currentlyBorrowedBooks
            ->setItem(row, 2, new QTableWidgetItem(trans.getDueDate().toString("dd/MM/yyyy")));
        int daysLeft = QDate::currentDate().daysTo(trans.getDueDate());
        ui->tableWidget_currentlyBorrowedBooks
            ->setItem(row,
                      3,
                      new QTableWidgetItem(daysLeft >= 0 ? QString::number(daysLeft) : "OVERDUE"));
        ui->tableWidget_currentlyBorrowedBooks
            ->setItem(row, 4, new QTableWidgetItem(trans.isOverdue() ? "Overdue" : "Active"));
    }
    justifyTableContents(ui->tableWidget_currentlyBorrowedBooks, {0});
}

// ==================== Form Management ====================

void MainWindow::clearBookForm()
{
    ui->lineEdit_bookCode->clear();
    ui->lineEdit_bookISBN->clear();
    ui->lineEdit_bookTitle->clear();
    ui->lineEdit_bookAuthor->clear();
    ui->comboBox_bookSubject->setCurrentIndex(0);
    ui->comboBox_bookGrade->setCurrentIndex(0);
    ui->doubleSpinBox_bookPrice->setValue(0.0);
    m_selectedBookCode.clear();
}

void MainWindow::clearLearnerForm()
{
    ui->lineEdit_addLearnerName->clear();
    ui->lineEdit_addLearnerSurname->clear();
    ui->lineEdit_addLearnerContact->clear();
    ui->comboBox_addLearnerGrade->setCurrentIndex(0);
    ui->dateEdit_addLearnerDOB->setDate(QDate::currentDate().addYears(-12));
    m_selectedLearnerId = -1;
}

void MainWindow::clearBorrowForm()
{
    ui->lineEdit_borrowBookCode->clear();
    ui->label_borrowBookDisplay->clear();
    ui->label_borrowBookAuthorDisplay->clear();
    ui->label_borrowBookISBNDisplay->clear();
    ui->label_borrowBookStatusDisplay->clear();
    ui->label_borrowBookCount->clear();
    ui->dateEdit_borrowDate->setDate(QDate::currentDate());
    ui->dateEdit_dueDate->setDate(Transaction::calculateDueDate(QDate::currentDate()));
    m_selectedLearnerId = -1;
    m_selectedBookCode.clear();
}

void MainWindow::clearReturnForm()
{
    ui->lineEdit_returnSearch->clear();
    ui->label_returnSelectedBook->clear();
    ui->tableWidget_returnBooks->setRowCount(0);
    ui->dateEdit_returnDate->setDate(QDate::currentDate());
    m_selectedTransactionKey = TransactionKey{};
}

void MainWindow::clearRegistrationForm()
{
    ui->lineEdit_regName->clear();
    ui->lineEdit_regSurname->clear();
    ui->lineEdit_regEmail->clear();
    ui->lineEdit_regContactNo->clear();
    ui->lineEdit_regSchoolName->clear();
    ui->lineEdit_regPassword->clear();
    ui->lineEdit_regRepeatPassword->clear();
    ui->lineEdit_regSecurityAnswer->clear();
    ui->QComboBox_regRole->setCurrentIndex(0);
    ui->QComboBox_securityQuestion->setCurrentIndex(0);
}

void MainWindow::fillBookForm(const Book &book)
{
    ui->label_editBookCode->setText(book.getBookCode());
    ui->lineEdit_editBookISBN->setText(book.getIsbn());
    ui->lineEdit_editBookTitle->setText(book.getTitle());
    ui->lineEdit_editBookAuthor->setText(book.getAuthor());
    ui->comboBox_editBookSubject->setCurrentText(book.getSubject());
    ui->comboBox_editBookGrade->setCurrentText(book.getGrade());
    ui->doubleSpinBox_editBookPrice->setValue(book.getPrice());
    ui->comboBox_editBookStatus->setCurrentText(book.getStatusString());
}

void MainWindow::fillLearnerForm(const Learner &learner)
{
    ui->lineEdit_editLearnerFirstName->setText(learner.getName());
    ui->lineEdit_editLearnerSurname->setText(learner.getSurname());
    ui->comboBox_editLearnerGrade->setCurrentText(learner.getGrade());
    ui->dateEdit_editLearnerDOB->setDate(learner.getDateOfBirth());
    ui->lineEdit_editLearnerContact->setText(learner.getContactNo());
}

void MainWindow::copyBookDataFromISBN()
{
    QString isbn = ui->lineEdit_bookISBN->text().trimmed();
    if (isbn.isEmpty()) {
        showErrorMessage("Please enter an ISBN first");
        return;
    }

    QVector<Book> allBooks = DatabaseManager::instance().getAllBooks();
    for (const Book &book : std::as_const(allBooks)) {
        if (book.getIsbn() == isbn) {
            ui->lineEdit_bookTitle->setText(book.getTitle());
            ui->lineEdit_bookAuthor->setText(book.getAuthor());
            ui->comboBox_bookSubject->setCurrentText(book.getSubject());
            ui->comboBox_bookGrade->setCurrentText(book.getGrade());
            ui->doubleSpinBox_bookPrice->setValue(book.getPrice());
            showSuccessMessage("Book data copied from ISBN. Please enter a unique book code.");
            return;
        }
    }
    showInfoMessage("No existing book found with this ISBN. Please fill in all fields.");
}

// ==================== Search & Filter ====================

void MainWindow::searchBooks(const QString &searchTerm)
{
    if (searchTerm.isEmpty()) {
        loadAllBooks();
        return;
    }
    populateBooksTable(DatabaseManager::instance().searchBooks(searchTerm));
}

void MainWindow::filterBooksByGrade(const QString &grade)
{
    populateBooksTable(DatabaseManager::instance().getBooksByGrade(grade));
}

void MainWindow::searchLearners(const QString &searchTerm)
{
    if (searchTerm.isEmpty()) {
        loadAllLearners();
        return;
    }
    populateLearnersTable(DatabaseManager::instance().searchLearners(searchTerm));
}

void MainWindow::filterLearnersByGrade(const QString &grade)
{
    populateLearnersTable(DatabaseManager::instance().getLearnersByGrade(grade));
}

void MainWindow::updateQuickSearchResults(const QString &searchTerm)
{
    QString trimmedSearchTerm = searchTerm.trimmed();
    QStringList results;

    if (trimmedSearchTerm.length() < 2) {
        m_quickSearchModel->setStringList(results);
        return;
    }

    const QString needle = trimmedSearchTerm.toLower();

    for (const Learner &learner : DatabaseManager::instance().getAllLearners()) {
        QString haystack = QString("%1 %2 %3 %4")
                               .arg(learner.getFullName(),
                                    learner.getGrade(),
                                    learner.getContactNo(),
                                    QString::number(learner.getId()))
                               .toLower();

        if (haystack.contains(needle)) {
            results.append(QString("Learner: %1 | Grade %2 | ID %3")
                               .arg(learner.getFullName(),
                                    learner.getGrade(),
                                    QString::number(learner.getId())));
        }
    }

    for (const Book &book : DatabaseManager::instance().getAllBooks()) {
        QString haystack = QString("%1 %2 %3 %4 %5")
                               .arg(book.getTitle(),
                                    book.getBookCode(),
                                    book.getAuthor(),
                                    book.getSubject(),
                                    book.getIsbn())
                               .toLower();

        if (haystack.contains(needle)) {
            results.append(QString("Book: %1 | Code %2 | Grade %3 | Status %4")
                               .arg(book.getTitle(),
                                    book.getBookCode(),
                                    book.getGrade(),
                                    book.getStatusString()));
        }
    }

    for (const Transaction &transaction : DatabaseManager::instance().getAllTransactions()) {
        Learner learner = DatabaseManager::instance().getLearnerById(transaction.getLearnerId());
        Book book = DatabaseManager::instance().getBookByCode(transaction.getBookCode());
        QString haystack = QString("%1 %2 %3 %4 %5 %6")
                               .arg(learner.getFullName(),
                                    QString::number(transaction.getLearnerId()),
                                    book.getTitle(),
                                    transaction.getBookCode(),
                                    transaction.getBorrowDate().toString("dd/MM/yyyy"),
                                    transaction.getStatusString())
                               .toLower();

        if (haystack.contains(needle)) {
            results.append(QString("Transaction: %1 - %2 | Learner %3 | Book %4 | %5 | %6")
                               .arg(learner.getFullName(),
                                    book.getTitle(),
                                    QString::number(transaction.getLearnerId()),
                                    transaction.getBookCode(),
                                    transaction.getBorrowDate().toString("dd/MM/yyyy"),
                                    transaction.getStatusString()));
        }
    }

    results.removeDuplicates();
    results.sort(Qt::CaseInsensitive);

    const int maximumResults = 30;
    if (results.size() > maximumResults) {
        results = results.mid(0, maximumResults);
    }

    m_quickSearchModel->setStringList(results);
    m_quickSearchCompleter->complete();
}

// ==================== Report Generation ====================

void MainWindow::generateBorrowReport(int learnerId)
{
    ui->textEdit_reportPreview->setHtml(generateReportHTML(learnerId, "Borrow"));
}

void MainWindow::generateReturnReport(int learnerId)
{
    ui->textEdit_reportPreview->setHtml(generateReportHTML(learnerId, "Return"));
}

QString MainWindow::generateReportHTML(int learnerId, const QString &reportType)
{
    Learner learner = DatabaseManager::instance().getLearnerById(learnerId);
    QVector<Transaction> transactions = DatabaseManager::instance().getTransactionsByLearnerId(
        learnerId);
    QString logoBase64 = getLogoAsBase64();

    QString html = "<html><head><style>";
    html += "body { font-family: Arial, sans-serif; }";
    html += "h2 { color: #2c3e50; }";
    html += "table { border-collapse: collapse; width: 100%; margin-top: 20px; margin-bottom: "
            "20px; }";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    html += "th { background-color: #3498db; color: white; }";
    html += ".logo img { max-width: 220px; height: auto; }";
    html += ".logo { max-width: 220px; height: auto; margin-bottom: 40px; }";
    html += "</style></head><body>";

    html += "<div class='logo'><img src='" + logoBase64 + "' alt='Logo'></div>";
    html += "<h2>Library " + reportType + " Report</h2>";
    html += "<p><strong>Learner:</strong> " + learner.getFullName() + "</p>";
    html += "<p><strong>Grade:</strong> " + learner.getGrade() + "</p>";
    html += "<p><strong>Date:</strong> " + QDate::currentDate().toString("dd MMMM yyyy") + "</p>";
    html += "<p><strong>Time:</strong> " + QTime::currentTime().toString("hh:mm") + "</p>";
    html += "<hr>";

    if (reportType == "Borrow") {
        html += "<h3>Currently Borrowed Books</h3>";
        QVector<Transaction> active = DatabaseManager::instance().getActiveTransactionsByLearnerId(
            learnerId);

        if (active.isEmpty()) {
            html += "<p>No books currently borrowed.</p>";
        } else {
            html += "<table><tr><th>Book "
                    "Code</th><th>Title</th><th>Subject</th><th>Author</th><th>Borrow "
                    "Date</th><th>Due Date</th><th>Status</th></tr>";
            for (const Transaction &trans : std::as_const(active)) {
                Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
                html += "<tr><td>" + book.getBookCode() + "</td><td>" + book.getTitle()
                        + "</td><td>" + book.getSubject() + "</td><td>" + book.getAuthor()
                        + "</td><td>" + trans.getBorrowDate().toString("dd/MM/yyyy") + "</td><td>"
                        + trans.getDueDate().toString("dd/MM/yyyy") + "</td><td>"
                        + (trans.isOverdue() ? "OVERDUE" : "Active") + "</td></tr>";
            }
            html += "</table>";
        }
    } else {
        html += "<h2>Return History</h2>";
        QVector<Transaction> returned;
        for (const Transaction &trans : std::as_const(transactions)) {
            if (trans.isReturned() || trans.isLost())
                returned.append(trans);
        }

        if (returned.isEmpty()) {
            html += "<p>No return history available.</p>";
        } else {
            html += "<table><tr><th>Book Code</th><th>Title</th><th>Subject</th><th>Borrow "
                    "Date</th><th>Return Date</th><th>Status</th></tr>";
            for (const Transaction &trans : std::as_const(returned)) {
                Book book = DatabaseManager::instance().getBookByCode(trans.getBookCode());
                html += "<tr><td>" + book.getBookCode() + "</td><td>" + book.getTitle()
                        + "</td><td>" + book.getSubject() + "</td><td>"
                        + trans.getBorrowDate().toString("dd/MM/yyyy") + "</td><td>"
                        + trans.getReturnDate().toString("dd/MM/yyyy") + "</td><td>"
                        + trans.getStatusString() + "</td></tr>";
            }
            html += "</table>";
        }
    }

    double totalDue = DatabaseManager::instance().calculateUnreturnedBooksAmount(learnerId);
    if (totalDue > 0) {
        html += "<hr><p><strong>Total Amount Due:</strong> R" + QString::number(totalDue, 'f', 2)
                + "</p>";
    }

    html += "<p><strong>Assisted By:</strong> "
            + AuthManager::instance().getCurrentUser().getFullName() + "</p>";
    html += "</body></html>";
    return html;
}

void MainWindow::printReport()
{
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) {
        ui->textEdit_reportPreview->document()->print(&printer);
        showSuccessMessage("Report printed successfully!");
    }
}

void MainWindow::saveReportAsPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Report as PDF",
                                                    "",
                                                    "PDF Files (*.pdf)");
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
            fileName += ".pdf";
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        ui->textEdit_reportPreview->document()->print(&printer);
        showSuccessMessage("Report saved as PDF: " + fileName);
    }
}

// ==================== Receipt ====================

QString MainWindow::generateReceiptHTML(const Payments &payment)
{
    Learner learner = DatabaseManager::instance().getLearnerById(payment.getLearnerId());
    User processor = DatabaseManager::instance().getUserById(payment.getProcessedBy());
    QVector<PaymentItem> items = DatabaseManager::instance().getPaymentItems(payment.getId());
    QString logoBase64 = getLogoAsBase64();

    QString html = "<html><head><style>";
    html += "body { font-family: Arial, sans-serif; }";
    html += "h2 { color: #2c3e50; text-align: left; }";
    html += "h3 { color: #34495e; border-bottom: 2px solid #3498db; padding-bottom: 5px; }";
    html += "table { border-collapse: collapse; width: 100%; margin-top: 10px; }";
    html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    html += "th { background-color: #3498db; color: white; }";
    html += ".info-section { margin-top: 20px; }";
    html += ".total { font-size: 18px; font-weight: bold; text-align: right; margin-top: 20px; }";
    html += ".logo img { max-width: 220px; height: auto; }";
    html += ".logo { max-width: 220px; height: auto; margin-bottom: 40px; }";
    html += "</style></head><body>";

    html += "<div class='logo'><img src='" + logoBase64 + "' alt='Logo'></div>";
    html += "<h2>PAYMENT RECEIPT</h2>";
    html += "<div class='info-section'>";
    html += "<p><strong>Receipt No:</strong> " + payment.getReceiptNo() + "</p>";
    html += "<p><strong>Date:</strong> "
            + payment.getPaymentDate().toString("dd MMMM yyyy hh:mm AP") + "</p>";
    html += "<p><strong>Learner:</strong> " + learner.getFullName()
            + " (ID: " + QString::number(learner.getId()) + ")</p>";
    html += "<p><strong>Grade:</strong> " + learner.getGrade() + "</p>";
    html += "<p><strong>Processed By:</strong> " + processor.getFullName() + "</p>";
    html += "</div>";

    html += "<h3>Payment Details</h3>";
    html += "<table><tr><th>Book Code</th><th>Book Title</th><th>Amount</th></tr>";

    double total = 0.0;
    for (const PaymentItem &item : std::as_const(items)) {
        Book book = DatabaseManager::instance().getBookByCode(item.getBookCode());
        html += "<tr><td>" + book.getBookCode() + "</td><td>" + book.getTitle() + "</td><td>R"
                + QString::number(item.getAmount(), 'f', 2) + "</td></tr>";
        total += item.getAmount();
    }

    html += "</table>";
    html += "<div class='total'>Total Amount: R" + QString::number(total, 'f', 2) + "</div>";

    if (!payment.getNotes().isEmpty()) {
        html += "<div class='info-section'><p><strong>Notes:</strong> " + payment.getNotes()
                + "</p></div>";
    }

    html += "<hr style='margin-top: 50px;'>";
    html += "<p style='text-align: center; font-size: 12px; color: #7f8c8d;'>";
    html += "Thank you for your payment<br>LibraCore Library Management System";
    html += "</p></body></html>";
    return html;
}

QString MainWindow::getLogoAsBase64()
{
    QFile file(":/icons/icons/utils/seagotle.png");
    if (!file.open(QIODevice::ReadOnly))
        return "";
    return "data:image/png;base64," + file.readAll().toBase64();
}

// ==================== User Profile ====================

void MainWindow::initializeProfilePage()
{
    loadUserProfile();
}

void MainWindow::loadUserProfile()
{
    User currentUser = AuthManager::instance().getCurrentUser();
    if (currentUser.getId() == -1) {
        QMessageBox::warning(this, "Error", "No user is logged in.");
        return;
    }

    QString initialLastName = currentUser.getName().isEmpty()
                                  ? currentUser.getSurname()
                                  : currentUser.getName().left(1).toUpper() + ". "
                                        + currentUser.getSurname();

    ui->label_profileInitialLast->setText(initialLastName);
    ui->label_profile_name->setText(currentUser.getName() + " " + currentUser.getSurname());
    ui->label_profile_email->setText(currentUser.getEmail());
    ui->label_profile_contact->setText(currentUser.getContactNo());
    ui->label_profile_school->setText(currentUser.getSchoolName());
    ui->label_profile_role->setText(currentUser.getRoleString());
    ui->label_profile_created_value->setText(currentUser.getCreatedAt().toString("yyyy-MM-dd"));

    QDateTime lastLogin = DatabaseManager::instance().getUserLastLogin(currentUser.getId());
    ui->label_profile_lastLogin_value->setText(
        lastLogin.isValid() ? lastLogin.toString("yyyy-MM-dd hh:mm AP") : "Never");

    updateProfileIcon();
}

void MainWindow::updateProfileIcon()
{
    User currentUser = AuthManager::instance().getCurrentUser();
    QString initialLastName = currentUser.getName().isEmpty()
                                  ? currentUser.getSurname()
                                  : currentUser.getName().left(1).toUpper() + ". "
                                        + currentUser.getSurname();

    ui->label_profileInitialLast->setText(initialLastName);
    ui->label_profile_name->setText(currentUser.getName() + " " + currentUser.getSurname());
    ui->label_profile_role->setText(currentUser.getRoleString());
}

void MainWindow::updateSecurityInfo() {}

void MainWindow::on_pushButton_editUserProfile_clicked()
{
    m_isAdminEditingUser = false;
    m_isAdminChangingUserPassword = false;
    showEditUserInfo();

    User currentUser = AuthManager::instance().getCurrentUser();
    ui->lineEdit_editProfile_name->setText(currentUser.getName());
    ui->lineEdit_editProfile_surname->setText(currentUser.getSurname());
    ui->lineEdit_editProfile_contact->setText(currentUser.getContactNo());
    ui->lineEdit_editProfile_email->setText(currentUser.getEmail());
    ui->lineEdit_editProfile_name->setFocus();
}

void MainWindow::on_pushButton_confirmEditProfile_clicked()
{
    QString name = ui->lineEdit_editProfile_name->text().trimmed();
    QString surname = ui->lineEdit_editProfile_surname->text().trimmed();
    QString contact = ui->lineEdit_editProfile_contact->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "First name cannot be empty.");
        ui->lineEdit_editProfile_name->setFocus();
        return;
    }
    if (!isPersonNameValid(name)) {
        QMessageBox::warning(this, "Validation Error", "First name must contain letters only.");
        ui->lineEdit_editProfile_name->setFocus();
        return;
    }
    if (surname.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Surname cannot be empty.");
        ui->lineEdit_editProfile_surname->setFocus();
        return;
    }
    if (!isPersonNameValid(surname)) {
        QMessageBox::warning(this, "Validation Error", "Surname must contain letters only.");
        ui->lineEdit_editProfile_surname->setFocus();
        return;
    }
    if (ui->lineEdit_editProfile_email->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Email cannot be empty.");
        ui->lineEdit_editProfile_email->setFocus();
        return;
    }

    QString email = ui->lineEdit_editProfile_email->text().trimmed();
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!emailRegex.match(email).hasMatch()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a valid email address.");
        ui->lineEdit_editProfile_email->setFocus();
        return;
    }

    if (!isContactNumberValid(contact)) {
        QMessageBox::warning(this,
                             "Validation Error",
                             "Contact number must be 10 digits and start with 0.");
        ui->lineEdit_editProfile_contact->setFocus();
        return;
    }

    User userToUpdate = m_isAdminEditingUser ? getSelectedAdminUser()
                                             : AuthManager::instance().getCurrentUser();
    if (userToUpdate.getId() == -1) {
        showErrorMessage("Please select a user to edit");
        showAdminLobby();
        return;
    }
    if (email != userToUpdate.getEmail() && DatabaseManager::instance().userExists(email)) {
        QMessageBox::warning(this, "Validation Error", "Email already registered.");
        ui->lineEdit_editProfile_email->setFocus();
        return;
    }

    userToUpdate.setName(name);
    userToUpdate.setSurname(surname);
    userToUpdate.setEmail(email);
    userToUpdate.setContactNo(contact);

    if (DatabaseManager::instance().updateUser(userToUpdate)) {
        if (m_isAdminEditingUser) {
            DatabaseManager::instance().logUserActivity(
                AuthManager::instance().getCurrentUserId(),
                "Edit User",
                QString("Edited user ID %1: %2 <%3>")
                    .arg(userToUpdate.getId())
                    .arg(userToUpdate.getFullName(), userToUpdate.getEmail()));
            m_isAdminEditingUser = false;
            QMessageBox::information(this, "Success", "User updated successfully!");
            showAdminLobby();
        } else {
            DatabaseManager::instance().logUserActivity(userToUpdate.getId(),
                                                        "Update Profile",
                                                        "Profile information updated");
            QMessageBox::information(this, "Success", "Profile updated successfully!");
            showUsersPage();
        }
    } else {
        QMessageBox::critical(this,
                              "Error",
                              "Failed to update profile: "
                                  + DatabaseManager::instance().getLastError());
    }
}

void MainWindow::on_pushButton_cancelEditProfile_clicked()
{
    ui->lineEdit_editProfile_name->clear();
    ui->lineEdit_editProfile_surname->clear();
    ui->lineEdit_editProfile_contact->clear();
    ui->lineEdit_editProfile_email->clear();
    if (m_isAdminEditingUser) {
        m_isAdminEditingUser = false;
        showAdminLobby();
    } else {
        showUsersPage();
    }
}

void MainWindow::on_pushButton_changeUserPassword_clicked()
{
    m_isAdminEditingUser = false;
    m_isAdminChangingUserPassword = false;
    showChangeUserPasswordPage();
    ui->lineEdit_currentPassword->setEnabled(true);
    ui->lineEdit_currentPassword->setPlaceholderText(QString());
    ui->lineEdit_currentPassword->clear();
    ui->lineEdit_newPassword->clear();
    ui->lineEdit_repeatPassword->clear();
    ui->lineEdit_currentPassword->setFocus();
}

void MainWindow::on_pushButton_confirmChangeUserPassword_clicked()
{
    QString currentPassword = ui->lineEdit_currentPassword->text();
    QString newPassword = ui->lineEdit_newPassword->text();
    QString repeatPassword = ui->lineEdit_repeatPassword->text();

    if (!m_isAdminChangingUserPassword && currentPassword.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter your current password.");
        ui->lineEdit_currentPassword->setFocus();
        return;
    }
    if (newPassword.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter a new password.");
        ui->lineEdit_newPassword->setFocus();
        return;
    }
    QString passwordError = AuthManager::instance().validatePassword(newPassword);
    if (!passwordError.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", passwordError);
        ui->lineEdit_newPassword->setFocus();
        return;
    }
    if (newPassword != repeatPassword) {
        QMessageBox::warning(this, "Validation Error", "New passwords do not match.");
        ui->lineEdit_repeatPassword->clear();
        ui->lineEdit_repeatPassword->setFocus();
        return;
    }
    if (!m_isAdminChangingUserPassword && currentPassword == newPassword) {
        QMessageBox::warning(this,
                             "Validation Error",
                             "New password must be different from current password.");
        ui->lineEdit_newPassword->clear();
        ui->lineEdit_repeatPassword->clear();
        ui->lineEdit_newPassword->setFocus();
        return;
    }

    User currentUser = AuthManager::instance().getCurrentUser();

    if (m_isAdminChangingUserPassword) {
        User selectedUser = getSelectedAdminUser();
        if (selectedUser.getId() == -1) {
            showErrorMessage("Please select a user");
            showAdminLobby();
            return;
        }
        if (!DatabaseManager::instance().changeUserPassword(selectedUser.getId(), newPassword)) {
            QMessageBox::warning(this, "Error", DatabaseManager::instance().getLastError());
            return;
        }

        DatabaseManager::instance().logUserActivity(
            currentUser.getId(),
            "Change User Password",
            QString("Reset password for user ID %1: %2 <%3>")
                .arg(selectedUser.getId())
                .arg(selectedUser.getFullName(), selectedUser.getEmail()));
        m_isAdminChangingUserPassword = false;
        clearChangePasswordForm();
        ui->lineEdit_currentPassword->setEnabled(true);
        ui->lineEdit_currentPassword->setPlaceholderText(QString());
        QMessageBox::information(this, "Success", "Password reset successfully!");
        showAdminLobby();
        return;
    }

    if (!AuthManager::instance().changePassword(currentPassword, newPassword)) {
        QMessageBox::warning(this, "Authentication Error", AuthManager::instance().getLastError());
        ui->lineEdit_currentPassword->clear();
        ui->lineEdit_currentPassword->setFocus();
        return;
    }

    DatabaseManager::instance().logUserActivity(currentUser.getId(),
                                                "Change Password",
                                                "Password changed successfully");
    clearChangePasswordForm();
    QMessageBox::information(this, "Success", "Password changed successfully!");
    showUsersPage();
}

void MainWindow::on_pushButton_cancelBackToUserProfile_clicked()
{
    clearChangePasswordForm();
    ui->lineEdit_currentPassword->setEnabled(true);
    ui->lineEdit_currentPassword->setPlaceholderText(QString());
    if (m_isAdminChangingUserPassword) {
        m_isAdminChangingUserPassword = false;
        showAdminLobby();
    } else {
        showUsersPage();
    }
}

void MainWindow::clearChangePasswordForm()
{
    ui->lineEdit_currentPassword->clear();
    ui->lineEdit_newPassword->clear();
    ui->lineEdit_repeatPassword->clear();
}
