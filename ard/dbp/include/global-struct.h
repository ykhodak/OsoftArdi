#pragma once

#include <QSqlQuery>
#include <QRgb>
#include <QPainter>

namespace ard 
{
	class topic;
};

template<class B, class P>
class ArdPimplWrapper: public B
{
public:
    ArdPimplWrapper() { m_o.reset(new P); }
    P* optr() { return m_o.get(); }
    const P* optr()const { return m_o.get(); }
    void reset_pimpl_obj(std::unique_ptr<P>&& d) { m_o = std::move(d); };
protected:
    std::unique_ptr<P> m_o;
};



class WaitCursor
{
public:
  WaitCursor();
  ~WaitCursor();
};

namespace ard {
    struct PainterGuard
    {
        PainterGuard(QPainter* p):m_p(p) { m_p->save(); };
        virtual ~PainterGuard() { m_p->restore(); }
        QPainter* m_p;
    };
}

#define PGUARD(P) ard::PainterGuard _pg1(P);

/**
   SqlQuery - internal query, used in all DB access code
*/
class SqlQuery: public QSqlQuery
{
  friend class ArdDB;
public:

  virtual ~SqlQuery();

  bool exec();
  bool execBatch();


protected:
  QVariantList m_batch_param1;

private:
  SqlQuery(QSqlDatabase& db):QSqlQuery(db){}

  SqlQuery(const SqlQuery& src):QSqlQuery(src){};
  SqlQuery& operator=(const SqlQuery&){ASSERT(0, "NA"); return *this;};

#ifdef _SQL_PROFILER
  void runProfiler(int duration, STRING_LIST& bt);
#endif
};

class TopicMatch
{
public:
  virtual ~TopicMatch(){}
  virtual bool match (ard::topic*) = 0;
};


class ArdNIL
{
public:
    static ArdNIL& nil();

    ArdNIL &  operator<<(double ) {return *this;}
    ArdNIL &  operator<<(const char * ) {return *this;}
    ArdNIL &  operator<<(const QString & ) {return *this;}
    ArdNIL &  operator<<(const QDateTime & )  {return *this;}
};

struct TextFilterContext
{
    QString key_str;
    bool    include_expanded_notes;
};

#ifdef ARD_PRINT_SQL
    #define sqlPrint() qDebug() << "[sql]"
#else
    #define sqlPrint() ArdNIL::nil()
#endif

