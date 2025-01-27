#pragma once

#include <functional>
#include "anfolder.h"


namespace ard
{
    template<class RandomIt>
    void killSilently(const RandomIt& first, const RandomIt& last)
    {
        const RandomIt it = first;
        while(it != last){
            (*it)->killSilently(false);
        }
    }


    template<class IT>
    THREAD_SET reduce2ethreads(IT begin, IT end)
    {
        THREAD_SET tset;
        IT i = begin;
        //for (auto f : lst) {
        while(i != end){
            auto e = dynamic_cast<ard::email*>(*i);
            if (e) {
                auto t = e->parent();
                if (t) {
                    tset.insert(t);
                }
            }
            else {
                auto t = dynamic_cast<ard::ethread*>(*i);
                if (t) {
                    tset.insert(t);
                }
            }
            i++;
        }
        return tset;
    };    
};

