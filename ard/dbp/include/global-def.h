#pragma once

class anContactGroup;
class FormFieldTopic;

namespace ard 
{
	class topic;
    class email;
    class board_link;
    class contact;
    class contact_group;
    class ethread;
	class locus_folder;
};

#define topic_ptr               ard::topic*
#define topic_cptr              const ard::topic*
#define email_ptr               ard::email*
#define email_cptr              const ard::email*
#define ethread_ptr             ard::ethread*
#define ethread_cptr            const ard::ethread*
#define contact_ptr             ard::contact*
#define contact_cptr            const ard::contact*
#define formfield_ptr           FormFieldTopic*
#define formfield_cptr          const FormFieldTopic*

namespace ard {
    email_ptr           as_email(topic_ptr f);
    email_cptr          as_email(topic_cptr f);
    ethread_ptr         as_ethread(topic_ptr f);
    ethread_cptr        as_ethread(topic_cptr f);
};

#define ENABLE_OBJ(A, V)if(A != NULL){A->setEnabled(V);A->setVisible(V);}
#define ENABLE_OBJ_LIST(C, V){for(auto& i : C)ENABLE_OBJ((i), V);};

#define SET_CELL(T) it=new QStandardItem(T);it->setFont(m_working_font); m->setItem(row, col, it);col++;



#define ADD_BUTTON2LAYOUT1(L, T, O, F)   b = new QPushButton(this); \
  b->setText(T);                            \
  gui::setButtonMinHeight(b);                       \
  connect(b, &QPushButton::released, O, F);             \
  L->addWidget(b);                          \


#define ADD_BUTTON2LAYOUT(L, T, F) ADD_BUTTON2LAYOUT1(L, T, this, F);

#define MODAL_DIALOG(L)     setLayout(L);   \
  gui::resizeWidget(this, QSize(0,0));      \
  setModal(true);               \

#define MODAL_DIALOG_SIZE(L, S) setupMainLayout(L); \
  setLayout(L);                     \
  gui::resizeWidget(this, S);               \
  setModal(true);                   \


#define ICON_WIDTH              gui::lineHeight()
#define IWIDTH(A)               (A > ICON_WIDTH ? A : ICON_WIDTH)

#define ADD_SPACER(L) {                                                 \
        QWidget* s = new QWidget();                                     \
        s->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); \
        L->addWidget(s);                                                \
    }                                                                   \


#define ADD_CONTEXT_MENU_IMG_ACTION(I, L, M){\
QToolButton *btn = new QToolButton(&m);\
btn->setText(L);\
btn->setIcon(utils::fromTheme(I));\
btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);\
btn->setIconSize(QSize(32, 32));\
QObject::connect(btn, &QToolButton::released, [&]() {m.close(); M(); });\
auto *a = new QWidgetAction(&m);\
a->setDefaultWidget(btn);\
m.addAction(a);\
}\


#define ADD_TEXT_MENU_ACTION(L, M){\
a = new QAction(L);\
m.addAction(a);\
QObject::connect(a, &QAction::triggered, [&]() {M();});\
}\


#ifdef ARD_BIG
    #define TEXT_BAR_BUTTON_SIZE    32
#else
    #define TEXT_BAR_BUTTON_SIZE    ICON_WIDTH
#endif

///January 1, 2000 - some contemporary time that makes sense
#define VALID_START_TIME_T  414086872

#define ADD_MENU_ACTION(T, D) a = new QAction(T, this);     \
    a->setData((int)D);                                     \
    m.addAction(a);                                         \


#define ARD_MARGIN  2

#define COLOR_PANEL_SEP QColor(255,127,39)  
#define DEFAULT_OPACITY         1.0
#define DIMMED_OPACITY          0.5

#define DEFAULT_MAIN_WND_WIDTH          300
#define BBOARD_DEFAULT_HEIGHT           1200
#define BBOARD_DELTA_EXPAND_HEIGHT      200
#define BBOARD_MAX_BOX_WIDTH            280

#define BBOARD_BAND_MIN_WIDTH			100
#define BBOARD_BAND_MAX_WIDTH			1200
#define BBOARD_BAND_DEFAULT_WIDTH		300
#define BBOARD_MAX_Y_DELTA				1200

typedef void(*TRY_ABORT_FUNCTION)(void*);
extern bool tryCatchAbort(TRY_ABORT_FUNCTION f, void* vparam, QString contextInfo);
