#ifndef NGEN_MDFRAME_DEFINITION_HPP
#define NGEN_MDFRAME_DEFINITION_HPP

#include <unordered_set>
#include <unordered_map>

#include "dimension.hpp"
#include "visitors.hpp"
#include "variable.hpp"

namespace ngen {

/**
 * A multi-dimensional, tagged data frame.
 *
 * This structure (somewhat) mimics the conceptual format
 * of NetCDF. In particular, we can think of an mdframe
 * as a multimap between dimension tuples to multi-dimensional arrays.
 * In other words, it is similar to an R data.frame or pandas dataframe
 * where the column types are multi-dimensional arrays.
 *
 * Heterogenous data types are handled via boost::variant types
 * over mdarray types.
 *
 * @section representation Frame Representation
 * Consider the dimensions: x, y, z; and
 * the variables:
 *   - v1<float>(x)
 *   - v2<double>(x, y)
 *   - v3<int>(x, y, z)
 *
 * Then, it follows that the corresponding mdframe (spanned over x):
 *
 *   x |  v1 |      v2 |      v3
 * --- | --- | ------- | -------
 *   0 | f_0 | [...]_0 | [[...]]_0
 *   1 | f_1 | [...]_1 | [[...]]_1
 * ... | ... |  ...    |   ...
 *   n | f_n | [...]_n | [[...]]_n
 *
 * where:
 * - v1: Vector of floats,   rank 1 -> [...]
 * - v2: Matrix of doubles,  rank 2 -> [[...]]
 * - v3: Tensor of integers, rank 3 -> [[[...]]]
 *
 * Alternatively, we can project down to a 2D representation by
 * unpacking the dimensions, such that:
 *
 *      dimensions ||                variables
 * =============== || ========================
 *   x |   y |   z ||   v1 |     v2 |       v3
 * --- | --- | --- || ---- | ------ | --------
 *   n |   m |   p || s[n] | d[n,m] | i[n,m,p]
 *
 */
class mdframe {
  public:
    using dimension     = detail::dimension;
    using dimension_set = std::unordered_set<dimension, dimension::hash>;
    using variable      = detail::variable<int, float, double>;
    using variable_map  = std::unordered_map<std::string, variable>;
    using size_type     = variable::size_type;

    /**
     * The variable value types this frame can support.
     * These are stored as a compile-time type list to
     * derive further type aliases.
     * @see detail::variable
     */
    using types = variable::types;

    using mdarray_variant = variable::mdarray_variant;

    // ------------------------------------------------------------------------
    // Dimension Member Functions
    // ------------------------------------------------------------------------
    
  private:
    /**
     * Get an iterator to a dimension, if it exists in the set
     * 
     * @param name Name of the dimension
     * @return dimension_set::const_iterator or dimension_set::end() if not found
     */
    dimension_set::const_iterator find_dimension(const std::string& name) const noexcept
    {
        return this->m_dimensions.find(dimension(name));
    }

    /**
     * Get an iterator to a variable, if it exists in the set
     * 
     * @param name Name of the variable
     * @return variable_set::const_iterator  or variable_set::end() if not found
     */
    variable_map::const_iterator find_variable(const std::string& name) const noexcept
    {
        return this->m_variables.find(name);
    }

  public:
    /**
     * Return a dimension if it exists. Otherwise, returns boost::none.
     * 
     * @param name Name of the dimension
     * @return boost::optional<detail::dimension> 
     */
    boost::optional<detail::dimension> get_dimension(const std::string& name) const noexcept
    {
        decltype(auto) pos = this->find_dimension(name);

        boost::optional<detail::dimension> result;
        if (pos != this->m_dimensions.end()) {
            result = *pos;
        }

        return result;
    }

    /**
     * Check if a dimension exists.
     * 
     * @param name Name of the dimension
     * @return true if the dimension exists in this mdframe.
     * @return false if the dimension does **not** exist in this mdframe.
     */
    bool has_dimension(const std::string& name) const noexcept
    {
        return this->find_dimension(name) != this->m_dimensions.end();
    }

    /**
     * Add a dimension with *unlimited* size to this mdframe.
     * 
     * @param name Name of the dimension.
     * @return mdframe& 
     */
    mdframe& add_dimension(const std::string& name)
    {
        this->m_dimensions.emplace(name);
        return *this;
    }

    /**
     * Add a dimension with a specified size to this mdframe,
     * or update an existing dimension.
     * 
     * @param name Name of the dimension.
     * @param size Size of the dimension.
     * @return mdframe& 
     */
    mdframe& add_dimension(const std::string& name, std::size_t size)
    {
        auto stat = this->m_dimensions.emplace(name, size);

        if (!stat.second) {
            stat.first->m_size = size;
        }

        return *this;
    }

    // ------------------------------------------------------------------------
    // Variable Member Functions
    // ------------------------------------------------------------------------

    /**
     * Return reference to a mdarray representing a variable, if it exists.
     * Otherwise, returns boost::none.
     * 
     * @param name Name of the variable.
     * @return boost::optional<variable>
     */
    variable& get_variable(const std::string& name) noexcept
    {
        return this->m_variables[name];
    }

    /**
     * Return reference to a mdarray representing a variable, if it exists.
     * Otherwise, returns boost::none.
     * 
     * @param name Name of the variable.
     * @return boost::optional<variable>
     */
    variable& operator[](const std::string& name) noexcept
    {
        return this->get_variable(name);
    }

    /**
     * Check if a variable exists.
     * 
     * @param name Name of the variable.
     * @return true if the variable exists in this mdframe.
     * @return false if the variables does **not** exist in this mdframe.
     */
    bool has_variable(const std::string& name) const noexcept
    {
       return this->find_variable(name) != this->m_variables.end();
    }

    /**
     * Add a (vector) variable definition to this mdframe.
     *
     * @note This member function is constrained by @c{Args}
     *       being constructible to a std::initializer_list of std::string
     *       (i.e. a list of std::string).
     * 
     * @tparam Arg list of std::string representing dimension names.
     * @param name Name of the variable.
     * @param dimensions Names of the dimensions this variable spans.
     * @return mdframe&
     */
    template<typename T, types::enable_if_supports<T, bool> = true>
    mdframe& add_variable(const std::string& name, std::initializer_list<std::string> dimensions)
    {
        std::vector<dimension> references;
        references.reserve(dimensions.size());

        for (const auto& d : dimensions) {
            auto dopt = this->get_dimension(d);
            if (dopt == boost::none) {
                throw std::runtime_error("not a dimension");
            }
            references.push_back(dopt.get());
        }

        this->m_variables[name] = variable::make<T>(name, references);
    
        return *this;
    }


    // ------------------------------------------------------------------------
    // Output Member Functions
    // ------------------------------------------------------------------------

    /**
     * Write this mdframe to a CSV file.
     *
     * The outputted CSV is a projected (or normalized)
     * table spanning all possible dimensions, similar
     * to the description of tidy data by Hadley Wickham (2014):
     * https://www.jstatsoft.org/article/view/v059i10
     *
     * @note The dimensions are not outputted to the CSV,
     *       since they only form the basis for the mdframe's
     *       dimensions, and do not inherently store any information.
     * 
     * @param path File path to output CSV
     * @param basis Dimension to span CSV over, defaults to "time"
     * @param header Should the CSV header be included?
     */
    void to_csv(const std::string& path, bool header = true) const;

    /**
     * Write this mdframe to a NetCDF file.
     * 
     * @param path File path to the output NetCDF
     */
    void to_netcdf(const std::string& path) const;

  private:
    dimension_set m_dimensions;
    variable_map  m_variables;
};

} // namespace ngen

#endif // NGEN_MDFRAME_DEFINITION_HPP
