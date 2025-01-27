#include <sstream>
#include <memory>
#include <QSplitter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QApplication>
#include <QTimer>
#include <QFileInfo>
#include <QLabel>
#include <QPainter>
#include "TDA_View.h"
#include "fileref.h"
#include "csv-util.h"

namespace ard 
{   
    enum class opt_type 
    {
        none,
        call,
        put
    };

    enum class vertical_type
    {
        none,
        short_put,
        long_put,
        short_call,
        long_call
    };

    enum class grouped_sort
    {
        none,
        by_symbol,
        by_expire,
        by_short_put,
        by_long_put,
        by_short_call,
        by_long_call,
        by_val,
        by_market_val, 
        by_pnl,
        by_pnl_percent,
        by_max_loss,
        by_max_profit
    };

    enum class vertical_sort
    {
        none,
        by_vert_type,
        by_symbol,
        by_expire,
        by_days,
        by_val,
        by_market_val,
        by_pnl,
        by_pnl_percent,
        by_qty,
        by_width,
        by_vert_price,
        by_vert_market,
        by_vert_target,
        by_price1,
        by_price2,
        by_debit_type,
        by_max_loss,
        by_max_profit,
        by_ror_un
    };

    enum class option_sort 
    {
        none,
        by_symbol,
        by_code,
        by_expire,
        by_type,
        by_strike,
        by_qty,
        by_price,
        by_mark_price,
        by_mark_val
    };

    QString opt_type2str(opt_type);
    bool is_debit_vertical(vertical_type);

    struct tda_option 
    {
        using ptr = std::unique_ptr<tda_option>;

        tda_option() {};

        bool            isLong()const;

        QString         symbol;
        QString         code;
        QDate           exp;
        double          strike;
        opt_type        opt{ opt_type::none };
        int             qty;
        double          trade_price;
        double          mark_price;
        double          mark_val;

        mutable QString vertical_group_str;

        QString         toString()const;
        void            build_group_str()const;
        static ptr      parse_line(const std::vector<QString>& lst);        
    };

    using S2OPT_LIST    = std::unordered_map<QString, std::vector<tda_option*>>;

    struct tda_vertical 
    {
        using ptr = std::unique_ptr<tda_vertical>;

        int             vid{-1};
        QString         symbol;
        int             qty;
        QDate           exp;
        int             days2exp;
        bool            is_debit;
        tda_option*     long_opt; 
        tda_option*     short_opt;
        vertical_type   vert_type{ vertical_type::none };
        double          vert_price;     
        double          vert_mark_price;
        double          vert_target_price;
        double          vert_val;
        double          vert_mark_val;
        double          vert_width;
        double          max_loss;
        double          max_profit;
        double          unrealized_ror;
        double          vert_pnl;
        double          vert_pnl_percent;
        QString         toString()const;

        double          price1()const;
        double          price2()const;
        QString         options_as_str()const;
        QString         symbol_with_expire()const;
        QString         vtype_as_string()const;
    };
    using VERT_LIST     = std::vector<tda_vertical*>;
    using VERT_TMAP     = std::map<vertical_type, VERT_LIST>;
    using VERT_SMAP     = std::map<QString, VERT_LIST>;
    using VERT_EMAP     = std::map<QDate, VERT_LIST>;
    using OPT_LIST      = std::vector<tda_option*>;
    using OPT_SMAP      = std::map<QString, OPT_LIST>;

    struct tda_grouped 
    {
        int             short_put_count{ 0 },
            long_put_count{ 0 },
            short_call_count{ 0 },
            long_call_count{ 0 };
        double          total_val{ 0 };
        double          total_mark_val{ 0 };
        double          total_pnl{ 0 };
        double          total_pnl_percent{ 0 };
        double          total_max_loss{ 0 };
        double          total_max_profit{ 0 };
    };

    struct tda_symbol_grouped: public tda_grouped
    {
        using ptr = std::unique_ptr<tda_symbol_grouped>;
        QString         symbol;
    };

    struct tda_expire_grouped : public tda_grouped
    {
        using ptr = std::unique_ptr<tda_expire_grouped>;
        QDate           expire;
    };

    struct tda_summary 
    {
        double  total_pnl{ 0 };
        double  total_max_loss{ 0 };
        double  total_max_profit{ 0 };
        double  account_margin{0};
        int     verticals_count{0};
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
        void reloadPricesSnaphost();
        QStandardItemModel* getVerticalsModelByType(vertical_type t);
        QStandardItemModel* getVerticalsModelByExpire(QDate);///is provided date is null we return all
        QStandardItemModel* getGroupedBySymbolModel();
        QStandardItemModel* getGroupedByExpireModel();
        QStandardItemModel* getOptionsModel(QString symbol);

        void toggle_symbol_grouped_sort(grouped_sort s);
        void toggle_expire_grouped_sort(grouped_sort s);
        void toggle_verticals_sort(vertical_sort s);
        void toggle_options_sort(option_sort s);

        ard::tda_vertical*                          getVerticalById(int vid);
        const ard::tda_summary&                     summary()const;
        const std::unordered_map<QString, double>&  prices_snapshot()const { return m_prices_snapshot; }

        QFont   working_font()const;
    protected:
        void loadStatementFile(QString statementFileName);      
        void build_verticals();
        void build_summary();
        bool add_option(ard::tda_option::ptr&& o);
        void add_vertical(ard::tda_vertical::ptr&& v);
        VERT_LIST getVerticalsByType(vertical_type t);
        VERT_LIST getVerticalsBySymbol(QString symbol);
        VERT_LIST getVerticalsByExpire(QDate exp);
        VERT_LIST getAllVerticals();
        OPT_LIST getOptionsBySymbol(QString symbol);        
        QStandardItemModel* makeVerticalsModel(ard::VERT_LIST& arr);

        tda_vertical::ptr make_vertical(tda_option* long_opt, tda_option* short_opt);
    protected:
        std::vector<tda_option::ptr>            m_options;
        std::vector<tda_vertical::ptr>          m_verticals;
        std::vector<tda_symbol_grouped::ptr>    m_grouped_by_symbol;
        std::vector<tda_expire_grouped::ptr>    m_grouped_by_expire;
        std::unordered_map<QString, double>     m_prices_snapshot;
        VERT_TMAP                               m_t2vertical;
        VERT_SMAP                               m_s2vertical;
        VERT_EMAP                               m_exp2vertical;
        OPT_SMAP                                m_s2options;
        ard::tda_summary                        m_summary;
        S2OPT_LIST                              m_code2opt_list;
        QFont                                   m_working_font;
        QBrush                                  m_brush_up, m_brush_down;
        QDate                                   m_working_curr_date;
        grouped_sort                            m_symbol_grouped_sort{ grouped_sort::by_pnl_percent };
        bool                                    m_reverse_symbol_grouped_sort{false};
        grouped_sort                            m_expire_grouped_sort{ grouped_sort::by_expire };
        bool                                    m_reverse_expire_grouped_sort{ false };
        vertical_sort                           m_vertical_sort{ vertical_sort::by_pnl_percent };
        bool                                    m_reverse_vertical_sort{ false };
        option_sort                             m_opt_sort{ option_sort::by_code};
        bool                                    m_reverse_opt_sort{false};
        QString                                 m_logDirRoot;
    };

    void sortVertList(VERT_LIST& arr, vertical_sort vs, bool reverse);
    void sortOptList(OPT_LIST& arr, option_sort vs, bool reverse);
};


QString ard::opt_type2str(opt_type t) 
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

QString trim_integral_str(QString s) 
{
    auto rv = s;
    rv.replace("$", "").replace("(", "").replace(")", "").replace("\"", "").replace(",", "").replace("%", "");
    return rv;
}

bool ard::is_debit_vertical(vertical_type t)
{
    if (t == vertical_type::long_call || t == vertical_type::long_put)
    {
        return true;
    }

    return false;
};

///tda_option
bool ard::tda_option::isLong()const 
{
    return (qty > 0);
};

ard::tda_option::ptr ard::tda_option::parse_line(const std::vector<QString>& lst)
{
    if (lst.size() >= 8) 
    {
        ard::tda_option::ptr rv(new tda_option);

        rv->symbol      = lst[0];
        rv->code        = lst[1];
        auto s          = lst[2];
        s.insert(s.length() - 2, "20");
        rv->exp         = QDate::fromString(s, "d MMM yyyy");
        rv->strike      = lst[3].toDouble();
        s               = lst[4];
        if (s == "PUT") {
            rv->opt = opt_type::put;
        }
        else if (s == "CALL") {
            rv->opt = opt_type::call;
        }
        rv->qty         = lst[5].toInt();
        rv->trade_price = lst[6].toDouble();
        rv->mark_price  = lst[7].toDouble();
        //s = trim_integral_str(lst[8]);
        rv->mark_val = std::abs(rv->qty) * 100 * rv->mark_price;//s.toDouble();
        return rv;
    }
    return nullptr;
};

QString ard::tda_option::toString()const 
{
    QString rv = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9")
        .arg(symbol, 6)
        .arg(code, 15)
        .arg(exp.toString(), 10)
        .arg(opt_type2str(opt))
        .arg(strike, 8)
        .arg(qty, 3)
        .arg(trade_price, 8)
        .arg(mark_price, 8)
        .arg(mark_val, 12);
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

///tda_vertical
double ard::tda_vertical::price1()const 
{
    double rv = 0.0;
    if (vert_type == vertical_type::short_put || vert_type == vertical_type::short_call) {
        rv = short_opt->trade_price;
    }
    else {
        rv = long_opt->trade_price;
    }
    return rv;
};

double ard::tda_vertical::price2()const 
{
    double rv = 0.0;
    if (vert_type == vertical_type::short_put || vert_type == vertical_type::short_call) {
        rv = long_opt->trade_price;
    }
    else {
        rv = short_opt->trade_price;        
    }
    return rv;
};

QString ard::tda_vertical::options_as_str()const 
{
    QString rv;
    if (short_opt) {
        rv = short_opt->toString();
    }
    if (long_opt) {
        rv += "\n";
        rv += long_opt->toString();
    }
    return rv;
};

QString ard::tda_vertical::symbol_with_expire()const 
{
    QString sym = symbol;
    while (sym.size() < 5)
        sym += " ";

    QString rv;
    switch (vert_type) 
    {
    case vertical_type::short_put:
    case vertical_type::short_call:
        rv = QString("%1 %2/%3").arg(sym).arg(short_opt->strike).arg(long_opt->strike);
        break;
    case vertical_type::long_put:
    case vertical_type::long_call:
        rv = QString("%1 %2/%3").arg(sym).arg(long_opt->strike).arg(short_opt->strike);
        break;
    default:break;
    }
    /*
    if (vert_type == vertical_type::short_put ||
        vert_type == vertical_type::short_call) 
    {
        rv = QString("%1 %2/%3").arg(symbol).arg(short_opt->strike).arg(long_opt->strike);
    }
    else 
    {
        rv = QString("%1 %2/%3").arg(symbol).arg(long_opt->strike).arg(short_opt->strike);
    }
    */
    return rv;
};

QString ard::tda_vertical::vtype_as_string()const 
{
    QString rv;
    switch (vert_type) 
    {
    case vertical_type::short_put:  rv = "S Put"; break;
    case vertical_type::long_put:   rv = "L Put"; break;
    case vertical_type::short_call: rv = "S Call"; break;
    case vertical_type::long_call:  rv = "L Call"; break;
    default:rv = "N";
    }
    return rv;
};


QString ard::tda_vertical::toString()const 
{
    QString rv = QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11")
        .arg(symbol)
        .arg(qty)
        .arg(long_opt->strike)
        .arg(short_opt->strike)
        .arg(vert_width)
        .arg(long_opt->trade_price)
        .arg(short_opt->trade_price)
        .arg(vert_price)
        .arg(max_loss)
        .arg(max_profit)
        .arg(unrealized_ror);
    return rv;
};

///tda
ard::tda::tda() 
{
    m_working_font = QApplication::font("QMenu");
    auto sz = m_working_font.pointSize();
    sz += 2;
    m_working_font.setPointSize(sz);

    m_brush_up = QBrush(color::LightGreen);
    m_brush_down = QBrush(color::LightRed);
};

void ard::tda::reloadFile(QString fileName, QString logRoot)
{
    m_options.clear();
    m_verticals.clear();
    m_grouped_by_symbol.clear();
    m_grouped_by_expire.clear();
    m_t2vertical.clear();
    m_s2vertical.clear();
    m_exp2vertical.clear();
    m_s2options.clear();
    m_code2opt_list.clear();

    m_logDirRoot = logRoot;

    m_working_curr_date = QDate::currentDate();
    loadStatementFile(fileName);
    reloadPricesSnaphost();
};

void ard::tda::loadStatementFile(QString statementFileName) 
{
    QFile f(statementFileName);
    if (f.exists()) {
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "failed to open TDA file " << statementFileName;
            return;
        }

        auto b = f.readAll();
        qDebug() << "reading TDA file " << statementFileName << b.size();
        auto s1 = b.toStdString();
        auto idx_options = s1.find("Options");
        if (idx_options == std::string::npos) {
            qWarning() << "'Options' section not found in" << statementFileName;
            return;
        }
        idx_options = s1.find('\n', idx_options) + 1;

        auto idx_pnl = s1.find("Profits and Losses", idx_options);
        if (idx_pnl == std::string::npos) {
            qWarning() << "'PNL' section not found in" << statementFileName;
            return;
        }

        auto idx_account_sum = s1.find("Account Summary", idx_pnl);
        if (idx_account_sum == std::string::npos) {
            qWarning() << "'Account Summary' section not found in" << statementFileName;
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
                            add_option(std::move(o));
                        }
                    }
                    else
                    {
                        qWarning() << "ignoring tda line" << line;
                    }
                }//for

                build_verticals();
            }//load_options

            bool load_summary = true;
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

void ard::tda::reloadPricesSnaphost()
{
    m_prices_snapshot.clear();
    QString snapshotFile = m_logDirRoot + "/live/snapshot.log";
    QFile f(snapshotFile);
    if (f.exists()) {
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "failed to open snapshot file " << snapshotFile;
            return;
        }
        auto b = f.readAll();
        qDebug() << "reading snapshot file " << snapshotFile << b.size();
        auto str = b.toStdString();
        std::stringstream ss(str);
        ard::ArdiCsv a;
        if (a.loadCsv(ss))
        {
            ard::VALUES_LIST& rlst = a.rows();
            for (auto& line : rlst)
            {
                if (line.size() >= 1)
                {
                    qDebug() << "snapshot" << line[0] << line[1];
                    m_prices_snapshot[line[0]] = line[1].toDouble();
                }
            }
        }
    }
};

bool ard::tda::add_option(ard::tda_option::ptr&& o) 
{
    if (o->symbol == "TSLA") 
    {
        int dbg = 0;
        dbg++;
    }
    /*
    auto i = m_code2opt_list.find(o->code);
    if (i != m_code2opt_list.end())
    { 
        qWarning() << "duplicate option code" << o->code;
        return false;
    }*/

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

    m_options.emplace_back(std::move(o));

    return true;
};

void ard::tda::add_vertical(ard::tda_vertical::ptr&& v)
{
    bool add2t = true;
    if (add2t)
    {
        auto j = m_t2vertical.find(v->vert_type);
        if (j == m_t2vertical.end())
        {
            std::vector<tda_vertical*> arr;
            arr.push_back(v.get());
            m_t2vertical[v->vert_type] = arr;
        }
        else
        {
            j->second.push_back(v.get());
        }
    }

    bool add2s = true;
    if (add2s) 
    {
        auto j = m_s2vertical.find(v->symbol);
        if (j == m_s2vertical.end())
        {
            std::vector<tda_vertical*> arr;
            arr.push_back(v.get());
            m_s2vertical[v->symbol] = arr;
        }
        else
        {
            j->second.push_back(v.get());
        }
    }

    bool add2e = true;
    if (add2e) 
    {
        auto j = m_exp2vertical.find(v->exp);
        if (j == m_exp2vertical.end())
        {
            std::vector<tda_vertical*> arr;
            arr.push_back(v.get());
            m_exp2vertical[v->exp] = arr;
        }
        else
        {
            j->second.push_back(v.get());
        }
    }

    v->vid = m_verticals.size();
    m_verticals.emplace_back(std::move(v));
};

QFont ard::tda::working_font()const 
{
    return m_working_font;
};

ard::tda_vertical::ptr ard::tda::make_vertical(tda_option* long_opt, tda_option* short_opt)
{
    std::unique_ptr<ard::tda_vertical> rv(new ard::tda_vertical);
    rv->qty = long_opt->qty;
    rv->long_opt = long_opt;
    rv->short_opt = short_opt;
    rv->symbol = long_opt->symbol;
    rv->exp = long_opt->exp;
    rv->days2exp = m_working_curr_date.daysTo(rv->exp);

    if (long_opt->opt != short_opt->opt) {
        qWarning() << "unsupported vertical" << long_opt->toString() << short_opt->toString();
        return nullptr;
    }

    rv->vert_width = std::abs(long_opt->strike - short_opt->strike);
    rv->vert_price = std::abs(long_opt->trade_price - short_opt->trade_price);
    rv->vert_mark_price = std::abs(long_opt->mark_price - short_opt->mark_price);   
    rv->is_debit = (long_opt->trade_price > short_opt->trade_price);
    rv->vert_val = rv->vert_price * rv->qty * 100;

    if (rv->is_debit)
    {
        rv->vert_mark_val = long_opt->mark_val - short_opt->mark_val;
        /// debit ///
        if (long_opt->opt == opt_type::put) 
        {
            rv->vert_type = vertical_type::long_put;
        }
        else if (long_opt->opt == opt_type::call) 
        {
            rv->vert_type = vertical_type::long_call;
        }
        rv->max_loss = rv->vert_price * rv->qty * 100;
        rv->max_profit = (rv->vert_width - rv->vert_price) * rv->qty * 100;
        rv->vert_pnl = rv->vert_mark_val - rv->vert_val;
        rv->vert_target_price = rv->vert_price * 1.5;
    }
    else
    {
        rv->vert_mark_val = short_opt->mark_val - long_opt->mark_val;
        /// credit ///
        if (long_opt->opt == opt_type::put)
        {
            rv->vert_type = vertical_type::short_put;
        }
        else if (long_opt->opt == opt_type::call)
        {
            rv->vert_type = vertical_type::short_call;
        }
        rv->max_loss = (rv->vert_width - rv->vert_price) * rv->qty * 100;
        rv->max_profit = rv->vert_price * rv->qty * 100;
        rv->vert_pnl = rv->vert_val - rv->vert_mark_val;
        rv->vert_target_price = rv->vert_price * 0.5;
    }

    //rv->max_loss = rv->max_loss * 100;
    //rv->max_profit = rv->max_profit * 100;
    //rv->vert_val = rv->vert_price * rv->qty * 100;
    
    //rv->vert_pnl = rv->vert_mark_val - rv->vert_val;
    if (std::abs(rv->vert_pnl) < 0.01)
        rv->vert_pnl = 0;

    if (rv->vert_val > 0) {
        rv->vert_pnl_percent = (rv->vert_pnl) / rv->vert_val;
    }

    //if (rv->vert_price > 0) {
    //  rv->vert_pnl_percent = (rv->vert_pnl) / rv->vert_price;
    //}


    if (rv->max_loss > 0) {
        rv->unrealized_ror = (rv->max_profit / rv->max_loss);
    }

    return rv;
};

void ard::tda::build_verticals()
{
    ard::S2OPT_LIST long_opt, short_opt;

    for (auto& o : m_options) 
    {
        if (o->isLong()) {
            auto i = long_opt.find(o->vertical_group_str);
            if (i != long_opt.end()) {
                std::vector<tda_option*>& lst = i->second;
                lst.push_back(o.get());
                //ASSERT(0, "duplicate option found in group/long") << o->vertical_group_str;
            }
            else {
                std::vector<tda_option*> lst;
                lst.push_back(o.get());
                long_opt[o->vertical_group_str] = lst;
            }
        }
        else {
            auto i = short_opt.find(o->vertical_group_str);
            if (i != short_opt.end()) {
                std::vector<tda_option*>& lst = i->second;
                lst.push_back(o.get());
                //ASSERT(0, "duplicate option found in group/short") << o->vertical_group_str;
            }
            else {
                std::vector<tda_option*> lst;
                lst.push_back(o.get());
                short_opt[o->vertical_group_str] = lst;
            }
        }
    }


    auto i = long_opt.begin();
    for (; i != long_opt.end();i++) 
    {
        std::vector<tda_option*>& lst = i->second;
        if (lst.empty())
            continue;
        auto ii = lst.begin();
        for (; ii != lst.end();) 
        {
            auto j = short_opt.find((*ii)->vertical_group_str);
            if (j == short_opt.end() || j->second.empty())
            {
                //qDebug() << "naked option" << (*ii)->vertical_group_str;
                ii++;
            }
            else 
            {
                std::vector<tda_option*>& lst2 = j->second;
                auto k = lst2.begin();
                ASSERT(k != lst2.end(), "unexpected empty list");
                if (k != lst2.end())
                {
                    auto v = make_vertical(*ii, *k);
                    if (v)
                    {
                        add_vertical(std::move(v));
                    }
                    lst2.erase(k);
                }
                ii = lst.erase(ii);
            }
        }
    }

    build_summary();
};

void ard::tda::build_summary()
{
    for (auto i : m_s2vertical) 
    {
        std::unique_ptr<ard::tda_symbol_grouped> sum(new ard::tda_symbol_grouped);
        sum->symbol = i.first;
        for(auto v : i.second)
        {
            switch (v->vert_type) 
            {
            case ard::vertical_type::short_put:     sum->short_put_count++; break;
            case ard::vertical_type::long_put:      sum->long_put_count++; break;
            case ard::vertical_type::short_call:    sum->short_call_count++; break;
            case ard::vertical_type::long_call:     sum->long_call_count++; break;
            case ard::vertical_type::none:break;
            }

            sum->total_val += v->vert_val;
            sum->total_mark_val += v->vert_mark_val;
            sum->total_pnl += v->vert_pnl;
            sum->total_max_loss += v->max_loss;
            sum->total_max_profit += v->max_profit;         
            if (sum->total_val > 0) {
                sum->total_pnl_percent = (sum->total_pnl) / sum->total_val;
            }           
        }

        m_grouped_by_symbol.push_back(std::move(sum));
    }

    //...
    for (auto i : m_exp2vertical)
    {
        std::unique_ptr<ard::tda_expire_grouped> sum(new ard::tda_expire_grouped);
        sum->expire = i.first;
        for (auto v : i.second)
        {
            switch (v->vert_type)
            {
            case ard::vertical_type::short_put:     sum->short_put_count++; break;
            case ard::vertical_type::long_put:      sum->long_put_count++; break;
            case ard::vertical_type::short_call:    sum->short_call_count++; break;
            case ard::vertical_type::long_call:     sum->long_call_count++; break;
            case ard::vertical_type::none:break;
            }

            sum->total_val += v->vert_val;
            sum->total_mark_val += v->vert_mark_val;
            sum->total_pnl += v->vert_pnl;
            sum->total_max_loss += v->max_loss;
            sum->total_max_profit += v->max_profit;
            if (sum->total_val > 0) {
                sum->total_pnl_percent = (sum->total_pnl) / sum->total_val;
            }
        }

        m_grouped_by_expire.push_back(std::move(sum));
    }
    //...

    for (auto& i : m_grouped_by_symbol)
    {
        m_summary.total_pnl += i->total_pnl;
        m_summary.total_max_loss += i->total_max_loss;
        m_summary.total_max_profit += i->total_max_profit;
    }

    m_summary.verticals_count   = m_verticals.size();
    m_summary.options_count     = m_options.size();
    m_summary.symbols_count     = m_grouped_by_symbol.size();

    std::sort(m_grouped_by_symbol.begin(), m_grouped_by_symbol.end(), [=](const tda_symbol_grouped::ptr& s1, const tda_symbol_grouped::ptr& s2) -> bool
    {
        return (s1->total_pnl < s2->total_pnl);
    }); 

    std::sort(m_grouped_by_expire.begin(), m_grouped_by_expire.end(), [=](const tda_expire_grouped::ptr& s1, const tda_expire_grouped::ptr& s2) -> bool
    {
        return (s1->total_pnl < s2->total_pnl);
    });

    m_summary.loosers.clear();
    m_summary.winners.clear();

    int count = 0;
    int idx = 0;
    while (idx < static_cast<int>(m_grouped_by_symbol.size()))
    {
        if (m_grouped_by_symbol[idx]->total_pnl > 0)
            break;
        if (idx > 3)
            break;
        m_summary.loosers.push_back(m_grouped_by_symbol[idx]->symbol);
        idx++;
    }

    idx = m_grouped_by_symbol.size() - 1;
    while (idx > 0) 
    {
        if (m_grouped_by_symbol[idx]->total_pnl < 0)
            break;
        if (count > 3)
            break;
        m_summary.winners.push_back(m_grouped_by_symbol[idx]->symbol);

        idx--;
        count++;
    }
};

ard::VERT_LIST ard::tda::getVerticalsByType(vertical_type t)
{
    auto i = m_t2vertical.find(t);
    if (i != m_t2vertical.end()) 
    {
        return i->second;
    }

    ard::VERT_LIST lst;
    return lst;
};

ard::tda_vertical* ard::tda::getVerticalById(int vid)
{
    ard::tda_vertical* rv = nullptr;
    if (vid >= 0 && vid < static_cast<int>(m_verticals.size()))
    {
        rv = m_verticals[vid].get();
    }
    return rv;
};

ard::VERT_LIST ard::tda::getAllVerticals()
{
    VERT_LIST rv;
    for (auto& i : m_verticals) {
        rv.push_back(i.get());
    }
    return rv;
};

void ard::sortVertList(ard::VERT_LIST& arr, ard::vertical_sort vs, bool reverse) 
{
    std::sort(arr.begin(), arr.end(), [=](const tda_vertical* v1, const tda_vertical* v2) -> bool
    {
        bool rv = false;
        switch (vs)
        {
        case vertical_sort::by_vert_type: 
        {
            int vt1 = static_cast<int>(v1->vert_type);
            int vt2 = static_cast<int>(v2->vert_type);
            rv = (vt1 < vt2);
        }break;
        case vertical_sort::by_symbol: rv = (v1->symbol < v2->symbol); break;
        case vertical_sort::by_expire: rv = (v1->exp < v2->exp); break;
        case vertical_sort::by_days: rv = (v1->days2exp < v2->days2exp); break;
        case vertical_sort::by_val: rv = (v1->vert_val < v2->vert_val); break;
        case vertical_sort::by_market_val: rv = (v1->vert_mark_val < v2->vert_mark_val); break;
        case vertical_sort::by_pnl: rv = (v1->vert_pnl < v2->vert_pnl); break;
        case vertical_sort::by_pnl_percent: rv = (v1->vert_pnl_percent < v2->vert_pnl_percent); break;
        case vertical_sort::by_qty: rv = (v1->qty < v2->qty); break;
        case vertical_sort::by_width: rv = (v1->vert_width < v2->vert_width); break;
        case vertical_sort::by_vert_price:rv = (v1->vert_price < v2->vert_price); break;
        case vertical_sort::by_vert_market:rv = (v1->vert_mark_price < v2->vert_mark_price); break;
        case vertical_sort::by_vert_target:rv = (v1->vert_target_price < v2->vert_target_price); break;
        case vertical_sort::by_price1: rv = (v1->price1() < v2->price1()); break;
        case vertical_sort::by_price2: rv = (v1->price2() < v2->price2()); break;
        case vertical_sort::by_debit_type: rv = (v1->vert_price < v2->vert_price); break;
        case vertical_sort::by_max_loss: rv = (v1->max_loss < v2->max_loss); break;
        case vertical_sort::by_max_profit: rv = (v1->max_profit < v2->max_profit); break;
        case vertical_sort::by_ror_un: rv = (v1->unrealized_ror < v2->unrealized_ror); break;
        }
        return rv;
    });

    if (reverse) {
        std::reverse(arr.begin(), arr.end());
    }
};

void ard::sortOptList(OPT_LIST& arr, option_sort os, bool reverse)
{
    std::sort(arr.begin(), arr.end(), [=](const tda_option* o1, const tda_option* o2) -> bool
    {
        bool rv = false;
        switch (os)
        {
        case option_sort::by_symbol:rv = (o1->symbol < o2->symbol); break;
        case option_sort::by_code:rv = (o1->code < o2->code); break;
        case option_sort::by_expire:rv = (o1->exp < o2->exp); break;
        case option_sort::by_strike:rv = (o1->strike < o2->strike); break;
        case option_sort::by_qty:rv = (o1->qty < o2->qty); break;
        case option_sort::by_price:rv = (o1->trade_price < o2->trade_price); break;
        case option_sort::by_mark_price:rv = (o1->mark_price < o2->mark_price); break;
        case option_sort::by_mark_val:rv = (o1->mark_val < o2->mark_val); break;
        }
        return rv;
    });

    if (reverse) {
        std::reverse(arr.begin(), arr.end());
    }
};

const ard::tda_summary& ard::tda::summary()const 
{
    return m_summary;
};


ard::VERT_LIST ard::tda::getVerticalsBySymbol(QString symbol) 
{
    auto i = m_s2vertical.find(symbol);
    if (i != m_s2vertical.end())
    {
        return i->second;
    }

    ard::VERT_LIST lst;
    return lst;
};

ard::VERT_LIST ard::tda::getVerticalsByExpire(QDate exp)
{
    auto i = m_exp2vertical.find(exp);
    if (i != m_exp2vertical.end())
    {
        return i->second;
    }

    ard::VERT_LIST lst;
    return lst;
};


ard::OPT_LIST ard::tda::getOptionsBySymbol(QString symbol) 
{
    auto i = m_s2options.find(symbol);
    if (i != m_s2options.end())
    {
        return i->second;
    }

    ard::OPT_LIST lst;
    return lst;
};

void ard::tda::toggle_symbol_grouped_sort(grouped_sort s)
{
    if (m_symbol_grouped_sort == s)
    {
        m_reverse_symbol_grouped_sort = !m_reverse_symbol_grouped_sort;
    }
    else {
        m_reverse_symbol_grouped_sort = false;
        m_symbol_grouped_sort = s;
    }
};

void ard::tda::toggle_expire_grouped_sort(grouped_sort s)
{
    if (m_expire_grouped_sort == s)
    {
        m_reverse_expire_grouped_sort = !m_reverse_expire_grouped_sort;
    }
    else {
        m_reverse_expire_grouped_sort = false;
        m_expire_grouped_sort = s;
    }
};

void ard::tda::toggle_verticals_sort(vertical_sort s) 
{
    if (m_vertical_sort == s) 
    {
        m_reverse_vertical_sort = !m_reverse_vertical_sort;
    }
    else 
    {
        m_reverse_vertical_sort = false;
        m_vertical_sort = s;
    }
};

void ard::tda::toggle_options_sort(option_sort s)
{
    if (m_opt_sort == s)
    {
        m_reverse_opt_sort = !m_reverse_opt_sort;
    }
    else 
    {
        m_reverse_opt_sort = false;
        m_opt_sort = s;
    }
};

QStandardItemModel* ard::tda::getGroupedBySymbolModel()
{
    QStringList lbls;
    lbls.push_back("T");
    lbls.push_back("sh-p");
    lbls.push_back("lg-p");
    lbls.push_back("sh-c");
    lbls.push_back("lg-c");
    lbls.push_back("Val");
    lbls.push_back("MktVal");
    lbls.push_back("pnl");
    lbls.push_back("pnl%");
    lbls.push_back("ML");
    lbls.push_back("MP");

    int row = 0;
    int col = 0;
    QStandardItem* it = nullptr;

    auto m = new QStandardItemModel();
    m->setHorizontalHeaderLabels(lbls);

    auto& arr = m_grouped_by_symbol;
    std::sort(arr.begin(), arr.end(), [=](const tda_symbol_grouped::ptr& s1, const tda_symbol_grouped::ptr& s2) -> bool
    {
        bool rv = false;
        switch (m_symbol_grouped_sort)
        {
        case grouped_sort::by_symbol:       rv = (s1->symbol < s2->symbol); break;
        case grouped_sort::by_short_put:    rv = (s1->short_put_count < s2->short_put_count);  break;
        case grouped_sort::by_long_put:     rv = (s1->long_put_count < s2->long_put_count);  break;
        case grouped_sort::by_short_call:   rv = (s1->short_call_count < s2->short_call_count); break;
        case grouped_sort::by_long_call:    rv = (s1->long_call_count < s2->long_call_count); break;
        case grouped_sort::by_val:          rv = (s1->total_val < s2->total_val); break;
        case grouped_sort::by_market_val:   rv = (s1->total_mark_val < s2->total_mark_val); break;
        case grouped_sort::by_pnl:          rv = (s1->total_pnl < s2->total_pnl); break;
        case grouped_sort::by_pnl_percent:  rv = (s1->total_pnl_percent < s2->total_pnl_percent); break;
        case grouped_sort::by_max_loss:     rv = (s1->total_max_loss < s2->total_max_loss); break;
        case grouped_sort::by_max_profit:   rv = (s1->total_max_profit < s2->total_max_profit); break;
        default:break;
        }

        return rv;
    });
    
    if (m_reverse_symbol_grouped_sort) {
        std::reverse(arr.begin(), arr.end());
    }

    for (auto& i : arr) 
    {
        SET_CELL(i->symbol); it->setData(i->symbol);
        SET_CELL(QString("%1").arg(i->short_put_count));
        SET_CELL(QString("%1").arg(i->long_put_count));
        SET_CELL(QString("%1").arg(i->short_call_count));
        SET_CELL(QString("%1").arg(i->long_call_count));
        SET_CELL(QString("%1$").arg(i->total_val));
        SET_CELL(QString("%1$").arg(i->total_mark_val));
        SET_CELL(QString("%1$").arg(i->total_pnl));
        if (i->total_pnl_percent > 0) {
            it->setBackground(m_brush_up);
        }
        else if (i->total_pnl_percent < 0) {
            it->setBackground(m_brush_down);
        }
        SET_CELL(QString("%1%").arg(i->total_pnl_percent * 100, 0, 'f', 1));
        SET_CELL(QString("%1$").arg(i->total_max_loss));
        SET_CELL(QString("%1$").arg(i->total_max_profit));

        row++;
        col = 0;
    }

    return m;
};

//...
QStandardItemModel* ard::tda::getGroupedByExpireModel()
{
    QStringList lbls;
    lbls.push_back("Exp");
    lbls.push_back("sh-p");
    lbls.push_back("lg-p");
    lbls.push_back("sh-c");
    lbls.push_back("lg-c");
    lbls.push_back("Val");
    lbls.push_back("MktVal");
    lbls.push_back("pnl");
    lbls.push_back("pnl%");
    lbls.push_back("ML");
    lbls.push_back("MP");

    int row = 0;
    int col = 0;
    QStandardItem* it = nullptr;

    auto m = new QStandardItemModel();
    m->setHorizontalHeaderLabels(lbls);

    auto& arr = m_grouped_by_expire;
    std::sort(arr.begin(), arr.end(), [=](const tda_expire_grouped::ptr& s1, const tda_expire_grouped::ptr& s2) -> bool
    {
        bool rv = false;
        switch (m_expire_grouped_sort)
        {
        case grouped_sort::by_expire:       rv = (s1->expire < s2->expire); break;
        case grouped_sort::by_short_put:    rv = (s1->short_put_count < s2->short_put_count); break;
        case grouped_sort::by_long_put:     rv = (s1->long_put_count < s2->long_put_count); break;
        case grouped_sort::by_short_call:   rv = (s1->short_call_count < s2->short_call_count); break;
        case grouped_sort::by_long_call:    rv = (s1->long_call_count < s2->long_call_count); break;
        case grouped_sort::by_val:          rv = (s1->total_val < s2->total_val); break;
        case grouped_sort::by_market_val:   rv = (s1->total_mark_val < s2->total_mark_val); break;
        case grouped_sort::by_pnl:          rv = (s1->total_pnl < s2->total_pnl); break;
        case grouped_sort::by_pnl_percent:  rv = (s1->total_pnl_percent < s2->total_pnl_percent); break;
        case grouped_sort::by_max_loss:     rv = (s1->total_max_loss < s2->total_max_loss); break;
        case grouped_sort::by_max_profit:   rv = (s1->total_max_profit < s2->total_max_profit); break;
        default: break;
        }
        return rv;
    });

    if (m_reverse_expire_grouped_sort) {
        std::reverse(arr.begin(), arr.end());
    }

    for (auto& i : arr)
    {
        SET_CELL(i->expire.toString("d MMM")); it->setData(i->expire);
        SET_CELL(QString("%1").arg(i->short_put_count));
        SET_CELL(QString("%1").arg(i->long_put_count));
        SET_CELL(QString("%1").arg(i->short_call_count));
        SET_CELL(QString("%1").arg(i->long_call_count));
        SET_CELL(QString("%1$").arg(i->total_val));
        SET_CELL(QString("%1$").arg(i->total_mark_val));
        SET_CELL(QString("%1$").arg(i->total_pnl));
        if (i->total_pnl_percent > 0) {
            it->setBackground(m_brush_up);
        }
        else if (i->total_pnl_percent < 0) {
            it->setBackground(m_brush_down);
        }
        SET_CELL(QString("%1%").arg(i->total_pnl_percent * 100, 0, 'f', 1));
        SET_CELL(QString("%1$").arg(i->total_max_loss));
        SET_CELL(QString("%1$").arg(i->total_max_profit));

        row++;
        col = 0;
    }

    return m;
};
//...

QStandardItemModel* ard::tda::getVerticalsModelByType(vertical_type t)
{
    /*
    QStringList lbls;
    lbls.push_back("T");
    lbls.push_back("E");
    lbls.push_back("D");
    lbls.push_back("Val");
    lbls.push_back("MktVal");
    lbls.push_back("pnl");
    lbls.push_back("pnl%");
    lbls.push_back("Q");
    bool is_debit = ard::is_debit_vertical(t);
    lbls.push_back("W");
    lbls.push_back(is_debit ? "long" : "short");
    lbls.push_back(is_debit ? "short" : "long");
    lbls.push_back(is_debit ? "D" : "C");
    lbls.push_back("ML");
    lbls.push_back("MP");
    lbls.push_back("ROR/U");

    int row = 0;
    int col = 0;
    QStandardItem* it = nullptr;


    auto m = new QStandardItemModel();
    m->setHorizontalHeaderLabels(lbls);
    
    auto arr = getVerticalsByType(t);
    ard::sortVertList(arr, m_vertical_sort, m_reverse_vertical_sort);
    for (auto v : arr)
    {
        QString symbol = v->symbol_with_expire();
        SET_CELL(symbol); it->setData(v->vid);
        SET_CELL(v->exp.toString("d MMM"));
        SET_CELL(QString("%1").arg(v->days2exp));
        SET_CELL(QString("%1$").arg(v->vert_val));
        SET_CELL(QString("%1$").arg(v->vert_mark_val));
        SET_CELL(QString("%1$").arg(v->vert_pnl));
        if (v->vert_pnl_percent > 0) {
            it->setBackground(m_brush_up);
        }
        else if (v->vert_pnl_percent < 0) {
            it->setBackground(m_brush_down);
        }
        SET_CELL(QString("%1%").arg(v->vert_pnl_percent * 100, 0, 'f', 1));
        SET_CELL(QString("%1").arg(v->qty));
        SET_CELL(QString("%1").arg(v->vert_width));
        SET_CELL(QString("%1").arg(v->price1()));
        SET_CELL(QString("%1").arg(v->price2()));
        SET_CELL(QString("%1").arg(v->vert_price));
        SET_CELL(QString("%1$").arg(v->max_loss));
        SET_CELL(QString("%1$").arg(v->max_profit));
        SET_CELL(QString("%1%").arg(v->unrealized_ror * 100, 0, 'f', 1));

        row++;
        col = 0;
    }

    return m;
    */

    auto arr = getVerticalsByType(t);
    return makeVerticalsModel(arr);
};

QStandardItemModel* ard::tda::getVerticalsModelByExpire(QDate exp_date)
{
    /*
    QStringList lbls;
    lbls.push_back("Type");
    lbls.push_back("Symbol");
    lbls.push_back("E");
    lbls.push_back("D");
    lbls.push_back("Val");
    lbls.push_back("MktVal");
    lbls.push_back("pnl");
    lbls.push_back("pnl%");
    lbls.push_back("Q");
    lbls.push_back("Price");
    lbls.push_back("Market");
    lbls.push_back("Target");
    lbls.push_back("ML");
    lbls.push_back("MP");

    int row = 0;
    int col = 0;
    QStandardItem* it = nullptr;


    auto m = new QStandardItemModel();
    m->setHorizontalHeaderLabels(lbls);

    ard::VERT_LIST arr;
    if (exp_date.isValid()) {
        arr = getVerticalsByExpire(exp_date);
    }
    else {
        arr = getAllVerticals();
    }
    ard::sortVertList(arr, m_vertical_sort, m_reverse_vertical_sort);
    for (auto v : arr)
    {
        SET_CELL(v->vtype_as_string()); it->setData(v->vid);
        SET_CELL(v->symbol_with_expire());
        SET_CELL(v->exp.toString("d MMM"));
        SET_CELL(QString("%1").arg(v->days2exp));
        SET_CELL(QString("%1$").arg(v->vert_val));
        SET_CELL(QString("%1$").arg(v->vert_mark_val));
        SET_CELL(QString("%1$").arg(v->vert_pnl));
        if (v->vert_pnl_percent > 0) {
            it->setBackground(m_brush_up);
        }
        else if (v->vert_pnl_percent < 0) {
            it->setBackground(m_brush_down);
        }
        SET_CELL(QString("%1%").arg(v->vert_pnl_percent * 100, 0, 'f', 1));
        SET_CELL(QString("%1").arg(v->qty));
        SET_CELL(QString("%1$").arg(v->vert_price));
        SET_CELL(QString("%1$").arg(v->vert_mark_price));
        SET_CELL(QString("%1$").arg(v->vert_target_price));
        SET_CELL(QString("%1$").arg(v->max_loss));
        SET_CELL(QString("%1$").arg(v->max_profit));

        row++;
        col = 0;
    }

    return m;
    */
    ard::VERT_LIST arr;
    if (exp_date.isValid()) {
        arr = getVerticalsByExpire(exp_date);
    }
    else {
        arr = getAllVerticals();
    }

    return makeVerticalsModel(arr);
};

QStandardItemModel* ard::tda::makeVerticalsModel(ard::VERT_LIST& arr) 
{
    QStringList lbls;
    lbls.push_back("Type");
    lbls.push_back("Symbol");
    lbls.push_back("E");
    lbls.push_back("D");
    lbls.push_back("Val");
    lbls.push_back("MktVal");
    lbls.push_back("pnl");
    lbls.push_back("pnl%");
    lbls.push_back("Q");
    lbls.push_back("Price");
    lbls.push_back("Market");
    lbls.push_back("Target");
    lbls.push_back("ML");
    lbls.push_back("MP");

    int row = 0;
    int col = 0;
    QStandardItem* it = nullptr;


    auto m = new QStandardItemModel();
    m->setHorizontalHeaderLabels(lbls);
    /*
    ard::VERT_LIST arr;
    if (exp_date.isValid()) {
        arr = getVerticalsByExpire(exp_date);
    }
    else {
        arr = getAllVerticals();
    }*/
    ard::sortVertList(arr, m_vertical_sort, m_reverse_vertical_sort);
    for (auto v : arr)
    {
        SET_CELL(v->vtype_as_string()); it->setData(v->vid);
        SET_CELL(v->symbol_with_expire());
        SET_CELL(v->exp.toString("d MMM"));
        SET_CELL(QString("%1").arg(v->days2exp));
        SET_CELL(QString("%1$").arg(v->vert_val));
        SET_CELL(QString("%1$").arg(v->vert_mark_val));
        SET_CELL(QString("%1$").arg(v->vert_pnl));
        if (v->vert_pnl_percent > 0) {
            it->setBackground(m_brush_up);
        }
        else if (v->vert_pnl_percent < 0) {
            it->setBackground(m_brush_down);
        }
        SET_CELL(QString("%1%").arg(v->vert_pnl_percent * 100, 0, 'f', 1));
        SET_CELL(QString("%1").arg(v->qty));
        SET_CELL(QString("%1$").arg(v->vert_price));
        SET_CELL(QString("%1$").arg(v->vert_mark_price));
        SET_CELL(QString("%1$").arg(v->vert_target_price));
        SET_CELL(QString("%1$").arg(v->max_loss));
        SET_CELL(QString("%1$").arg(v->max_profit));

        row++;
        col = 0;
    }

    return m;
};

QStandardItemModel* ard::tda::getOptionsModel(QString symbol)
{
    QStringList lbls;
    lbls.push_back("Symbol");
    lbls.push_back("Code");
    lbls.push_back("Exp");
    lbls.push_back("Type");
    lbls.push_back("Strike");
    lbls.push_back("Q");
    lbls.push_back("Price");
    lbls.push_back("Mark");
    lbls.push_back("MarkVal");

    int row = 0;
    int col = 0;
    QStandardItem* it = nullptr;


    auto m = new QStandardItemModel();
    m->setHorizontalHeaderLabels(lbls);
    auto arr = getOptionsBySymbol(symbol);
    ard::sortOptList(arr, m_opt_sort, m_reverse_opt_sort);
    for (auto o : arr)
    {
        SET_CELL(o->symbol);
        SET_CELL(o->code);
        SET_CELL(o->exp.toString("d MMM"));
        SET_CELL(o->opt == opt_type::call ? "C" : "P");
        SET_CELL(QString("%1").arg(o->strike));
        SET_CELL(QString("%1").arg(o->qty));
        SET_CELL(QString("%1").arg(o->trade_price));
        SET_CELL(QString("%1").arg(o->mark_price));
        SET_CELL(QString("%1$").arg(o->mark_val));
        row++;
        col = 0;
    };

    return m;
};

/*
static ard::vertical_type tabidx2vtype(int idx) 
{
    ard::vertical_type rv = ard::vertical_type::none;
    switch (idx)
    {
    case 0:rv = ard::vertical_type::short_put;break;
    case 1:rv = ard::vertical_type::long_put;break;
    case 2:rv = ard::vertical_type::short_call;break;
    case 3:rv = ard::vertical_type::long_call;break;
    }
    return rv;
}*/

static ard::vertical_type optview2vtype(ard::opt_view_type ov)
{
    ard::vertical_type rv = ard::vertical_type::none;

    switch (ov) 
    {
    case ard::opt_view_type::verticals_short_put:   rv = ard::vertical_type::short_put; break;
    case ard::opt_view_type::verticals_long_put:    rv = ard::vertical_type::long_put; break;
    case ard::opt_view_type::verticals_short_call:  rv = ard::vertical_type::short_call; break;
    case ard::opt_view_type::verticals_long_call:   rv = ard::vertical_type::long_call; break;
    }
    return rv;
}

static ard::opt_view_type vtype2optview(ard::vertical_type vt)
{
    ard::opt_view_type rv = ard::opt_view_type::none;
    switch (vt) 
    {
    case ard::vertical_type::short_put: rv = ard::opt_view_type::verticals_short_put; break;
    case ard::vertical_type::long_put:  rv = ard::opt_view_type::verticals_long_put; break;
    case ard::vertical_type::short_call:rv = ard::opt_view_type::verticals_short_call; break;
    case ard::vertical_type::long_call: rv = ard::opt_view_type::verticals_long_call; break;
    }
    return rv;
}


class VertView : public QTableView 
{
protected:
    void paintEvent(QPaintEvent *e)override 
    {
        QTableView::paintEvent(e);

        QItemSelectionModel *sm = selectionModel();
        if (sm && sm->hasSelection())
        {
            QModelIndexList lst = sm->selectedIndexes();
            auto i = lst.begin();
            if (i != lst.end()) 
            {
                auto idx = *i;
                auto r = idx.row();
                auto y = rowViewportPosition(r);
                auto h = rowHeight(r);
                auto v = viewport();
                auto rc = v->rect();
                QPainter p(v);
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QBrush(color::Navy), 2));
                p.drawRect(0, y, rc.width(), h);
            }
        }
    };
};

class VerticalCustomView : public QWidget 
{
public:
    virtual void set_vertical(ard::tda* t, ard::tda_vertical* v)
    {
        m_tda = t; m_vertical = v;
        if (m_tda && m_vertical)
        {
            m_font = m_tda->working_font();
            m_current_underlaying_price = 0.0;

            auto& m = m_tda->prices_snapshot();
            auto i = m.find(m_vertical->symbol);
            if (i != m.end())
            {
                m_current_underlaying_price = i->second;
            }
        }
        update();
    }

    void update_snapshot()
    {
        if (m_tda && m_vertical)
        {
            auto& m = m_tda->prices_snapshot();
            auto i = m.find(m_vertical->symbol);
            if (i != m.end())
            {
                m_current_underlaying_price = i->second;
                update();
            }
        }
    }
protected:
    int draw_label(QPainter& p, QFont& fnt, QString text, int left, int top, QRgb bkclr = color::White)
    {
        PGUARD(&p);
        QFontMetrics fm(fnt);
        auto f_sz = fm.boundingRect(text);
        QRect rclb(left, top, f_sz.width(), f_sz.height());
        p.setPen(Qt::NoPen);
        if (bkclr != color::Black) {
            p.setBrush(QBrush(bkclr));
        }
        auto rc_frame = rclb;
        rc_frame.setLeft(rc_frame.left() - ARD_MARGIN);
        rc_frame.setTop(rc_frame.top() - ARD_MARGIN);
        rc_frame.setHeight(rc_frame.height() + 2 * ARD_MARGIN);
        rc_frame.setWidth(rc_frame.width() + 2 * ARD_MARGIN);
        p.drawRect(rc_frame);
        p.setPen(QPen(color::Black));
        p.setFont(fnt);
        p.drawText(rclb, Qt::AlignCenter, text);
        return left + f_sz.width();
    }

protected:
    ard::tda_vertical*  m_vertical{ nullptr };
    ard::tda*           m_tda{ nullptr };
    QFont               m_font;
    double              m_current_underlaying_price{ 0 };
};

class VerticalPnlView : public VerticalCustomView
{
public:
    VerticalPnlView() 
    { 
        int ctrl_h = m_chart_height + 20;
        setMinimumHeight(ctrl_h);
        setMaximumHeight(ctrl_h);
        setMaximumWidth(300);
        setMinimumWidth(300);
        m_price_axe_mid_y = ctrl_h / 2;
    };
    void set_vertical(ard::tda* t, ard::tda_vertical* v)override
    { 
        VerticalCustomView::set_vertical(t,v);
        //m_tda = t; m_vertical = v; 
        if (m_vertical && t) 
        {
            //m_font = m_tda->working_font();
            m_fm = QFontMetrics(m_font);

            if (m_vertical->long_opt->strike > m_vertical->short_opt->strike) {
                m_sp1 = m_vertical->short_opt->strike;
                m_sp2 = m_vertical->long_opt->strike;
            }
            else {
                m_sp1 = m_vertical->long_opt->strike;
                m_sp2 = m_vertical->short_opt->strike;
            }
            double max_price = std::max(m_vertical->max_profit, m_vertical->max_loss);
            m_min_price = std::min(m_vertical->max_profit, m_vertical->max_loss);
            int max_y = m_chart_height / 2;
            if (max_price > 0.0) 
            {
                m_price_axe_px_per_val_delta = max_y / max_price;
            }

        }
         
    }
protected:
    void paintEvent(QPaintEvent *)override
    {
        auto rc = rect();
        //rc.setWidth(300);

        int wspace = ARD_MARGIN * 2;
        int ymid = m_price_axe_mid_y;//(rc.top() + rc.bottom()) / 2;
        int xstart = rc.left() + 10;
        //int profit_y = rc.top() + rc.height() / 4;
        //int loss_y = rc.bottom() - rc.height() / 4;
        int p1_pos = xstart + rc.width() / 3;
        int p2_pos = xstart + 2 * rc.width() / 3;
        double pdelta = m_sp2 - m_sp1;
        double price_x_per_val_delta = 1.0;
        if (p2_pos != p1_pos && pdelta > 0) {
            price_x_per_val_delta = (p2_pos - p1_pos)/ pdelta;
        }
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        //QPen pen(color::Silver);
        p.setPen(QPen(color::Gray));
        p.drawLine(rc.left() + wspace, ymid, rc.right() - wspace, ymid);
        p.drawLine(xstart, rc.top() + wspace, xstart, rc.bottom() - wspace);
        p.drawLine(p1_pos, ymid - ARD_MARGIN, p1_pos, ymid + ARD_MARGIN);
        p.drawLine(p2_pos, ymid - ARD_MARGIN, p2_pos, ymid + ARD_MARGIN);
        //p.drawLine(rc.left() + ARD_MARGIN * 5, ymid, rc.right() - ARD_MARGIN * 5, ymid);
        //int x1        
        if (m_vertical && m_tda)
        {
            int profit_y = price2ypos(m_vertical->max_profit);
            int loss_y = price2ypos(-m_vertical->max_loss);
            int pnl_y = price2ypos(m_vertical->vert_pnl);

            /// draw pnl line
            p.setPen(QPen(QBrush(color::Navy), 2));
            if (m_vertical->vert_type == ard::vertical_type::long_call ||
                m_vertical->vert_type == ard::vertical_type::short_put)
            {
                p.drawLine(xstart, loss_y, p1_pos, loss_y);
                p.drawLine(p1_pos, loss_y, p2_pos, profit_y);
                p.drawLine(p2_pos, profit_y, rc.right(), profit_y);
            }
            else {
                p.drawLine(xstart, profit_y, p1_pos, profit_y);
                p.drawLine(p1_pos, profit_y, p2_pos, loss_y);
                p.drawLine(p2_pos, loss_y, rc.right(), loss_y);
            }

            p.setPen(QPen(QBrush(color::Fuchsia), 2, Qt::DotLine));
            p.drawLine(xstart, pnl_y, rc.right(), pnl_y);

            p.setPen(QPen(color::Black));
            auto f_sz = m_fm.boundingRect("3450.78");
            p.setFont(m_font);
            QRect rclb = rc;
            auto x1 = draw_label(p, m_font, QString("%1").arg(m_vertical->max_profit), xstart + ARD_MARGIN, profit_y - f_sz.height() / 2);
            auto x2 = draw_label(p, m_font, QString("-%1").arg(m_vertical->max_loss), xstart + ARD_MARGIN, loss_y - f_sz.height() / 2);
            auto x_pnl = std::max(x1, x2) + 4 * ARD_MARGIN;
            QRgb bkclr = color::Green;
            if (m_vertical->vert_pnl < 0)
                bkclr = color::Red;
            draw_label(p, m_font, QString("%1").arg(m_vertical->vert_pnl), x_pnl, pnl_y - f_sz.height() / 2, bkclr);
            rclb = rc;
            rclb.setTop(ymid + ARD_MARGIN * 2);         
            auto s = QString("%1").arg(m_sp1);
            f_sz = m_fm.boundingRect(s);
            rclb.setLeft(p1_pos - f_sz.width() / 2);
            p.drawText(rclb, Qt::AlignLeft, s);

            s = QString("%1").arg(m_sp2);
            f_sz = m_fm.boundingRect(s);
            rclb.setLeft(p2_pos - f_sz.width() / 2);
            p.drawText(rclb, Qt::AlignLeft, s);

            auto fnt = m_tda->working_font();
            auto sz = fnt.pointSize();
            sz += 2;
            fnt.setPointSize(sz);
            fnt.setBold(true);
            QFontMetrics fm1(fnt);
            f_sz = fm1.boundingRect(m_vertical->symbol);

            draw_label(p, fnt, m_vertical->symbol, rclb.right() - f_sz.width(), rc.top());

            if (m_current_underlaying_price > 0.0) 
            {       
                int curr_underlaying_price_x = p1_pos + (m_current_underlaying_price - m_sp1) * price_x_per_val_delta;
                if (curr_underlaying_price_x > rc.right())
                    curr_underlaying_price_x = rc.right();
                if (curr_underlaying_price_x < xstart + ARD_MARGIN)
                    curr_underlaying_price_x = xstart + ARD_MARGIN;
                //qDebug() << "price_x_per_val_delta=" << price_x_per_val_delta << "p1_pos=" << p1_pos << "x=" << curr_underlaying_price_x;
                p.setPen(QPen(QBrush(color::NavySelected), 2));
                p.drawLine(curr_underlaying_price_x, rc.top() + wspace, curr_underlaying_price_x, rc.bottom() - wspace);
            }
        }

    };

    int price2ypos(double price)const
    {
        int rv = m_price_axe_mid_y - m_price_axe_px_per_val_delta * price;
        return rv;
    };


protected:
    double              m_sp1, m_sp2;
    double              m_min_price{ 0.0 };
    double              m_price_axe_px_per_val_delta{ 0.0 };
    const int           m_chart_height{100};
    int                 m_price_axe_mid_y{ 0 };
    QFontMetrics        m_fm{QFont()};
};

class VerticalStatusView : public VerticalCustomView
{
public:
;

protected:
    void paintEvent(QPaintEvent *)override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        auto rc = rect();
        int xstart = rc.left() + 10;
//      int wspace = ARD_MARGIN * 2;
        if (m_current_underlaying_price > 0)
        {
            auto fnt = m_tda->working_font();
            //auto sz = fnt.pointSize();
            //sz += 2;
            //fnt.setPointSize(sz);
            fnt.setBold(true);
            //p.setFont(fnt);
            draw_label(p, fnt, QString("%1").arg(m_current_underlaying_price), xstart + ARD_MARGIN, rc.top(), color::Black);
        }
    }
};

extern QString tbar_style;
ard::TDA_View::TDA_View()
{
    m_tda.reset(new ard::tda);
    
    m_grouped_view = new QTableView();
    m_grouped_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_vert_view = new VertView();
    m_vert_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_group_tab = new QTabBar();
    m_group_tab->setShape(QTabBar::RoundedWest);
    m_group_tab->setStyleSheet(tbar_style);

    m_vert_tab = new QTabBar();
    m_vert_tab->setShape(QTabBar::TriangularNorth);
    m_vert_tab->setStyleSheet(tbar_style);

    m_opt_list = new QPlainTextEdit();
    m_opt_list->setMinimumHeight(40);
    m_opt_list->setMaximumHeight(40);
    m_opt_list->setReadOnly(true);


    QVBoxLayout* lt_main = new QVBoxLayout();
    gui::setupBoxLayout(lt_main);
    auto spl = new QSplitter(Qt::Horizontal);
    
    spl->setStretchFactor(0, 1);
    spl->setStretchFactor(1, 3);
    
    QList<int> sz_lst;
    sz_lst.push_back(300);
    sz_lst.push_back(700);
    spl->setSizes(sz_lst);
    
    lt_main->addWidget(spl);
    setLayout(lt_main);
    //spl->addWidget(m_grouped_view);

    /// grouped by.. ///
    QHBoxLayout* lt_gr = new QHBoxLayout();
    gui::setupBoxLayout(lt_gr);
    lt_gr->addWidget(m_group_tab);
    lt_gr->addWidget(m_grouped_view);
    QWidget* w_gr = new QWidget();
    w_gr->setLayout(lt_gr);
    spl->addWidget(w_gr);

    m_vert_pnl_view = new VerticalPnlView;
    m_vert_status_view = new VerticalStatusView;
    QHBoxLayout* lt_pnl = new QHBoxLayout();
    lt_pnl->addWidget(m_vert_pnl_view);
    lt_pnl->addWidget(m_vert_status_view);
    /// verticals ///
    QVBoxLayout* lt_vert = new QVBoxLayout();
    gui::setupBoxLayout(lt_vert);
    lt_vert->addWidget(m_vert_tab);
    lt_vert->addWidget(m_vert_view);
    lt_vert->addLayout(lt_pnl);
    lt_vert->addWidget(m_opt_list); 
    QWidget* w_vert = new QWidget();
    w_vert->setLayout(lt_vert);
    spl->addWidget(w_vert);


    QWidget* status_space = new QWidget();
    status_space->setMaximumHeight(20);
    QHBoxLayout* lt_status = new QHBoxLayout();
    gui::setupBoxLayout(lt_status);
    status_space->setLayout(lt_status);
    lt_main->addWidget(status_space);
    //lt_main->addLayout(lt_status);
    m_status_label = new QLabel();
    m_sum_label = new QLabel();
    m_winners_label = new QLabel();
    m_loosers_label = new QLabel();
    lt_status->addWidget(m_status_label);
    lt_status->addWidget(m_winners_label);
    lt_status->addWidget(m_loosers_label);
    lt_status->addWidget(m_sum_label);
    m_status_label->setFont(m_tda->working_font());
    auto fnt_bold = m_tda->working_font();
    fnt_bold.setBold(true);
    m_sum_label->setFont(fnt_bold);
    m_winners_label->setFont(m_tda->working_font());
    m_loosers_label->setFont(m_tda->working_font());

    m_winners_label->setStyleSheet("QLabel { background-color : rgb(176, 215, 176); }");
    m_loosers_label->setStyleSheet("QLabel { background-color : rgb(255, 192, 192); }");

    m_group_tab->addTab("by exp");
    m_group_tab->addTab("by symbol");   

    m_vert_tab->addTab("verticals");
    m_vert_tab->addTab("short put vert");
    m_vert_tab->addTab("long put vert");
    m_vert_tab->addTab("short call vert");
    m_vert_tab->addTab("long call vert");
    m_vert_tab->addTab("options");

    /// group tab selection ///
    connect(m_group_tab, &QTabBar::currentChanged, [=](int) {
        refreshGroupView();
    });

    /// vert type selection ///
    connect(m_vert_tab, &QTabBar::currentChanged, [=](int){
        refreshOptionView();
    });

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

        if (!res.second.isEmpty())
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
        }

    });
    
    /// symbols sort ///
    if (m_grouped_view)
    {
        auto h = m_grouped_view->horizontalHeader();
        if (h)
        {           
            connect(h, &QHeaderView::sectionClicked, [&](int idx)
            {
                auto gr_view_type = currGroupView();
                grouped_sort sm_sort = grouped_sort::none;
                switch (idx)
                {
                case 0: 
                {
                    if (gr_view_type == group_view_type::expire) {
                        sm_sort = grouped_sort::by_expire;
                    }
                    else {
                        sm_sort = grouped_sort::by_symbol;
                    }
                } break;
                case 1:sm_sort = grouped_sort::by_short_put; break;
                case 2:sm_sort = grouped_sort::by_long_put; break;
                case 3:sm_sort = grouped_sort::by_short_call; break;
                case 4:sm_sort = grouped_sort::by_long_call; break;
                case 5:sm_sort = grouped_sort::by_val; break;
                case 6:sm_sort = grouped_sort::by_market_val; break;
                case 7:sm_sort = grouped_sort::by_pnl; break;
                case 8:sm_sort = grouped_sort::by_pnl_percent; break;
                case 9:sm_sort = grouped_sort::by_max_loss; break;
                case 10:sm_sort = grouped_sort::by_max_profit; break;
                }
                if (sm_sort != grouped_sort::none)
                {
                    if (gr_view_type == group_view_type::expire) {
                        m_tda->toggle_expire_grouped_sort(sm_sort);
                    }
                    else {
                        m_tda->toggle_symbol_grouped_sort(sm_sort);
                    }
                    refreshGroupView();
                }
            });
        }
    }


    /// verticals sort ///
    if (m_vert_view)
    {
        auto h = m_vert_view->horizontalHeader();
        if (h)
        {
            connect(h, &QHeaderView::sectionClicked, [&](int idx)
            {
                auto vt = currOptView();
                switch (vt) 
                {
                case opt_view_type::none:break;
                case opt_view_type::verticals_by_expire:
                case opt_view_type::verticals_short_put:
                case opt_view_type::verticals_long_put:
                case opt_view_type::verticals_short_call:
                case opt_view_type::verticals_long_call:
                {
                    /// verticals view ///
                    vertical_sort vs = vertical_sort::none;
                    switch (idx)
                    {
                    case 0:vs = vertical_sort::by_vert_type; break;
                    case 1:vs = vertical_sort::by_symbol; break;
                    case 2:vs = vertical_sort::by_expire; break;
                    case 3:vs = vertical_sort::by_days; break;
                    case 4:vs = vertical_sort::by_val; break;
                    case 5:vs = vertical_sort::by_market_val; break;
                    case 6:vs = vertical_sort::by_pnl; break;
                    case 7:vs = vertical_sort::by_pnl_percent; break;
                    case 8:vs = vertical_sort::by_qty; break;
                    case 9:vs = vertical_sort::by_vert_price; break;
                    case 10:vs = vertical_sort::by_vert_market; break;
                    case 11:vs = vertical_sort::by_vert_target; break;
                    case 12:vs = vertical_sort::by_max_loss; break;
                    case 13:vs = vertical_sort::by_max_profit; break;
                    }
                    if (vs != vertical_sort::none)
                    {
                        m_tda->toggle_verticals_sort(vs);
                        refreshOptionView();
                    }
                }break;
                case opt_view_type::options_for_group: 
                {
                    ard::option_sort os = option_sort::none;
                    switch (idx)
                    {
                    case 0:os = option_sort::by_symbol; break;
                    case 1:os = option_sort::by_code; break;
                    case 2:os = option_sort::by_expire; break;
                    case 3:os = option_sort::by_type; break;
                    case 4:os = option_sort::by_strike; break;
                    case 5:os = option_sort::by_qty; break;
                    case 6:os = option_sort::by_price; break;
                    case 7:os = option_sort::by_mark_price; break;
                    case 8:os = option_sort::by_mark_val; break;
                    }
                    if (os != option_sort::none)
                    {
                        m_tda->toggle_options_sort(os);
                        refreshOptionView();
                    }
                }break;
                /*
                case opt_view_type::verticals_short_put:
                case opt_view_type::verticals_long_put:
                case opt_view_type::verticals_short_call:
                case opt_view_type::verticals_long_call: 
                {
                    /// verticals view ///
                    vertical_sort vs = vertical_sort::none;
                    switch (idx)
                    {
                    case 0:vs = vertical_sort::by_symbol; break;
                    case 1:vs = vertical_sort::by_expire; break;
                    case 2:vs = vertical_sort::by_days; break;
                    case 3:vs = vertical_sort::by_val; break;
                    case 4:vs = vertical_sort::by_market_val; break;
                    case 5:vs = vertical_sort::by_pnl; break;
                    case 6:vs = vertical_sort::by_pnl_percent; break;
                    case 7:vs = vertical_sort::by_qty; break;
                    case 8:vs = vertical_sort::by_width; break;
                    case 9:vs = vertical_sort::by_price1; break;
                    case 10:vs = vertical_sort::by_price2; break;
                    case 11:vs = vertical_sort::by_debit_type; break;
                    case 12:vs = vertical_sort::by_max_loss; break;
                    case 13:vs = vertical_sort::by_max_profit; break;
                    case 14:vs = vertical_sort::by_ror_un; break;
                    }
                    if (vs != vertical_sort::none)
                    {
                        m_tda->toggle_verticals_sort(vs);
                        refreshOptionView();
                    }
                }break;
                */
                }
            });
        }
    }
};

ard::TDA_View::~TDA_View() 
{
    detach();
};

ard::group_view_type ard::TDA_View::currGroupView()const 
{
    ard::group_view_type rv = group_view_type::none;
    switch (m_group_tab->currentIndex()) 
    {
    case 0: rv = group_view_type::expire; break;
    case 1: rv = group_view_type::symbol; break;
    }
    return rv;
};

ard::opt_view_type  ard::TDA_View::currOptView()const 
{
    ard::opt_view_type rv = opt_view_type::none;
    switch (m_vert_tab->currentIndex()) 
    {
    case 0: rv = opt_view_type::verticals_by_expire; break;
    case 1: rv = opt_view_type::verticals_short_put; break;
    case 2: rv = opt_view_type::verticals_long_put; break;
    case 3: rv = opt_view_type::verticals_short_call; break;
    case 4: rv = opt_view_type::verticals_long_call; break;
    case 5: rv = opt_view_type::options_for_group; break;
    }
    return rv;
};

void ard::TDA_View::setCurrOptView(ard::opt_view_type ov) 
{
    int idx = -1;
    switch (ov) 
    {
    case opt_view_type::verticals_by_expire:    idx = 0; break;
    case opt_view_type::verticals_short_put:    idx = 1; break;
    case opt_view_type::verticals_long_put:     idx = 2; break;
    case opt_view_type::verticals_short_call:   idx = 3; break;
    case opt_view_type::verticals_long_call:    idx = 4; break;
    case opt_view_type::options_for_group:      idx = 5; break;
    }

    if (idx != -1) 
    {
        if (idx == m_vert_tab->currentIndex()) {
            refreshOptionView();
        }
        else {
            m_vert_tab->setCurrentIndex(idx);
        }
    }
};

void ard::TDA_View::prepareTableView(QTableView* v) 
{
    auto h = v->horizontalHeader();
    int columns = v->model()->columnCount();
    for (int i = 0; i < columns; i++) {
        h->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }
};

void ard::TDA_View::refreshGroupView()
{
    m_sum_label->setText("");

    QStandardItemModel* m = nullptr;
    auto gv = currGroupView();
    if (gv == group_view_type::expire){
        m = m_tda->getGroupedByExpireModel();
    }
    else if (gv == group_view_type::symbol){
        m = m_tda->getGroupedBySymbolModel();
    }
    if (m)
    {
        m_grouped_view->setModel(m);
        prepareTableView(m_grouped_view);
    }

    if (gv == group_view_type::expire) {
        setupExpireGroupedView();
    }
    else {
        setupSymbolGroupedView();
    }
};

void ard::TDA_View::setupExpireGroupedView() 
{
    connect(m_grouped_view->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &, const QItemSelection &)
    {
        QStandardItemModel* dm = dynamic_cast<QStandardItemModel*>(m_grouped_view->model());
        QModelIndex idx = m_grouped_view->currentIndex();
        if (idx.isValid())
        {
            bool switch_to_all_vert_byexp_list = false;
            vertical_type vt = vertical_type::none;
            auto col = idx.column();
            switch (col)
            {
            case 0:switch_to_all_vert_byexp_list = true; break;
            case 1:vt = vertical_type::short_put; break;
            case 2:vt = vertical_type::long_put; break;
            case 3:vt = vertical_type::short_call; break;
            case 4:vt = vertical_type::long_call; break;
            }

            if (switch_to_all_vert_byexp_list) 
            {               
                auto it = dm->itemFromIndex(idx);
                if (it) {
                    auto dt = it->data().toDate();
                    //qDebug() << "vert-by-exp" << dt;
                    if (m_curr_exp_date != dt)
                    {
                        m_curr_exp_date = dt;
                        //setCurrOptView(ard::opt_view_type::verticals_by_expire);
                    }
                    setCurrOptView(ard::opt_view_type::verticals_by_expire);
                }
            }
            else 
            {
                bool upd_vert_exp_view = m_curr_exp_date.isValid();
                auto idx1 = idx.sibling(idx.row(), 0);
                if (idx1.isValid())
                {
                    auto it1 = dm->itemFromIndex(idx1);
                    if (it1) {
                        m_curr_exp_date = it1->data().toDate();
                    }
                }
                //m_curr_exp_date = QDate();
                if (vt != vertical_type::none)
                {
                    ard::opt_view_type ov = vtype2optview(vt);
                    if (ov != ard::opt_view_type::none) {
                        setCurrOptView(ov);
                    }
                }
                else 
                {
                    if (upd_vert_exp_view) {
                        auto ov = currOptView();
                        if (ov == ard::opt_view_type::verticals_by_expire) {
                            refreshOptionView();
                        }
                    }
                }
            }

            selectVerticalByExpDate(m_curr_exp_date);
        }//current index


        auto sm = m_grouped_view->selectionModel();
        if (sm->hasSelection())
        {
            QModelIndexList list = sm->selectedIndexes();
            updateSelectionSumLabel(list);
        }
    });
};

void ard::TDA_View::setupSymbolGroupedView() 
{
    connect(m_grouped_view->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &, const QItemSelection &)
    {
        QModelIndex idx = m_grouped_view->currentIndex();
        if (idx.isValid())
        {
            bool switch_to_opt_list = false;
            vertical_type vt = vertical_type::none;
            auto col = idx.column();
            switch (col)
            {
            case 0:switch_to_opt_list = true; break;
            case 1:vt = vertical_type::short_put; break;
            case 2:vt = vertical_type::long_put; break;
            case 3:vt = vertical_type::short_call; break;
            case 4:vt = vertical_type::long_call; break;
            }

            if (vt != vertical_type::none)
            {
                ard::opt_view_type ov = vtype2optview(vt);
                if (ov != ard::opt_view_type::none) {
                    setCurrOptView(ov);
                }
            }
            auto idx1 = idx.sibling(idx.row(), 0);
            if (idx1.isValid())
            {
                QString symbol = m_grouped_view->model()->data(idx1).toString();
                if (m_curr_symbol != symbol)
                {
                    m_curr_symbol = symbol;
                    auto vt = currOptView();
                    switch (vt) 
                    {
                    case opt_view_type::options_for_group: {
                        refreshOptionView();
                    }break;
                    default:break;
                    }
                }
            }

            if (switch_to_opt_list) {
                setCurrOptView(ard::opt_view_type::options_for_group);
            }
            else {
                //qDebug() << "locate symbol:" << m_curr_symbol;
                selectVerticalBySymbol(m_curr_symbol);
            }
        }

        auto sm = m_grouped_view->selectionModel();
        if (sm->hasSelection())
        {
            QModelIndexList list = sm->selectedIndexes();
            updateSelectionSumLabel(list);
        }
    });
};

void ard::TDA_View::refreshOptionView()
{
    m_opt_list->setPlainText("");
    m_sum_label->setText("");

    QStandardItemModel* m = nullptr;

    auto ov = currOptView();
    switch (ov) 
    {
    case opt_view_type::verticals_by_expire: 
    {
        m = m_tda->getVerticalsModelByExpire(m_curr_exp_date);
        if (m) 
        {
            m_vert_view->setModel(m);
            setupVerticalView(m);
        }
    }break;

    case opt_view_type::verticals_short_put:
    case opt_view_type::verticals_long_put:
    case opt_view_type::verticals_short_call:
    case opt_view_type::verticals_long_call: 
    {
        /// one of verticals page ///
        auto vt = optview2vtype(ov);
        if (vt != vertical_type::none) {
            m = m_tda->getVerticalsModelByType(vt);
        }
        if (m)
        {
            setupVerticalView(m);
        }
    }break;
    case opt_view_type::options_for_group:
    {
        /// options page ///
        m = m_tda->getOptionsModel(m_curr_symbol);
        if (m)
        {
            m_vert_view->setModel(m);
            prepareTableView(m_vert_view);

            connect(m_vert_view->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &, const QItemSelection &)
            {
                QItemSelectionModel *sm = m_vert_view->selectionModel();
                QModelIndexList list = sm->selectedIndexes();
                if (list.size() > 0)
                {
                    updateSelectionSumLabel(list);
                }
            });
        }
    }break;
    }
};

void ard::TDA_View::setupVerticalView(QStandardItemModel* m) 
{
    RETURN_VOID_ON_ASSERT(m, "expected model");
    m_vert_view->setModel(m);
    prepareTableView(m_vert_view);

    connect(m_vert_view->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &, const QItemSelection &)
    {
        QStandardItemModel* dm = dynamic_cast<QStandardItemModel*>(m_vert_view->model());
        if (dm)
        {
            QItemSelectionModel *sm = m_vert_view->selectionModel();
            QModelIndexList list = sm->selectedIndexes();
            if (list.size() > 0)
            {
                /// refresh options list ///
                QModelIndex idx = list.at(0);
                auto idx1 = idx.sibling(idx.row(), 0);
                if (idx1.isValid())
                {
                    auto it = dm->itemFromIndex(idx1);
                    if (it) {
                        auto vid = it->data().toInt();
                        auto v = m_tda->getVerticalById(vid);
                        if (v)
                        {
                            auto s = v->options_as_str();
                            m_opt_list->setPlainText(s);
                            m_vert_pnl_view->set_vertical(m_tda.get(), v);
                            m_vert_status_view->set_vertical(m_tda.get(), v);
                        }
                    }
                }
                updateSelectionSumLabel(list);
                m_vert_view->viewport()->update();
            }
        }
    });
};

void ard::TDA_View::updateSelectionSumLabel(QModelIndexList& lst) 
{
    double sum = 0;
    for (auto i : lst)
    {
        auto s = trim_integral_str(i.data().toString());
        sum += s.toDouble();
    }
    m_sum_label->setText(QString("      [%1$]").arg(sum));
};

void ard::TDA_View::selectVerticalBySymbol(QString symbol) 
{
    QStandardItemModel* dm = dynamic_cast<QStandardItemModel*>(m_vert_view->model());
    if (dm)
    {
        QItemSelectionModel *sm = m_vert_view->selectionModel();
        auto Max = dm->rowCount();
        for (int i = 0; i < Max; i++)
        {
            auto idx = dm->index(i, 0);
            auto it = dm->itemFromIndex(idx);
            if (it) 
            {
                auto vid = it->data().toString().toInt();
                auto v = m_tda->getVerticalById(vid);
                if (v && v->symbol == symbol)
                {
                    sm->select(idx, QItemSelectionModel::Select);
                    break;
                }
            }
        }
    }
};

void ard::TDA_View::selectVerticalByExpDate(QDate exp) 
{
    QStandardItemModel* dm = dynamic_cast<QStandardItemModel*>(m_vert_view->model());
    if (dm)
    {
        QItemSelectionModel *sm = m_vert_view->selectionModel();
        auto Max = dm->rowCount();
        for (int i = 0; i < Max; i++)
        {
            auto idx = dm->index(i, 0);
            auto it = dm->itemFromIndex(idx);
            if (it)
            {
                auto vid = it->data().toString().toInt();
                auto v = m_tda->getVerticalById(vid);
                if (v && v->exp == exp)
                {
                    sm->select(idx, QItemSelectionModel::Select);
                    break;
                }
            }
        }
    }
};

void ard::TDA_View::attachFileRef(ard::fileref* r) 
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

void ard::TDA_View::detach()
{
    if (m_tda_file_ref)
    {
        m_tda_file_ref->release();
        m_tda_file_ref = nullptr;
    }
};

std::pair<QString, QString> ard::TDA_View::getStatementFileName()const
{
    std::pair<QString, QString> rv;
    if (m_tda_file_ref) 
    {
        rv = m_tda_file_ref->getRefFileName();
        /*
        auto c = m_tda_file_ref->annotation();
        if (!c.isEmpty()) 
        {
            auto idx = c.indexOf("logs:");
            if (idx != -1) {
                rv.second = c.mid(idx + 5);
            }
        }*/
    }
    return rv;
};

void ard::TDA_View::reloadTDAfile()
{
    RETURN_VOID_ON_ASSERT(m_tda_file_ref, "expected TDA ref object");
    RETURN_VOID_ON_ASSERT(m_tda, "expected TDA object");
    auto res = getStatementFileName();
    m_vert_pnl_view->set_vertical(nullptr, nullptr);
    m_statement_load_time = QDateTime::currentDateTime();

    m_tda->reloadFile(res.first, res.second);
    refreshGroupView();
    refreshOptionView();    

    //auto vcount = m_tda->verticals().size();
    //auto ocount = m_tda->options().size();
    auto sum = m_tda->summary();
    m_status_label->setText(QString("pnl:%1, max loss:%2, max profit: %3, vert: %4, opt: %5, symbols: %6")
        .arg(sum.total_pnl)
        .arg(sum.total_max_loss)
        .arg(sum.total_max_profit)
        .arg(sum.verticals_count)
        .arg(sum.options_count)
        .arg(sum.symbols_count));

    QString s = " [ ";
    for (auto i : sum.winners) {
        s += i;
        s += " ";
    }
    s += " ] ";
    m_winners_label->setText(s);
    s = " [ ";
    for (auto i : sum.loosers) {
        s += i;
        s += " ";
    }
    s += " ] ";
    m_loosers_label->setText(s);

    if (!sum.loosers.empty()) 
    {
        m_curr_symbol = sum.loosers[0];
    }
    else if (!sum.winners.empty()) {
        m_curr_symbol = sum.winners[0];
    }

    m_curr_exp_date = QDate();
};

void ard::TDA_View::reloadPricesSnapshot() 
{
    if (m_tda)
    {
        m_tda->reloadPricesSnaphost();
        if (m_vert_status_view) {
            m_vert_status_view->update_snapshot();
        }
        if (m_vert_pnl_view) {
            m_vert_pnl_view->update_snapshot();
        }
    }
};