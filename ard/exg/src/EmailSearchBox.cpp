#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QMenu>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include "EmailSearchBox.h"
#include "custom-widgets.h"
#include "flowlayout.h"
#include "gmail/GmailRoutes.h"
#include "locus_folder.h"

using namespace googleQt;

std::pair<bool, QString> EmailSearchBox::edit_qstr(QString qstr)
{
    std::pair<bool, QString> rv;
    EmailSearchBox d(qstr);
    d.exec();
    rv.first = d.m_accepted;
    rv.second = d.m_result_qstr.trimmed();
    return rv;
};


EmailSearchBox::EmailSearchBox(QString qstr)
{
    QVBoxLayout *v_main = new QVBoxLayout;
    //m_fgroup = new QButtonGroup(this);

    QVBoxLayout *v_top = new QVBoxLayout;
    v_main->addLayout(v_top);
    v_top->addWidget(new QLabel("Include emails using following rule"));

    QVBoxLayout *v_customf = new QVBoxLayout;
    //v_main->addLayout(v_customf);

    m_edit = new QPlainTextEdit;
    m_result_qstr = qstr;
    v_customf->addWidget(m_edit);
    m_edit->setPlainText(m_result_qstr);


    ///flow battons layout
    FlowLayout* bl = new FlowLayout;
    v_customf->addLayout(bl);

    m_customf_space = new QWidget;
    m_customf_space->setLayout(v_customf);
    v_main->addWidget(m_customf_space);
    

    QPushButton* b = nullptr;

#define ADD_BUTTON(T)  b = new QPushButton(T);  \
    gui::setButtonMinHeight(b);                 \
    bl->addWidget(b);                       \

    ADD_BUTTON("from:");
    QObject::connect(b, &QPushButton::released, [=]() 
    {
        addSearchOpt("from:");
    });

    ADD_BUTTON("to:");
    QObject::connect(b, &QPushButton::released, [=]()
    {
        addSearchOpt("to:");
    });

    ADD_BUTTON("subject:");
    QObject::connect(b, &QPushButton::released, [=]()
    {
        addSearchOpt("subject:");
    });

    if (ard::gmail()) {
        ADD_BUTTON("label:");
        QObject::connect(b, &QPushButton::released, [=]()
        {
            assert_return_void(ard::gmail(), "expected gmail");
            assert_return_void(ard::gmail()->cacheRoutes(), "expected gmail cache");

            QMenu m(this);
            ard::setup_menu(&m);
            connect(&m, &QMenu::triggered, [&](QAction* a)
            {
                addSearchOpt(QString("label:%1").arg(a->text()));
            });

            auto labels = ard::gmail()->cacheRoutes()->getLoadedLabels();
            for (auto lb : labels)
            {
                QAction* a1 = new QAction(lb->labelName(), this);
                m.addAction(a1);
            }

            m.exec(QCursor::pos());
        });
    }

    ADD_BUTTON("has:");
    QObject::connect(b, &QPushButton::released, [=]()
    {
        ::STRING_LIST has_opt;
        has_opt.push_back("attachment");
        has_opt.push_back("youtube");
        has_opt.push_back("drive");
        has_opt.push_back("document");
        has_opt.push_back("spreadsheet");
        has_opt.push_back("presentation");
        has_opt.push_back("userlabels");
        has_opt.push_back("nouserlabels");

        QMenu m(this);
        ard::setup_menu(&m);
        connect(&m, &QMenu::triggered, [&](QAction* a)
        {
            addSearchOpt(QString("has:%1").arg(a->text()));
        });

        for (auto s : has_opt) {
            QAction* a1 = new QAction(s, this);
            m.addAction(a1);
        }
        m.exec(QCursor::pos());
    });


    ADD_BUTTON("is:");
    QObject::connect(b, &QPushButton::released, [=]()
    {
        ::STRING_LIST is_opt;
        is_opt.push_back("starred");
        is_opt.push_back("snoozed");
        is_opt.push_back("unread");
        is_opt.push_back("read");

        QMenu m(this);
        ard::setup_menu(&m);
        connect(&m, &QMenu::triggered, [&](QAction* a)
        {
            addSearchOpt(QString("is:%1").arg(a->text()));
        });

        for (auto s : is_opt) {
            QAction* a1 = new QAction(s, this);
            m.addAction(a1);
        }
        m.exec(QCursor::pos());
    });

    ADD_BUTTON("after:");
    QObject::connect(b, &QPushButton::released, [=]()
    {
        ::STRING_LIST after_opt;
        after_opt.push_back("after");
        after_opt.push_back("before");
        after_opt.push_back("older");
        after_opt.push_back("newer");

        QMenu m(this);
        ard::setup_menu(&m);
        connect(&m, &QMenu::triggered, [&](QAction* a)
        {            
            addSearchOpt(QString("%1:").arg(a->text()));
        });

        for (auto s : after_opt) {
            QAction* a1 = new QAction(s, this);
            m.addAction(a1);
        }
        m.exec(QCursor::pos());
    });
    m_or_check = new QCheckBox("OR");
    bl->addWidget(m_or_check);
    ///end flow buttons

    QHBoxLayout *h_2 = new QHBoxLayout;
    /*
    m_or_check = new QCheckBox("OR");
    h_2->addWidget(m_or_check);
    */
    ard::addBoxButton(h_2, "OK", [&]()
                     {
        m_result_qstr = m_edit->toPlainText().trimmed();
        m_accepted = true;
        close(); 
    });

    ard::addBoxButton(h_2, "Cancel", [&]()
    {
        m_accepted = false;
        close();
    });

//    ard::addBoxButton(h_2, "Clear", [&]() {m_edit->setPlainText("");});

    v_main->addLayout(h_2);
    ////....
#ifndef ARD_BIG
    /*
    m_edit->setMaximumHeight(gui::lineHeight() * 2);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    v_customf->addWidget(spacer);
    */
#endif //ARD_BIG
    ////....


    MODAL_DIALOG(v_main);
};

void EmailSearchBox::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
#ifdef _DEBUG
    QRect rc = geometry();
    QString s = QString("geometry[%1x%2]").arg(rc.width()).arg(rc.height());
    setWindowTitle(s);
#endif
}

void EmailSearchBox::addSearchOpt(QString s) 
{
    QString search = m_edit->toPlainText().trimmed();

    if (m_or_check->isChecked()) {
        auto idx = search.lastIndexOf("OR");
        //qDebug() << "OR-pos" << idx << "len=" << search.size() << "IN" << search;
        if (idx != search.size() - 2)
        {
            search += " OR ";
        }
        else {
            search += " ";
        }
        search += s;
    }
    else 
    {
        search += " ";
        search += s;
    }
    m_edit->setPlainText(search);
    m_edit->moveCursor(QTextCursor::End);
    m_edit->setFocus();
};

/*
void EmailSearchBox::filterTypeChanged() 
{
    assert_return_void(m_fgroup, "expected buttons group");
    assert_return_void(m_customf_space, "expected custom filter space");

    switch (m_fgroup->checkedId()) {
    case 1: {
        m_customf_space->setEnabled(false);
        m_labels->setEnabled(false);
    }break;
    case 2: {
        m_customf_space->setEnabled(false);
        m_labels->setEnabled(true);
    }break;
    case 3: {
        m_customf_space->setEnabled(true);
        m_labels->setEnabled(false);
    }break;
    }
};
*/
