#pragma once

#include <memory>
#include <QSqlQuery>
#include "a-db-utils.h"
#include "anfolder.h"
#include "tooltopic.h"
#include "ansyncdb.h"

namespace ard{
    class KRingRoot;
    class KRingKey;

    class kring_model
    {
    public:
        kring_model(ArdDB* db);
		~kring_model();

        ard::KRingRoot*             keys_root();
        const ard::KRingRoot*       keys_root()const;       

    protected:
        ard::KRingRoot*   m_kring_root{ nullptr };        
    };

    class KRingRoot : public RootTopic
    {
    public:
        DECLARE_ROOT(KRingRoot, objKRingRoot, ESingletonType::keysHolder);
        QString                 title()const override { return "Key Ring"; };

        bool                    isRingLocked()const;
        bool                    lockRing();
        bool                    guiUnlockRing();

        bool                    decryptKRing(QString pwd);

        ard::KRingKey*          addKey(QString key_title);
        QString                 pwd()const { return m_pwd; }
        QString                 hint()const { return m_hint; }

        TOPICS_LIST             filteredItems(QString indexStr = "*");

        static ard::KRingKey*   hoistedRingKeyInFilter();
        static void             setHoistedRingKeyInFilter(ard::KRingKey*);
    protected:
        QString                 m_pwd, m_hint;
    };

    /**
        We are one key with properties:
        Title Login Pwd Link Note
    */
    class KRingKey : public ard::topic 
    {
    public:
        KRingKey();

        KRingRoot*              kroot();
        const KRingRoot*        kroot()const;
        anKRingKeyExt*          kext();
        const anKRingKeyExt*    kext()const;
        anKRingKeyExt*          ensureKExt();
        QString                 objName()const override;

        static KRingKey*        createNewKey(QString key_title);

        QString                 keyTitle()const;
        QString                 keyLogin()const;
        QString                 keyPwd()const;
        QString                 keyLink()const;
        QString                 keyNote()const;

        void                    setKeyData(QString title, QString login, QString pwd, QString link, QString note);

        ard::aes_result         decryptKey(QString pwd);
        bool                    canBeMemberOf(const topic_ptr)const override;
        EOBJ                    otype()const override { return objKRingKey; };
        QString                 impliedTitle()const override;
        FieldParts              fieldValues(EColumnType column_type, QString type_label)const override;
        void                    setFieldValues(EColumnType column_type, QString type_label, const FieldParts& parts)override;
        ENoteView               noteViewType()const override { return ENoteView::None; };
        bool                    hasFatFinger()const override { return false; };
        bool                    isEmptyTopic()const override { return false; }
        cit_prim_ptr            create()const override;
        TOPICS_LIST             produceFormTopics(std::set<EColumnType>* include_columns = nullptr,
            std::set<EColumnType>* exclude_columns = nullptr)override;
        QString                 fieldMergedValue(EColumnType column_type, QString type_label)const override;
        EColumnType             formNotesColumn()const override { return EColumnType::KRingNotes; }
        bool                    hasText4SearchFilter(const TextFilterContext& fc)const override;
    protected:
        void mapExtensionVar(cit_extension_ptr e)override;

    protected:
        anKRingKeyExt*          m_kext{ nullptr };
    };

    /**
        anKRingKeyExt       
    */
    class anKRingKeyExt : public ardiExtension<anKRingKeyExt, KRingKey>
    {
        DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extRingKey, "rkey-ext", "ard_ext_kring");
    public:

        ///default constructor
        anKRingKeyExt();
        ///for recovering from DB
        anKRingKeyExt(topic_ptr _owner, QSqlQuery& q);

        void   assignSyncAtomicContent(const cit_primitive* other)override;
        snc::cit_primitive* create()const override;
        bool   isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
        QString calcContentHashString()const override;
        uint64_t contentSize()const override;
        bool isModified()const { return m_modified; }
        void clearModified() { m_modified = false; }

        QByteArray encryptedContent()const;
        bool    lockContent();
        bool    isLocked()const {return m_locked;}

    protected:

        void fromJson(const QJsonObject& js);
        void toJson(QJsonObject& js)const;
        void clearContent();
        mutable QByteArray  m_encryptedContent;
        bool        m_locked{ true };
        bool        m_modified{ false };
        QString     m_keyTitle, m_keyLogin, m_keyPwd, m_keyLink, m_keyNote;

        friend class KRingKey;
    };
};
