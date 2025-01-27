#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <iterator>
#include <chrono>
#include <functional>

namespace nytest
{
    class CSmallestIn95
    {
    public:
        static void run_test();

        void push_back(double v){
            auto it = std::lower_bound(m_data.begin(), m_data.end(), v);
            m_data.insert(it, v);
        };

        void emplace_input(const std::vector<double>&& input){
            m_data = std::move(input);
            std::sort(m_data.begin(), m_data.end());
        }

        std::pair<int,double> get_95()const{
            auto idx = 95 * m_data.size() / 100;
            std::pair<int,double> rv = {idx, m_data[idx]};
            return rv;
        }
        size_t size()const{return m_data.size();}
        std::pair<double,double> value_range()const{
            std::pair<double,double> rv;
            if(!m_data.empty()){
                rv.first = m_data[0];
                rv.second = m_data[m_data.size()-1];
            }
            return rv;
        }
    protected:
        std::vector<double> m_data;
    };
};

void nytest::CSmallestIn95::run_test()
{
    std::cout << "-------- running SmallestIn95 Test ------" << std::endl;

    CSmallestIn95 t;
    const int input_size = 128000000;
    std::default_random_engine re;
    std::vector<double> random_input;
    auto start = std::chrono::high_resolution_clock::now();
    random_input.reserve(input_size);
    std::uniform_real_distribution<double> unif(1,10000);
    for(int i = 0; i < input_size; i++){
        auto val = unif(re);
        random_input.push_back(val);
    }

    auto finish = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    std::cout << "prepared [" << random_input.size() << "] random numbers: " << elapsed << " (ms)." << std::endl;

    /// set input
    start = std::chrono::high_resolution_clock::now();
    t.emplace_input(std::move(random_input));
    finish = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    std::cout << "input created as [" << random_input.size() << "] sorted numbers: " << elapsed << " (ms)." << std::endl;

    std::function<void(double)> add_number_and_calc95 = [&t](double v)
        {
            auto start = std::chrono::high_resolution_clock::now();
            t.push_back(v);
            auto n = t.get_95();    
            auto finish = std::chrono::high_resolution_clock::now();
            auto r = t.value_range();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
            std::cout << "1 new number added: [" << t.size() << "]: " << elapsed << " (ms)." << std::endl;
            std::cout << "number95: " << n.second << " at pos " << n.first << " in range [" << r.first << "," << r.second << "] " << std::endl;
            std::cout << std::endl;    
        };

    add_number_and_calc95(212.3);

    while(true){
        std::cout << "enter number:";
        std::string s;
        double val = 0.0;
        std::cin >> s;
        try{
            val = std::stod(s);
            add_number_and_calc95(val);
        }
        catch(std::invalid_argument&)
            {
                return;
            }
    }
    /*
    ---------- console output ----------
    Intel(R) Xeon(R) CPU           X5680  @ 3.33GHz


    prepared [128000000] random numbers: 2529 (ms).
    input created as [128000000] sorted numbers: 17442 (ms).
    1 new number added: [128000001]: 971 (ms).
    number95: 9499.79 at pos 121600000 in range [1.00012,10000]
    */
};

namespace nytest
{
    class Tiler 
    {
        using TILE_STRIPE = std::vector<int>;
    public:
        static void run_test();

        size_t buildGalery() 
        {
            std::vector<int> tstripe;

            TILE_STRIPE st2 = { 2 }; 
            TILE_STRIPE st3 = { 3 };        

            st2.reserve(32);
            st3.reserve(32);

            auto start = std::chrono::system_clock::now();
            makeStripes(st2);
            makeStripes(st3);
            size_t res = 2 * m_stripes2 * m_stripes3;
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::system_clock::now() - start;
            m_galery_build_time = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            return res;
        };
    protected:
        void makeStripes(TILE_STRIPE st) 
        {
#define ADD_TAILS(N);if (st[0] == 2)m_stripes2 +=N;else m_stripes3 += N;

            int sum = std::accumulate(st.begin(), st.end(), 0);
            
            /* slower version 
            if(sum < 30){
                auto st3 = st;
                st.push_back(2);
                makeStripes(st);

                st3.push_back(3);
                makeStripes(st3);                
            }
            else if(sum == 30){
                ADD_TAILS(1);
            }
            */             
            
            switch (sum) {
            case 21:{ /// ..333 ..2223 ..2232 ..2322 ..3222
                ADD_TAILS(5);
            }break;
            case 22:{ /// ..332 or ..323 or ..233 or ..2222
                ADD_TAILS(4);
            }break;                
            case 23:{ /// ..223 or ..232 or ..322
                ADD_TAILS(3);
            }break;
            case 24: /// ..33 or ..222
            case 25:{ /// ..23 or ..32
                ADD_TAILS(2);
            }break;                
            default: {
                auto st3 = st;
                st.push_back(2);
                makeStripes(st);

                st3.push_back(3);
                makeStripes(st3);           
            }
            }            

#undef ADD_TAILS
        };
    protected:
        size_t          m_stripes2{ 0 }, 
                        m_stripes3{ 0 };
        size_t          m_galery_build_time { 0 };
    };
}

void nytest::Tiler::run_test() 
{
    Tiler t;
    auto res = t.buildGalery();
    std::cout << "-------- running Tile Test ------" << std::endl;
    std::cout << "galery-size:" << res << " ("  << t.m_stripes2 << ", " << t.m_stripes3 << ")" << std::endl;
    std::cout << "galery-build-time:" << t.m_galery_build_time << " microsec." << std::endl;
    /*
    ---------- console output ----------
    Intel(R) Xeon(R) CPU           X5680  @ 3.33GHz

    galery-size:1764192 (1081, 816)
    galery-build-time:141 microsec.
    */
};


void run_nytest()
{
    nytest::Tiler::run_test();
    nytest::CSmallestIn95::run_test();
}
