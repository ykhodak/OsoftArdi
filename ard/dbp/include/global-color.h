#pragma once
#include <set>
#include <QSqlQuery>
#include <QRgb>

namespace color
{
    enum Palette
    {
        palUnknown = -1,
        palNormal,
        palTrue1
    };

    enum class Color 
    {
        Black   = 0,
        Maroon  = 1,
        Green   = 2,
        Olive   = 3,
        Navy    = 4,
        Purple  = 5,
        Teal    = 6,
        Gray    = 7,
        Silver  = 8,
        Red     = 9,
        Lime    = 10,
        Yellow  = 11,
        Blue    = 12,
        Fuchsia = 13,
        Aqua    = 14,
        White   = 15
    };

    const QRgb&     getColorByClrIndex(int nIdx, color::Palette pal = color::palNormal);
    int             getColorIndexByColor(const QRgb& v);
    QString         getColorStringByClrIndex(int nIdx, color::Palette pal = color::palNormal);
    QString         getColorArStr(int nIdx);
    std::set<int>   getAvailableColorIndexes(color::Palette pal);
    QRgb            invert(QRgb c);    
    QRgb            getColorByHashIndex(char idx);
    void            prepareColors();
    std::pair<char, char> calcColorHashIndex(QString emailAddress);

    extern const QRgb LOCATOR_ANI;
    extern const QRgb SELECTED_ITEM_BK;
    extern const QRgb MSELECTED_ITEM_BK;
    extern const QRgb HOT_SELECTED_ITEM_BK;
    extern const QRgb HOVER_ITEM_BK;
    extern const QRgb Black;
    extern const QRgb Maroon;
    extern const QRgb Green;
    extern const QRgb Olive;
    extern const QRgb Navy;
    extern const QRgb Purple;
    extern const QRgb Teal;
    extern const QRgb Gray;
    extern const QRgb Silver;
    extern const QRgb Red;
    extern const QRgb Lime;
    extern const QRgb Yellow;
    extern const QRgb Blue;
    extern const QRgb Fuchsia;
    extern const QRgb Aqua;
    extern const QRgb White;
    extern const QRgb Gray_1;
    extern const QRgb Yellow_1;
    extern const QRgb ReutersChart;


    extern const QRgb True1Maroon;
    extern const QRgb True1Green;
    extern const QRgb True1Olive;
    extern const QRgb True1Navy;
    extern const QRgb True1Purple;
    extern const QRgb True1Teal;
    extern const QRgb True1Gray;
    extern const QRgb True1Silver;
    extern const QRgb True1Red;
    extern const QRgb True1Lime;
    extern const QRgb True1Yellow;
    extern const QRgb True1Blue;
    extern const QRgb True1Fuchsia;
    extern const QRgb True1Aqua;


    extern const QRgb LightMaroon;
    extern const QRgb LightGreen;
    extern const QRgb LightOlive;
    extern const QRgb LightNavy;
    extern const QRgb LightPurple;
    extern const QRgb LightTeal;
    extern const QRgb LightGray;
    extern const QRgb LightSilver;
    extern const QRgb LightRed;
    extern const QRgb LightLime;
    extern const QRgb LightYellow;
    extern const QRgb LightBlue;
    extern const QRgb LightFuchsia;
    extern const QRgb LightAqua;


    extern const QRgb  SoftYellow;
    extern const QRgb  NavySelected;
    extern const QRgb  YellowSelected;
}
