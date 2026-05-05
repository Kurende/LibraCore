#ifndef PAYMENTS_H
#define PAYMENTS_H

#include <QString>
#include <QDateTime>

class Payments {
public:
    Payments();
    Payments(int id, const QString& receiptNo, int learnerId, double amount,
            int processedBy, const QDateTime& paymentDate, const QString& notes);

    int getId() const { return m_id; }
    QString getReceiptNo() const { return m_receiptNo; }
    int getLearnerId() const { return m_learnerId; }
    double getAmount() const { return m_amount; }
    int getProcessedBy() const { return m_processedBy; }
    QDateTime getPaymentDate() const { return m_paymentDate; }
    QString getNotes() const { return m_notes; }

    void setId(int id) { m_id = id; }
    void setReceiptNo(const QString& receiptNo) { m_receiptNo = receiptNo; }
    void setLearnerId(int learnerId) { m_learnerId = learnerId; }
    void setAmount(double amount) { m_amount = amount; }
    void setProcessedBy(int processedBy) { m_processedBy = processedBy; }
    void setPaymentDate(const QDateTime& paymentDate) { m_paymentDate = paymentDate; }
    void setNotes(const QString& notes) { m_notes = notes; }

    QString generateReceiptNo();

private:
    int m_id;
    QString m_receiptNo;
    int m_learnerId;
    double m_amount;
    int m_processedBy;
    QDateTime m_paymentDate;
    QString m_notes;
};

#endif // PAYMENTS_H