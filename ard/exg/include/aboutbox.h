#pragma once

#include <QDialog>
#include <QUrl>

class QTextBrowser;
class QNetworkReply;
class QLineEdit;

class AboutBox : public QDialog
{
	Q_OBJECT
public:
	AboutBox();
	//virtual ~AboutBox();

	public slots:
	void navigate(const QUrl &);
	//void showEvent(QShowEvent * e);
	//void showStorageInfo();
	void checkVersion();
	//void checkVersionDownloadFinished(QNetworkReply*);

protected:
	void closeEvent(QCloseEvent *e);

	QTextBrowser*  bview;
	QLineEdit* m_gmail_userid;
};
