#include <QToolButton>
#include <QStyle>
#include <QKeyEvent>
#include <QDebug>
#include <QTimer>
#include "search_edit.h"
#include "a-db-utils.h"

ard::search_edit::search_edit(QWidget *parent)
    : QLineEdit(parent)
{
    setAccessibleName("ard::search_edit");
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateSearchText(const QString&)));
}

void ard::search_edit::updateSearchText(const QString& )
{
  emit applyCommand();
}

void ard::search_edit::resyncColor()
{
    //static    QString ssActive = "QLineEdit{background: red;color:black}";
	static    QString ssActive = "QLineEdit{background: #fababd;color:black}";
    static    QString ssNormal = "QLineEdit{background: none;color:none}";

    bool bUpdate = false;

    if (gui::searchFilterIsActive())
    {

        if (styleSheet() != ssActive)
        {
            setStyleSheet(ssActive);
            bUpdate = true;
        }
    }
    else
    {
        if (styleSheet() != ssNormal)
        {
            setStyleSheet(ssNormal);
            bUpdate = true;
        }
    }

    if (bUpdate)
    {
        update();
    }
};

void ard::search_edit::signalColor()
{
  m_storedStyle = styleSheet();
  setStyleSheet("QLineEdit{background: yellow;color:black}");
  QTimer::singleShot(1000, this, SLOT(restoreStyle()));
};

void ard::search_edit::restoreStyle()
{
  setStyleSheet(m_storedStyle);
  update();
};

void ard::search_edit::keyPressEvent(QKeyEvent * e)
{
    QLineEdit::keyPressEvent(e);

    switch (e->key())
    {
    case Qt::Key_Return:
    {
        emit applyCommand();
    }break;
    case Qt::Key_Escape:
    {
        emit escapeCommand();
    }break;
    }
};

void ard::search_edit::focusInEvent(QFocusEvent *e)
{
    QLineEdit::focusInEvent(e);
    emit focusIn();
};

void ard::search_edit::paintEvent(QPaintEvent * e)
{
    QLineEdit::paintEvent(e);
	//if (!gui::searchFilterIsActive())
	/*
	auto s = text();
	if(s.isEmpty())
	{
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		QRect rc_ico = rect();
		int ico_w = rc_ico.height();
		//rc_ico.setWidth(ico_w);
		//rc_ico.setLeft(rc_ico.left() - ico_w);
		rc_ico.setLeft(rc_ico.right() - ico_w);
		rc_ico.setBottom(rc_ico.top() + ico_w);
		p.setOpacity(0.7);
		p.drawPixmap(rc_ico, getIcon_SearchGlass());
	}*/
};
