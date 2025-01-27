#include <QApplication>
#include <iostream>
#include <QComboBox>
#include <QFileIconProvider>
#include <QApplication>
#include <QDomDocument>
#include <QDomNodeList>
#include <QMenu>
#include <QFileDialog>
#include <QDesktopWidget>
#include "email.h"
#include "ethread.h"
#include "contact.h"
#include "MainWindow.h"
#include "ansyncdb.h"
#include "ardmodel.h"
#include "TabControl.h"
#include "OutlineMain.h"
#include "OutlineScene.h"
#include "small_dialogs.h"
#include "SearchBox.h"
#include "OutlineMain.h"
#include "syncpoint.h"
#include "address_book.h"
#include "EmailSearchBox.h"
#include "NoteFrameWidget.h"
#include "NoteEdit.h"
#include "ADbgTester.h"
#include "ansearch.h"
#include "custom-boxes.h"
#include "CardPopupMenu.h"

#include "google/endpoint/ApiAppInfo.h"
#include "google/endpoint/ApiAuthInfo.h"
#include "google/endpoint/GoogleWebAuth.h"
#include "csv-util.h"
#include "rule.h"

#ifdef ARD_OPENSSL
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/aes.h>
#endif //ARD_OPENSSL

using namespace googleQt;

#ifdef _DEBUG
/*
class ArdAES
{
    //maybe strart here: https://github.com/bricke/Qt-AES
public:
    /// returns pairs - encrypted data + md5
    static std::pair<QByteArray, QByteArray> encrypt(QString str, QString key);
    static QByteArray decrypt(QByteArray data, QString key, QString md5hash);

private:
    struct ArdEncrData {
        int         etype;
        QDateTime   encr_time;
        QByteArray  iv;
        QByteArray  data;
    };

    static ArdEncrData build_encr_structure(QString str, QString key);
};


ArdAES::ArdEncrData ArdAES::build_encr_structure(QString str, QString)
{
    ArdAES::ArdEncrData enc;
    enc.etype = 1;
    enc.encr_time = QDateTime::currentDateTime();
    enc.iv = QCryptographicHash::hash(enc.encr_time.toString(Qt::ISODate).toLocal8Bit(), QCryptographicHash::Md5);
    enc.iv = QCryptographicHash::hash(enc.iv, QCryptographicHash::Md5);
    QByteArray br = str.toUtf8();
    /// we encrypt arr here
    enc.data = br;
    return enc;
};

std::pair<QByteArray, QByteArray> ArdAES::encrypt(QString data, QString key)
{
    auto enc = build_encr_structure(data, key);
    std::pair<QByteArray, QByteArray> rv;
    QDataStream bds(&rv.first, QIODevice::WriteOnly);
    bds.setVersion(QDataStream::Qt_5_5);
    bds << enc.etype;
    bds << enc.encr_time;
    bds << enc.iv;
    bds << enc.data;
    rv.second = QCryptographicHash::hash(rv.first, QCryptographicHash::Md5);
    return rv;
};

QByteArray ArdAES::decrypt(QByteArray data, QString key, QString md5hash)
{
    QDataStream bds(&data, QIODevice::ReadOnly);

    ArdAES::ArdEncrData enc;
    bds >> enc.etype;
    bds >> enc.encr_time;
    bds >> enc.iv;
    bds >> enc.data;
    return enc.data;
};
*/


class DbgStyledWindow : public QWidget 
{
public:
    DbgStyledWindow() 
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
        installEventFilter(this);
        setMouseTracking(true);

        auto box = new QVBoxLayout();
        box->setMargin(25);
        QLabel* l1 = new QLabel("one");
        QLabel* l2 = new QLabel("two");
        QTextEdit *e = new QTextEdit;
        e->setPlainText("test edit");
        box->addWidget(l1);
        box->addWidget(l2);
        box->addWidget(e);
        setLayout(box);
        setCursor(Qt::SizeHorCursor);
    }
    
    bool eventFilter(QObject *obj, QEvent *event)override 
    {
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *pMouse = dynamic_cast<QMouseEvent *>(event);
            if (pMouse) {
                setCursor(Qt::SizeHorCursor);
                event->accept();
                return true;
            }
        }
        return QWidget::eventFilter(obj, event);

    };  
};

void ADbgTester::show_styled_window() 
{
    DbgStyledWindow* w = new DbgStyledWindow;
    w->setObjectName("MyWindow");
    w->setStyleSheet("QWidget#MyWindow{background-color:gray;}");
    w->show();
};

void ADbgTester::add10contacts() 
{
    assert_return_void(ard::isDbConnected(), "expected open DB");
    auto cr = ard::db()->cmodel()->croot();
    const int contacts2generate = 10;

    auto r = gui::edit("c1-", "prefix for contacts");
    if (r.first) {
        for (int i = 0; i < contacts2generate; i++) {
            auto firstName = QString("%1-FName%2_%3").arg(r.second).arg("gen").arg(i);
            auto lastName = QString("%1-LName%2_%3").arg(r.second).arg("gen").arg(i);
            auto c = cr->addContact(firstName, lastName);
            if (c) {
                FieldParts fp_email;
                fp_email.add(FieldParts::Email, QString("c%1@yahoo.com").arg(i));
                c->setFieldValues(EColumnType::ContactEmail, "main", fp_email);

                FieldParts fp_phone;
                fp_phone.add(FieldParts::Phone, QString("1-%1-123").arg(i));
                c->setFieldValues(EColumnType::ContactPhone, "cell", fp_phone);

                FieldParts fp_addr;
                fp_addr.add(FieldParts::AddrStreet, QString("main str %1").arg(i));
                fp_addr.add(FieldParts::AddrCity, QString("Las Vegas"));
                fp_addr.add(FieldParts::AddrRegion, QString("Nevada"));
                fp_addr.add(FieldParts::AddrZip, QString("1121%1").arg(i));
                fp_addr.add(FieldParts::AddrCountry, QString("USA"));
                c->setFieldValues(EColumnType::ContactAddress, "home", fp_addr);
            }
        }
    }

    cr->ensurePersistant(-1);
};


void ADbgTester::list_q_data()
{
}

bool ADbgTester::convert_media(QString image_source_path, QString image_dest_path, int quality)
{
    QImageReader r;
    r.setFileName(image_source_path);
    r.setQuality(quality);
    if (!r.canRead()) {
        ASSERT(0, "load_media - can't read") << image_source_path;
        return false;
    }

    auto img = r.read();
    QImage mp2 = img.scaled(MP2_WIDTH, MP2_HEIGHT, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return mp2.save(image_dest_path);
}


void ADbgTester::check_label_bits() 
{
    QString sql_update;
    sql_update = QString("UPDATE %1gmail_msg SET msg_labels=(~(msg_labels&%2))&(msg_labels|%2) WHERE acc_id=%3 AND (msg_labels&%2 = %2)")
    .arg("META")
    .arg(123)
    .arg(111);
    qDebug() << "sql=" << sql_update;   
};

void ADbgTester::print_current_topic() 
{
    auto f = ard::currentTopic();
    if (f) {
        qDebug() << "current-topic" << f->title() << "id=" << f->id() << "dataDb=" << f->dataDb();
    }
};

void ADbgTester::adopt_curr_thread() 
{
    auto t = ard::currentThread();
    if (!t) {
        auto f = ard::currentTopic();
        if (f) {
            auto e = dynamic_cast<ard::email*>(f);
            if (e) {
                t = e->parent();
            }
        }
    }

    if (!t) {
		ard::errorBox(ard::mainWnd(), "Select thread or email to proceed");
        return;
    }

    qDebug() << "about to adopt thread" << t->dbgHint();
    auto gm = ard::gmail_model();
    if (gm ) {
        auto p = gm->adoptThread(t);
        if (p) {
            p->ensurePersistant(-1);
            qDebug() << "adapted" << t->dbgHint();
        }
    }
};

void ADbgTester::show_adopted_threads() 
{
    AdoptedThreadsBox::showThreads();
};

void ADbgTester::simple_crypt()
{
    /*
    QString str("some data string.");
    QString pwd("pwd key");
    auto b = ard::encryptText(pwd, str);
    qDebug() << "encr:" << b;
    auto s2 = ard::decryptText(pwd, b);
    qDebug() << "decr:" << str;
    */
};

void ADbgTester::test_popup() 
{
    ADbgTestPopup::testPopup();
};

void ADbgTester::test_short_hash() 
{   
    //QString str("imported");
    std::vector<QString> lst = {"A", "B", "C", "one", "two", "thre", "imported", "john", "anna", "peter"};
    for (auto& str : lst) {
        char h = 0;
        for (auto& s : str) {
            //qDebug() << "row=" << s.row() << " " << s.toLatin1();
            h ^= s.toLatin1();
        }

        h %= 8;

        qDebug() << "hash" << str << h;
    }
};

void ADbgTester::test_format_notes()
{
    auto f = ard::currentTopic();
    if (f) {
        TOPICS_LIST lst;
        lst.push_back(f);
        FormatFontDlg::format(lst);
    }
};

#else

void ADbgTester::print_version()
{
    qWarning() << get_app_version_as_string();
};

#endif

#ifdef ARD_BETA
void ADbgTester::check_beta_expire() 
{
    extern bool checkBetaExpire();
    checkBetaExpire();
};
#endif


#ifdef _DEBUG
void ADbgTestPopup::testPopup() 
{
    ADbgTestPopup* w = new ADbgTestPopup;
    w->show();
};

ADbgTestPopup::ADbgTestPopup() 
{
    setAttribute(Qt::WA_DeleteOnClose);
};

#ifdef ARD_OPENSSL
/// https://github.com/saju/misc/blob/master/misc/openssl_aes.c

/**
* Create a 256 bit key and IV using the supplied key_data. salt can be added for taste.
* Fills in the encryption and decryption ctx objects and returns 0 on success
**/
int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx,
    EVP_CIPHER_CTX *d_ctx)
{
    int i, nrounds = 5;
    unsigned char key[32], iv[32];

    /*
    * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
    * nrounds is the number of times the we hash the material. More rounds are more secure but
    * slower.
    */
    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
    if (i != 32) {
        printf("Key size is %d bits - should be 256 bits\n", i);
        return -1;
    }

    EVP_CIPHER_CTX_init(e_ctx);
    EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(d_ctx);
    EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

    return 0;
}

/*
* Encrypt *len bytes of data
* All data going in & out is considered binary (unsigned char[])
*/
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
    /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
    int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *ciphertext = (unsigned char*)malloc(c_len);

    /* allows reusing of 'e' for multiple encryption cycles */
    EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);

    /* update ciphertext, c_len is filled with the length of ciphertext generated,
    *len is the size of plaintext in bytes */
    EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);

    /* update ciphertext with the final remaining bytes */
    EVP_EncryptFinal_ex(e, ciphertext + c_len, &f_len);

    *len = c_len + f_len;
    return ciphertext;
}

/*
* Decrypt *len bytes of ciphertext
*/
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
    /* plaintext will always be equal to or lesser than length of ciphertext*/
    int p_len = *len, f_len = 0;
    unsigned char *plaintext = (unsigned char *)malloc(p_len);

    EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
    EVP_DecryptFinal_ex(e, plaintext + p_len, &f_len);

    *len = p_len + f_len;
    return plaintext;
}

void ADbgTester::test_openssl_base()
{
    /// "opaque" encryption, decryption ctx structures that libcrypto uses to record
    /// status of enc/dec operations 
    //EVP_CIPHER_CTX en, de;
    EVP_CIPHER_CTX *en = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX *de = EVP_CIPHER_CTX_new();


    /* 8 bytes to salt the key_data during key generation. This is an example of
    compiled in salt. We just read the bit pattern created by these two 4 byte
    integers on the stack as 64 bits of contigous salt material -
    ofcourse this only works if sizeof(int) >= 4 */
    unsigned int salt[] = { 12345, 54321 };
    const char* key1 = "key123";
    unsigned char *key_data = (unsigned char *)key1;
    int key_data_len, i;
    key_data_len = strlen(key1);

    const char *input[] = { "a", "abcd", "this is a test", "this is a bigger test",
        "\nWho are you ?\nI am the 'Doctor'.\n'Doctor' who ?\nPrecisely!",
        NULL };

    /* gen key and iv. init the cipher ctx object */
    if (aes_init(key_data, key_data_len, (unsigned char *)&salt, en, de)) {
        //printf("Couldn't initialize AES cipher\n");
        qWarning() << "Couldn't initialize AES cipher";
        return;
    }

    /* encrypt and decrypt each input string and compare with the original */
    for (i = 0; input[i]; i++) {
        char *plaintext;
        unsigned char *ciphertext;
        int olen, len;

        /* The enc/dec functions deal with binary data and not C strings. strlen() will
        return length of the string without counting the '\0' string marker. We always
        pass in the marker byte to the encrypt/decrypt functions so that after decryption
        we end up with a legal C string */
        olen = len = strlen(input[i]) + 1;

        ciphertext = aes_encrypt(en, (unsigned char *)input[i], &len);
        plaintext = (char *)aes_decrypt(de, ciphertext, &len);
        qDebug() << ciphertext;

        if (strncmp(plaintext, input[i], olen)) {
            qDebug() << QString("FAIL: enc/dec failed for %1").arg(input[i]);
            //printf("FAIL: enc/dec failed for \"%s\"\n", input[i]);
        }
        else {
            qDebug() << QString("OK: enc/dec ok for %1").arg(plaintext);
            //printf("OK: enc/dec ok for \"%s\"\n", plaintext);
        }

        free(ciphertext);
        free(plaintext);
    }

    EVP_CIPHER_CTX_free(en);
    EVP_CIPHER_CTX_free(de);
};

void ADbgTester::test_openssl_crypto()
{
    QString str2encrypt = "some long string..";
    for (int i = 0; i < 100; i++) {
        str2encrypt += QString("123-test-%1").arg(i);
        QByteArray data = str2encrypt.toStdString().c_str();
        QString pwd = QString("pwd123123-%1").arg(i);
        auto eb = ard::aes_encrypt(pwd, data);
        auto db = ard::aes_decrypt(pwd, eb);
        if (data == db) {
            qDebug() << i << ". encr-ok" << data << "[" << eb << "]";
        }
        else {
            ASSERT(0, "descrypt error");
            qDebug() << i << ". encr-err";
        }
    }
};

void ADbgTester::test_crypto_archive() 
{
    QString str2encrypt = "some long string..";
    for (int i = 0; i < 100; i++) {
        str2encrypt += QString("123-test-%1").arg(i);
        QByteArray data = str2encrypt.toStdString().c_str();
        QString pwd = QString("pwd123123-%1").arg(i);
        auto er = ard::aes_encrypt_archive(pwd, "hint1", data);
        auto dr = ard::aes_decrypt_archive(pwd, er.second);
        if (data == dr.second) {
            qDebug() << i << ". encr-ok" << data << "[" << dr.second << "]";
        }
        else {
            ASSERT(0, "descrypt error");
            qDebug() << i << ". encr-err";
        }
    }
};

void ADbgTester::test_crypto_config()
{

};

void ADbgTester::test_crypto_commit_pwd()
{
#ifdef _DEBUG   
    if (!ard::CryptoConfig::cfg().hasPasswordChangeRequest()) {
		ard::errorBox(ard::mainWnd(), "No password change requrest detected.");
        return;
    }

    if (ard::confirmBox(this, "Please confirm commit pwd. Warning. Don't do it if not sure!")) {
        auto r = ard::CryptoConfig::cfg().commitSyncPasswordChange();
        if (r != ard::aes_status::ok) {
			ard::errorBox(ard::mainWnd(), QString("Commit passsword failed with error '%1'").arg(ard::aes_status2str(r)));
            return;
        }
    }
#endif
};

void ADbgTester::test_crypto_promote_pwd_key() 
{
#ifdef _DEBUG
    auto& cfg = ard::CryptoConfig::cfg();
    if (!cfg.hasPassword()) {
		ard::errorBox(ard::mainWnd(), "No password");
        return;
    }

    if (ard::confirmBox(this, "Please confirm promoting pwd key tag. Warning. Don't do it if not sure!")) {
        auto cr = cfg.syncCrypto();
        auto r = gui::edit(QString("%1").arg(cr.second.tag), "Tag:", true);
        if (r.first && !r.second.isEmpty()) {
            auto num = r.second.toInt();
            cfg.promoteSyncPasswordKeyTag(num);
            cr = cfg.syncCrypto();
			ard::messageBox(this, QString("Tag promoted to '%1'").arg(cr.second.tag));
        }
    }

#endif
};

void ADbgTester::test_crypto_old_pwd_box() 
{
#ifdef _DEBUG
    ard::aes_result last_dec_res = { ard::aes_status::err, 3, "this is hint", QDate::currentDate() };
    auto s = SyncOldPasswordBox::getOldPassword(last_dec_res);
	ard::messageBox(this, s);
#endif
};

#endif //ARD_OPENSSL


void ADbgTester::test_print_fonts() 
{
    qDebug() << "======== font families ======";
    QFontDatabase fdb;
    auto fnt_families = fdb.families();
    for (auto& s : fnt_families) {
        qDebug() << "font-family" << s;
    }
};

#endif //_DEBUG

void ADbgTester::test_print_app_state() 
{
    auto aw = qApp->activeWindow();
    auto fw = qApp->focusWidget();
    if (aw) {
        qInfo() << "active-window" << aw->accessibleName();
        qInfo() << "focus-widget" << fw->accessibleName();
    }
};