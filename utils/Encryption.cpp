#include "Encryption.h"
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>

const char Encryption::SALT_DELIMITER = ':';

Encryption::Encryption() {
}

QString Encryption::generateSalt(int length) {
    const QString possibleCharacters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    QString salt;
    salt.reserve(length);

    for (int i = 0; i < length; ++i) {
        int index = QRandomGenerator::global()->bounded(possibleCharacters.length());
        salt.append(possibleCharacters.at(index));
    }

    return salt;
}

QString Encryption::hashWithSalt(const QString& password, const QString& salt) {
    QString saltedPassword = salt + password + salt;
    QByteArray data = saltedPassword.toUtf8();

    QByteArray firstPass  = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QByteArray secondPass = QCryptographicHash::hash(firstPass + data, QCryptographicHash::Sha512);

    return QString(secondPass.toHex());
}

QString Encryption::encryptionKeyFilePath() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + "/.libracore";
    }

    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return dir.filePath("encryption.key");
}

QString Encryption::generateEncryptionKey() {
    QByteArray key;
    key.resize(32);

    for (int i = 0; i < key.size(); i += 4) {
        quint32 value = QRandomGenerator::global()->generate();
        for (int j = 0; j < 4 && i + j < key.size(); ++j) {
            key[i + j] = static_cast<char>((value >> (j * 8)) & 0xFF);
        }
    }

    return QString::fromLatin1(key.toBase64());
}

QString Encryption::getEncryptionKey() {
    const QString keyPath = encryptionKeyFilePath();

    QFile existingKeyFile(keyPath);
    if (existingKeyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString key = QString::fromUtf8(existingKeyFile.readAll()).trimmed();
        if (!key.isEmpty()) {
            return key;
        }
    }

    QString key = generateEncryptionKey();
    QSaveFile newKeyFile(keyPath);
    if (newKeyFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        newKeyFile.write(key.toUtf8());
        newKeyFile.commit();
        QFile::setPermissions(keyPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    return key;
}

QString Encryption::hashPassword(const QString& password) {
    QString salt = generateSalt(32);
    QString hash = hashWithSalt(password, salt);
    return salt + SALT_DELIMITER + hash;
}

bool Encryption::verifyPassword(const QString& password, const QString& storedHash) {
    int delimiterIndex = storedHash.indexOf(SALT_DELIMITER);
    if (delimiterIndex == -1) {
        return false;
    }

    QString salt = storedHash.left(delimiterIndex);
    QString expectedHash = storedHash.mid(delimiterIndex + 1);

    return hashWithSalt(password, salt) == expectedHash;
}

QString Encryption::encryptText(const QString& text) {
    QByteArray textData = text.toUtf8();
    QByteArray keyData  = getEncryptionKey().toUtf8();
    QString    salt     = generateSalt(16);
    QByteArray saltData = salt.toUtf8();

    QByteArray encrypted;
    encrypted.reserve(textData.size());

    for (int i = 0; i < textData.size(); ++i) {
        char encryptedByte = textData[i]
                             ^ keyData[i % keyData.size()]
                             ^ saltData[i % saltData.size()];
        encrypted.append(encryptedByte);
    }

    QByteArray saltLengthByte;
    saltLengthByte.append(static_cast<char>(saltData.size()));

    return QString((saltLengthByte + saltData + encrypted).toBase64());
}

QString Encryption::decryptText(const QString& encryptedText) {
    QByteArray raw = QByteArray::fromBase64(encryptedText.toUtf8());

    if (raw.isEmpty()) {
        return QString();
    }

    int saltLength = static_cast<unsigned char>(raw[0]);
    if (raw.size() < 1 + saltLength) {
        return QString();
    }

    QByteArray saltData  = raw.mid(1, saltLength);
    QByteArray encrypted = raw.mid(1 + saltLength);
    QByteArray keyData   = getEncryptionKey().toUtf8();

    QByteArray decrypted;
    decrypted.reserve(encrypted.size());

    for (int i = 0; i < encrypted.size(); ++i) {
        char decryptedByte = encrypted[i]
                             ^ keyData[i % keyData.size()]
                             ^ saltData[i % saltData.size()];
        decrypted.append(decryptedByte);
    }

    return QString::fromUtf8(decrypted);
}
