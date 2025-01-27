#pragma once

#include <QDialog>
class QTabWidget;
class QLineEdit;
class QComboBox;
class QLabel;

namespace ard 
{
	class roc_box : public QDialog
	{
		Q_OBJECT
	public:
		static void showBox();
	protected:
		roc_box();
		void updateRoc();

		QTabWidget*     m_main_tab{ nullptr };
		QLineEdit*		m_days{ nullptr };
		QLineEdit*		m_price1{ nullptr };
		QLineEdit*		m_price2{ nullptr };
		QLineEdit*		m_roc{ nullptr };
		QLabel*			m_pivot{ nullptr };
		QLabel*			m_profit{ nullptr };
		QComboBox*		m_margin{ nullptr };
	};
};