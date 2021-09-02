module bmitestbmi
  use test_model
  use bmif_2_0
  use, intrinsic :: iso_c_binding, only: c_ptr, c_loc, c_f_pointer
  implicit none

  type, extends (bmi) :: bmi_test_bmi
     private
     type (test_bmi_model) :: model
   contains
     procedure :: get_component_name => test_component_name
!      procedure :: get_input_item_count => test_input_item_count
!      procedure :: get_output_item_count => test_output_item_count
!      procedure :: get_input_var_names => test_input_var_names
!      procedure :: get_output_var_names => test_output_var_names
!      procedure :: initialize => test_initialize
!      procedure :: finalize => test_finalize
!      procedure :: get_start_time => test_start_time
!      procedure :: get_end_time => test_end_time
!      procedure :: get_current_time => test_current_time
!      procedure :: get_time_step => test_time_step
!      procedure :: get_time_units => test_time_units
!      procedure :: update => test_update
!      procedure :: update_until => test_update_until
!      procedure :: get_var_grid => test_var_grid
!      procedure :: get_grid_type => test_grid_type
!      procedure :: get_grid_rank => test_grid_rank
!      procedure :: get_grid_shape => test_grid_shape
!      procedure :: get_grid_size => test_grid_size
!      procedure :: get_grid_spacing => test_grid_spacing
!      procedure :: get_grid_origin => test_grid_origin
!      procedure :: get_grid_x => test_grid_x
!      procedure :: get_grid_y => test_grid_y
!      procedure :: get_grid_z => test_grid_z
!      procedure :: get_grid_node_count => test_grid_node_count
!      procedure :: get_grid_edge_count => test_grid_edge_count
!      procedure :: get_grid_face_count => test_grid_face_count
!      procedure :: get_grid_edge_nodes => test_grid_edge_nodes
!      procedure :: get_grid_face_edges => test_grid_face_edges
!      procedure :: get_grid_face_nodes => test_grid_face_nodes
!      procedure :: get_grid_nodes_per_face => test_grid_nodes_per_face
!      procedure :: get_var_type => test_var_type
!      procedure :: get_var_units => test_var_units
!      procedure :: get_var_itemsize => test_var_itemsize
!      procedure :: get_var_nbytes => test_var_nbytes
!      procedure :: get_var_location => test_var_location
!      procedure :: get_value_int => test_get_int
!      procedure :: get_value_float => test_get_float
!      procedure :: get_value_double => test_get_double
!      generic :: get_value => &
!           get_value_int, &
!           get_value_float, &
!           get_value_double
! !      procedure :: get_value_ptr_int => test_get_ptr_int
! !      procedure :: get_value_ptr_float => test_get_ptr_float
! !      procedure :: get_value_ptr_double => test_get_ptr_double
! !      generic :: get_value_ptr => &
! !           get_value_ptr_int, &
! !           get_value_ptr_float, &
! !           get_value_ptr_double
! !      procedure :: get_value_at_indices_int => test_get_at_indices_int
! !      procedure :: get_value_at_indices_float => test_get_at_indices_float
! !      procedure :: get_value_at_indices_double => test_get_at_indices_double
! !      generic :: get_value_at_indices => &
! !           get_value_at_indices_int, &
! !           get_value_at_indices_float, &
! !           get_value_at_indices_double
!      procedure :: set_value_int => test_set_int
!      procedure :: set_value_float => test_set_float
!      procedure :: set_value_double => test_set_double
!      generic :: set_value => &
!           set_value_int, &
!           set_value_float, &
!           set_value_double
! !      procedure :: set_value_at_indices_int => test_set_at_indices_int
! !      procedure :: set_value_at_indices_float => test_set_at_indices_float
! !      procedure :: set_value_at_indices_double => test_set_at_indices_double
! !      generic :: set_value_at_indices => &
! !           set_value_at_indices_int, &
! !           set_value_at_indices_float, &
! !           set_value_at_indices_double
! !      procedure :: print_model_info
  end type bmi_test_bmi

  private
  public :: bmi_test_bmi

  character (len=BMI_MAX_COMPONENT_NAME), target :: &
       component_name = "Testing BMI Fortran Model"

contains

  ! Get the name of the model.
  function test_component_name(this, name) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), pointer, intent(out) :: name
    integer :: bmi_status

    name => component_name
    bmi_status = BMI_SUCCESS
  end function test_component_name

end module bmitestbmi
