#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include "roc.h"
#include "anfolder.h"

void ard::roc_box::showBox() 
{
	roc_box d;
	d.exec();
};

ard::roc_box::roc_box() 
{
	auto lt_main = new QVBoxLayout;
	QHBoxLayout *h_btns = new QHBoxLayout;

	m_main_tab = new QTabWidget;
	m_main_tab->setTabPosition(QTabWidget::East);
	lt_main->addWidget(m_main_tab);	

	auto w_roc = new QWidget;
	auto lt_roc = new QHBoxLayout;
	w_roc->setLayout(lt_roc);
	m_main_tab->addTab(w_roc, "ROC");
	auto fnt = QApplication::font("QMenu");
	fnt.setPixelSize(36);
	auto fnt_bold = fnt;
	fnt_bold.setBold(true);
//	QLabel* lb = nullptr;
	m_margin = new QComboBox;
	m_margin->setEditable(true);
	m_margin->addItems({ "40", "8", "1", "2", "5"});

	auto spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto ltg = new QGridLayout;
	lt_roc->addLayout(ltg);

	std::function<QLabel* (QString)>new_label = [&](QString s) {auto lb = new QLabel(s); lb->setFont(fnt); return lb; };
	std::function<QLineEdit* (QLineEdit*&)>new_edit = [&](QLineEdit*& e) {e = new QLineEdit; e->setFont(fnt); e->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); return e; };

	m_pivot = new_label("--");
	m_pivot->setFont(fnt_bold);

	m_profit = new_label("--");
	m_profit->setFont(fnt_bold);

	ltg->addWidget(new_label("D"), 0, 0);
	ltg->addWidget(new_edit(m_days), 0, 1);

	ltg->addWidget(new_label("M(in-K)"), 1, 0);
	ltg->addWidget(m_margin, 1, 1);

	//-----

	ltg->addWidget(new_label("P1"), 0, 2);
	ltg->addWidget(new_edit(m_price1), 0, 3);

	ltg->addWidget(new_label("P2"), 1, 2);
	ltg->addWidget(new_edit(m_price2), 1, 3);

	//--

	ltg->addWidget(new_label("ROC"), 0, 4);
	ltg->addWidget(new_edit(m_roc), 0, 5);

	ltg->addWidget(new_label("Pivot"), 1, 4);
	ltg->addWidget(m_pivot, 1, 5);

	ltg->addWidget(new_label("Profit"), 2, 4);
	ltg->addWidget(m_profit, 2, 5);


	connect(m_days, &QLineEdit::textChanged, [&](const QString&) {updateRoc();});
	connect(m_price1, &QLineEdit::textChanged, [&](const QString&){updateRoc();});
	connect(m_price2, &QLineEdit::textChanged, [&](const QString&) {updateRoc(); });
	connect(m_margin, &QComboBox::currentTextChanged, [&](const QString&) {updateRoc(); });
	
	m_days->setText("45");
	m_price1->setText("14");
	m_price2->setText("0");

	m_days->setStyleSheet("QLineEdit {background-color: #99d9ea;}");
	m_price1->setStyleSheet("QLineEdit {background-color: #ff7f27;}");
	m_price2->setStyleSheet("QLineEdit {background-color: #85e61d;}");

	spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	lt_main->addWidget(spacer);
	lt_main->addLayout(h_btns);
	QPushButton* b = nullptr;
	ADD_BUTTON2LAYOUT(h_btns, "Close", &QPushButton::close);
	MODAL_DIALOG_SIZE(lt_main, QSize(900, 300));
};

void ard::roc_box::updateRoc() 
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