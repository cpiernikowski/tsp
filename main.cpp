#include <cstdlib>
#define MY_DEBUG
#include "complete_weighted_graph.hpp"
#include <iostream> // debug?f

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* const* argv) {
    CWGraph cwg(4);
    cwg.add_edge({1, 2}, 12, 0.4);
    cwg.add_edge({1, 3}, 4, 0.7);
    cwg.add_edge({1, 4}, 14, 0.2);
    cwg.add_edge({2, 3}, 8, 0.8);
    cwg.add_edge({2, 4}, 7, 0.6);
    cwg.add_edge({3, 4}, 5, 0.4);

    std::cout << (std::string)cwg;
    Ant a(cwg);
    a.travel_whole_journey();


    

    std::cout << std::endl;
    for (const auto& e : a.get_path()) {
        std::cout << e << " ";
    }

    std::cout << std::endl << a.path_weight();
    
    
    



    return EXIT_SUCCESS;
}