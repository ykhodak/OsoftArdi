#pragma once
#include <vector>
#include <unordered_set>
#include <functional>

namespace ard 
{
	void        error(QString s);
	void		trail(QString);
};

extern QString getLastStackFrames(int frames);
extern void get_stacktrace(std::vector<QString>& bt);

#define ASSERT(A, T) if(!(A))qWarning() << "[error]" << __FILE__ << ":" << __LINE__ << " [stack]:" << getLastStackFrames(8) << ":" << T

#define MAGIC_MEM_CHECK_NUMBER    12345671

#ifdef _DEBUG
#ifndef ARD_BENCHMARK
#define ARD_BENCHMARK
#endif //ARD_BENCHMARK

#define ASSERT_VALID(S) if(S->counter() == 0 || S->counter() > 10000 || S->magic_number() != MAGIC_MEM_CHECK_NUMBER){ASSERT(0, "invalid object:") << S->dbgHint() << "|" << S->counter() << S->magic_number();};
#define dbg_print(S)	qDebug() << S
#else
#define ASSERT_VALID(S)
#define dbg_print(S)
#endif

#define assert_valid(S) ASSERT_VALID(S)

/**
   ARD_BREAK_ON_ASSERT
   enable debug break on VS involves __debugbreak() call
   if it's enabled program will stop execution and promt will offer
   option to join VS debugger. Usefull for debug mode but not always
*/

#ifdef _MSC_VER
#pragma warning (disable : 4267)
#ifdef _DEBUG
#define ARD_BREAK_ON_ASSERT
#endif
#endif //_MSC_VER


#ifdef ARD_BREAK_ON_ASSERT
#undef ASSERT
#define ASSERT(A, T) if(!(A)){__debugbreak();}; if(!(A))qWarning() << "[error]" << __FILE__ << ":" << __LINE__ << " [stack]:" << getLastStackFrames(8) << ":" << T
#endif //ARD_BREAK_ON_ASSERT


#define assert_return_false(A, T){ASSERT(A, T);if(!(A)) return false;}
#define assert_return_void(A, T){ASSERT(A, T);if(!(A)) return;}
#define assert_return_empty(A, T){ASSERT(A, T);if(!(A)) return "";}
#define assert_return_null(A, T){ASSERT(A, T);if(!(A)) return NULL;}
#define assert_return_0(A, T){ASSERT(A, T);if(!(A)) return 0;}


#ifdef ARD_BENCHMARK
#define CRUN(F) {QDateTime t1 = QDateTime::currentDateTime();\
                F();                                        \
                QString s1 = QString("[%1 ms]").arg(t1.msecsTo(QDateTime::currentDateTime()));\
                QString s = QString("[crun]%1 %2").arg(s1, 10).arg(#F);\
                qWarning() << s; \
                }\

#else
#define CRUN(F) F();
#endif //ARD_BENCHMARK

