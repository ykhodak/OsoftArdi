#pragma once

#include <QString>
#include <QWidget>

class ADbgTester
{
    friend void test_runner();
public:
    static void test_print_app_state();

#ifdef _DEBUG
    static void print_current_topic();

    static void add10contacts();
    static bool convert_media(QString image_source_path, QString image_dest_path, int quality);
    static void list_q_data();
    static void check_label_bits();
    static void adopt_curr_thread();
    static void show_adopted_threads();
    static void simple_crypt();
    //static void printProjects();
    static void show_styled_window();
    //static void test_labels_rebuid();
    static void test_short_hash();
    static void test_popup();
    static void test_format_notes();
    static void test_print_fonts(); 
#ifdef ARD_OPENSSL
    static void test_openssl_base();
    static void test_openssl_crypto();
    static void test_crypto_config();
    static void test_crypto_commit_pwd();
    static void test_crypto_promote_pwd_key();
    static void test_crypto_old_pwd_box();
    static void test_crypto_archive();
#endif //ARD_OPENSSL

#else
    static void print_version();
#endif

#ifdef ARD_BETA
    static void check_beta_expire();
#endif
};


#ifdef _DEBUG
class ADbgTestPopup : public QWidget
{
    Q_OBJECT
public:
    ADbgTestPopup();
    static void testPopup();
};
#endif