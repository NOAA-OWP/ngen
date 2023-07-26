#ifndef NGEN_MDFRAME_HANDLERS_CSV_HPP
#define NGEN_MDFRAME_HANDLERS_CSV_HPP

#include <fstream>

#include "mdframe.hpp"
#include "cartesian.hpp"

namespace ngen {

inline void mdframe::to_csv(const std::string& path, bool header) const
{
    std::ofstream output(path);
    if (!output)
        throw std::runtime_error("failed to open file " + path);
    
    std::string header_line = "";

    std::vector<variable> variable_subset;
    // will be at most # of vars
    variable_subset.reserve(this->m_variables.size());

    // Get subset of variables that span over the given basis
    size_type max_rank = 0;
    for (const auto& pair : this->m_variables) {
        const auto& dims = pair.second.dimensions();
        variable_subset.push_back(pair.second);

        size_type rank = pair.second.rank();
        if (rank > max_rank)
            max_rank = rank;
        
        if (header)
            header_line += pair.first + ",";
    }

    // Calculate total number of rows across all subdimensions (not including header)
    size_type rows = 1;
    std::vector<size_type> shape;
    shape.reserve(this->m_dimensions.size());
    for (const auto& dim : this->m_dimensions) {
        rows += dim.size();
        shape.push_back(dim.size());
    }

    if (header && header_line != "") {
        header_line.pop_back();
        output << header_line << std::endl;
    }

    // Create an index between the variable's dimensions, and the dimensions
    // of the mdframe, such that when a variable needs to be passed an index,
    // the dimensions will be ordered correctly.
    //
    // i.e. { value(x, t, y) : {2, 1, 3} }
    std::unordered_map<std::string, std::vector<size_type>> vd_index;
    for (auto var : variable_subset) {
        vd_index[var.name()] = std::vector<size_type>{};
        std::vector<size_type>& variable_index = vd_index[var.name()];

        variable_index.reserve(var.rank());
        for (const auto& dim : var.dimensions()) {
            variable_index.push_back(
                // Push back the index within the mdframe's store
                // for this dimension that var spans over
                std::distance(
                    this->m_dimensions.begin(),
                    this->m_dimensions.find(dim)
                )
            );
        }
    }

    detail::visitors::to_string_visitor visitor{};
    std::string output_line = "";
    std::vector<size_type> buffer(max_rank, 0);
    std::vector<std::vector<size_type>> indices;
    indices.reserve(rows);

    ngen::cartesian_indices(shape, buffer, 0, indices);
    
    std::vector<size_type> index_buffer(max_rank);
    for (const auto& index : indices) {
        for (auto var : variable_subset) {
            boost::span<size_type> index_view{ index_buffer.data(), var.rank() };
            boost::span<size_type> vd_view{vd_index[var.name()]};
            for (size_type i = 0; i < var.rank(); i++)
                index_view[i] = index.at(vd_view[i]);

            decltype(auto) value = var.at(index_view);
            output_line += value.apply_visitor(visitor) + ",";
        }

        output_line.pop_back();
        output << output_line << std::endl;
        output_line.clear();
    }
}

} // namespace ngen

#endif // NGEN_MDFRAME_HANDLERS_CSV_HPP
