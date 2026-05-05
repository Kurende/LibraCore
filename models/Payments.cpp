#include "Payments.h"
#include <QDateTime>

Payments::Payments()
    : m_id(-1)
    , m_learnerId(-1)
    , m_amount(0.0)
    , m_processedBy(-1)
    , m_paymentDate(QDateTime::currentDateTime())
{
}

Payments::Payments(int id, const QString& receiptNo, int learnerId, double amount,
                 int processedBy, const QDateTime& paymentDate, const QString& notes)
    : m_id(id)
    , m_receiptNo(receiptNo)
    , m_learnerId(learnerId)
    , m_amount(amount)
    , m_processedBy(processedBy)
    , m_paymentDate(paymentDate)
    , m_notes(notes)
{
}

QString Payments::generateReceiptNo() {
   
    QDateTime now = QDateTime::currentDateTime();
    return QString("RCP-%1-%2")
        .arg(now.toString("yyyyMMdd"))
        .arg(now.toString("HHmmss"));
}
