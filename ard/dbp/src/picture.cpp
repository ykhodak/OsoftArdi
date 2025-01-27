#include <QFileInfo>
#include "a-db-utils.h"
#include "picture.h"
#include "a-db-utils.h"

ard::picture::picture(QString title) :ard::thumb_topic(title) 
{

};


QPixmap	ard::picture::image() 
{
	if (!m_thumb_flags.isImageQueried) {
		m_thumb_flags.isImageQueried = 1;
		auto s = imageFileName();
		if (QFile::exists(s)) {
			m_pic.load(s);
		}
		else {
			m_pic = emptyThumb();
			m_thumb_flags.isImageEmpty = 1;
		}
	}

	return m_pic;
};

QPixmap	ard::picture::emptyThumb()const
{
	return getIcon_EmptyPicture();
};

QString	ard::picture::imageFileName()const 
{
	auto s = QString("%1p-%2.png").arg(ard_dir(dirMedia)).arg(syid());
	return s;
};

void ard::picture::setFromImage(const QImage& img) 
{
	auto db = dataDb();
	assert_return_void(db, "expected DB");
	ASSERT(IS_VALID_DB_ID(id()), "expected valid DBID for picture");
	guiEnsureSyid(db, "P");
	m_pic = QPixmap::fromImage(img);
	auto s = imageFileName();
	if (!m_pic.isNull()) {
		m_pic.save(s);
		captureMediaModTime();
	}
	setThumbnail(m_pic);
};

void ard::picture::do_set_local_mod_time(time_t t)
{
	auto e = pext();
	if (e) 
	{
		e->m_local_media_time = t;
		if (e->hasDBRecord()) {
			if (dataDb() && isSyncDbAttached()) {
				e->setSyncModified();
				e->ask4persistance(np_SYNC_INFO);
			}
			e->ask4persistance(np_ATOMIC_CONTENT);
			e->ensureExtPersistant(dataDb());
		}
	}
};

void ard::picture::reloadMedia() 
{
	auto s = imageFileName();
	QFileInfo fi(s);
	if (fi.exists())
	{
		m_pic.load(s);
		setThumbnail(m_pic);
		ard::asyncExec(AR_PictureLoaded, this);
		//ard::asyncExec(AR_UpdateGItem, this);

		auto e = ensurePExt();
		if (e) {
			do_set_local_mod_time(fi.lastModified().toTime_t());
		}
	}
};

QSize ard::picture::calcBlackboardTextBoxSize()const
{
	return QSize{ MP1_WIDTH + 50, MP1_HEIGHT + gui::shortLineHeight() };
};

bool ard::picture::isEmptyTopic()const 
{
	auto r = ard::thumb_topic::isEmptyTopic();
	if (!r)
		return false;

	auto s = imageFileName();
	if (QFile::exists(s))
		return false;
	return true;
};

void ard::picture::removeExternalStorage() 
{
	auto s = imageFileName();
	if (QFile::exists(s)) 
	{ 
		if (!QFile::remove(s))
			qDebug() << "failed to delete image" << s; 
	}

	ard::thumb_topic::removeExternalStorage();
};

void ard::picture::captureMediaModTime() 
{
	auto s = imageFileName();
	QFileInfo fi(s);
	if (fi.exists()) 
	{
		time_t media_mod_time = fi.lastModified().toTime_t();
		auto e = ensurePExt();
		if (e) 
		{
			if (e->m_local_media_time == 0 || e->m_local_media_time < media_mod_time) 
			{
				do_set_local_mod_time(media_mod_time);
			}
		}
	}
};

bool ard::picture::isMediaModified()const 
{
	bool rv = false;
	auto e = pext();
	if (e)
	{
		if (e->m_local_media_time != 0)
		{
			rv = true;
			auto s = imageFileName();
			QFileInfo fi(s);
			if (fi.exists())
			{
				rv = (e->m_local_media_time != fi.lastModified().toTime_t());
			}
		}
	}
	return rv;
};

cit_prim_ptr ard::picture::create()const
{
	return new ard::picture;
};

topic_ptr ard::picture::clone()const
{
	auto db = dataDb();
	assert_return_null(db, "expected DB");

	SYID2SYID sy2sy;

	auto rv = dynamic_cast<ard::picture*>(create());
	rv->assignContent(this);
	rv->m_title = "clone of " + rv->m_title;
	rv->guiEnsureSyid(db, "PC");

	auto src_file_name = imageFileName();
	if (QFile::exists(src_file_name)) 
	{
		auto dest_file_name = rv->imageFileName();
		auto res = QFile::copy(src_file_name, dest_file_name);
		ASSERT(res, "failed to copy image") << src_file_name << dest_file_name;

		QPixmap pm;
		pm.load(dest_file_name);
		rv->setThumbnail(pm);
	}

	return rv;
};

ard::picture_ext* ard::picture::pext()
{
	return ensurePExt();
};

const ard::picture_ext* ard::picture::pext()const
{
	ard::picture* ThisNonCost = (ard::picture*)this;
	return ThisNonCost->ensurePExt();
};

ard::picture_ext* ard::picture::ensurePExt()
{
	ASSERT_VALID(this);
	if (m_pext)
		return m_pext;

	ensureExtension(m_pext);
	return m_pext;
};

void ard::picture::mapExtensionVar(cit_extension_ptr e)
{
	topic::mapExtensionVar(e);
	if (e->ext_type() == snc::EOBJ_EXT::extPicture) {
		ASSERT(!m_pext, "duplicate picture ext");
		m_pext = dynamic_cast<ard::picture_ext*>(e);
		ASSERT(m_pext, "expected picture ext");
	}
};


/**
	picture_ext
*/
ard::picture_ext::picture_ext() 
{

};

ard::picture_ext::picture_ext(topic_ptr _owner, QSqlQuery& q)
{
	attachOwner(_owner);
	m_mod_counter = q.value(1).toInt();
	m_cloud_file_id = q.value(2).toByteArray();
	m_local_media_time = q.value(3).value<time_t>();
	_owner->addExtension(this);
};

/*ard::picture* ard::picture_ext::owner()
{
	auto rv = dynamic_cast<ard::picture*>(cit_owner());
	return rv;
};

const ard::picture* ard::picture_ext::owner()const
{
	auto rv = dynamic_cast<const ard::picture*>(cit_owner());
	return rv;
};*/

void ard::picture_ext::assignSyncAtomicContent(const cit_primitive* _other)
{
	assert_return_void(_other, "expected board extension");
	auto* other = dynamic_cast<const ard::picture_ext*>(_other);
	assert_return_void(other, QString("expected board extension %1").arg(_other->dbgHint()));
	m_cloud_file_id = other->cloud_file_id();
	ask4persistance(np_ATOMIC_CONTENT);
};

snc::cit_primitive* ard::picture_ext::create()const
{
	return new ard::picture_ext;
};

bool ard::picture_ext::isAtomicIdenticalTo(const cit_primitive* _other, int&)const
{
	assert_return_false(_other, "expected item [1]");
	auto* other = dynamic_cast<const ard::picture_ext*>(_other);
	assert_return_false(other, "expected item [2]");
	if (m_cloud_file_id != other->m_cloud_file_id)
	{
		QString s = QString("ext-ident-err:%1").arg(extName());
		sync_log(s);
		on_identity_error(s);
		return false;
	}
	return true;
};

QString ard::picture_ext::calcContentHashString()const
{
	QString rv;
	return rv;
};

uint64_t ard::picture_ext::contentSize()const
{
	uint64_t rv = 0;
	return rv;
};

