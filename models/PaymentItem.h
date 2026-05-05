#ifndef PAYMENTITEM_H
#define PAYMENTITEM_H
#include <QString>

class PaymentItem {
public:
    PaymentItem();
    PaymentItem(int id, int paymentId, int transactionId, const QString& bookCode, double amount);

    int getId() const { return m_id; }
    int getPaymentId() const { return m_paymentId; }
    int getTransactionId() const { return m_transactionId; }
    QString getBookCode() const { return m_bookCode; }
    double getAmount() const { return m_amount; }

    void setId(int id) { m_id = id; }
    void setPaymentId(int paymentId) { m_paymentId = paymentId; }
    void setTransactionId(int transactionId) { m_transactionId = transactionId; }
    void setBookCode(const QString& bookCode) { m_bookCode = bookCode; }
    void setAmount(double amount) { m_amount = amount; }

private:
    int m_id;
    int m_paymentId;
    int m_transactionId;
    QString m_bookCode;
    double m_amount;
};

#endif // PAYMENTITEM_H