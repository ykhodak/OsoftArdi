#pragma once

#include <QLineEdit>

namespace ard
{
	class search_edit : public QLineEdit
	{
		Q_OBJECT
	public:
		search_edit(QWidget *parent = 0);

		void  signalColor();
		void  resyncColor();

	signals:
		void  applyCommand();
		void  escapeCommand();
		void  focusIn();

	protected:
		void  keyPressEvent(QKeyEvent * event)override;
		void  focusInEvent(QFocusEvent *event)override;
		void  paintEvent(QPaintEvent * event)override;

		private slots:
		void updateSearchText(const QString &text);
		void restoreStyle();

	protected:
		QString m_storedStyle;
	};
}