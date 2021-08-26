#ifndef NGEN_BMI_FORTRAN_COMMON_H
#define NGEN_BMI_FORTRAN_COMMON_H

/**
 * The extern free functions from the Nextgen common Fortran static library for integrating with Fortran BMI modules.
 * 
 * Each declaration corresponds to a free proxy function in the common integration library.  Every proxy function in 
 * turn corresponds to some BMI function, which in the Fortran module is expected to be implemented as a procedure on 
 * some Fortran object.  A proxy function will accept an opaque handle to the Fortran object, along with the other
 * expected arguments to the corresponding BMI function procedure.  The proxy then calls the appropriate procedure on
 * the passed Fortran BMI object.
 */

/** Initialize, run, finalize (IRF) */
extern int initialize(void *fortran_bmi_handle, const char *config_file);

extern int update(void *fortran_bmi_handle);

extern int update_until(void *fortran_bmi_handle, double then);

extern int finalize(void *fortran_bmi_handle);

/* Exchange items */
extern int get_component_name(void *fortran_bmi_handle, char *name);

extern int get_input_item_count(void *fortran_bmi_handle, int *count);

extern int get_output_item_count(void *fortran_bmi_handle, int *count);

extern int get_input_var_names(void *fortran_bmi_handle, char **names);

extern int get_output_var_names(void *fortran_bmi_handle, char **names);

/* Variable information */
extern int get_var_grid(void *fortran_bmi_handle, const char *name, int *grid);

extern int get_var_type(void *fortran_bmi_handle, const char *name, char *type);

extern int get_var_units(void *fortran_bmi_handle, const char *name, char *units);

extern int get_var_itemsize(void *fortran_bmi_handle, const char *name, int *size);

extern int get_var_nbytes(void *fortran_bmi_handle, const char *name, int *nbytes);

extern int get_var_location(void *fortran_bmi_handle, const char *name, char *location);

/* Time information */
extern int get_current_time(void *fortran_bmi_handle, double *time);

extern int get_start_time(void *fortran_bmi_handle, double *time);

extern int get_end_time(void *fortran_bmi_handle, double *time);

extern int get_time_units(void *fortran_bmi_handle, char *units);

extern int get_time_step(void *fortran_bmi_handle, double *time_step);

/* Getters */
extern int get_value(void *fortran_bmi_handle, const char *name, void *dest);

extern int get_value_ptr(void *fortran_bmi_handle, const char *name, void **dest_ptr);

extern int get_value_at_indices(void *fortran_bmi_handle, const char *name, void *dest, int *inds, int count);

/* Setters */
extern int set_value(void *fortran_bmi_handle, const char *name, void *src);

extern int set_value_at_indices(void *fortran_bmi_handle, const char *name, int *inds, int count, void *src);

/* Grid information */
extern int get_grid_rank(void *fortran_bmi_handle, int grid, int *rank);

extern int get_grid_size(void *fortran_bmi_handle, int grid, int *size);

extern int get_grid_type(void *fortran_bmi_handle, int grid, char *type);

/* Uniform rectilinear */
extern int get_grid_shape(void *fortran_bmi_handle, int grid, int *shape);

extern int get_grid_spacing(void *fortran_bmi_handle, int grid, double *spacing);

extern int get_grid_origin(void *fortran_bmi_handle, int grid, double *origin);

/* Non-uniform rectilinear, curvilinear */
extern int get_grid_x(void *fortran_bmi_handle, int grid, double *x);

extern int get_grid_y(void *fortran_bmi_handle, int grid, double *y);

extern int get_grid_z(void *fortran_bmi_handle, int grid, double *z);

/* Unstructured */
extern int get_grid_node_count(void *fortran_bmi_handle, int grid, int *count);

extern int get_grid_edge_count(void *fortran_bmi_handle, int grid, int *count);

extern int get_grid_face_count(void *fortran_bmi_handle, int grid, int *count);

extern int get_grid_edge_nodes(void *fortran_bmi_handle, int grid, int *edge_nodes);

extern int get_grid_face_edges(void *fortran_bmi_handle, int grid, int *face_edges);

extern int get_grid_face_nodes(void *fortran_bmi_handle, int grid, int *face_nodes);

extern int get_grid_nodes_per_face(void *fortran_bmi_handle, int grid, int *nodes_per_face);

#endif //NGEN_BMI_FORTRAN_COMMON_H
