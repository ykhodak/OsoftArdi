#pragma once

#include <QTableView>
#include <QTimer>
#include "fileref.h"

class QTableView;
class QLabel;

namespace ard 
{
	class fileref;
	class tda;

	class tda_view : public QWidget 
	{
	public:
		tda_view();
		virtual ~tda_view();

		void                        attachFileRef(ard::fileref*);
		void                        detach();
		void                        reloadTDAfile();
	protected:
		std::pair<QString, QString> getStatementFileName()const;
		void						initTimer();
		void                        updateSelectionSumLabel(QModelIndexList& lst);

		fileref*					m_tda_file_ref{ nullptr };
		std::unique_ptr<tda>		m_tda;
		QTimer						m_file_check_timer;
		QDateTime					m_statement_load_time;

		QTabWidget*					m_main_tab{ nullptr };
		QTabWidget*					m_detail_tab{ nullptr };
		QTableView*					m_trades_view{ nullptr };
		QLabel*						m_sum_label{ nullptr };
		QLabel*						m_avg_label{ nullptr };
	};
};