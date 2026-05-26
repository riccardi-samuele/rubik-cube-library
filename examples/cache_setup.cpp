#include "rubik/pruning_tables.hpp"

#include <iostream>

int main()
{
    std::cout << "cache_dir: " << rubik::pruning_tables::cacheDirectory() << "\n";

    const auto& cornerOrientation = rubik::pruning_tables::cornerOrientation();
    const auto& edgeOrientation = rubik::pruning_tables::edgeOrientation();
    const auto& sliceEdges = rubik::pruning_tables::sliceEdges();

    std::cout << "corner_orientation_states: " << cornerOrientation.size() << "\n";
    std::cout << "edge_orientation_states: " << edgeOrientation.size() << "\n";
    std::cout << "slice_edge_states: " << sliceEdges.size() << "\n";
    return 0;
}
