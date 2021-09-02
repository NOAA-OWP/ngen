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

  function register_bmi(this) result(bmi_status) bind(C, name="register_bmi")
   use, intrinsic:: iso_c_binding, only: c_ptr, c_loc, c_int
   use iso_c_bmif_2_0
   implicit none
   type(c_ptr) :: this ! If not value, then from the C perspective `this` is a void**
   integer(kind=c_int) :: bmi_status
   !Create the momdel instance to use
   !Definitely need to carefully undertand and document the semantics of the save attribute here
   type(bmi_test_bmi), target, save :: bmi_model !need to ensure scope/lifetime, use save attribute
   !Create a simple pointer wrapper
   type(box), pointer :: bmi_box

   !allocate the pointer box
   allocate(bmi_box)
   !allocate(bmi_box%ptr, source=bmi_model)
   !associate the wrapper pointer the created model instance
   bmi_box%ptr => bmi_model
   !Return the pointer to box
   this = c_loc(bmi_box)
   bmi_status = BMI_SUCCESS
 end function register_bmi
end module bmitestbmi
