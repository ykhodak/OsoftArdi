#include "kring.h"
//#include "project-resources.h"
#include "snc-tree.h"


IMPLEMENT_ROOT(ard, KRingRoot, "Keys", ard::KRingKey);

/**
    kring_model
*/
ard::kring_model::kring_model(ArdDB* db) 
{
    m_kring_root = new ard::KRingRoot(db);
};

ard::kring_model::~kring_model() 
{
	if (m_kring_root)
		m_kring_root->release();
};

ard::KRingRoot* ard::kring_model::keys_root() {return m_kring_root;};
const ard::KRingRoot* ard::kring_model::keys_root()const {return m_kring_root;};

/**
    KRingRoot
*/
bool ard::KRingRoot::isRingLocked()const
{
    return m_pwd.isEmpty();
};

bool ard::KRingRoot::lockRing() 
{
    if (isRingLocked()) {
        ASSERT(0, "expected unlocked Kring");
        return false;
    }

    bool has_modified_key = false;
    auto k_list = items();
    for (auto f : k_list) {
        auto k = dynamic_cast<ard::KRingKey*>(f);
        if (k) {
            auto e = k->kext();
            if (e && e->isModified()) {
                has_modified_key = true;
                break;
            }
        }
    }

    if (has_modified_key) {
        ensurePersistant(-1);
    }

    for (auto f : k_list) {
        auto k = dynamic_cast<ard::KRingKey*>(f);
        if (k) {
            auto e = k->kext();
            e->lockContent();
        }
    }

    m_pwd = m_hint = "";

    asyncExec(AR_ToggleKRingLock);
    return true;
};

bool ard::KRingRoot::guiUnlockRing() 
{
    bool rv = false;

    if (!isRingLocked()) {
        return true;
    }

    auto k_list = items();
    if (k_list.empty()) {
        /// basically we define password for the first time
        auto res = gui::change_password("Please provide master password to encrypt all your keys.", false);
        if (!res.password.isEmpty()) {
            m_pwd = res.password;
            m_hint = res.hint;
        }

        rv = !m_pwd.isEmpty();
    }
    else {
        QString pwd = gui::enter_password("Please enter master password.");
        if (!pwd.isEmpty()) {
            rv = decryptKRing(pwd);
            if (!rv) {
				ard::errorBox(ard::mainWnd(), "Incorrect pasword.");
            }
        }
    }

    if (rv) {
        asyncExec(AR_ToggleKRingLock);
    }
    return rv;
};

bool ard::KRingRoot::decryptKRing(QString pwd)
{
    bool rv = true;
    m_pwd = pwd;
    m_hint = "";

    auto k_list = items();
    for (auto f : k_list) {
        auto k = dynamic_cast<ard::KRingKey*>(f);
        if (k) {
            auto res = k->decryptKey(pwd);
            if (res.status != ard::aes_status::ok) {
                rv = false;
                m_pwd = "";
                m_hint = "";
                qWarning("RKey decription failed");
                return false;
            }
            else {
                if (m_hint.isEmpty()) {
                    m_hint = res.hint;
                }
            }
        }
    }

    return rv;
};

ard::KRingKey*  ard::KRingRoot::addKey(QString key_title) 
{
    ASSERT_VALID(this);

    if (m_pwd.isEmpty()) {
        ard::asyncExec(AR_SetKeyRingPwd);
        return nullptr;
    }

    auto* rv = KRingKey::createNewKey(key_title);
    addItem(rv);
    ensurePersistant(1);
    return rv;
};

ard::KRingKey* _hoistedKRingInFilter = nullptr;

ard::KRingKey* ard::KRingRoot::hoistedRingKeyInFilter()
{
    return _hoistedKRingInFilter;
};

void ard::KRingRoot::setHoistedRingKeyInFilter(ard::KRingKey* k)
{
    _hoistedKRingInFilter = k;
};

TOPICS_LIST ard::KRingRoot::filteredItems(QString indexStr /*= "*"*/)
{
    ASSERT_VALID(this);
    bool useIdx = (indexStr != "*" && !indexStr.isEmpty());
    TOPICS_LIST rv;
    auto c_list = items();
    for (auto& i : c_list)
    {
        auto c = dynamic_cast<ard::KRingKey*>(i);
        bool addit = true;
        if (addit && useIdx) {
            QString s = c->impliedTitle().left(1).toLower();
            if (s.isEmpty()) {
                addit = false;
            }
            else {
                addit = (indexStr.indexOf(s) != -1);
            }
        }

        if (!addit) {
            if (c == ard::KRingRoot::hoistedRingKeyInFilter()) {
                addit = true;
            }
        }

        if (addit) {
            rv.push_back(c);
        }
    }

    return rv;
};

/**
    KRingKey
*/
ard::KRingKey::KRingKey() 
{

};

ard::anKRingKeyExt* ard::KRingKey::kext() 
{
    return ensureKExt();
};

const ard::anKRingKeyExt* ard::KRingKey::kext()const 
{
    ard::KRingKey* ThisNonCost = (ard::KRingKey*)this;
    return ThisNonCost->ensureKExt();
};

ard::anKRingKeyExt* ard::KRingKey::ensureKExt()
{
    ASSERT_VALID(this);
    if (m_kext)
        return m_kext;

    ensureExtension(m_kext);
    return m_kext;
};

QString ard::KRingKey::objName()const 
{
    return "key";
};

cit_prim_ptr ard::KRingKey::create()const 
{
    return new ard::KRingKey;
};

ard::KRingKey* ard::KRingKey::createNewKey(QString key_title)
{
    auto rv = new KRingKey;
    auto e = rv->ensureKExt();
    if (e) {
        e->m_keyTitle = key_title;
        e->m_modified = true;
        e->m_locked = false;
    }

    return rv;
};

QString ard::KRingKey::impliedTitle()const
{
    ASSERT_VALID(this);
    QString rv;
    if (m_kext) {
        rv = m_kext->m_keyTitle;
    }
    return rv;
};


bool ard::KRingKey::canBeMemberOf(const topic_ptr f)const
{
    ASSERT_VALID(this);
    bool rv = false;
    if (dynamic_cast<const ard::KRingRoot*>(f) != nullptr) {
        rv = true;
    }
    return rv;
};

void ard::KRingKey::mapExtensionVar(cit_extension_ptr e)
{
    topic::mapExtensionVar(e);
    if (e->ext_type() == snc::EOBJ_EXT::extRingKey) {
        ASSERT(!m_kext, "duplicate kring ext");
        m_kext = dynamic_cast<ard::anKRingKeyExt*>(e);
        ASSERT(m_kext, "expected kring ext");
    }
};


ard::KRingRoot* ard::KRingKey::kroot()
{
    auto p = parent();
    if (!p) {
        ASSERT(0, "expected kroot");
        return nullptr;
    }
    auto rv = dynamic_cast<ard::KRingRoot*>(p);
    return rv;
};

const ard::KRingRoot* ard::KRingKey::kroot()const
{
    auto p = parent();
    if (!p) {
        ASSERT(0, "expected kroot");
        return nullptr;
    }
    auto rv = dynamic_cast<const ard::KRingRoot*>(p);
    return rv;
};

ard::aes_result ard::KRingKey::decryptKey(QString pwd)
{
    ard::aes_result rv;
    rv.status = ard::aes_status::err;

    if (!m_kext) {
        ASSERT(0, "expected key extension");
        rv.status = ard::aes_status::ok;
        return rv;
    }   

    if (!m_kext->isLocked()) {
        ASSERT(0, "already unlocked rkey");
        return rv;
    }

    m_kext->m_locked = true;

    if (m_kext->m_encryptedContent.size() == 0) {
        rv.status = ard::aes_status::ok;
        qWarning() << "empty key content";
        return rv;
    }

    auto res = ard::aes_decrypt_archive(pwd, m_kext->m_encryptedContent);
    rv = res.first;
    if (rv.status != ard::aes_status::ok){
        ASSERT(0, "key decrypt failed") << ard::aes_status2str(rv.status);
        return rv;
    }

    if (res.second.isEmpty()) {
        ASSERT(0, "key decrypt failed - empty content");
        rv.status = ard::aes_status::err;
        return rv;
    }

    auto doc = QJsonDocument::fromJson(res.second);
    if (!doc.isNull()) {
        auto js = doc.object();
        m_kext->fromJson(js);
        m_kext->m_locked = false;
    }
    else {
        ASSERT(0, "key decrypt failed - invalid content");
        rv.status = ard::aes_status::corrupted;
        return rv;
    }

    return rv;
};

QString ard::KRingKey::keyTitle()const 
{
    QString rv;
    if (m_kext) {
        rv = m_kext->m_keyTitle;
    }
    return rv;
};

QString ard::KRingKey::keyLogin()const 
{
    QString rv;
    if (m_kext) {
        rv = m_kext->m_keyLogin;
    }
    return rv;
};

QString ard::KRingKey::keyPwd()const 
{
    QString rv;
    if (m_kext) {
        rv = m_kext->m_keyPwd;
    }
    return rv;
};

QString ard::KRingKey::keyLink()const 
{
    QString rv;
    if (m_kext) {
        rv = m_kext->m_keyLink;
    }
    return rv;
};

QString ard::KRingKey::keyNote()const 
{
    QString rv;
    if (m_kext) {
        rv = m_kext->m_keyNote;
    }
    return rv;
};

void ard::KRingKey::setKeyData(QString title, QString login, QString pwd, QString link, QString note)
{
    ASSERT_VALID(this);
    if (m_kext && !m_kext->isLocked()) {
        m_kext->m_keyTitle  = title;
        m_kext->m_keyLogin  = login;
        m_kext->m_keyPwd    = pwd;
        m_kext->m_keyLink   = link;
        m_kext->m_keyNote   = note;
        m_kext->m_modified = true;

        if (isSyncDbAttached()) {
            m_kext->setSyncModified();
            m_kext->ask4persistance(np_SYNC_INFO);
        }
        m_kext->ask4persistance(np_ATOMIC_CONTENT);
        m_kext->ensureExtPersistant(dataDb());
    }
};

FieldParts ard::KRingKey::fieldValues(EColumnType column_type, QString type_label)const 
{
    Q_UNUSED(type_label);
    ASSERT_VALID(this);
    FieldParts rv;
    if (m_kext && !m_kext->isLocked()) {
        switch (column_type) {
        case EColumnType::KRingTitle:       
        {
            rv.add(FieldParts::KRingTitle, keyTitle());
        }break;
        case EColumnType::KRingLogin: 
        {
            rv.add(FieldParts::KRingLogin, keyLogin());
        }break;
        case EColumnType::KRingPwd: 
        {
            rv.add(FieldParts::KRingPwd, keyPwd());
        }break; 
        case EColumnType::KRingLink: 
        {
            rv.add(FieldParts::KRingLink, keyLink());
        }break; 
        case EColumnType::KRingNotes: 
        {
            rv.add(FieldParts::KRingNotes, keyNote());
        }break;
        default:break;
        }
    }

    return rv;
};

void ard::KRingKey::setFieldValues(EColumnType column_type, QString type_label, const FieldParts& fp) 
{
    Q_UNUSED(type_label);
    ASSERT_VALID(this);
    if (m_kext && !m_kext->isLocked()) {
        switch (column_type) {
        case EColumnType::KRingTitle:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for ktitle %1").arg(fp.parts().size()));
            assert_return_void((fp.parts()[0].first == FieldParts::KRingTitle), QString("expected ktitle part for title %1").arg(static_cast<int>(fp.parts()[0].first)));
            QString str = fp.parts()[0].second;
            if (m_kext->m_keyTitle.compare(str) != 0) {
                m_kext->m_keyTitle = str;
                m_kext->m_modified = true;
            }
        }break;
        case EColumnType::KRingLogin:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for klogin %1").arg(fp.parts().size()));
            assert_return_void((fp.parts()[0].first == FieldParts::KRingLogin), QString("expected klogin part %1").arg(static_cast<int>(fp.parts()[0].first)));
            QString str = fp.parts()[0].second;
            if (m_kext->m_keyLogin.compare(str) != 0) {
                m_kext->m_keyLogin = str;
                m_kext->m_modified = true;
            }
        }break;
        case EColumnType::KRingPwd:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for kpwd %1").arg(fp.parts().size()));
            assert_return_void((fp.parts()[0].first == FieldParts::KRingPwd), QString("expected kpwd part %1").arg(static_cast<int>(fp.parts()[0].first)));
            QString str = fp.parts()[0].second;
            if (m_kext->m_keyPwd.compare(str) != 0) {
                m_kext->m_keyPwd = str;
                m_kext->m_modified = true;
            }
        }break;
        case EColumnType::KRingLink:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for klink %1").arg(fp.parts().size()));
            assert_return_void((fp.parts()[0].first == FieldParts::KRingLink), QString("expected klink part %1").arg(static_cast<int>(fp.parts()[0].first)));
            QString str = fp.parts()[0].second;
            if (m_kext->m_keyLink.compare(str) != 0) {
                m_kext->m_keyLink = str;
                m_kext->m_modified = true;
            }
        }break;
        case EColumnType::KRingNotes:
        {
            assert_return_void((fp.parts().size() == 1), QString("expected one part for knote %1").arg(fp.parts().size()));
            assert_return_void((fp.parts()[0].first == FieldParts::KRingNotes), QString("expected knote part %1").arg(static_cast<int>(fp.parts()[0].first)));
            QString str = fp.parts()[0].second;
            if (m_kext->m_keyNote.compare(str) != 0) {
                m_kext->m_keyNote = str;
                m_kext->m_modified = true;
            }
        }break;
        default:break;
        }
        

        if (m_kext->m_modified) {
            if (isSyncDbAttached()) {
                m_kext->setSyncModified();
                m_kext->ask4persistance(np_SYNC_INFO);
            }
            m_kext->ask4persistance(np_ATOMIC_CONTENT);
            m_kext->ensureExtPersistant(dataDb());
        }
    }
};

TOPICS_LIST ard::KRingKey::produceFormTopics(std::set<EColumnType>*,
    std::set<EColumnType>*)
{
    ASSERT_VALID(this);
    TOPICS_LIST lst;
    FormFieldTopic* f = nullptr;

#define ADD_FIELD(F) f = new FormFieldTopic(this, F, "");lst.push_back(f);
    ADD_FIELD(EColumnType::KRingTitle);
    ADD_FIELD(EColumnType::KRingLogin);
    ADD_FIELD(EColumnType::KRingPwd);
    ADD_FIELD(EColumnType::KRingLink);
#undef ADD_FIELD

    return lst;
}

QString ard::KRingKey::fieldMergedValue(EColumnType column_type, QString)const
{
    ASSERT_VALID(this);
    QString rv;
    if (m_kext) {
        switch (column_type)
        {
        case EColumnType::KRingTitle:   {rv = m_kext->m_keyTitle; }break;
        case EColumnType::KRingLogin:   {rv = m_kext->m_keyLogin; }break;
        case EColumnType::KRingPwd:     {rv = m_kext->m_keyPwd; }break;
        case EColumnType::KRingLink:    {rv = m_kext->m_keyLink; }break;
        case EColumnType::KRingNotes:   {rv = m_kext->m_keyNote; }break;
        default:ASSERT(0, QString("Unsupported kring column %1").arg(static_cast<int>(column_type)));
        }
    }
    return rv;
};

bool ard::KRingKey::hasText4SearchFilter(const TextFilterContext& fc)const 
{
    bool rv = true;
    if (m_kext && !m_kext->isLocked()) {
        rv =    (m_kext->m_keyTitle.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1) ||
                (m_kext->m_keyLogin.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1) ||
                (m_kext->m_keyPwd.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1) ||
                (m_kext->m_keyLink.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1) || 
                (m_kext->m_keyNote.indexOf(fc.key_str, 0, Qt::CaseInsensitive) != -1);
    }

    return rv;
};

/**
    anKRingKeyExt
*/
ard::anKRingKeyExt::anKRingKeyExt()
{

};

ard::anKRingKeyExt::anKRingKeyExt(topic_ptr _owner, QSqlQuery& q)
{
    attachOwner(_owner);
    m_encryptedContent = q.value(1).toByteArray();
    clearContent();
    m_mod_counter = q.value(2).toInt();
    _owner->addExtension(this);
};

/*ard::KRingKey* ard::anKRingKeyExt::owner() 
{
    auto rv = dynamic_cast<ard::KRingKey*>(cit_owner());
    return rv;
};

const ard::KRingKey* ard::anKRingKeyExt::owner()const 
{
    auto rv = dynamic_cast<const ard::KRingKey*>(cit_owner());
    return rv;
};*/


bool ard::anKRingKeyExt::lockContent() 
{
    if (m_modified) {
        ASSERT(0, "modified data can't be locked");
        return false;
    }
    clearContent();
    m_locked = true;
    return true;
};

void ard::anKRingKeyExt::assignSyncAtomicContent(const cit_primitive* _other)
{
    assert_return_void(_other, "expected prj extension");
    const ard::anKRingKeyExt* other = dynamic_cast<const ard::anKRingKeyExt*>(_other);
    assert_return_void(other, QString("expected crypto extension %1").arg(_other->dbgHint()));
    m_encryptedContent = other->m_encryptedContent;
    ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::anKRingKeyExt::create()const
{
    return new ard::anKRingKeyExt;
};

bool ard::anKRingKeyExt::isAtomicIdenticalTo(const cit_primitive* _other, int&)const
{
	assert_return_false(_other, "expected item [1]");
    const ard::anKRingKeyExt* other = dynamic_cast<const ard::anKRingKeyExt*>(_other);
	assert_return_false(other, "expected item [2]");

    if (m_encryptedContent != other->m_encryptedContent)
    {
        QString s = QString("ext-ident-err:%1").arg(extName());
        sync_log(s);
        on_identity_error(s);
        return false;
    }
    return true;
};

QString ard::anKRingKeyExt::calcContentHashString()const
{
    QString rv = QCryptographicHash::hash(m_encryptedContent, QCryptographicHash::Md5).toHex();
    return rv;
};

uint64_t ard::anKRingKeyExt::contentSize()const
{
    uint64_t rv = m_encryptedContent.size();
    return rv;
};

QByteArray ard::anKRingKeyExt::encryptedContent()const 
{
    if (m_modified) {
        auto f = owner();
        if (!f) {
            ASSERT(0, "expected kringext owner");
            return QByteArray();
        }
        auto kr = f->kroot();
        if (!kr) {
            ASSERT(0, "expected kroot");
            return QByteArray();
        }

        auto pwd = kr->pwd();
        auto hint = kr->hint();
        if (pwd.isEmpty()) {
            ASSERT(0, "kroot pwd is not set");
            return QByteArray();
        }

        QJsonObject js;
        toJson(js);
        QJsonDocument doc(js);
        auto bd = doc.toJson(QJsonDocument::Indented);
        auto res = ard::aes_encrypt_archive(pwd, hint, bd);
        if (res.first != ard::aes_status::ok) {
            ASSERT(0, "failed to encrypt key") << ard::aes_status2str(res.first);
            return QByteArray();
        }

        m_encryptedContent = res.second;
    }

    return m_encryptedContent;
};

void ard::anKRingKeyExt::fromJson(const QJsonObject& js)
{
    m_keyTitle  = QByteArray::fromBase64(js["title"].toString().toUtf8());
    m_keyLogin  = QByteArray::fromBase64(js["login"].toString().toUtf8());
    m_keyPwd    = QByteArray::fromBase64(js["pwd"].toString().toUtf8());
    m_keyLink   = QByteArray::fromBase64(js["link"].toString().toUtf8());
    m_keyNote   = QByteArray::fromBase64(js["note"].toString().toUtf8());
};

void ard::anKRingKeyExt::toJson(QJsonObject& js)const
{
    js["title"]     = QString(m_keyTitle.toUtf8().toBase64());
    js["login"]     = QString(m_keyLogin.toUtf8().toBase64());
    js["pwd"]       = QString(m_keyPwd.toUtf8().toBase64());
    js["link"]      = QString(m_keyLink.toUtf8().toBase64());
    js["note"]      = QString(m_keyNote.toUtf8().toBase64());
};

static QString decorateGuiStr(const QByteArray& ba)
{
    static char ctbl[] = ".o01";
    constexpr char b = sizeof(ctbl);

    QString rv;
    for (const auto& c : ba) {
        char i = c % b;
        char v = '.'; //ctbl[i];
        if (i < b) {
            v = ctbl[i];
        }
        else {
            ASSERT(0, "idx err");
        }
        rv += v;
    }
    return rv;
}

void ard::anKRingKeyExt::clearContent()
{
    m_keyTitle  = "";
    m_keyLogin  = "";
    m_keyPwd    = "";
    m_keyLink   = "";
    m_keyNote   = "";


    if (m_encryptedContent.size() > 0) {
        auto tmp    = m_encryptedContent.left(64);
        m_keyTitle  = decorateGuiStr(tmp);
        m_keyLogin  = decorateGuiStr(QCryptographicHash::hash(tmp.left(32), QCryptographicHash::Md5));
        m_keyPwd    = decorateGuiStr(QCryptographicHash::hash(tmp.left(16), QCryptographicHash::Md5));
        m_keyLink   = decorateGuiStr(QCryptographicHash::hash(tmp.left(32), QCryptographicHash::Md5));
        m_keyNote = m_keyLogin;
    }
};
