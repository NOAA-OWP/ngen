#include "mdframe.hpp"
#include <fstream>
#include <unordered_set>

namespace io {
namespace visitors {

struct mdvalue_to_string : public boost::static_visitor<std::string>
{
    template<typename T>
    std::string operator()(T& v) { return std::to_string(v); }
};

}
}

void generate_indices(
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
        generate_indices(shape, index, dimension + 1, output);
    }
}

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

    dimension_set subdim = this->m_dimensions;
    // will be at most # of dims
    // subdim.reserve(this->m_dimensions.size());

    // Get subset of variables that span over the given basis
    size_type max_rank = 0;
    for (const auto& pair : this->m_variables) {
        const auto& dims = pair.second.dimensions();
        // if (std::find(dims.begin(), dims.end(), basis) != dims.end()) {
            subvar.push_back(pair.second);
            // for (const auto& dim : dims) {
            //     subdim.insert(this->get_dimension(dim).get());
            // }
            
            size_type rank = pair.second.rank();
            if (rank > max_rank) {
                max_rank = rank;
            }
            

            if (header) {
                header_line += pair.first + ",";
            }
        // }
    }

    // Calculate total number of rows across all subdimensions (not including header)
    size_type rows = 1;
    std::vector<size_type> shape;
    shape.reserve(subdim.size());
    for (const auto& dim : subdim) {
        rows *= dim.size();
        shape.push_back(dim.size());
    }

    // Finish creating header and output if it's wanted
    if (header && header_line != "") {
        // remove last comma
        header_line.pop_back();
        output << header_line << std::endl;
    }

    // Iterate over rows, and output lines
    std::string output_line = "";
    std::vector<std::vector<size_type>> indices;
    indices.reserve(rows);
    std::vector<size_type> index(max_rank, 0);
    io::visitors::mdvalue_to_string visitor;
    generate_indices(shape, index, 0, indices);
    for (const auto& index : indices) {
        for (auto var : subvar) {
            const std::vector<size_type> tmp(index.begin(), index.begin() + var.rank());
            auto value = var.at(tmp);
            output_line += value.apply_visitor(visitor) + ",";
        }

        output_line.pop_back();
        output << output_line << std::endl;
        output_line.clear();
    }
}