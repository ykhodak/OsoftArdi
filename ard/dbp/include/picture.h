#pragma once
#include "anfolder.h"

namespace ard 
{
	class picture : public ard::thumb_topic
	{
	public:
		picture(QString title = "");

		QPixmap					image();
		void					setFromImage(const QImage& img);

		QSize					calcBlackboardTextBoxSize()const override;
		bool					isEmptyTopic()const override;
		QString					imageFileName()const;
		void					captureMediaModTime();
		bool					isMediaModified()const;
		void					reloadMedia();
		EOBJ                    otype()const override { return objPicture; };
		ENoteView				noteViewType()const override { return ENoteView::Picture; };
		QString					objName()const override { return "picture"; }
		cit_prim_ptr            create()const override;
		topic_ptr               clone()const override;

		picture_ext*            pext();
		const picture_ext*      pext()const;
		picture_ext*            ensurePExt();
	protected:
		QString					thumbnailFileNamePrefix()const override { return "p"; };
		QPixmap					emptyThumb()const override;
		void					removeExternalStorage()override;
		void					mapExtensionVar(cit_extension_ptr e)override;
		void					do_set_local_mod_time(time_t);

		picture_ext*			m_pext{ nullptr };
		mutable QPixmap         m_pic;
	};

	/// picture details
	class picture_ext : public ardiExtension<picture_ext, picture>
	{
		DECLARE_DB_EXTENSION_PERSISTANT(snc::EOBJ_EXT::extPicture, "picture-ext", "ard_ext_picture");
	public:
		///default constructor
		picture_ext();
		///for recovering from DB
		picture_ext(topic_ptr _owner, QSqlQuery& q);

		QString				cloud_file_id()const { return m_cloud_file_id; }
		time_t				local_media_time()const { return m_local_media_time; }

		void   assignSyncAtomicContent(const cit_primitive* other)override;
		snc::cit_primitive* create()const override;
		bool   isAtomicIdenticalTo(const cit_primitive* other, int& iflags)const override;
		QString calcContentHashString()const override;
		uint64_t contentSize()const override;
		
	protected:
		time_t			m_local_media_time{0};
		QString			m_cloud_file_id;
		friend class picture;
	};
}
