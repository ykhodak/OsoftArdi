#pragma once


class QByteArray;
class SyncPoint;
class ADbgTester;

namespace ard {
    struct SyncCrypto;

    enum class aes_status
    {
        ok,
        err,
        unsupported_archive,
        old_archive_key_tag,
        old_provided_key_tag,
        corrupted,
        decr_error,
        incorrect_old_pwd,
        file_access_error,
        file_not_found,
        password_not_defined,
        mem_alloc_error,
        account_is_undefined
    };

    struct aes_result 
    {
        aes_status  status;
        uint64_t    key_tag;
        QString     hint;
        QDate       encr_date;
    };

    struct gui_pwd_info 
    {
        QString password;
        QString old_password;
        QString hint;
    };

    QByteArray  aes_encrypt(QString pwd, const QByteArray& b);
    QByteArray  aes_decrypt(QString pwd, const QByteArray& b);
    std::pair<ard::aes_status, QByteArray>  aes_encrypt_archive(QString pwd, QString hint, const QByteArray& b);
    std::pair<ard::aes_result, QByteArray> aes_decrypt_archive(QString pwd, const QByteArray& b);


    std::pair<ard::aes_status, QString> aes_archive_recover_hint(QString file_name);

    QString get_crypto_config_dir();
    QString aes_status2str(ard::aes_status st);

    struct SyncCrypto {
        QByteArray      key;
        QByteArray      iv;
        uint64_t        tag;
        QString         hint;

        bool isValid()const;
        void clear();
    };

#ifdef ARD_OPENSSL
    ard::aes_status aes_archive_encrypt_file(QString sourceFile, QString encrFile);
    ard::aes_result aes_archive_decrypt_file(QString encrFile, QString destFile, SyncCrypto* enforce_key = nullptr);

    class CryptoConfig
    {
    public:
        static CryptoConfig& cfg();

        aes_status  reloadCryptoConfig();
        aes_status  storeCryptoConfig();
        /// delete crypto file and clear pwd
        aes_status  purgeCryptoConfig();
        void        clearConfig();
        aes_status  request2ChangeSyncPassword(QString pwd, QString old_pwd, QString hint);
        aes_status  tryOldSyncPassword(QString pwd);

        bool                        hasPassword()const;
        std::pair<bool, SyncCrypto> syncCrypto()const;

        bool                        hasPasswordChangeRequest()const;
        std::pair<bool, SyncCrypto> syncLimboCrypto()const;

        bool                        hasTryOldPassword()const;
        std::pair<bool, SyncCrypto> tryOldCrypto()const;

    protected:
        CryptoConfig();
        virtual ~CryptoConfig();

        /// we commit new pwd after first successfull sync
        aes_status          commitSyncPasswordChange();
        /// if our instance is old relative to master DB but pwd is still good
        /// we simply update our key tag up to master
        aes_status          promoteSyncPasswordKeyTag(uint64_t key_tag);
        /// we tried to decrypt old DB with old pwd, it probably worked, we don't need it any more
        aes_status          clearTryOldSyncPassword();
        void                toJson(QJsonObject& js)const;
        void                fromJson(const QJsonObject& js);

        static ard::CryptoConfig theCfg;
        SyncCrypto      m_sync_crypto;//this is current active pwd, used to encrypt master DB last time
        SyncCrypto      m_request2change_sync_crypto;//this pwd was set but remote DB not yet encrypted, after it's encrypts pwd will become m_sync_crypto
        SyncCrypto      m_try_old_pwd;//we are not persistant, just to encypr old arch and then encrypt with current

        friend class ::SyncPoint;
#ifdef _DEBUG
        friend class ::ADbgTester;
#endif
    };
#endif //ARD_OPENSSL
}