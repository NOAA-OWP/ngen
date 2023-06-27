#include "mdframe.hpp"
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace io {
namespace visitors {

struct mdvalue_to_string : public boost::static_visitor<std::string>
{
    template<typename T>
    std::string operator()(T& v) { return std::to_string(v); }
};

}
}

void io::detail::cartesian_indices(
    const std::vector<std::size_t>&        shape,
    std::vector<std::size_t>&              index,
    std::size_t                            dimension,
    std::vector<std::vector<std::size_t>>& output
)
{
    if (dimension == shape.size()) {
        output.push_back(index);
        return;
    }

    const auto& size = shape[dimension];
    for (std::size_t i = 0; i < size; i++) {
        index[dimension] = i;
        io::detail::cartesian_indices(shape, index, dimension + 1, output);
    }
}

// TODO: The logic of this function is kind of messy, but it works... might be worth a refactor later on.
void io::mdframe::to_csv(const std::string& path, bool header) const
{
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to open file " + path);
    }

    std::string header_line = "";

    std::vector<variable> subvar;
    // will be at most # of vars
    subvar.reserve(this->m_variables.size());

    // Get subset of variables that span over the given basis
    size_type max_rank = 0;
    for (const auto& pair : this->m_variables) {
        const auto& dims = pair.second.dimensions();
            subvar.push_back(pair.second);
            
            size_type rank = pair.second.rank();
            if (rank > max_rank) {
                max_rank = rank;
            }
            

            if (header) {
                header_line += pair.first + ",";
            }
    }

    // Calculate total number of rows across all subdimensions (not including header)
    size_type rows = 1;
    std::vector<size_type> shape;
    shape.reserve(this->m_dimensions.size());
    for (const auto& dim : this->m_dimensions) {
        rows *= dim.size();
        shape.push_back(dim.size());
    }

    // Finish creating header and output if it's wanted
    if (header && header_line != "") {
        // remove last comma
        header_line.pop_back();
        output << header_line << std::endl;
    }

    // Create an index between the variable's dimensions, and the dimensions
    // of the mdframe, such that when a variable needs to be passed an index,
    // the dimensions will be ordered correctly.
    //
    // i.e. { value(x, t, y) : {2, 1, 3} }
    std::unordered_map<std::string, std::vector<size_type>> vd_index;
    for (auto var : subvar) {
        vd_index[var.name()] = std::vector<size_type>{};
        vd_index[var.name()].reserve(var.rank());
        for (const auto& dim : var.dimensions()) {
            vd_index[var.name()].push_back(
                std::distance(
                    this->m_dimensions.begin(),
                    this->m_dimensions.find(dim)
                )
            );
        }
    }

    io::visitors::mdvalue_to_string visitor;
    std::string output_line = "";
    std::vector<size_type> buffer(max_rank, 0);

    // contains n-size tuples of md-indices in the nested vectors
    std::vector<std::vector<size_type>> indices;
    indices.reserve(rows);
    io::detail::cartesian_indices(shape, buffer, 0, indices);
    for (const auto& index : indices) {
        for (auto var : subvar) {
            // TODO: this won't work for variables with rank < the variable with the largest rank
            std::vector<size_type> tmp;
            tmp.reserve(var.rank());
            for (const auto& i : vd_index[var.name()]) {
                tmp.push_back(index.at(i));
            }

            auto value = var.at(tmp);
            output_line += value.apply_visitor(visitor) + ",";
        }

        output_line.pop_back();
        output << output_line << std::endl;
        output_line.clear();
    }
}
