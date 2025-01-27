#include <vector>
#include <QDebug>
#include <unordered_map>
#include "global-color.h"
#include "snc-assert.h"

/**
color
*/
const QRgb color::LOCATOR_ANI = qRgb(255, 203, 213);
const QRgb color::SELECTED_ITEM_BK = qRgb(182, 189, 210);
const QRgb color::MSELECTED_ITEM_BK = qRgb(202, 209, 230);
const QRgb color::HOT_SELECTED_ITEM_BK = qRgb(162, 169, 190);
const QRgb color::HOVER_ITEM_BK = qRgb(182, 218, 182); //qRgb(213, 217, 230);
const QRgb color::Black = qRgb(0, 0, 0);
const QRgb color::Maroon = qRgb(192, 128, 128);
const QRgb color::Green = qRgb(134, 247, 182);//qRgb(128,192,128);
const QRgb color::Olive = qRgb(192, 192, 128);
const QRgb color::Navy = qRgb(128, 128, 192);
const QRgb color::Purple = qRgb(227, 185, 255);//qRgb(192,128,192);
const QRgb color::Teal = qRgb(0, 128, 128);
const QRgb color::Gray = qRgb(128, 128, 128);
const QRgb color::Silver = qRgb(192, 192, 192);
const QRgb color::Red = qRgb(243, 180, 186);//qRgb(255,192,192);
const QRgb color::Lime = qRgb(192, 255, 192);
const QRgb color::Yellow = qRgb(255, 255, 192);
const QRgb color::Blue = qRgb(157, 189, 255);//qRgb(192,192,255);
const QRgb color::Fuchsia = qRgb(255, 0, 255);
const QRgb color::Aqua = qRgb(192, 255, 255);
const QRgb color::White = qRgb(255, 255, 255);
const QRgb color::Gray_1 = qRgb(64, 64, 64);
const QRgb color::Yellow_1 = qRgb(255, 255, 128);
const QRgb color::ReutersChart = qRgb(221, 123, 48);

// const QRgb color::TrueBlack =     qRgb(0,0,0);

const QRgb color::True1Maroon = qRgb(128, 0, 0);
const QRgb color::True1Green = qRgb(12, 174, 81);//*
const QRgb color::True1Olive = qRgb(128, 128, 0);
const QRgb color::True1Navy = qRgb(0, 0, 128);
const QRgb color::True1Purple = qRgb(160, 17, 255);//*
const QRgb color::True1Teal = qRgb(0, 128, 128);
const QRgb color::True1Gray = qRgb(128, 128, 128);
const QRgb color::True1Silver = qRgb(192, 192, 192);
const QRgb color::True1Red = qRgb(221, 44, 60);//*
const QRgb color::True1Lime = qRgb(0, 255, 0);
const QRgb color::True1Yellow = qRgb(255, 255, 0);
const QRgb color::True1Blue = qRgb(6, 88, 254);//*
const QRgb color::True1Fuchsia = qRgb(255, 0, 255);
const QRgb color::True1Aqua = qRgb(0, 255, 255);


const QRgb color::LightMaroon = qRgb(225, 193, 193);
const QRgb color::LightGreen = qRgb(176, 215, 176);
const QRgb color::LightOlive = qRgb(192, 192, 128);
const QRgb color::LightNavy = qRgb(188, 188, 222);
const QRgb color::LightPurple = qRgb(210, 164, 210);
const QRgb color::LightTeal = qRgb(0, 232, 232);
const QRgb color::LightGray = qRgb(221, 221, 221);
const QRgb color::LightSilver = qRgb(221, 221, 221);
const QRgb color::LightRed = qRgb(255, 192, 192);
const QRgb color::LightLime = qRgb(192, 255, 192);
const QRgb color::LightYellow = qRgb(255, 255, 192);
const QRgb color::LightBlue = qRgb(192, 192, 255);
const QRgb color::LightFuchsia = qRgb(255, 170, 255);
const QRgb color::LightAqua = qRgb(192, 255, 255);


const QRgb  color::SoftYellow = qRgb(255, 255, 175);
const QRgb  color::NavySelected = qRgb(49, 106, 197);
const QRgb  color::YellowSelected = qRgb(106, 106, 49);


QRgb color::invert(QRgb c)
{
    if (c == color::Navy ||
        c == color::Gray ||
        c == color::Gray_1 ||
        c == color::NavySelected ||
        c == color::Teal)
        return color::White;

    if (c == color::Green ||
        c == color::Olive ||
        c == color::Maroon ||
        c == color::Purple ||
        c == color::Red ||
        c == color::Blue ||
        c == color::Lime ||
        c == color::Yellow ||
        c == color::Fuchsia ||
        c == color::Aqua ||
        c == SELECTED_ITEM_BK)
        return color::Black;

    int r = 255 - qRed(c);// ^ 0x80;
    int g = 255 - qGreen(c);// ^ 0x80;
    int b = 255 - qBlue(c);// ^ 0x80;
    return qRgb(r, g, b);
    //    return ( 0x00FFFFFF & ~( c ) );
}

static std::unordered_map<unsigned int, int> g__rgb2idx;
static std::unordered_map<int, unsigned int> g__idx2rgb;
static void define_colors_map()
{
    if (g__rgb2idx.empty())
    {
#define ADD_CLR(I, C)g__rgb2idx[C] = I;g__idx2rgb[I] = C;

        ADD_CLR(0, color::Black);
        ADD_CLR(1, color::Maroon);
        ADD_CLR(2, color::Green);
        ADD_CLR(3, color::Olive);
        ADD_CLR(4, color::Navy);
        ADD_CLR(5, color::Purple);
        ADD_CLR(6, color::Teal);
        ADD_CLR(7, color::Gray);
        ADD_CLR(8, color::Silver);
        ADD_CLR(9, color::Red);
        ADD_CLR(10, color::Lime);
        ADD_CLR(11, color::Yellow);
        ADD_CLR(12, color::Blue);
        ADD_CLR(13, color::Fuchsia);
        ADD_CLR(14, color::Aqua);
        ADD_CLR(15, color::White);
        ADD_CLR(16, color::Gray_1);
        ADD_CLR(17, color::Yellow_1);
        ADD_CLR(18, color::SoftYellow);
#undef ADD_CLR
    }
}

int color::getColorIndexByColor(const QRgb& v)
{
    define_colors_map();
    auto i = g__rgb2idx.find(v);
    if (i != g__rgb2idx.end()) {
        return i->second;
    }
    return -1;
};

const QRgb& color::getColorByClrIndex(int nIdx, color::Palette pal /*= palNormal*/)
{
    define_colors_map();
    if (pal == color::palNormal)
    {
        auto i = g__idx2rgb.find(nIdx);
        if (i != g__idx2rgb.end()) {
            return i->second;
        }

        /*
        switch (nIdx) {
        case 0: return color::Black;
        case 1: return color::Maroon;
        case 2: return color::Green;
        case 3: return color::Olive;
        case 4: return color::Navy;
        case 5: return color::Purple;
        case 6: return color::Teal;
        case 7: return color::Gray;
        case 8: return color::Silver;
        case 9: return color::Red;
        case 10: return color::Lime;
        case 11: return color::Yellow;
        case 12: return color::Blue;
        case 13: return color::Fuchsia;
        case 14: return color::Aqua;
        case 15: return color::White;
        case 16: return color::Gray_1;
        case 17: return color::Yellow_1;
        case 18: return color::SoftYellow;
        };*/
    }
    else if (pal == color::palTrue1)
    {
        switch (nIdx) {
        case 0: return color::Black;
        case 1: return color::True1Maroon;
        case 2: return color::True1Green;
        case 3: return color::True1Olive;
        case 4: return color::True1Navy;
        case 5: return color::True1Purple;
        case 6: return color::True1Teal;
        case 7: return color::True1Gray;
        case 8: return color::True1Silver;
        case 9: return color::True1Red;
        case 10: return color::True1Lime;
        case 11: return color::True1Yellow;
        case 12: return color::True1Blue;
        case 13: return color::True1Fuchsia;
        case 14: return color::True1Aqua;
        case 15: return color::White;
        };
    }
    /*
    else if(pal == color::palLight)
    {
    switch(nIdx){
    case 0: return color::Black;
    case 1: return color::LightMaroon;
    case 2: return color::LightGreen;
    case 3: return color::LightOlive;
    case 4: return color::LightNavy;
    case 5: return color::LightPurple;
    case 6: return color::LightTeal;
    case 7: return color::LightGray;
    case 8: return color::LightSilver;
    case 9: return color::LightRed;
    case 10: return color::LightLime;
    case 11: return color::LightYellow;
    case 12: return color::LightBlue;
    case 13: return color::LightFuchsia;
    case 14: return color::LightAqua;
    case 15: return color::White;
    };
    }*/

    static QRgb rv = qRgb(0, 0, 0);
    return rv;
}

QString color::getColorStringByClrIndex(int nIdx, color::Palette pal /*= color::palNormal*/)
{
    if (pal == color::palNormal)
    {
        switch (nIdx) {
        case 2: return "134, 247, 182";
        case 5: return "227, 185, 255";
        case 7: return "128, 128, 128";
        case 9: return "243, 180, 186";
        case 11:return "255, 255, 192";
        case 12:return "157, 189, 255";
        case 15:return "255, 255, 255";     
        }
    }
    else if (pal == color::palTrue1)
    {
        switch (nIdx) {
        case 5: return "160, 17, 255";
        case 7: return "64, 64, 64";
        case 9: return "221, 44, 60";
        case 12:return "6, 88, 254";
        case 2: return "12, 174, 81";
        case 15:return "255, 255, 255";
        }
    }
    return "";
};

QString color::getColorArStr(int nIdx)
{
    switch (nIdx) {
    case 0: return "Black";
    case 1: return "Maroon";
    case 2: return "Green";
    case 3: return "Olive";
    case 4: return "Navy";
    case 5: return "Purple";
    case 6: return "Teal";
    case 7: return "Gray";
    case 8: return "Silver";
    case 9: return "Red";
    case 10: return "Lime";
    case 11: return "Yellow";
    case 12: return "Blue";
    case 13: return "Fuchsia";
    case 14: return "Aqua";
    case 15: return "White";
    }
    return "Undefined";
}

std::set<int> color::getAvailableColorIndexes(color::Palette pal)
{
    std::set<int> rv;
    switch (pal)
    {
    case color::palNormal: {
        for (int i = 0; i < 19; i++) {
            rv.insert(i);
        }
    }break;
    case color::palTrue1:
    {
        rv.insert(5);
        rv.insert(9);
        rv.insert(12);
        rv.insert(2);
    }break;
    default:break;
    }
    return rv;
};

static std::vector<QRgb> hash_cl_lst;
void color::prepareColors() 
{
#ifdef _DEBUG
    ASSERT(hash_cl_lst.empty(), "run prepare colors only once");
#endif
    /*
    hash_cl_lst.push_back(True1Maroon);
    hash_cl_lst.push_back(True1Green);
    hash_cl_lst.push_back(True1Olive);
    hash_cl_lst.push_back(True1Navy);
    hash_cl_lst.push_back(True1Purple);
    hash_cl_lst.push_back(True1Red);
    */
    
    hash_cl_lst.push_back(Maroon);
    hash_cl_lst.push_back(Green);
    hash_cl_lst.push_back(Olive);
    hash_cl_lst.push_back(Navy);
    hash_cl_lst.push_back(Purple);
    hash_cl_lst.push_back(Red);

    /*
    hash_cl_lst.push_back(LightMaroon);
    hash_cl_lst.push_back(LightGreen);
    hash_cl_lst.push_back(LightOlive);
    hash_cl_lst.push_back(LightNavy);
    hash_cl_lst.push_back(LightPurple);
    hash_cl_lst.push_back(LightRed);
    */
};


QRgb color::getColorByHashIndex(char idx)
{
    QRgb rv = Qt::black;
    if (idx >= 0 && idx < static_cast<char>(hash_cl_lst.size())) {
        rv = hash_cl_lst[idx];
    }
    return rv;
};

static char calcColorHashIndex1(QString str) 
{
    char h = 0;
    for (auto& s : str) {
        h ^= s.toLatin1();
    }
    h %= hash_cl_lst.size();
    return h;
};


std::pair<char, char> color::calcColorHashIndex(QString emailAddress)
{
    std::pair<char, char> rv = { 0,0 };
    if (!emailAddress.isEmpty()) {
        rv.first = calcColorHashIndex1(emailAddress);
        rv.second = '-';
        for (auto& c : emailAddress) {
            if (c.isLetter()) {
                rv.second = c.toUpper().toLatin1();
                break;
            }
        }
    }
    return rv;
};
