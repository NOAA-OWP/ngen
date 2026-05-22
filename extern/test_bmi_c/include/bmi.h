// Taken from: https://github.com/csdms/bmi-c/blob/master/bmi.h (from commit e6f9f8a)
//
// Copyright (c) 2014 Community Surface Dynamics Modeling System

/*
Made available under the following license:

The MIT License (MIT)

Copyright (c) 2014 Community Surface Dynamics Modeling System

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#ifndef BMI_H
#define BMI_H

#if defined(__cplusplus)
extern "C" {
#endif

#define BMI_SUCCESS (0)
#define BMI_FAILURE (1)

#define BMI_MAX_UNITS_NAME (2048)
#define BMI_MAX_TYPE_NAME (2048)
#define BMI_MAX_COMPONENT_NAME (2048)
#define BMI_MAX_VAR_NAME (2048)
#define BMI_MAX_LOCATION_NAME (2048) //OWP Custom

typedef struct Bmi {
    void *data;

    /* Initialize, run, finalize (IRF) */
    int (*initialize)(struct Bmi *self, const char *config_file);

    int (*update)(struct Bmi *self);

    int (*update_until)(struct Bmi *self, double then);

    int (*finalize)(struct Bmi *self);

    /* Exchange items */
    int (*get_component_name)(struct Bmi *self, char *name);

    int (*get_input_item_count)(struct Bmi *self, int *count);

    int (*get_output_item_count)(struct Bmi *self, int *count);

    int (*get_input_var_names)(struct Bmi *self, char **names);

    int (*get_output_var_names)(struct Bmi *self, char **names);

    /* Variable information */
    int (*get_var_grid)(struct Bmi *self, const char *name, int *grid);

    int (*get_var_type)(struct Bmi *self, const char *name, char *type);

    int (*get_var_units)(struct Bmi *self, const char *name, char *units);

    int (*get_var_itemsize)(struct Bmi *self, const char *name, int *size);

    int (*get_var_nbytes)(struct Bmi *self, const char *name, int *nbytes);

    int (*get_var_location)(struct Bmi *self, const char *name, char *location);

    /* Time information */
    int (*get_current_time)(struct Bmi *self, double *time);

    int (*get_start_time)(struct Bmi *self, double *time);

    int (*get_end_time)(struct Bmi *self, double *time);

    int (*get_time_units)(struct Bmi *self, char *units);

    int (*get_time_step)(struct Bmi *self, double *time_step);

    /* Getters */
    int (*get_value)(struct Bmi *self, const char *name, void *dest);

    int (*get_value_ptr)(struct Bmi *self, const char *name, void **dest_ptr);

    int (*get_value_at_indices)(struct Bmi *self, const char *name, void *dest, int *inds, int count);

    /* Setters */
    int (*set_value)(struct Bmi *self, const char *name, void *src);

    int (*set_value_at_indices)(struct Bmi *self, const char *name, int *inds, int count, void *src);

    /* Grid information */
    int (*get_grid_rank)(struct Bmi *self, int grid, int *rank);

    int (*get_grid_size)(struct Bmi *self, int grid, int *size);

    int (*get_grid_type)(struct Bmi *self, int grid, char *type);

    /* Uniform rectilinear */
    int (*get_grid_shape)(struct Bmi *self, int grid, int *shape);

    int (*get_grid_spacing)(struct Bmi *self, int grid, double *spacing);

    int (*get_grid_origin)(struct Bmi *self, int grid, double *origin);

    /* Non-uniform rectilinear, curvilinear */
    int (*get_grid_x)(struct Bmi *self, int grid, double *x);

    int (*get_grid_y)(struct Bmi *self, int grid, double *y);

    int (*get_grid_z)(struct Bmi *self, int grid, double *z);

    /* Unstructured */
    int (*get_grid_node_count)(struct Bmi *self, int grid, int *count);

    int (*get_grid_edge_count)(struct Bmi *self, int grid, int *count);

    int (*get_grid_face_count)(struct Bmi *self, int grid, int *count);

    int (*get_grid_edge_nodes)(struct Bmi *self, int grid, int *edge_nodes);

    int (*get_grid_face_edges)(struct Bmi *self, int grid, int *face_edges);

    int (*get_grid_face_nodes)(struct Bmi *self, int grid, int *face_nodes);

    int (*get_grid_nodes_per_face)(struct Bmi *self, int grid, int *nodes_per_face);
} Bmi;


#if defined(__cplusplus)
}
#endif

#endif
