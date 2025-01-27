#include <sstream>
#include <QFileInfo>
#include <QLabel>
#include <QComboBox>
#include <QTableView>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QStandardItem>
#include <QStandardItemModel>
#include "tda_view.h"
#include "csv-util.h"

namespace ard 
{
	struct tda_option;

	using OPT_LIST		= std::vector<ard::tda_option*>;
	using S2OPT_LIST	= std::unordered_map<QString, OPT_LIST>;
	using OPT_SMAP		= std::map<QString, OPT_LIST>;

	enum class opt_type
	{
		none,
		call,
		put
	};

	struct tda_option
	{
		using ptr = std::unique_ptr<tda_option>;

		tda_option() {};

		bool            isLong()const;

		QString         symbol;
		QString         code;
		QDate           exp, trade_time;
		double          strike;
		opt_type        opt{ opt_type::none };
		int             qty;
		double          trade_price;
		double          mark_price;
		double          mark_val;
		bool			is_to_open{ true };
		//tda_option*		open_trade{nullptr};
		tda_option*		close_trade{ nullptr };

		mutable QString vertical_group_str;

		QString         toString()const;
		void            build_group_str()const;
		static ptr      parse_line(const std::vector<QString>& lst);
		static ptr      parseTrade(const std::vector<QString>& lst);
	};

	struct trade_pair
	{
		tda_option*		open_trade{ nullptr };
		//tda_option*		close_trade{ nullptr };
		double			close_price{0.0};
		double			profit;
		double			roc{0.0};
		int				days;
	};

	using trades_list = std::vector<trade_pair>;
	using margin_map = std::unordered_map<QString, double>;

	struct tda_summary
	{
		double  total_pnl{ 0 };
		double  total_max_loss{ 0 };
		double  total_max_profit{ 0 };
		double  account_margin{ 0 };
		int     verticals_count{ 0 };
		int     options_count{ 0 };
		int     symbols_count{ 0 };
		std::vector<QString>    winners;
		std::vector<QString>    loosers;
	};

	class tda
	{
	public:
		tda();

		void reloadFile(QString statementFileName, QString logRoot);
		QStandardItemModel* getTradesModel();
		
	protected:
		void	loadActivePositions(QString statementFileName);
		void	loadTradesHistory(QString statementFileName);
		bool	addOption(ard::tda_option::ptr&& o);
		void	findTradePairs();
		QStandardItemModel* makeTradesModel(const trades_list& lst);

		S2OPT_LIST							m_code2opt_list;
		OPT_SMAP							m_s2options;
		std::vector<tda_option::ptr>		m_options;
		trades_list							m_trades;
		margin_map							m_mrg_map;
		ard::tda_summary                    m_summary;
		QFont                               m_working_font;
	};


	class roc_widget : public QWidget
	{
	public:
		roc_widget();
		void updateRoc();

		QLineEdit*		m_days{ nullptr };
		QLineEdit*		m_price1{ nullptr };
		QLineEdit*		m_price2{ nullptr };
		QLineEdit*		m_roc{ nullptr };
		QLabel*			m_pivot{ nullptr };
		QLabel*			m_profit{ nullptr };
		QComboBox*		m_margin{ nullptr };
	};
};

static QString opt_type2str(ard::opt_type t)
{
	QString rv;
	switch (t)
	{
	case ard::opt_type::none:   rv = "E"; break;
	case ard::opt_type::call:   rv = "C"; break;
	case ard::opt_type::put:    rv = "P"; break;
	}
	return rv;
};

static QString trim_integral_str(QString s)
{
	auto rv = s;
	rv.replace("$", "").replace("(", "").replace(")", "").replace("\"", "").replace(",", "").replace("%", "");
	return rv;
}


ard::roc_widget::roc_widget() 
{
	auto lt_roc = new QHBoxLayout;
	setLayout(lt_roc);

	auto fnt = QApplication::font("QMenu");
	fnt.setPixelSize(36);
	auto fnt_bold = fnt;
	fnt_bold.setBold(true);
	//	QLabel* lb = nullptr;
	m_margin = new QComboBox;
	m_margin->setEditable(true);
	m_margin->addItems({ "40", "8", "1", "2", "5" });

	//auto spacer = new QWidget();
	//spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	//std::function<QWidget* (void)>new_spacer = []()
	//{
	//	auto spacer = new QWidget();
	//	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//	return spacer;
	//};

	auto ltg = new QGridLayout;
	lt_roc->addLayout(ltg);

	std::function<QLabel* (QString)>new_label = [&](QString s) {auto lb = new QLabel(s); lb->setFont(fnt); return lb; };
	std::function<QLineEdit* (QLineEdit*&)>new_edit = [&](QLineEdit*& e) {e = new QLineEdit; e->setFont(fnt); e->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); return e; };

	m_pivot = new_label("--");
	m_pivot->setFont(fnt_bold);

	m_profit = new_label("--");
	m_profit->setFont(fnt_bold);

	std::function<void(int, QString, QWidget*, QString, QWidget*)>add_column = 
		[&](int col, QString row1name, QWidget* row1w, QString row2name, QWidget* row2w)
	{
		ltg->addWidget(new_label(row1name), 0, col);
		ltg->addWidget(row1w, 0, col+1);
		ltg->addWidget(new_label(row2name), 1, col);
		ltg->addWidget(row2w, 1, col+1);
		//ltg->addWidget(new_spacer(), 0, col+2);
		//ltg->addWidget(new_spacer(), 1, col+2);
	};

	add_column(0, "D", new_edit(m_days), "M(in-K)", m_margin);
	add_column(2, "P1", new_edit(m_price1), "P2", new_edit(m_price2));
	add_column(4, "ROC", new_edit(m_roc), "Pivot", m_pivot);
	//ltg->addWidget(new_label("D"), 0, 0);
	//ltg->addWidget(new_edit(m_days), 0, 1);
	//ltg->addWidget(new_label("M(in-K)"), 1, 0);
	//ltg->addWidget(m_margin, 1, 1);

	//-----

	//ltg->addWidget(new_label("P1"), 0, 2);
	//ltg->addWidget(new_edit(m_price1), 0, 3);
	//ltg->addWidget(new_label("P2"), 1, 2);
	//ltg->addWidget(new_edit(m_price2), 1, 3);
	//--
	//ltg->addWidget(new_label("ROC"), 0, 4);
	//ltg->addWidget(new_edit(m_roc), 0, 5);
	//ltg->addWidget(new_label("Pivot"), 1, 4);
	//ltg->addWidget(m_pivot, 1, 5);

	ltg->addWidget(new_label("Profit"), 2, 4);
	ltg->addWidget(m_profit, 2, 5);

	ltg->setColumnStretch(1, 1);
	ltg->setColumnStretch(3, 1);
	ltg->setColumnStretch(5, 1);

	connect(m_days, &QLineEdit::textChanged, [&](const QString&) {updateRoc(); });
	connect(m_price1, &QLineEdit::textChanged, [&](const QString&) {updateRoc(); });
	connect(m_price2, &QLineEdit::textChanged, [&](const QString&) {updateRoc(); });
	connect(m_margin, &QComboBox::currentTextChanged, [&](const QString&) {updateRoc(); });

	m_days->setText("45");
	m_price1->setText("14");
	m_price2->setText("0");

	m_days->setStyleSheet("QLineEdit {background-color: #99d9ea;}");
	m_price1->setStyleSheet("QLineEdit {background-color: #ff7f27;}");
	m_price2->setStyleSheet("QLineEdit {background-color: #85e61d;}");
}

void ard::roc_widget::updateRoc()
{
	auto days = m_days->text().toInt();
	auto price1 = m_price1->text().toDouble();
	auto price2 = m_price2->text().toDouble();
	auto margin = m_margin->currentText().toDouble();
	if (days > 0 && price1 > 0 && margin > 0) {
		auto profit = std::abs(price1 - price2);
		auto price = profit;
		double roc = (100 * 256 * price * 100) / (days * margin * 1000);
		m_roc->setText(QString("%1%").arg(roc, 0, 'g', 3));
		auto pivot_price = margin * days * 0.78 / 100;
		m_pivot->setText(QString("%1").arg(pivot_price, 0, 'g', 3));
		m_profit->setText(QString("%1").arg(profit, 0, 'g', 3));
	}
	else {
		m_roc->setText("err");
	}
};

/**
tda
*/
ard::tda::tda() 
{
	m_working_font = QApplication::font("QMenu");
	auto sz = m_working_font.pointSize();
	sz += 2;
	m_working_font.setPointSize(sz);

};


void ard::tda::reloadFile(QString statementFileName, QString logRoot) 
{
	m_mrg_map["SPX"] = 40;
	m_mrg_map["SPY"] = 8;
	m_code2opt_list.clear();
	m_s2options.clear();
	m_options.clear();
	m_trades.clear();

	Q_UNUSED(logRoot);
	loadTradesHistory(statementFileName);
};

QStandardItemModel* ard::tda::makeTradesModel(const trades_list& lst)
{
	QStringList lbls;
	lbls.push_back("Symbol");
	lbls.push_back("Strike");
	lbls.push_back("OpenDate");
	lbls.push_back("Days");
	lbls.push_back("Open");
	lbls.push_back("Close");
	lbls.push_back("Profit");
	lbls.push_back("ROC");

	int row = 0;
	int col = 0;
	QStandardItem* it = nullptr;
	auto m = new QStandardItemModel();
	m->setHorizontalHeaderLabels(lbls);
	for (auto& p : lst)
	{
		SET_CELL(p.open_trade->symbol);
		SET_CELL(QString("%1").arg(p.open_trade->strike));
		SET_CELL(p.open_trade->trade_time.toString("MM/dd/yy"));
		SET_CELL(QString("%1").arg(p.days));
		SET_CELL(QString("%1").arg(p.open_trade->trade_price));
		SET_CELL(QString("%1").arg(p.close_price/*close_trade->trade_price*/));
		SET_CELL(QString("%1").arg(p.profit));
		SET_CELL(QString("%1%").arg(p.roc * 100, 0, 'f', 1));
		row++;
		col = 0;
	}
	return m;
};

QStandardItemModel* ard::tda::getTradesModel() 
{
	ard::trades_list lst;
	for (auto& i : m_trades) {
		auto j = m_mrg_map.find(i.open_trade->symbol);
		if (j != m_mrg_map.end()) {
			lst.push_back(i);
		}
	}
	//m_mrg_map

	return makeTradesModel(lst);
};

void ard::tda::loadTradesHistory(QString statementFileName) 
{
	QFile f(statementFileName);
	if (f.exists()) 
	{
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
			ard::error(QString("failed to open TDA file [%1]").arg(statementFileName));
			return;
		}

		auto b = f.readAll();
		ard::trail(QString("readind tda-statement/history [%1] [%2]").arg(statementFileName).arg(b.size()));
		auto s1 = b.toStdString();
		auto idx_trades = s1.find("Account Trade History");
		if (idx_trades == std::string::npos) {
			ard::error(QString("'Account Trade History' section not found [%1]").arg(statementFileName));
			return;
		}
		idx_trades = s1.find('\n', idx_trades) + 1;

		auto idx_equities = s1.find("Equities", idx_trades);
		if (idx_equities == std::string::npos) {
			ard::error(QString("'Equities' section not found [%1]").arg(statementFileName));
			return;
		}
		

		bool load_options = true;
		if (load_options)
		{
			auto str_opt = s1.substr(idx_trades, idx_equities - idx_trades);
			std::stringstream ss(str_opt);
			ard::ArdiCsv a;
			if (a.loadCsv(ss))
			{
				ard::VALUES_LIST& rlst = a.rows();
				for (auto& line : rlst)
				{
					if (line.size() >= 13)
					{
						auto o = ard::tda_option::parseTrade(line);
						if (o) {
							addOption(std::move(o));
						}
					}
				}
			}
			findTradePairs();
		}
	}
};

void ard::tda::findTradePairs() 
{
	for (auto& o : m_options) 
	{
		if (!o->is_to_open) 
		{
			for (auto& i : m_options) {
				if (i->is_to_open && i->close_trade == nullptr) 
				{
					if (i->symbol == o->symbol &&
						std::abs(i->qty) == std::abs(o->qty) &&
						i->strike == o->strike &&
						i->trade_time <= o->trade_time)
					{
						//qDebug() << "pair-found" << o->toString() << "|" << i->toString();
						//->open_trade = i.get();
						i->close_trade = o.get();
						trade_pair p;
						p.open_trade = i.get();
						//p.close_trade = o.get();
						p.close_price = o->trade_price;
						p.profit = p.open_trade->trade_price - p.close_price;
						p.days = p.open_trade->trade_time.daysTo(o->trade_time);
						if (p.days == 0)p.days = 1;
						auto i = m_mrg_map.find(o->symbol);
						if (i != m_mrg_map.end()) {
							auto margin = 1000.0 * i->second;
							p.roc = (256 * 100.0 * p.profit)/(margin * p.days);
						}
						//p.roc = (p.profit)
						m_trades.push_back(p);						
						break;
					}
				}
			}
		}
	}

	//QDate now = QDate::currentDate();

	/// run on expired options
	/*for (auto& o : m_options)
	{
		if (o->is_to_open && !o->close_trade)
		{
			if (now > o->exp) 
			{
				qDebug() << "ykh-expired" << o->toString();
			}
		}
	}*/

	std::sort(m_trades.begin(), m_trades.end(), [](const trade_pair& p1, const trade_pair& p2) 
	{
		if (p1.open_trade->symbol == p2.open_trade->symbol) {
			return (p1.open_trade->trade_time < p2.open_trade->trade_time);
		}
		return(p1.open_trade->symbol < p2.open_trade->symbol);
	});

	//for (auto& p : m_trades) 
	//{
		//qDebug() << "<<pair" << p.open_trade->symbol << p.open_trade->trade_price << p.close_trade->trade_price << p.profit << p.days << p.open_trade->strike << p.roc;
	//}
};

void ard::tda::loadActivePositions(QString statementFileName)
{
	QFile f(statementFileName);
	if (f.exists()) {
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
			ard::error(QString("failed to open TDA file [%1]").arg(statementFileName));
			return;
		}

		auto b = f.readAll();
		ard::trail(QString("readind tda-statement/active-positions [%1] [%2]").arg(statementFileName).arg(b.size()));
		auto s1 = b.toStdString();
		auto idx_options = s1.find("Options");
		if (idx_options == std::string::npos) {
			ard::error(QString("'Options' section not found [%1]").arg(statementFileName));
			return;
		}
		idx_options = s1.find('\n', idx_options) + 1;

		auto idx_pnl = s1.find("Profits and Losses", idx_options);
		if (idx_pnl == std::string::npos) {
			ard::error(QString("'PNL' section not found [%1]").arg(statementFileName));
			return;
		}

		auto idx_account_sum = s1.find("Account Summary", idx_pnl);
		if (idx_account_sum == std::string::npos) {
			ard::error(QString("'Account Summary' section not found [%1]").arg(statementFileName));
			return;
		}

		/// we parse everything between 'Options' and 'Profits and Losses'
		bool load_options = true;
		if (load_options)
		{
			auto str_opt = s1.substr(idx_options, idx_pnl - idx_options);
			std::stringstream ss(str_opt);
			ard::ArdiCsv a;
			if (a.loadCsv(ss))
			{
				ard::VALUES_LIST& rlst = a.rows();
				for (auto& line : rlst)
				{
					if (line.size() >= 8)
					{
						auto o = ard::tda_option::parse_line(line);
						if (o)
						{
							addOption(std::move(o));
						}
					}
					else
					{
						qWarning() << "ignoring tda line" << line;
					}
				}//for

				//build_verticals();
			}//load_options

			bool load_summary = false;
			if (load_summary)
			{
				idx_pnl = s1.find('\n', idx_pnl);
				auto str_sum = s1.substr(idx_pnl, idx_account_sum - idx_pnl);
				std::stringstream ss(str_sum);
				ard::ArdiCsv a;
				if (a.loadCsv(ss))
				{
					ard::VALUES_LIST& rlst = a.rows();
					for (auto& line : rlst)
					{
						if (line.size() >= 5)
						{
							if (line[1] == "OVERALL TOTALS")
							{
								m_summary.account_margin = trim_integral_str(line[5]).toDouble();
							}
						}
					}
				}
			}
		}
	}
};

bool ard::tda::addOption(ard::tda_option::ptr&& o)
{
	switch (o->opt)
	{
	case opt_type::call:
	case opt_type::put:
		break;
	default:
		qWarning() << "tda/invalid option" << o->toString();
		return false;
	}

	if (o->qty == 0)
	{
		qWarning() << "tda/zero quantity, ignored" << o->toString();
		return false;
	}

	o->build_group_str();

	auto i = m_code2opt_list.find(o->code);
	if (i != m_code2opt_list.end())
	{
		i->second.push_back(o.get());
	}
	else
	{
		std::vector<tda_option*> lst;
		lst.push_back(o.get());
		m_code2opt_list[o->code] = lst;
	}
	//m_code2opt_list[o->code] = o.get();
	auto j = m_s2options.find(o->symbol);
	if (j == m_s2options.end())
	{
		std::vector<tda_option*> arr;
		arr.push_back(o.get());
		m_s2options[o->symbol] = arr;
	}
	else
	{
		j->second.push_back(o.get());
	}

		

	//ard::trail(QString("loaded-option: %1").arg(o->toString()));
	m_options.emplace_back(std::move(o));	
	return true;
};


/**
tda_option
*/
QString ard::tda_option::toString()const
{
	QString rv = QString("%1 %2 %3 %4 %5 %6 %7")
		.arg(symbol, 6)
		.arg(code, 15)
		.arg(exp.toString(), 10)
		.arg(opt_type2str(opt))
		.arg(strike, 8)
		.arg(qty, 3)
		.arg(trade_price, 8);
		//.arg(mark_price, 8)
		//.arg(mark_val, 12);
	return rv;
};

void ard::tda_option::build_group_str()const
{
	vertical_group_str = QString("%1 %2 %3 %4")
		.arg(symbol)
		.arg(std::abs(qty))
		.arg(exp.toString())
		.arg(opt_type2str(opt));
};

bool ard::tda_option::isLong()const
{
	return (qty > 0);
};

ard::tda_option::ptr ard::tda_option::parse_line(const std::vector<QString>& lst)
{
	if (lst.size() >= 8)
	{
		ard::tda_option::ptr rv(new tda_option);

		rv->symbol = lst[0];
		rv->code = lst[1];
		auto s = lst[2];
		s.insert(s.length() - 2, "20");
		rv->exp = QDate::fromString(s, "d MMM yyyy");
		rv->strike = lst[3].toDouble();
		s = lst[4];
		if (s == "PUT") {
			rv->opt = opt_type::put;
		}
		else if (s == "CALL") {
			rv->opt = opt_type::call;
		}
		rv->qty = lst[5].toInt();
		rv->trade_price = lst[6].toDouble();
		rv->mark_price = lst[7].toDouble();
		//s = trim_integral_str(lst[8]);
		rv->mark_val = std::abs(rv->qty) * 100 * rv->mark_price;//s.toDouble();
		return rv;
	}
	return nullptr;
};


ard::tda_option::ptr ard::tda_option::parseTrade(const std::vector<QString>& lst)
{
	if (lst.size() >= 13)
	{		
		auto s = lst[2];
		if (s != "SINGLE")return nullptr;
		//s = lst[lst.size()-1];
		//if (s != "FILLED")return nullptr;

		ard::tda_option::ptr rv(new tda_option);
		s = lst[1];
		auto idx = s.indexOf(" ");
		s = s.left(idx).trimmed();
		rv->trade_time = QDate::fromString(s, "MM/dd/yy");
		if (!rv->trade_time.isValid())return nullptr;
		s = lst[4];
		if (s.size() > 1) {
			//if (s[0] == '+' || s[0] == '-')s = s.remove(0, 1);
			rv->qty = s.toInt();
		}
		s = lst[5];
		if (s == "TO OPEN")rv->is_to_open = true;
		else if (s == "TO CLOSE")rv->is_to_open = false;
		else return nullptr;

		rv->symbol = lst[6]; 
		s = lst[7];
		s.insert(s.length() - 2, "20");
		rv->exp = QDate::fromString(s, "d MMM yyyy");
		rv->strike = lst[8].toDouble();
		s = lst[9];
		if (s == "PUT") {
			rv->opt = opt_type::put;
		}
		else if (s == "CALL") {
			rv->opt = opt_type::call;
		}
		rv->trade_price = lst[10].toDouble();
		return rv;
	}
	return nullptr;
};

/**
tda_view
*/
ard::tda_view::tda_view() 
{
	m_tda.reset(new ard::tda);
	initTimer();

	auto lt_main = new QVBoxLayout;
	setLayout(lt_main);

	//auto spacer = new QWidget();
	//spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	//lt_main->addWidget(spacer);
	m_trades_view = new QTableView;
	m_trades_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

	m_main_tab = new QTabWidget;
	m_main_tab->addTab(m_trades_view, "Trades");

	auto rw = new ard::roc_widget;
	rw->setMaximumHeight(150);
	m_detail_tab = new QTabWidget;
	m_detail_tab->setTabPosition(QTabWidget::East);
	m_detail_tab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	m_detail_tab->addTab(rw, "ROC");

	lt_main->addWidget(m_main_tab);
	lt_main->addWidget(m_detail_tab);

	///status box
	QWidget* status_space = new QWidget();
	status_space->setMaximumHeight(20);
	QHBoxLayout* lt_status = new QHBoxLayout();
	gui::setupBoxLayout(lt_status);
	status_space->setLayout(lt_status);
	lt_main->addWidget(status_space);

	m_sum_label = new QLabel();
	lt_status->addWidget(m_sum_label);
	m_avg_label = new QLabel();
	lt_status->addWidget(m_avg_label);

};

ard::tda_view::~tda_view() 
{
	detach();
};

void ard::tda_view::initTimer() 
{
	/// timer for file refresh ///
	m_file_check_timer.setInterval(5000);
	connect(&m_file_check_timer, &QTimer::timeout, this, [=]()
	{
		auto res = getStatementFileName();
		if (!res.first.isEmpty())
		{
			QFileInfo fi(res.first);
			if (fi.exists())
			{
				if (!m_statement_load_time.isValid() || fi.lastModified() > m_statement_load_time)
				{
					qDebug() << "TDA statement file modification detected, reloading..";
					reloadTDAfile();
					m_statement_load_time = QDateTime::currentDateTime();
				}
			}
		}

		/*if (!res.second.isEmpty())
		{
		QString snapshotFile = res.second + "/live/snapshot.log";
		QFileInfo fi(snapshotFile);
		if (fi.exists())
		{
		if (!m_snapshot_load_time.isValid() || fi.lastModified() > m_snapshot_load_time)
		{
		qDebug() << "Snapshot file modification detected, reloading..";
		reloadPricesSnapshot();
		m_snapshot_load_time = QDateTime::currentDateTime();
		}
		}
		}*/
	});
};

void ard::tda_view::attachFileRef(ard::fileref* r)
{
	detach();
	m_tda_file_ref = r;
	if (m_tda_file_ref)
	{
		LOCK(m_tda_file_ref);
		reloadTDAfile();
		m_file_check_timer.start();
	}
};

void ard::tda_view::detach()
{
	if (m_tda_file_ref)
	{
		m_tda_file_ref->release();
		m_tda_file_ref = nullptr;
	}
};

std::pair<QString, QString> ard::tda_view::getStatementFileName()const
{
	std::pair<QString, QString> rv;
	if (m_tda_file_ref)
	{
		rv = m_tda_file_ref->getRefFileName();
	}
	return rv;
};

void ard::tda_view::reloadTDAfile() 
{
	assert_return_void(m_tda_file_ref, "expected TDA ref object");
	assert_return_void(m_tda, "expected TDA object");
	auto res = getStatementFileName();
	m_statement_load_time = QDateTime::currentDateTime();
	m_tda->reloadFile(res.first, res.second);

	auto m = m_tda->getTradesModel();
	m_trades_view->setModel(m);
	connect(m_trades_view->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &, const QItemSelection &)
	{
		auto sm = m_trades_view->selectionModel();
		if (sm->hasSelection())
		{
			QModelIndexList list = sm->selectedIndexes();
			updateSelectionSumLabel(list);
		}
	});
};

void ard::tda_view::updateSelectionSumLabel(QModelIndexList& lst)
{
	if (!lst.empty())
	{
		double sum = 0;
		for (auto i : lst)
		{
			auto s = trim_integral_str(i.data().toString());
			sum += s.toDouble();
		}
		double avg = sum / lst.size();
		m_avg_label->setText(QString("AVG:[%1]").arg(avg));
		m_sum_label->setText(QString("SUM:[%1]").arg(sum));		
	}
};