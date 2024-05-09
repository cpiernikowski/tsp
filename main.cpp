#include <cstdlib>
#define MY_DEBUG
#include "complete_weighted_graph.hpp"
#include <iostream> // debug?

int main(int argc, const char* const* argv) {
    CWGraph cwg(4);
    cwg.add_edge({1, 2}, 12, 0.4);
    cwg.add_edge({1, 3}, 4, 0.7);
    cwg.add_edge({1, 4}, 14, 0.2);
    cwg.add_edge({2, 3}, 8, 0.8);
    cwg.add_edge({2, 4}, 7, 0.6);
    cwg.add_edge({3, 4}, 5, 0.4);

    std::cout << (std::string)cwg;
    Ant a(cwg);
    a.step();
    std::cout << a.current();
    a.step();
    std::cout << a.current();
    a.step();
    std::cout << a.current();

    std::cout << std::endl;
    for (const auto& e : a.get_path()) {
        std::cout << e << " ";
    }
    
    
    



    return EXIT_SUCCESS;
}