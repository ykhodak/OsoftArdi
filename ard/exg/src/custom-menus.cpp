#include "custom-menus.h"
#include "anfolder.h"

using namespace ard::menu;

std::pair<MCmd, uint32_t> ard::menu::unpackMcmd(QAction* a)
{
    UMcmdData d;
    d.d = a->data().toULongLong();
    std::pair<MCmd, uint32_t> rv;
    rv.first = static_cast<MCmd>(d.c);
    rv.second = d.v;
    return rv;
}

uint64_t ard::menu::packMcmd(MCmd c, uint32_t v)
{
    UMcmdData d;
    d.c = static_cast<uint32_t>(c);
    d.v = v;
    return d.d;
}

int ard::menu::buildTopicsMenu(QMenu& m, TOPICS_LIST& topics2add, TOPICS_SET& topics2skip) 
{
    int rv = 0;
    QAction* a = nullptr;

#ifdef ARD_BIG
#define MAX_LIST_SIZE 16
#else
#define MAX_LIST_SIZE 4
#endif

    int idx = 0;
    for (auto& i : topics2add) {
        auto k = topics2skip.find(i);
        if (k == topics2skip.end()) {
            ADD_MCMD2(i->title(), MCmd::select_one_folder, i->id());
            idx++;
        }
        if (idx > MAX_LIST_SIZE)
            break;
        rv++;
    }
    m.addSeparator();
#undef MAX_LIST_SIZE

    return rv;
};

