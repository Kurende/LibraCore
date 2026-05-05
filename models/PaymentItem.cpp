#include "PaymentItem.h"

PaymentItem::PaymentItem()
    : m_id(-1)
    , m_paymentId(-1)
    , m_transactionId(-1)
    , m_bookCode("")
    , m_amount(0.0)
{
}

PaymentItem::PaymentItem(int id, int paymentId, int transactionId, const QString& bookCode, double amount)
    : m_id(id)
    , m_paymentId(paymentId)
    , m_transactionId(transactionId)
    , m_bookCode(bookCode)
    , m_amount(amount)
{
}