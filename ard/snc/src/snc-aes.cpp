#include  <memory>
#include  <QByteArray>
#include  <QCryptographicHash>
#include  <QDebug>
#include  <QFile>
#include  <QJsonDocument>
#include  <QDataStream>
#include  <QJsonObject>
#include  <QDateTime>
#include  <QBuffer>
#include "snc-aes.h"
#include "snc-assert.h"

#ifdef ARD_OPENSSL
#include <openssl/aes.h>

extern QString aes_account_login();

//////////////////
/// based on https://github.com/jhnstrk/qtOpenSsl/blob/master/src/qcryptostream.h
//////////////////
class AesCrypt {
public:
    enum eAesLen { Aes128 = 16, Aes192 = 24, Aes256 = 32 };

    AesCrypt();
    ~AesCrypt();
    void initialize(const QByteArray & key, const QByteArray & initVec);
    void uninitialize();

    QByteArray encrypt(const QByteArray & input);
    QByteArray decrypt(const QByteArray & input);
private:
    class Private;
    std::unique_ptr<Private> d;
};


class AesCrypt::Private {
public:
    AesCrypt::eAesLen aesLen;
    QByteArray key;
    QByteArray initVec;

    AES_KEY enc_key;
    AES_KEY dec_key;
    unsigned char iv[AES_BLOCK_SIZE];
};

AesCrypt::AesCrypt() : d(new AesCrypt::Private)
{
    d->aesLen = AesCrypt::Aes256;
    ::memset(d->iv, 0, sizeof(d->iv));
    ::memset(&d->enc_key, 0, sizeof(AES_KEY));
    ::memset(&d->dec_key, 0, sizeof(AES_KEY));
};

AesCrypt::~AesCrypt()
{

};

void AesCrypt::uninitialize()
{
    ::memset(d->iv, 0, sizeof(d->iv));
    ::memset(&d->enc_key, 0, sizeof(AES_KEY));
    ::memset(&d->dec_key, 0, sizeof(AES_KEY));
};

void AesCrypt::initialize(const QByteArray & key, const QByteArray & initVec)
{
    d->key = key;
    d->initVec = initVec;

    if (key.size() != d->aesLen) {
        ASSERT(0, QString("Bad key length, expected %1 provided %2").arg(d->aesLen).arg(key.size()));
        //qWarning() << "Bad key length, expected" << d->aesLen;
        d->key.resize(d->aesLen);
    }

    if (initVec.size() != AES_BLOCK_SIZE) {
        ASSERT(0, QString("Bad key length, expected %1 provided %2").arg(AES_BLOCK_SIZE).arg(initVec.size()));
        //qWarning() << "Bad initialization vector length";
        d->initVec.resize(AES_BLOCK_SIZE);
    }

    int status = -1;
    status = AES_set_encrypt_key(reinterpret_cast<const unsigned char *>(d->key.constData()),
        d->aesLen * 8, &d->enc_key);
    if (status != 0) {
        qWarning() << "Status" << status;
    }

    status = AES_set_decrypt_key(reinterpret_cast<const unsigned char *>(d->key.constData()),
        d->aesLen * 8, &d->dec_key);
    if (status != 0) {
        qWarning() << "Status" << status;
    }

    ::memcpy(d->iv, d->initVec.constData(), AES_BLOCK_SIZE);
};

QByteArray AesCrypt::encrypt(const QByteArray & input)
{
    int numFullBlocks = (input.size()) / AES_BLOCK_SIZE;
    int numBlocks;
    int fullLen;

    ///we assume NoPadding
    numFullBlocks = (input.size() + (AES_BLOCK_SIZE - 1)) / AES_BLOCK_SIZE;
    numBlocks = numFullBlocks;
    fullLen = input.size();

    const int encsLen = numBlocks * AES_BLOCK_SIZE;
    QByteArray out;
    out.resize(encsLen);

    AES_cbc_encrypt(
        reinterpret_cast<const unsigned char *>(input.constData()),
        reinterpret_cast<unsigned char *>(out.data()),
        fullLen,
        &d->enc_key,
        d->iv,
        AES_ENCRYPT);

    return out;
}

QByteArray AesCrypt::decrypt(const QByteArray & input)
{
    QByteArray out;
    out.resize(input.size());

    AES_cbc_encrypt(
        reinterpret_cast<const unsigned char *>(input.constData()),
        reinterpret_cast<unsigned char *>(out.data()),
        input.size(),
        &d->dec_key,
        d->iv,
        AES_DECRYPT);

    return out;
}
#endif //ARD_OPENSSL

#define P1_KEY { 83,-16,91,75,-19,117,-128,-84,-71,-5,-21,-82,77,91,93,-113,19,17,-89,-57,-69,-127,106,110,111,45,-42,0,50,44,72,28 }
#define P1_IV {50,-26,24,-53,12,-48,-7,69,-23,7,73,-52,80,12,124,-22}

#define HINT_KEY { 53,16,-33,75,-14,117,-18,-84,-51,-5,-21,-82,37,91,93,-113,19,17,-89,-57,-69,-27,6,110,111,45,-42,0,50,44,12,28 }
#define HINT_IV {41,-16,55,-53,12,-95,-7,69,-23,2,93,62,40,13,120,-22}


#define DECLARE_KEY_IV(K, I)    char _key_buf[] = K;\
                                char _iv_buf[] = I;\
                                QByteArray key;\
                                QByteArray iv;\
                                for(const char& c : _key_buf)key.append(c);\
                                for(const char& c : _iv_buf)iv.append(c);\





static QByteArray aes_encrypt(const QByteArray& key, const QByteArray& iv, const QByteArray& b)
{
#ifdef ARD_OPENSSL
    AesCrypt _crypto;
    _crypto.initialize(key, iv);
    auto arr_zip = qCompress(b, 9);
    auto res = _crypto.encrypt(arr_zip);
    return res;
#else
    Q_UNUSED(key);
    Q_UNUSED(iv);
    Q_UNUSED(b);
    ASSERT(0, "openssl not enabled");
    return QByteArray();
#endif
}

static QByteArray aes_decrypt(const QByteArray& key, const QByteArray& iv, const QByteArray& b)
{
#ifdef ARD_OPENSSL
    AesCrypt _crypto;
    _crypto.initialize(key, iv);
    auto arr_zip = _crypto.decrypt(b);
    auto res = qUncompress(arr_zip);
    return res;
#else
    Q_UNUSED(key);
    Q_UNUSED(iv);
    Q_UNUSED(b);
    ASSERT(0, "openssl not enabled");
    return QByteArray();
#endif
}

static QByteArray encrypt_hint(QString hint)
{
    DECLARE_KEY_IV(HINT_KEY, HINT_IV);
    return aes_encrypt(key, iv, QByteArray(hint.toStdString().c_str()).toBase64());
};

static QString decrypt_hint(const QByteArray& b)
{
    DECLARE_KEY_IV(HINT_KEY, HINT_IV);
    auto d = aes_decrypt(key, iv, b);
    return QString::fromUtf8(QByteArray::fromBase64(d).toStdString().c_str());
};

union UE
{
    uint64_t val;
    struct
    {
        uint8_t v1;
        uint8_t v2;
        uint8_t v3;
        uint8_t v4;
        uint32_t rest;
    };
};


static QByteArray aes_archive_encrypt(const QByteArray& key, const QByteArray& iv, const QByteArray& b)
{
    QByteArray rv;
    QByteArray enc_arr = aes_encrypt(key, iv, b);
    auto content_hash = QCryptographicHash::hash(enc_arr, QCryptographicHash::Md5);
    ASSERT(content_hash.size() == 16, "expected 128 bid Md5");

    QDateTime dt = QDateTime::currentDateTime();
    auto hash2 = QCryptographicHash::hash(content_hash, QCryptographicHash::Sha256);

    QDataStream ds(&rv, QIODevice::WriteOnly);
    ds.setVersion(QDataStream::Qt_5_5);
    ds << content_hash;
    ds << dt;
    ds << hash2;
    ds << enc_arr;
    return rv;
};

static std::pair<ard::aes_result, QByteArray> aes_archive_decrypt(const QByteArray& key, const QByteArray& iv, const QByteArray& b)
{
    std::pair<ard::aes_result, QByteArray> rv;
    rv.first.status = ard::aes_status::err;
    rv.first.key_tag = 0;

    QByteArray content_hash, enc_arr, hash2;
    QDateTime dt;

    QDataStream ds(b);
    ds >> content_hash;
    ds >> dt;
    ds >> hash2;
    ds >> enc_arr;

    auto recalc_content_hash = QCryptographicHash::hash(enc_arr, QCryptographicHash::Md5);
    ASSERT(recalc_content_hash.size() == 16, "expected 128 bid Md5");
    if (content_hash != recalc_content_hash) {
        rv.first.status = ard::aes_status::corrupted;
        return rv;
    }

    QByteArray arr = aes_decrypt(key, iv, enc_arr);
    if (arr.size() == 0) {
        rv.first.status = ard::aes_status::decr_error;
    }
    else {
        rv.first.status = ard::aes_status::ok;
        rv.second = arr;
    }
    return rv;
};

/*
static std::pair<ard::aes_status, QString> recover_hint_from_enc_barray(const QByteArray& b)
{
    std::pair<ard::aes_status, QString> rv;
    rv.first = ard::aes_status::err;

    uint64_t ard_v_tag, arch_key_tag;
    QByteArray content_hash, enc_arr, hash2, hint;
    QDateTime dt;

    QDataStream ds(b);
    ds >> ard_v_tag;
    ds >> ard_v_tag;
    ds >> arch_key_tag;
    ds >> content_hash;
    ds >> hint;
    ds >> dt;
    ds >> hash2;
    ds >> enc_arr;

    rv.second = decrypt_hint(hint);

    auto recalc_content_hash = QCryptographicHash::hash(enc_arr, QCryptographicHash::Md5);
    ASSERT(recalc_content_hash.size() == 16, "expected 128 bid Md5");
    if (content_hash != recalc_content_hash) {
        rv.first = ard::aes_status::corrupted;
        return rv;
    }
    
    rv.first = ard::aes_status::ok;
    return rv;
};
*/

std::pair<ard::aes_status, QString> ard::aes_archive_recover_hint(QString file_name)
{
    std::pair<ard::aes_status, QString> rv;
    rv.first = ard::aes_status::err;

    if (!QFile::exists(file_name)) {
        rv.first = ard::aes_status::file_not_found;
        return rv;
    }

    QFile sfile(file_name);
    if (!sfile.open(QIODevice::ReadOnly))
    {
        ASSERT(0, "Failed to open file") << file_name;
        rv.first = ard::aes_status::file_access_error;
        return rv;
    }

    uint64_t ard_v_tag, arch_key_tag, hint_size, encr_date_val;
    QByteArray hint_barr;
    
    sfile.read((char *)&ard_v_tag, sizeof(uint64_t));
    sfile.read((char *)&arch_key_tag, sizeof(uint64_t));
    sfile.read((char *)&encr_date_val, sizeof(uint64_t));
    sfile.read((char *)&hint_size, sizeof(uint64_t));
    hint_barr = sfile.read(hint_size);

    rv.second = decrypt_hint(hint_barr);
    rv.first = ard::aes_status::ok;
    return rv;    
    /*
    auto encr_ar = sfile.readAll();
    sfile.close();
    return recover_hint_from_enc_barray(encr_ar);
    */
};

QString ard::aes_status2str(ard::aes_status st)
{
    QString rv = "";
    switch (st) {
    case ard::aes_status::ok:                   rv = ""; break;
    case ard::aes_status::err:                  rv = "Error"; break;
    case ard::aes_status::unsupported_archive:  rv = "Unsupported archive"; break;
    case ard::aes_status::old_archive_key_tag:  rv = "Old archive tag"; break;
    case ard::aes_status::old_provided_key_tag: rv = "Old tag"; break;
    case ard::aes_status::corrupted:            rv = "Corrupted archive"; break;
    case ard::aes_status::incorrect_old_pwd:    rv = "Incorrect old password"; break;
    case ard::aes_status::file_access_error:    rv = "File access error"; break;
    case ard::aes_status::file_not_found:       rv = "File not found"; break;
    case ard::aes_status::password_not_defined: rv = "Pasword not defined"; break;
    case ard::aes_status::mem_alloc_error:      rv = "New memory allocation error"; break;
    case ard::aes_status::account_is_undefined: rv = "Account is undefined"; break;
    case ard::aes_status::decr_error:           rv = "Decryption Error"; break;
    };

    return rv;
};


#define DECL_CRYPTO(P)  auto _key = QCryptographicHash::hash((P.toUtf8()), QCryptographicHash::Sha256);\
                        auto _iv = QCryptographicHash::hash((P.toUtf8()), QCryptographicHash::Md5);\
                        _iv = QCryptographicHash::hash(_iv, QCryptographicHash::Md5);\


QByteArray ard::aes_encrypt(QString pwd, const QByteArray& b)
{
    DECL_CRYPTO(pwd);
    return aes_encrypt(_key, _iv, b);
};

QByteArray ard::aes_decrypt(QString pwd, const QByteArray& b)
{
    DECL_CRYPTO(pwd);
    return aes_decrypt(_key, _iv, b);
};

ard::aes_status __aes_encrypt_archive_file(QIODevice& outFile,
    const QByteArray& key,
    const QByteArray& iv,
    const QByteArray& in_ar,
    uint64_t key_tag,
    QString pwd_hint)
{
    auto out_ar = aes_archive_encrypt(key, iv, in_ar);
    auto hint_barr = encrypt_hint(pwd_hint);

    QDate encrDate = QDate::currentDate();
    uint64_t ard_v_tag, hint_size, encr_date_val;
    UE uc;
    uc.v1 = '3';
    uc.v2 = 'a';
    uc.v3 = 'r';
    uc.v4 = 'd';
    uc.rest = 0;

    ard_v_tag = uc.val;
    hint_size = hint_barr.size();
    encr_date_val = encrDate.toJulianDay();

    outFile.write((char *)&ard_v_tag, sizeof(uint64_t));
    outFile.write((char *)&key_tag, sizeof(uint64_t));
    outFile.write((char *)&encr_date_val, sizeof(uint64_t));
    outFile.write((char *)&hint_size, sizeof(uint64_t));
    outFile.write(hint_barr);
    outFile.write(out_ar);
    auto sz = outFile.size();
    qDebug() << sz;
    //outFile.close();
    return ard::aes_status::ok;
}

std::pair<ard::aes_result, QByteArray> __aes_decrypt_archive_file(QIODevice& enFile,
    const QByteArray& key,
    const QByteArray& iv,
    uint64_t key_tag)
{
    std::pair<ard::aes_result, QByteArray> rv;
    rv.first.status = ard::aes_status::err;
    rv.first.key_tag = 0;

    auto fsz = enFile.size();
    if (fsz < 10) {
        rv.first.status = ard::aes_status::corrupted;
        return rv;
    }

    uint64_t ard_v_tag, arch_key_tag, hint_size, encr_date_val;
    QByteArray hint_barr, encr_ar;

    try {        
        enFile.read((char *)&ard_v_tag, sizeof(uint64_t));
        enFile.read((char *)&arch_key_tag, sizeof(uint64_t));
        enFile.read((char *)&encr_date_val, sizeof(uint64_t));
        enFile.read((char *)&hint_size, sizeof(uint64_t));
        hint_barr = enFile.read(hint_size);

        UE uc;
        uc.val = ard_v_tag;
        encr_ar = enFile.readAll();
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/arch-decr" << ba.what();
        rv.first.status = ard::aes_status::mem_alloc_error;
        return rv;
    }

    //enFile.close();

    if (encr_ar.size() < 10) {
        rv.first.status = ard::aes_status::corrupted;
        return rv;
    }

    rv = aes_archive_decrypt(key, iv, encr_ar);
    rv.first.key_tag = arch_key_tag;
    rv.first.hint = decrypt_hint(hint_barr);
    rv.first.encr_date = QDate::fromJulianDay(encr_date_val);

    if (rv.first.status == ard::aes_status::decr_error) {
        if (arch_key_tag != key_tag) {
            if (arch_key_tag < key_tag) {
                rv.first.status = ard::aes_status::old_archive_key_tag;
            }
            else if (arch_key_tag > key_tag) {
                rv.first.status = ard::aes_status::old_provided_key_tag;
            }
        }
        qWarning() << QString("encrypt error - %1'").arg(ard::aes_status2str(rv.first.status));
    }

    return rv;
}

std::pair<ard::aes_status, QByteArray> ard::aes_encrypt_archive(QString pwd, QString hint, const QByteArray& b)
{
    std::pair<ard::aes_status, QByteArray> rv;
    rv.first = ard::aes_status::err;

    QBuffer buf;
    try {
        if (!buf.open(QIODevice::ReadWrite))
        {
            ASSERT(0, "Failed to open mem buf");
            rv.first = ard::aes_status::file_access_error;
            return rv;
        }
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/compress" << ba.what();
        rv.first = ard::aes_status::mem_alloc_error;
        return rv;
    }

    DECL_CRYPTO(pwd);
    rv.first = __aes_encrypt_archive_file(buf, _key, _iv, b, 1, hint);  
    if (rv.first != ard::aes_status::ok) {
        rv.second = QByteArray();
    }
    else {
        buf.seek(0);
        rv.second = buf.readAll();
    }
    buf.close();
    return rv;
};

std::pair<ard::aes_result, QByteArray> ard::aes_decrypt_archive(QString pwd, const QByteArray& b)
{
    std::pair<ard::aes_result, QByteArray> rv;
    rv.first.status = ard::aes_status::err;

    uint64_t key_tag = 1;
    QBuffer buf;
    buf.setData(b);
    if (!buf.open(QIODevice::ReadOnly))
    {
        ASSERT(0, "Failed to open mem file");
        rv.first.status = ard::aes_status::file_access_error;
        return rv;
    }

    DECL_CRYPTO(pwd);
    auto r = __aes_decrypt_archive_file(buf, _key, _iv, key_tag);
    rv.first = r.first;
    if (r.first.status == ard::aes_status::ok) {
        rv.second = r.second;
    }
    buf.close();
    return rv;
};


ard::aes_status __aes_encrypt_barray2file(QString destFile, 
    const QByteArray& key, 
    const QByteArray& iv, 
    const QByteArray& in_ar, 
    uint64_t key_tag, 
    QString pwd_hint)
{
    QFile dfile(destFile);
    try {
        if (!dfile.open(QIODevice::WriteOnly))
        {
            ASSERT(0, "Failed to open dest file") << destFile;
            return ard::aes_status::file_access_error;
        }
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception/compress" << ba.what();
        return ard::aes_status::mem_alloc_error;
    }
    auto res = __aes_encrypt_archive_file(dfile, key, iv, in_ar, key_tag, pwd_hint);
    dfile.close();
    return res;
};

std::pair<ard::aes_result, QByteArray> __aes_decrypt_file2barray(QString encrFile,
    const QByteArray& key,
    const QByteArray& iv,
    uint64_t key_tag) 
{
    std::pair<ard::aes_result, QByteArray> rv;
    rv.first.status = ard::aes_status::err;
    rv.first.key_tag = 0;

    if (!QFile::exists(encrFile)) {
        rv.first.status = ard::aes_status::file_not_found;
        return rv;
    }

    QFile sfile(encrFile);
    if (!sfile.open(QIODevice::ReadOnly))
    {
        ASSERT(0, "Failed to open file") << encrFile;
        rv.first.status = ard::aes_status::file_access_error;
        return rv;
    }

    rv = __aes_decrypt_archive_file(sfile, key, iv, key_tag);
    sfile.close();
    return rv;
};


/**
    SyncCrypto
*/
bool ard::SyncCrypto::isValid()const
{
    bool rv = false;
    if (key.size() == 32 && iv.size() == 16 && tag > 0) {
        rv = true;
    }
    return rv;
};

void ard::SyncCrypto::clear()
{
    key.clear();
    iv.clear();
    tag = 0;
    hint = "";
};

#ifdef ARD_OPENSSL
ard::aes_status ard::aes_archive_encrypt_file(QString sourceFile, QString destFile)
{
    ard::SyncCrypto cr;
    if (ard::CryptoConfig::cfg().hasPasswordChangeRequest()) {
        auto r = ard::CryptoConfig::cfg().syncLimboCrypto();
        if (r.first) {
            cr = r.second;
        }
    }
    else {
        auto r = ard::CryptoConfig::cfg().syncCrypto();
        if (!r.first) {
            return ard::aes_status::password_not_defined;
        }
        else {
            cr = r.second;
        }
    }

    if (!cr.isValid()) {
        return ard::aes_status::password_not_defined;
    }

    QByteArray in_ar;
    try {
        QFile sfile(sourceFile);
        if (!sfile.open(QIODevice::ReadOnly))
        {
            ASSERT(0, "Failed to open source file") << sourceFile;
            return ard::aes_status::file_access_error;
        }
        in_ar = sfile.readAll();
        sfile.close();
    }
    catch (std::bad_alloc& ba)
    {
        qWarning() << "bad_alloc-exception" << ba.what();
        return ard::aes_status::mem_alloc_error;
    }

    return __aes_encrypt_barray2file(destFile, cr.key, cr.iv, in_ar, cr.tag, cr.hint);
};

ard::aes_result ard::aes_archive_decrypt_file(QString encrFile, QString destFile, ard::SyncCrypto* enforce_key)
{
    ard::aes_result rv = { aes_status::err, 0, "", QDate() };
    auto& cfg = ard::CryptoConfig::cfg();
    ard::SyncCrypto cr;

    if (enforce_key) {
        cr = *enforce_key;
    }
    else {
        cr = cfg.syncCrypto().second;
    }

    if (!cr.isValid()) {
        rv.status = ard::aes_status::password_not_defined;
        return rv;
    }

    auto res = __aes_decrypt_file2barray(encrFile, cr.key, cr.iv, cr.tag);
    rv = res.first;
    if (rv.status == ard::aes_status::ok) {
        QFile dfile(destFile);
        if (!dfile.open(QIODevice::WriteOnly))
        {
            ASSERT(0, "Failed to open dest file") << destFile;
            rv.status = ard::aes_status::file_access_error;
            return rv;
        }
        dfile.write(res.second);
        dfile.close();
    }

    return rv;
};


/**
    CryptoConfig
*/
ard::CryptoConfig ard::CryptoConfig::theCfg;

ard::CryptoConfig& ard::CryptoConfig::cfg()
{
    return theCfg;
};

ard::CryptoConfig::CryptoConfig()
{
};

ard::CryptoConfig::~CryptoConfig()
{

};

bool ard::CryptoConfig::hasPassword()const 
{
    return m_sync_crypto.isValid();
};

bool ard::CryptoConfig::hasPasswordChangeRequest()const 
{
    return m_request2change_sync_crypto.isValid();
};

bool ard::CryptoConfig::hasTryOldPassword()const 
{
    return m_try_old_pwd.isValid();
};

std::pair<bool, ard::SyncCrypto> ard::CryptoConfig::syncCrypto()const
{
    std::pair<bool, SyncCrypto> rv;
    rv.first = false;
    if (hasPassword()) {
        rv.second = m_sync_crypto;
        rv.first = true;
    }
    return rv;
};

std::pair<bool, ard::SyncCrypto> ard::CryptoConfig::syncLimboCrypto()const
{
    std::pair<bool, SyncCrypto> rv;
    rv.first = false;
    if (hasPasswordChangeRequest()) {
        rv.second = m_request2change_sync_crypto;
        rv.first = true;
    }
    return rv;
};

std::pair<bool, ard::SyncCrypto> ard::CryptoConfig::tryOldCrypto()const
{
    std::pair<bool, SyncCrypto> rv;
    rv.first = false;
    if (hasTryOldPassword()) {
        rv.second = m_try_old_pwd;
        rv.first = true;
    }
    return rv;
};

void ard::CryptoConfig::clearConfig()
{
    m_sync_crypto.clear();
};

static QString make_config_file_path(QString usedid)
{
    auto cdb_path = ard::get_crypto_config_dir();
    QString enc_db_file_name = cdb_path + QString("/gd-%1.aes").arg(usedid);
    return enc_db_file_name;
}

ard::aes_status ard::CryptoConfig::storeCryptoConfig()
{
    QString usedid = aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    DECLARE_KEY_IV(P1_KEY, P1_IV);

    QJsonObject js;
    toJson(js);
    QJsonDocument doc(js);
    auto bd = doc.toJson();

    auto enc_db_file_name = make_config_file_path(usedid);
    //  get_crypto_config_dir();
    //QString enc_db_file_name = cdb_path + "/a.aes";

    if (QFile::exists(enc_db_file_name)) {
        if (!QFile::remove(enc_db_file_name)) {
            ASSERT(0, "ERROR, failed to delete") << enc_db_file_name;
            return ard::aes_status::file_access_error;
        }
    }

    __aes_encrypt_barray2file(enc_db_file_name, key, iv, bd, 1, "what is hint?");
    //..
#ifdef _DEBUG
    bool debug_print_of_raw_config = true;
    if (debug_print_of_raw_config) {
        auto cdb_path = ard::get_crypto_config_dir();
        QString debug_file_name = cdb_path + "/a.ini";
        QFile dbg_file(debug_file_name);
        if (!dbg_file.open(QIODevice::WriteOnly)) {
            return ard::aes_status::file_access_error;
        };
        dbg_file.write(bd);
        dbg_file.close();
    }
#endif
    //..

    return reloadCryptoConfig();
};

ard::aes_status ard::CryptoConfig::reloadCryptoConfig()
{
    QString usedid =  aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    clearConfig();

    auto enc_db_file_name = make_config_file_path(usedid);
    //auto cdb_path = get_crypto_config_dir();
    //QString enc_db_file_name = cdb_path + "/a.aes";

    DECLARE_KEY_IV(P1_KEY, P1_IV);
    auto res = __aes_decrypt_file2barray(enc_db_file_name, key, iv, 1);
    if (res.first.status == ard::aes_status::ok) {
        auto doc = QJsonDocument::fromJson(res.second);
        if (!doc.isNull()) {
            auto js = doc.object();
            fromJson(js);
            return ard::aes_status::ok;
        }
        return ard::aes_status::err;
    }
    return ard::aes_status::err;
};

ard::aes_status ard::CryptoConfig::purgeCryptoConfig()
{
    QString usedid = aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    auto enc_db_file_name = make_config_file_path(usedid);
    if (QFile::exists(enc_db_file_name)) {
        if (!QFile::remove(enc_db_file_name)) {
            ASSERT(0, "Failed do remove crypto config file") << enc_db_file_name;
        }
    }

    return ard::aes_status::ok;
};

void ard::CryptoConfig::toJson(QJsonObject& js)const
{
    std::function<void(const SyncCrypto&, QString)> cr2json = [&js](const SyncCrypto& cr, QString lbl) 
    {
        js[QString("%1-key").arg(lbl)] = static_cast<QString>(cr.key.toBase64());
        js[QString("%1-iv").arg(lbl)] = static_cast<QString>(cr.iv.toBase64());
        js[QString("%1-tag").arg(lbl)] = static_cast<int>(cr.tag);
        js[QString("%1-hint").arg(lbl)] = cr.hint;
    };

    cr2json(m_sync_crypto, "sync");
    cr2json(m_request2change_sync_crypto, "req2sync");
};

void ard::CryptoConfig::fromJson(const QJsonObject& js)
{
    std::function<void(SyncCrypto&, QString)> json2cr = [&js](SyncCrypto& cr, QString lbl) 
    {
        cr.key = QByteArray::fromBase64(js[QString("%1-key").arg(lbl)].toString().toLatin1());
        cr.iv = QByteArray::fromBase64(js[QString("%1-iv").arg(lbl)].toString().toLatin1());
        cr.tag = js[QString("%1-tag").arg(lbl)].toInt();
        cr.hint = js[QString("%1-hint").arg(lbl)].toString();
    };

    json2cr(m_sync_crypto, "sync");
    json2cr(m_request2change_sync_crypto, "req2sync");
};

ard::aes_status ard::CryptoConfig::request2ChangeSyncPassword(QString pwd, QString old_pwd, QString hint)
{
    QString usedid = aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    if (hasPassword()) {
        ///verify old password
        DECL_CRYPTO(old_pwd);
        if (_key != m_sync_crypto.key ||
            _iv != m_sync_crypto.iv)
        {
            return ard::aes_status::incorrect_old_pwd;
        }
    }

    DECL_CRYPTO(pwd);
    m_request2change_sync_crypto.key = _key;
    m_request2change_sync_crypto.iv = _iv;
    m_request2change_sync_crypto.tag = m_sync_crypto.tag + 1;
    m_request2change_sync_crypto.hint = hint;
    return storeCryptoConfig();
};

ard::aes_status ard::CryptoConfig::tryOldSyncPassword(QString pwd)
{
    DECL_CRYPTO(pwd);
    m_try_old_pwd.key = _key;
    m_try_old_pwd.iv = _iv;
    m_try_old_pwd.tag = 1;
    m_try_old_pwd.hint = "";
    ard::aes_status rv = ard::aes_status::err;
    if (hasTryOldPassword()) {
        rv = ard::aes_status::ok;
    }
    return rv;
};

ard::aes_status ard::CryptoConfig::commitSyncPasswordChange()
{
    QString usedid = aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    if (!hasPasswordChangeRequest()) {
        return ard::aes_status::ok;
    }

    m_sync_crypto = m_request2change_sync_crypto;
    m_request2change_sync_crypto.clear();
    return storeCryptoConfig();
};

ard::aes_status ard::CryptoConfig::promoteSyncPasswordKeyTag(uint64_t key_tag)
{
    QString usedid = aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    qWarning() << "AES/promoting key tag from " << m_sync_crypto.tag << "to" << key_tag;
    m_sync_crypto.tag = key_tag;
    return storeCryptoConfig();
};

ard::aes_status ard::CryptoConfig::clearTryOldSyncPassword() 
{
    QString usedid = aes_account_login();
    if (usedid.isEmpty()) {
        return aes_status::account_is_undefined;
    }

    qWarning() << "AES/clear try old pwd";
    m_try_old_pwd.clear();
    return ard::aes_status::ok;
};

#endif //ARD_OPENSSL