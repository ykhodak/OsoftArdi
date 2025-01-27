#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPlainTextEdit>
#include <anfolder.h>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QCheckBox>
#include <QButtonGroup>
#include <QRadioButton>
#include "UFolderBox.h"
#include "EmailSearchBox.h"
#include "GoogleClient.h"
#include "gmail/GmailRoutes.h"
#include "global-struct.h"
#include "ansearch.h"
#include "locus_folder.h"

class SingletonUFolderBox2 : public QDialog
{
public:
    static bool openBox(ard::locus_folder* f);

    SingletonUFolderBox2(ard::locus_folder* f);

    void processOK();
protected:
	ard::locus_folder*  m_folder{ nullptr };
    QCheckBox*      m_chkShowInTab{ nullptr };
    //ColorButtonPanel* m_cbox{ nullptr };
#ifdef ARD_ENABLE_RENAME_SINGLETON
    QComboBox*      m_title{ nullptr };
#endif //ARD_ENABLE_RENAME_SINGLETON
    bool            m_accepted{ false };
};

void testX() {
    auto f = dynamic_cast<ard::locus_folder*>(ard::currentTopic());
    if (f) {
        SingletonUFolderBox2::openBox(f);
    }
}

bool SingletonUFolderBox2::openBox(ard::locus_folder* f)
{
    SingletonUFolderBox2 d(f);
    d.exec();
    return d.m_accepted;

};

SingletonUFolderBox2::SingletonUFolderBox2(ard::locus_folder* f) : m_folder(f)
{
#ifdef ARD_ENABLE_RENAME_SINGLETON
    m_title = new QComboBox;
#endif //ARD_ENABLE_RENAME_SINGLETON
    QVBoxLayout *v_main = new QVBoxLayout;
    int idx = 0;
    QGridLayout* gl = new QGridLayout;
    v_main->addLayout(gl);
    if (m_folder) {
        if (!m_folder->isDefaultLocused()) {
            m_chkShowInTab = new QCheckBox("Show In Locus Tab");
            m_chkShowInTab->setChecked(m_folder->isInLocusTab());
            gl->addWidget(new QLabel("Locus Tab:"), idx, 0);
            gl->addWidget(m_chkShowInTab, idx, 1);
            idx++;
        }
    }

#ifdef ARD_ENABLE_RENAME_SINGLETON
    gl->addWidget(new QLabel("Title:"), idx, 0);
    gl->addWidget(m_title, idx, 1);
    idx++;

    STRING_LIST names = f->suggestSingletonLabelName();
    int idx2sel = -1;
    auto name = m_folder->title();
    for (auto s : names) {
        m_title->addItem(s);
        if (name == s) {
            idx2sel = m_title->count() - 1;
        }
    }

    if (idx2sel != -1) {
        m_title->setCurrentIndex(idx2sel);
    }
#endif//ARD_ENABLE_RENAME_SINGLETON

    //m_cbox = new ColorButtonPanel(m_folder->colorIndex());
    //v_main->addWidget(m_cbox);

    QPushButton* b = nullptr;
    auto h = new QHBoxLayout;
    ADD_BUTTON2LAYOUT(h, "OK", &SingletonUFolderBox2::processOK);
    ADD_BUTTON2LAYOUT(h, "Cancel", [=](){m_accepted = false; close(); });
    v_main->addLayout(h);
    MODAL_DIALOG(v_main);
};

void SingletonUFolderBox2::processOK()
{
    bool data_changed = false;
#ifdef ARD_ENABLE_RENAME_SINGLETON
    auto name = m_title->currentText().trimmed();
    if (name != m_folder->title()) {
        if (!m_folder->canRenameToAnything()) {
            m_folder->setTitle(name);
            qDebug() << "title-changed " << name << "|" << m_folder->title();
            data_changed = true;
        }
    }
#endif //ARD_ENABLE_RENAME_SINGLETON

    /*if (m_cbox) {
        auto cidx = m_cbox->colorIndex();
        if (cidx != m_folder->colorIndex()) {
            m_folder->setColorIndex(cidx);
            data_changed = true;
        }
    }*/

    if (m_chkShowInTab && m_chkShowInTab->isChecked() != m_folder->isInLocusTab()) {
        m_folder->setInLocusTab(m_chkShowInTab->isChecked());
        data_changed = true;
    }

    if (data_changed) {
        ard::asyncExec(AR_RebuildLocusTab);
    }

    close();
};


bool ard::guiEditUFolder(topic_ptr f, QString boxHeader)
{
    assert_return_false(f, "expected topic");
    if (f->getSingletonType() != ESingletonType::none) {
        auto uf = dynamic_cast<ard::locus_folder*>(f);
        assert_return_false(uf, "expected u-topic");
        return SingletonUFolderBox2::openBox(uf);
    }

    return LocusBox::editFolder(f, boxHeader);
};

bool LocusBox::addFolder()
{
    LocusBox d(nullptr, "");
    d.exec();
    return d.m_accepted;
};

bool LocusBox::editFolder(topic_ptr f, QString boxHeader)
{
    LocusBox d(f, boxHeader);
    d.exec();
    return d.m_accepted;
};

LocusBox::LocusBox(topic_ptr f, QString boxHeader):m_folder(f)
{
    static STRING_LIST defaultFolderNames;
    if (defaultFolderNames.empty()) {
#define ADD_DEF_FOLDER(P,N) {defaultFolderNames.push_back(N);}
        //ADD_DEF_FOLDER(draft, "Drafts - new emails (not sent), notes in progress.");
        ADD_DEF_FOLDER(food, "Food - recipes, menus, restaurants, wines");
        ADD_DEF_FOLDER(child, "Children - things to do with them");
        ADD_DEF_FOLDER(book, "Books to read");
        ADD_DEF_FOLDER(music, "Music to download");
        ADD_DEF_FOLDER(movie, "Movies to see");
        ADD_DEF_FOLDER(gift, "Gift ideas");
        ADD_DEF_FOLDER(web, "Web sites to explore");
        ADD_DEF_FOLDER(weekend, "Weekend trips to take");
        ADD_DEF_FOLDER(idea, "Ideas - Misc.");
#undef ADD_DEF_FOLDER
    }

    m_folder = f;

    m_chkShowInTab = new QCheckBox("Show In Locus Tab");
    QVBoxLayout *v_main = new QVBoxLayout;
    m_name = new QLineEdit();
    if (m_folder) {
        m_name->setText(m_folder->title());
        m_chkShowInTab->setChecked(m_folder->isInLocusTab());
    }

    if (!m_folder) {
        m_combo_def = new QComboBox();
        m_combo_def->addItem("<select name>", QVariant(""));
        for (auto& i : defaultFolderNames) {
            m_combo_def->addItem(i);
        }
        connect(m_combo_def, SIGNAL(currentIndexChanged(int)),
            this, SLOT(defIndexChanged(int)));
    }

    if (!boxHeader.isEmpty()) {
        QLabel* l = new QLabel(boxHeader);
        l->setWordWrap(true);
        v_main->addWidget(l);
    }

    int idx = 0;
    QGridLayout* gl = new QGridLayout;
    v_main->addLayout(gl);
    gl->addWidget(new QLabel("Locus Tab:"), idx, 0);
    gl->addWidget(m_chkShowInTab, idx, 1);
    idx++;

    gl->addWidget(new QLabel("Title:"), idx, 0);
    gl->addWidget(m_name, idx, 1);
    idx++;

    if (!m_folder) {
        gl->addWidget(new QLabel("Sample:"), idx, 0);
        gl->addWidget(m_combo_def, idx, 1);
        idx++;
    }

    ard::EColor clr_idx = ard::EColor::none;
    if (m_folder) {
        clr_idx = m_folder->colorIndex();
    }
    //m_cbox = new ColorButtonPanel(clr_idx);
    //v_main->addWidget(m_cbox);

    QPushButton* b = nullptr;
    auto h = new QHBoxLayout;
    ADD_BUTTON2LAYOUT(h, "OK", &LocusBox::processOK);
    ADD_BUTTON2LAYOUT(h, "Cancel", &LocusBox::processCancel);
    v_main->addLayout(h);
    m_name->setFocus(Qt::OtherFocusReason);
    MODAL_DIALOG(v_main);
};

bool LocusBox::createNewFolder()
{
    assert_return_false(!m_folder, "don't create folder twice");
    auto croot = ard::CustomSortersRoot();
    assert_return_false(croot, "expected sorters root");

    QString name = m_name->text().trimmed();
    if (name.isEmpty())
    {
		ard::errorBox(this, "Please enter folder name.");
        return false;
    }

    if (croot->findTopicByTitle(name)) {
		ard::errorBox(this, QString("Folder '%1' already exists, please provide another name.").arg(name));
        return false;
    }

    ArdDB* db = ard::db();
    m_folder = db->createCustomFolderByName(name);
    if (!m_folder) {
		ard::errorBox(this, QString("Failed to create folder '%1'.").arg(name));
        return false;
    }

    /*if (m_cbox) {
        auto cidx = m_cbox->colorIndex();
        if (cidx != m_folder->colorIndex()) {
            m_folder->setColorIndex(cidx);
        }
    }*/

    if (m_chkShowInTab && m_chkShowInTab->isChecked() != m_folder->isInLocusTab()) {
        m_folder->setInLocusTab(m_chkShowInTab->isChecked());
        ard::asyncExec(AR_RebuildLocusTab);
    }

    if (!m_folder)
    {
        ard::messageBox(this,QString("ERROR, failed to create folder '%1'").arg(name));
    }
    else
    {
        croot->ensurePersistant(-1);
        return true;
    }
    return false;
};


void LocusBox::processOK()
{
    m_accepted = false;
    auto croot = ard::CustomSortersRoot();
    assert_return_void(croot, "expected sorters root");
  
    QString name = m_name->text().trimmed();
    if(name.isEmpty())
        {
		ard::errorBox(this, "Please enter folder name.");
            return;
        }

    if (m_folder) {
        auto data_changed = false;

        ///edit existing folder
        if (m_chkShowInTab->isChecked() != m_folder->isInLocusTab()) {
            m_folder->setInLocusTab(m_chkShowInTab->isChecked());
            data_changed = true;
        }

        if (m_folder->title() != name) {
            auto f = croot->findTopicByTitle(name);
            if (f) {
                if (f != m_folder) {
					ard::errorBox(this, QString("Folder '%1' already exists, please provide another name.").arg(name));
                    return;
                }
            }
            m_folder->setTitle(name);
            data_changed = true;
        }//title

        /*if (m_cbox) {
            auto cidx = m_cbox->colorIndex();
            if (cidx != m_folder->colorIndex()) {
                m_folder->setColorIndex(cidx);
                data_changed = true;
            }
        }*/
        if (data_changed) {
            ard::asyncExec(AR_RebuildLocusTab);
        }

        m_accepted = true;
    }
    else {
        if (createNewFolder()) {
            m_accepted = true;
        }
    }

    close();
};

void LocusBox::processCancel()
{
    m_accepted = false;
    close();
};

void LocusBox::defIndexChanged(int idx)
{
    if(idx != 0){
        QString s = m_combo_def->itemText(idx);
        m_name->setText(s); 
    }
};
