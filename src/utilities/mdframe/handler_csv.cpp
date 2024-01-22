#include <fstream>

#include "mdframe/mdframe.hpp"

#include <boost/core/span.hpp>

namespace ngen {

/**
 * @brief 
 * 
 * @param shape 
 * @param index 
 * @param dimension 
 * @param output 
 */
void cartesian_indices_recurse(
    boost::span<const std::size_t>         shape,
    std::size_t                            current_dimension_index,
    std::vector<std::size_t>&              index_buffer,
    std::vector<std::vector<std::size_t>>& output
)
{
    if (current_dimension_index == shape.size()) {
        output.push_back(index_buffer);
        return;
    }

    for (std::size_t i = 0; i < shape[current_dimension_index]; i++) {
        index_buffer[current_dimension_index] = i;
        cartesian_indices_recurse(shape, current_dimension_index + 1, index_buffer, output);
    }
}

void cartesian_indices(const boost::span<const std::size_t> shape, std::vector<std::vector<std::size_t>>& output)
{
    std::vector<std::size_t> index_buffer(shape.size());
    cartesian_indices_recurse(shape, 0, index_buffer, output);
}

void mdframe::to_csv(const std::string& path, bool header) const
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

    if (variable_subset.empty()) {
        throw std::runtime_error("cannot output CSV with no output variables");
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
    for (const auto& var : variable_subset) {
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
    std::vector<std::vector<size_type>> indices;
    indices.reserve(rows);

    ngen::cartesian_indices(shape, indices);
    
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

