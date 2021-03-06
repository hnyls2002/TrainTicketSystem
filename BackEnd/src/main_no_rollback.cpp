#include "System_for_FrontEnd.hpp"
#include <iostream>

int main() {
/*
    system("rm index*");
    system("rm record*");
    system("rm roll*");
    freopen("2.in", "r", stdin);
    freopen("my.out", "w", stdout);
*/
    std::ios::sync_with_stdio(false);
    clock_t sts = clock();
    hnyls2002::System sys;
    //sys.GetSize();
    //sys.GetCachedSize();
    //sys.GetRollBackSize();
    std::string str;
    while (getline(std::cin, str)) {
        bool res = sys.Opt(str);
        std::cout << "__end__" << std::endl;
        if (res)break;
    }
    clock_t end = clock();
    std::cerr << "Running Time : Using  " << (double) (end - sts) / CLOCKS_PER_SEC << " seconds " << std::endl;
    return 0;
}
