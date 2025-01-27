#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <functional>
#include <algorithm>
#include <cctype>

using namespace std;

namespace lutest
{
    template<class T>
    struct LuListNode {
        T val;
        LuListNode *next;
        LuListNode(const T& x) : val(x), next(nullptr) {}   
    };

    template<class T>
    void pushLuNode(LuListNode<T>** h, const T& v){
        LuListNode<T>* n = new LuListNode<T>(v);
        n->next = *h;
        *h = n;
    }

    template<class T>
    void printLuList(LuListNode<T>* h){
        while(h){
            std::cout << h->val << " ";
            h = h->next;
        }
        std::cout << endl;
    }

    template<class T>
    std::vector<T> commonLuListNodes(LuListNode<T>* h1, LuListNode<T>* _h2){
        std::vector<T> rv;
        while(h1){
            auto h2 = _h2;
            while(h2){
                if(h1->val == h2->val){
                    rv.push_back(h1->val);
                }
                h2 = h2->next;
            }
            h1 = h1->next;
        }
        return rv;
    };

    template<class T>
    class Vector
    {
    public:
        using iterator = T*;
        //typedef T* iterator;

        Vector();
        Vector(size_t size);
        Vector(size_t size, const T& initial);
        Vector(const Vector<T>& v);
        Vector<T>& operator =(const Vector<T>& v);
        ~Vector();

        size_t capacity()const{return m_capacity;}
        size_t size()const{return m_size;}
        void   reserve(size_t capacity);
        bool   empty()const{return (m_size == 0);}
        void   push_back(const T& v);
        

        iterator begin();
        iterator end();
        T & front();
        T & back();

    private:
        size_t m_size, m_capacity;
        T* m_buffer{nullptr};
    };

    template<class T>
    Vector<T>::Vector()
    {
        m_size = m_capacity = 0;
        m_buffer = nullptr;
    }

    template<class T>
    Vector<T>::Vector(size_t size)
    {
        m_size = m_capacity = 0;
        m_buffer = new T[m_capacity];
    };

    template<class T>
    Vector<T>::Vector(size_t size, const T& initial)
    {
        m_size = m_capacity = size;
        m_buffer = new T[m_capacity];
        for(int i = 0; i < size; i++){
            m_buffer[i] = initial;
        }
    };

    template<class T>
    Vector<T>::~Vector(){
        delete []m_buffer;
    }

    template<class T>
    Vector<T>& Vector<T>::operator =(const Vector<T>& v)
    {
        if(this == &v)
            return *this;

        delete []m_buffer;
        m_size = v.m_size;
        m_capacity = v.m_capacity;
        m_buffer = new T[m_capacity];
        for(int i = 0; i < m_size; i++){
            m_buffer = v.m_buffer[i];
        }
        return *this;
    };

    template<class T>
    typename Vector<T>::iterator Vector<T>::begin()
    {
        return m_buffer;
    };
    
    template<class T>
    typename Vector<T>::iterator Vector<T>::end()
    {
        return m_buffer + size();
    };

    template<class T>
    T & Vector<T>::front()
    {
        return m_buffer[0];
    };

    template<class T>
    T & Vector<T>::back()
    {
        return m_buffer[m_size - 1];
    };

    template<class T>
    void Vector<T>::push_back(const T& v)
    {
        if(m_size >= m_capacity){
            reserve(m_capacity + 5);
        }
        m_buffer[m_size++] = v;
    };

    template<class T>
    void Vector<T>::reserve(size_t capacity)
    {
        if(!m_buffer){
            m_size = m_capacity = 0;
        }

        T* new_buff = new T[capacity];
        size_t sz1 = (capacity < m_size) ? capacity : m_size;
        copy(new_buff, m_buffer, sizeof(m_buffer));
        m_capacity = capacity;
        delete []m_buffer;
        m_buffer = new_buff;
    };
    ///------ end vector ------------
    
    class basket
    {
    public:
        enum class InstrumentType
        {
            none,
            spot,
            future,
            forward,
            fxswap
        };

        class BSecurity
        {
        public:
        protected:
            BSecurity(){};
            BSecurity(const std::string& symbol):m_symbol(symbol){};
            InstrumentType instrumentTyep()const{return m_inst_type;}
        protected:
            std::string m_symbol;
            InstrumentType m_inst_type{InstrumentType::none};
        };//BSecurity

        class CSpot: public BSecurity{
        public:
            
        protected:
            double m_price;
        };

        class CFuture: public CSpot
        {
        public:
            
        private:
            double     m_future_price;
            time_t     m_future_time;
        };
    };



    class miA
    {
    public:
        virtual ~miA(){};
        //    protected:
        //     int m_valA;
    };

    class miB: public virtual miA{};
    class miC: public virtual miA{};

    //    class miD: public miB, public miC{};
    class miD:public miA{};
};


static void lu_nodes_test()
{
    std::cout << "<< run_lutest" << std::endl;

    lutest::LuListNode<int>* h1 = nullptr;
    lutest::LuListNode<int>* h2 = nullptr;

    for(int i = 0; i < 100; i++){
        lutest::pushLuNode(&h1, i);
    }

    for(int i = 20; i < 120; i++){
        lutest::pushLuNode(&h2, i);
    }

    std::cout << "------- list 1" << std::endl;
    printLuList(h1);
    std::cout << "------- list 2" << std::endl;
    printLuList(h2);

    std::cout << "------- common nodes" << std::endl;
    auto lst = commonLuListNodes(h1, h2);
    for(auto& v : lst){
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

void run_lutest()
{
    lutest::miD d;
    std::cout << "sizeof(d)=" << sizeof(d) 
              << " sizeof(int)=" << sizeof(int) 
              << " sizeof(void*)=" << sizeof(void *) << std::endl;
    //  lutest::Vector<int> arr;
}
