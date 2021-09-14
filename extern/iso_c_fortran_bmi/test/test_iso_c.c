#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

extern register_bmi(void *);
extern initialize(void *, char *);
extern update(void *);
extern update_until(void*, double *);
extern finalize(void*);
extern get_component_name(void *, char *);
extern get_input_item_count(void*, int *);
extern get_output_item_count(void*, int *);
extern get_input_var_names(void*, char**);
extern get_output_var_names(void*, char**);
extern get_var_grid(void*, char*, int*);
extern get_var_type(void*, char*, char*);
extern get_var_units(void*, char*, char*);
extern get_var_itemsize(void*, char*, int*);
extern get_var_nbytes(void*, char*, int*);
extern get_var_location(void*, char*, char*);
extern get_current_time(void*, double *);
extern get_start_time(void*, double *);
extern get_end_time(void*, double *);
extern get_time_units(void*, char *);
extern get_time_step(void*, double *);
extern get_value_int(void*, char*, int*);
extern get_value_float(void*, char*, float*);
extern get_value_double(void*, char*, double*);
extern set_value_int(void*, char*, int*);
extern set_value_float(void*, char*, float*);
extern set_value_double(void*, char*, double*);
extern get_grid_rank(void*, int*, int*);
extern get_grid_size(void*, int*, int*);
extern get_grid_type(void*, int*, char*);
extern get_grid_shape(void*, int*, int*);
extern get_grid_spacing(void*, int*, double*);
extern get_grid_origin(void*, int*, double*);
extern get_grid_x(void*, int*, double*);
extern get_grid_y(void*, int*, double*);
extern get_grid_z(void*, int*, double*);
extern get_grid_node_count(void*, int*, int*);
extern get_grid_edge_count(void*, int*, int*);
extern get_grid_face_count(void*, int*, int*);
extern get_grid_edge_nodes(void*, int*, int*);
extern get_grid_face_edges(void*, int*, int*);
extern get_grid_face_nodes(void*, int*, int*);
extern get_grid_nodes_per_face(void*, int*, int*);

int BMI_SUCCESS = 0;
int BMI_MAX_VAR_NAME = 2048;

void check_status(int* status, char* name){
    printf("%s: ", name);
    if(*status == BMI_SUCCESS){
        printf("SUCCESS\n");
    }
    else{
        printf("FAILURE\n");
        exit(*status);
    }
}

int main(int argc, char** argv)
{
    void** bmi_handle;
    void** bmi_handle2;
    int status = -1;
    int i = 0;

    char name[2048];
    char type[2048];
    char location[2048];

    status = register_bmi(&bmi_handle);
    check_status(&status, "register");
    //register_bmi(&bmi_handle2);

    //Note, the name MUST be namelist.input or the reader breaks
    char init_file[2048] = "namelist.input";
    //char init_file[2048] = "test.ini";
    status = initialize(&bmi_handle, init_file);
    check_status(&status, "initialize");
    
    update(&bmi_handle);
    check_status(&status, "update");
    double t = 3600.0;
    status = update_until(&bmi_handle, &t);
    check_status(&status, "update_until");

    status = get_component_name(&bmi_handle, name);
    check_status(&status, "get_component_name");
    printf("Name: %s\n", name);

    //Check input item handling
    int count = -1;
    status = get_input_item_count(&bmi_handle, &count);
    check_status(&status, "get_input_item_count");
    printf("input_item_count: %ld\n", count);
    char** names;
    names = malloc(sizeof(char*)*count);
 
    for(i = 0; i < count; i++){
        names[i] = malloc(sizeof(char)*BMI_MAX_VAR_NAME);
        //names[i] = "Hello World\0";
        sprintf(names[i], "Hello World %d", i);
    }
    status = get_input_var_names(&bmi_handle, names);
    check_status(&status, "get_input_var_names");
    for(i = 0; i < count; i++){
        printf("%s\n", names[i]);
    }
    for(i = 0; i < count; i++)
    {
        free(names[i]);
    }
    free(names);

    //Check output item handling
    count = -1;
    status = get_output_item_count(&bmi_handle, &count);
    check_status(&status, "get_output_item_count");
    printf("output_item_count: %ld\n", count);

    names = malloc(sizeof(char*)*count);

    for(i = 0; i < count; i++){
        names[i] = malloc(sizeof(char)*BMI_MAX_VAR_NAME);
        //names[i] = "Hello World\0";
        sprintf(names[i], "Hello World %d", i);
    }
    status = get_output_var_names(&bmi_handle, names);
    check_status(&status, "get_output_var_names");
    for(i = 0; i < count; i++){
        printf("%s\n", names[i]);
    }
    for(i = 0; i < count; i++)
    {
        free(names[i]);
    }
    free(names);

    int grid = -1;
    status = get_var_grid(&bmi_handle, "QINSUR", &grid);
    printf("get_var_grid for QINSUR: %ld\n", grid);
    check_status(&status, "get_var_grid");

    status = get_var_type(&bmi_handle, "QINSUR", type);
    printf("get_var_type for QINSUR: %s\n", type);
    check_status(&status, "get_var_type");

    status = get_var_units(&bmi_handle, "QINSUR", type);
    printf("get_var_units for QINSUR: %s\n", type);
    check_status(&status, "get_var_units");

    int size = -1;
    status = get_var_itemsize(&bmi_handle, "QINSUR", &size);
    printf("get_var_itemsize for QINSUR: %d\n", size);
    check_status(&status, "get_var_itemsize");

    size = -1;
    status = get_var_nbytes(&bmi_handle, "QINSUR", &size);
    printf("get_var_nbytes for QINSUR: %d\n", size);
    check_status(&status, "get_var_nbytes");

    status = get_var_location(&bmi_handle, "QINSUR", location);
    printf("get_var_location for QINSUR: %s\n", location);
    check_status(&status, "get_var_location");

    double time = -1.0;
    status = get_current_time(&bmi_handle, &time);
    printf("get_current_time: %f\n", time);
    check_status(&status, "get_current_time");

    time = -1.0;
    status = get_start_time(&bmi_handle, &time);
    printf("get_start_time: %f\n", time);
    check_status(&status, "get_start_time");

    time = -1.0;
    status = get_end_time(&bmi_handle, &time);
    printf("get_end_time: %f\n", time);
    check_status(&status, "get_end_time");

    char time_units[2048];
    status = get_time_units(&bmi_handle, time_units);
    printf("get_time_units: %s\n", time_units);
    check_status(&status, "get_time_units");

    time = -1.0;
    status = get_time_step(&bmi_handle, &time);
    printf("get_time_step: %f\n", time);
    check_status(&status, "get_time_step");

    int value = -2;
    status = get_value_int(&bmi_handle, "none", &value);
    printf("get_value_int: %d\n", value);
    // check_status(&status, "get_value_int");

    float value_f = -2.0;
    status = get_value_float(&bmi_handle, "QINSUR", &value_f);
    printf("get_value_float QINSUR: %f\n", value_f);
    check_status(&status, "get_value_float");

    double value_d = -2.0;
    status = get_value_double(&bmi_handle, "QINSUR", &value_d);
    printf("get_value_double QINSUR: %f\n", value_d);
    // check_status(&status, "get_value_double");

    value = 2;
    status = set_value_int(&bmi_handle, "none", &value);
    printf("set_value_int: %d\n", value);
    // check_status(&status, "set_value_int");

    value_f = 2.0;
    status = set_value_float(&bmi_handle, "QINSUR", &value_f);
    printf("set_value_float QINSUR: %f\n", value_f);
    check_status(&status, "set_value_float");
    value_f = -2.0;
    status = get_value_float(&bmi_handle, "QINSUR", &value_f);
    printf("get_value_float QINSUR: %f\n", value_f);
    check_status(&status, "get_value_float");

    value_d = -2.0;
    status = set_value_double(&bmi_handle, "QINSUR", &value_d);
    printf("set_value_double QINSUR: %f\n", value_d);
    // check_status(&status, "set_value_double");

    int rank = -2;
    grid = 0;
    status = get_grid_rank(&bmi_handle, &grid, &rank);
    printf("get_grid_rank %d: %d\n", grid, rank);
    check_status(&status, "get_grid_rank");

    int grid_size = -2;
    grid = 0;
    status = get_grid_size(&bmi_handle, &grid, &grid_size);
    printf("get_grid_size %d: %d\n", grid, grid_size);
    check_status(&status, "get_grid_size");

    char grid_type[2048];
    grid = 0;
    status = get_grid_type(&bmi_handle, &grid, grid_type);
    printf("get_grid_type %d: %s\n", grid, grid_type);
    check_status(&status, "get_grid_type");

    int grid_shape = -2;
    grid = 0;
    status = get_grid_shape(&bmi_handle, &grid, &grid_shape);
    printf("get_grid_shape %d: %d\n", grid, grid_shape);
    // check_status(&status, "get_grid_shape");

    double grid_spacing = -2.0;
    grid = 0;
    status = get_grid_spacing(&bmi_handle, &grid, &grid_spacing);
    printf("get_grid_spacing %d: %lf\n", grid, grid_spacing);
    // check_status(&status, "get_grid_spacing");
    
    double grid_origin = -2;
    grid = 0;
    status = get_grid_origin(&bmi_handle, &grid, &grid_origin);
    printf("get_grid_origin %d: %lf\n", grid, grid_origin);
    // check_status(&status, "get_grid_origin");

    double xs[1] = {-2.0};
    grid = 0;
    status = get_grid_x(&bmi_handle, &grid, &xs);
    printf("get_grid_xs %d: %lf\n", grid, xs[0]);
    check_status(&status, "get_grid_x");

    double ys[1] = {-2.0};
    grid = 0;
    status = get_grid_y(&bmi_handle, &grid, &ys);
    printf("get_grid_ys %d: %lf\n", grid, ys[0]);
    check_status(&status, "get_grid_y");


    double zs[1] = {-2.0};
    grid = 0;
    status = get_grid_z(&bmi_handle, &grid, &zs);
    printf("get_grid_zs %d: %lf\n", grid, zs[0]);
    check_status(&status, "get_grid_z");

    int node_count = -2;
    grid = 0;
    status = get_grid_node_count(&bmi_handle, &grid, &node_count);
    printf("get_grid_node_count %d: %d\n", grid, node_count);
    check_status(&status, "get_grid_node_count");

    int edge_count = -2;
    grid = 0;
    status = get_grid_edge_count(&bmi_handle, &grid, &edge_count);
    printf("get_grid_edge_count %d: %d\n", grid, edge_count);
    // check_status(&status, "get_grid_edge_count");

    int face_count = -2;
    grid = 0;
    status = get_grid_face_count(&bmi_handle, &grid, &face_count);
    printf("get_grid_face_count %d: %d\n", grid, face_count);
    // check_status(&status, "get_grid_face_count");

    int edge_nodes[1] = {-2.0};
    grid = 0;
    status = get_grid_edge_nodes(&bmi_handle, &grid, &edge_nodes);
    printf("get_grid_edge_nodes %d: %d\n", grid, edge_nodes[0]);
    // check_status(&status, "get_grid_edge_nodes");

    int face_edges[1] = {-2.0};
    grid = 0;
    status = get_grid_face_edges(&bmi_handle, &grid, &face_edges);
    printf("get_grid_face_edges %d: %d\n", grid, face_edges[0]);
    // check_status(&status, "get_grid_face_edges");

    int face_nodes[1] = {-2.0};
    grid = 0;
    status = get_grid_face_nodes(&bmi_handle, &grid, &face_nodes);
    printf("get_grid_face_nodes %d: %d\n", grid, face_nodes[0]);
    // check_status(&status, "get_grid_face_nodes");

    int nodes_per_face[1] = {-2.0};
    grid = 0;
    status = get_grid_nodes_per_face(&bmi_handle, &grid, &nodes_per_face);
    printf("get_grid_nodes_per_face %d: %d\n", grid, nodes_per_face[0]);
    // check_status(&status, "get_grid_nodes_per_face");

    status = finalize(&bmi_handle);
    check_status(&status, "finalize");


}