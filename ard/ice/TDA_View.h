#pragma once
#include <QTableView>
#include "PopupCard.h"

class QLabel;
class VerticalPnlView;
class VerticalStatusView;

namespace ard 
{
    class fileref;
    class tda;

    enum class group_view_type
    {
        none,
        symbol,
        expire
    };

    enum class opt_view_type
    {
        none,
        verticals_by_expire,
        verticals_short_put,
        verticals_long_put,
        verticals_short_call,
        verticals_long_call,
        options_for_group
    };


    class TDA_View : public QWidget
    {
    public:
        TDA_View();
        ~TDA_View();

        void                        attachFileRef(ard::fileref* );
        void                        detach();
        void                        reloadTDAfile();
        void                        reloadPricesSnapshot();
    protected:
        void                        refreshGroupView();
        void                        refreshOptionView();
        std::pair<QString,QString>  getStatementFileName()const;
        void                        updateSelectionSumLabel(QModelIndexList& lst);
        void                        prepareTableView(QTableView* v);
        void                        setupExpireGroupedView();
        void                        setupSymbolGroupedView();
        void                        setupVerticalView(QStandardItemModel* m);
        void                        selectVerticalBySymbol(QString symbol);
        void                        selectVerticalByExpDate(QDate exp);

        ard::group_view_type        currGroupView()const;
        ard::opt_view_type          currOptView()const;
        void                        setCurrOptView(ard::opt_view_type ov);

        fileref*                m_tda_file_ref{nullptr};
        std::unique_ptr<tda>    m_tda;
        QTabBar                 *m_group_tab{nullptr}, *m_vert_tab{ nullptr };

        QTableView*             m_grouped_view{ nullptr };
        QTableView*             m_vert_view{ nullptr };     
        VerticalPnlView*        m_vert_pnl_view{ nullptr };
        VerticalStatusView*     m_vert_status_view{ nullptr };
        QPlainTextEdit*         m_opt_list{ nullptr };
        QLabel*                 m_status_label{nullptr};
        QLabel*                 m_winners_label{ nullptr };
        QLabel*                 m_loosers_label{ nullptr };
        QLabel*                 m_sum_label{ nullptr };
        QDateTime               m_statement_load_time, m_snapshot_load_time;
        QTimer                  m_file_check_timer;
        QString                 m_curr_symbol;
        QDate                   m_curr_exp_date;
    };
};