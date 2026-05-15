#ifndef ENCRYPTION_H
#define ENCRYPTION_H
#include <QString>
#include <QCryptographicHash>
#include <QRandomGenerator>

class Encryption {
public:
    static QString hashPassword(const QString& password);
    static bool verifyPassword(const QString& password, const QString& storedHash);

    static QString encryptText(const QString& text);
    static QString decryptText(const QString& encryptedText);

private:
    Encryption();

    static QString generateSalt(int length = 32);
    static QString hashWithSalt(const QString& password, const QString& salt);
    static QString getEncryptionKey();
    static QString generateEncryptionKey();
    static QString encryptionKeyFilePath();
    static const char SALT_DELIMITER;
};

#endif // ENCRYPTION_H
