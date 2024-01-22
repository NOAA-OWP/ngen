#ifndef NGEN_GEOPACKAGE_SQLITE_H
#define NGEN_GEOPACKAGE_SQLITE_H

#include <memory>
#include <vector>

#include <sqlite3.h>

#include <boost/core/span.hpp>
#include "traits.hpp"

namespace ngen {
namespace sqlite {

struct sqlite_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;

    sqlite_error(const std::string& origin_func, int code, const std::string& extra = "");
};

class database
{
    //! Deleter used to provide smart pointer support for sqlite3 structs.
    struct deleter
    {
        void operator()(sqlite3* db)
        {
            sqlite3_close_v2(db);
        }

        void operator()(sqlite3_stmt* stmt)
        {
            sqlite3_finalize(stmt);
        }
    };

    using sqlite_t = std::unique_ptr<sqlite3, deleter>;
    using stmt_t   = std::unique_ptr<sqlite3_stmt, deleter>;

  public:
    struct iterator
    {
        iterator() = delete;

        explicit iterator(stmt_t&& stmt);

        //! Check if a row iterator is finished
        //! @return true if next() returned SQLITE_DONE
        //! @return false if there is more rows available
        bool done() const noexcept;

        //! Step into the next row of a SQLite query
        //! 
        //! If the query is finished, next() acts idempotently,
        //! but will change done() to return true.
        //! @return sqlite_iter& returns itself
        iterator& next();

        //! Restart an iteration to its initial state.
        //! next() must be called after calling this.
        //! 
        //! @return sqlite_iter& returns itself
        iterator& restart();

        //! Get the current row index for the iterator
        //! @return int the current row index,
        //!         or -1 if next() hasn't been called
        int current_row() const noexcept;

        //! Get the number of columns within this iterator
        //! @return int number of columns in query
        int num_columns() const noexcept;

        //! Return the column index for a named column 
        //! 
        //! @param name column name to search for
        //! @return int index of given column name, or -1 if not found.
        int find(const std::string& name) const noexcept;

        //! Get a span of column names
        //! @return column names as a span of strings
        const boost::span<const std::string> columns() const noexcept;

        //! Get a span of column types
        //!
        //! See https://www.sqlite.org/datatype3.html for type affinities.
        //! The integers are the affinity for data types.
        //! @return column types as a span of ints
        const boost::span<const int> types() const noexcept;


        //! Get a column value from a row iterator by index
        //! 
        //! @tparam T Type to parse value as, i.e. int
        //! @param col Column index to parse
        //! @return T value at column `col`
        template<typename Tp>
        Tp get(int col) const;

        //! Get a column value from a row iterator by name
        //! 
        //! @tparam T Type to parse value as, i.e. int
        //! @param col Column name
        //! @return T value at the named column
        template<typename Tp>
        Tp get(const std::string& col) const
        { return get<Tp>(find(col)); }

      private:

        sqlite3_stmt* ptr_() const noexcept;
        void          handle_get_index_(int col) const;

        stmt_t stmt_;
    
        int step_ = -1;
        int done_ = false;
        int ncol_ = 0;

        std::vector<std::string> names_;
        std::vector<int>         types_;
    }; // struct iterator

    // Prevent default construction and copy
    // construction/assignment.
    database()                           = delete;
    database(const database&)            = delete;
    database& operator=(const database&) = delete;

    // Allow move operations
    database(database&&)            = default;
    database& operator=(database&&) = default;
    ~database()                     = default;

    //! Construct a new sqlite object from a path to database
    //! @param path File path to sqlite3 database
    explicit database(const std::string& path);


    //! Return the originating sqlite3 database pointer
    //! @return sqlite3*
    sqlite3* connection() const noexcept;

    //! Check if SQLite database contains a given table
    //! @param table name of table
    //! @return true if table does exist
    //! @return false if table does not exist
    bool contains(const std::string& table);

    //! Query the SQLite Database and get the result
    //! @param statement String query with parameters
    //! @param binds text parameters to bind to statement
    //! @return SQLite row iterator
    iterator query(const std::string& statement, const boost::span<const std::string> binds = {});

    //! Query the SQLite Database with a bound statement and get the result
    //! @param statement String query with parameters
    //! @param params parameters to bind to statement
    //! @return SQLite row iterator
    template<
        typename... Ts,
        std::enable_if_t<
            ngen::traits::all_is_convertible<std::string, Ts...>::value,
            bool
        > = true
    >
    iterator query(const std::string& statement, const Ts&... params)
    {
        std::array<std::string, sizeof...(params)> binds = { params... };
        return query(statement, binds);
    }

  private:
    sqlite_t conn_ = nullptr;
};

} // namespace sqlite
} // namespace ngen

#endif // NGEN_GEOPACKAGE_SQLITE_H
