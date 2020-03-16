#ifndef GRIDPOLYGON_H
#define GRIDPOLYGON_H

#include <boost/geometry/geometries/box.hpp>

namespace bg = boost::geometry;

typedef bg::model::d2::point_xy<double> point_t;
typedef bg::model::polygon<point_t> polygon_type;

#include <boost/array.hpp>
#include <boost/multi_array.hpp>

typedef boost::multi_array<polygon_type, 2> polygon_grid_type;
typedef boost::multi_array<bool, 2> bool_grid_type;
typedef boost::multi_array<double, 2> weight_grid_type;

polygon_grid_type create_ns_cells(point_t max_corner, point_t min_corner, double x_width, double y_width)
{
    // get the values for the first grid cell
    double y1 = max_corner.y();
    double y2 = max_corner.y() - y_width;


    // calculate the the size of the grid
    int x_size = static_cast<int>(std::ceil(max_corner.x() - min_corner.x()));
    int y_size = static_cast<int>(std::ceil(max_corner.y() - min_corner.y()));

    // create the boost multi_array
    polygon_grid_type cells{boost::extents[x_size][y_size]};

    // set the inital row cell index position
    int r = 0;

    // move through y dimension
    while (y1 > min_corner.y())
    {
        // move through x dimension
        double x1 = min_corner.x();
        double x2 = min_corner.x() + x_width;
        int c = 0;

        while (x1 < max_corner.x())
        {
            polygon_type p;
            //assign points to polygon
            bg::append(p, point_t(x1,y1));
            bg::append(p, point_t(x2,y1));
            bg::append(p, point_t(x2,y2));
            bg::append(p, point_t(x1,y2));
            bg::append(p, point_t(x1,y1));

            // assign polygon to grid and move column index
            cells[r][c++] = p;

            // advance x
            x1 = x2;
            x2 += x_width;
        }

        // advance y
        y1 = y2;
        y2 -= y_width;
        ++r;
    }

    return cells;
}

bool_grid_type generate_grid_mask_for_polygon(const polygon_type& poly, polygon_grid_type& grid)
{
    const polygon_grid_type::size_type* shape = grid.shape();

    bool_grid_type mask{boost::extents[shape[0]][shape[1]]};

    for(unsigned long i = 0; i < shape[0]; ++i)
    {
        for(unsigned long j = 0; j < shape[1]; ++j )
        {
            mask[i][j] = (bg::within(grid[i][j],poly)) ? true: false;
        }
    }

    return mask;
}

polygon_grid_type create_grid_for_polygon(const polygon_type& p, double x_width, double y_width )
{
    bg::model::box<point_t> box;

    bg::envelope(p, box);

    return create_ns_cells(box.min_corner(),box.max_corner(),x_width,y_width);
}

#endif // GRIDPOLYGON_H_INCLUDED
