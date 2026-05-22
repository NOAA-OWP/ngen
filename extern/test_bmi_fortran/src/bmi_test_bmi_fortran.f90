module bmitestbmi
  
#ifdef NGEN_ACTIVE
  use bmif_2_0_iso
#else
  use bmif_2_0
#endif

  use test_model
  use bmi_grid
  use, intrinsic :: iso_c_binding, only: c_ptr, c_loc, c_f_pointer
  implicit none
  integer :: DEFAULT_TIME_STEP_SIZE = 3600
  integer :: DEFAULT_TIME_STEP_COUNT = 24

  type, extends (bmi) :: bmi_test_bmi
     private
     type (test_bmi_model) :: model
   contains
     procedure :: get_component_name => test_component_name
     procedure :: get_input_item_count => test_input_item_count
     procedure :: get_output_item_count => test_output_item_count
     procedure :: get_input_var_names => test_input_var_names
     procedure :: get_output_var_names => test_output_var_names
     procedure :: initialize => test_initialize
     procedure :: finalize => test_finalize
     procedure :: get_start_time => test_start_time
     procedure :: get_end_time => test_end_time
     procedure :: get_current_time => test_current_time
     procedure :: get_time_step => test_time_step
     procedure :: get_time_units => test_time_units
     procedure :: update => test_update
     procedure :: update_until => test_update_until
     procedure :: get_var_grid => test_var_grid
     procedure :: get_grid_type => test_grid_type
     procedure :: get_grid_rank => test_grid_rank
     procedure :: get_grid_shape => test_grid_shape
     procedure :: get_grid_size => test_grid_size
     procedure :: get_grid_spacing => test_grid_spacing
     procedure :: get_grid_origin => test_grid_origin
     procedure :: get_grid_x => test_grid_x
     procedure :: get_grid_y => test_grid_y
     procedure :: get_grid_z => test_grid_z
     procedure :: get_grid_node_count => test_grid_node_count
     procedure :: get_grid_edge_count => test_grid_edge_count
     procedure :: get_grid_face_count => test_grid_face_count
     procedure :: get_grid_edge_nodes => test_grid_edge_nodes
     procedure :: get_grid_face_edges => test_grid_face_edges
     procedure :: get_grid_face_nodes => test_grid_face_nodes
     procedure :: get_grid_nodes_per_face => test_grid_nodes_per_face
     procedure :: get_var_type => test_var_type
     procedure :: get_var_units => test_var_units
     procedure :: get_var_itemsize => test_var_itemsize
     procedure :: get_var_nbytes => test_var_nbytes
     procedure :: get_var_location => test_var_location
     procedure :: get_value_int => test_get_int
     procedure :: get_value_float => test_get_float
     procedure :: get_value_double => test_get_double
     generic :: get_value => &
          get_value_int, &
          get_value_float, &
          get_value_double
     procedure :: get_value_ptr_int => test_get_ptr_int
     procedure :: get_value_ptr_float => test_get_ptr_float
     procedure :: get_value_ptr_double => test_get_ptr_double
     generic :: get_value_ptr => &
          get_value_ptr_int, &
          get_value_ptr_float, &
          get_value_ptr_double
     procedure :: get_value_at_indices_int => test_get_at_indices_int
     procedure :: get_value_at_indices_float => test_get_at_indices_float
     procedure :: get_value_at_indices_double => test_get_at_indices_double
     generic :: get_value_at_indices => &
          get_value_at_indices_int, &
          get_value_at_indices_float, &
          get_value_at_indices_double
     procedure :: set_value_int => test_set_int
     procedure :: set_value_float => test_set_float
     procedure :: set_value_double => test_set_double
     generic :: set_value => &
          set_value_int, &
          set_value_float, &
          set_value_double
     procedure :: set_value_at_indices_int => test_set_at_indices_int
     procedure :: set_value_at_indices_float => test_set_at_indices_float
     procedure :: set_value_at_indices_double => test_set_at_indices_double
     generic :: set_value_at_indices => &
          set_value_at_indices_int, &
          set_value_at_indices_float, &
          set_value_at_indices_double
!      procedure :: print_model_info
  end type bmi_test_bmi

  private
  public :: bmi_test_bmi

  character (len=BMI_MAX_COMPONENT_NAME), target :: &
       component_name = "Testing BMI Fortran Model"
  
  type(GridType) :: grids(3)

  character (len=BMI_MAX_VAR_NAME), target :: &
    grid_meta_vars(8) = [character(BMI_MAX_VAR_NAME):: "grid_1_shape", "grid_1_spacing", "grid_1_units", "grid_1_origin", &
                                                       "grid_2_shape", "grid_2_spacing", "grid_2_units", "grid_2_origin"    ]
  character (len=BMI_MAX_VAR_NAME), target :: &
    grid_meta_vars_types(8) = [character(BMI_MAX_VAR_NAME):: "integer", "double precision", "integer", "double precision", & 
                                                             "integer", "double precision", "integer", "double precision" ]
  ! Exchange items

  character (len=BMI_MAX_VAR_NAME), target :: &
    output_items(6) = [character(BMI_MAX_VAR_NAME):: 'OUTPUT_VAR_1', 'OUTPUT_VAR_2', 'OUTPUT_VAR_3', 'GRID_VAR_2', 'GRID_VAR_3', 'GRID_VAR_4']

  character (len=BMI_MAX_VAR_NAME), target :: &
    input_items(4) = [character(BMI_MAX_VAR_NAME):: 'INPUT_VAR_1', 'INPUT_VAR_2', 'INPUT_VAR_3', 'GRID_VAR_1']

  character (len=BMI_MAX_TYPE_NAME) :: &
    output_type(6) = [character(BMI_MAX_TYPE_NAME):: 'double precision', 'real', 'integer', 'real', 'double precision', 'double precision']

  character (len=BMI_MAX_TYPE_NAME) :: &
    input_type(4) = [character(BMI_MAX_TYPE_NAME):: 'double precision', 'real', 'integer', 'double precision']

  integer :: output_grid(6) = [0, 0, 0, 1, 2, 2]
  integer :: input_grid(4) = [0, 0, 0, 1]

  character (len=BMI_MAX_UNITS_NAME) :: &
    output_units(6) = [character(BMI_MAX_UNITS_NAME):: 'm', 'm', 's', 'm', 'm', 'm']

  character (len=BMI_MAX_UNITS_NAME) :: &
    input_units(4) = [character(BMI_MAX_UNITS_NAME):: 'm', 'm', 's', 'm']

  character (len=BMI_MAX_LOCATION_NAME) :: &
    output_location(6) = [character(BMI_MAX_LOCATION_NAME):: 'node', 'node', 'node', 'node', 'node', 'node']

  character (len=BMI_MAX_LOCATION_NAME) :: &
    input_location(4) = [character(BMI_MAX_LOCATION_NAME):: 'node', 'node', 'node', 'node']

contains

subroutine assert(condition, msg)
  ! If condition == .false., it aborts the program.
  !
  ! Arguments
  ! ---------
  !
  logical, intent(in) :: condition
  character(len=*), intent(in), optional :: msg
  !
  ! Example
  ! -------
  !
  ! call assert(a == 5)
  
  if (.not. condition) then
    print *, "Assertion Failed.", msg
    stop 1
  end if
  end subroutine

function read_init_config(model, config_file) result(bmi_status)
  use, intrinsic :: iso_fortran_env, only: stderr => error_unit
  implicit none
  class(test_bmi_model), intent(out) :: model
  character (len=*), intent(in) :: config_file
  integer :: bmi_status
  !namelist inputs
  integer(kind=4) :: epoch_start_time
  integer :: num_time_steps, time_step_size
  double precision :: model_end_time
  !locals
  integer :: rc, fu
  character(len=1000) :: line
  !namelists
  namelist /test/ epoch_start_time, num_time_steps, time_step_size, model_end_time

  !init values
  epoch_start_time = -1
  num_time_steps = 0
  time_step_size = DEFAULT_TIME_STEP_SIZE
  model_end_time = 0

  ! Check whether file exists.
  inquire (file=config_file, iostat=rc)

  if (rc /= 0) then
      write (stderr, '(3a)') 'Error: input file "', trim(config_file), '" does not exist.'
      bmi_status = BMI_FAILURE
      return
  end if
  ! Open and read Namelist file.
  open (action='read', file=trim(config_file), iostat=rc, newunit=fu)
  read (nml=test, iostat=rc, unit=fu)
  if (rc /= 0) then
      backspace(fu)
      read(fu,fmt='(A)') line
      write(stderr,'(A)') &
       'Invalid line in namelist: '//trim(line)
      write (stderr, '(a)') 'Error: invalid Namelist format.'
      bmi_status = BMI_FAILURE
  else
    if (epoch_start_time == -1 ) then
      !epoch_start_time wasn't found in the name list, log the error and return
      write (stderr, *) "Config param 'epoch_start_time' not found in config file"
      bmi_status = BMI_FAILURE
      return
    end if
    !Update the model with all values found in the namelist
    model%epoch_start_time = epoch_start_time
    model%num_time_steps = num_time_steps
    model%time_step_size = time_step_size
    model%model_end_time = model_end_time
    bmi_status = BMI_SUCCESS
  end if
  close (fu)
end function read_init_config

! BMI initializer.
function test_initialize(this, config_file) result (bmi_status)
  class (bmi_test_bmi), intent(out) :: this
  character (len=*), intent(in) :: config_file
  integer :: bmi_status
  !initialize the internal grid meta data structures
  call grids(1)%init(0, 0, scalar, none) !the scalar grid
  call grids(2)%init(1, 2, rectilinear, none) !the 2D grid
  call grids(3)%init(2, 3, rectilinear, none) !the 3D grid

  if (len(config_file) > 0) then
     bmi_status = read_init_config(this%model, config_file)
     this%model%current_model_time = 0.0
     if ( this%model%num_time_steps == 0 .and. this%model%model_end_time == 0) then
        this%model%num_time_steps = DEFAULT_TIME_STEP_COUNT
     end if
     
     call assert ( this%model%model_end_time /= 0 .or. this%model%num_time_steps /= 0, &
                   "Both model_end_time and num_time_steps are 0" )

     if ( this%model%model_end_time == 0) then
        call assert( this%model%num_time_steps /= 0 )
        this%model%model_end_time = this%model%current_model_time + (this%model%num_time_steps * this%model%time_step_size)
     end if

     call assert( this%model%model_end_time /= 0, &
                  "model_end_time 0 after attempting to compute from num_time_steps" )
  
     if ( this%model%model_end_time /= 0 ) then
        this%model%num_time_steps = (this%model%model_end_time - this%model%current_model_time) / this%model%time_step_size
     end if

     bmi_status = BMI_SUCCESS
  else
     bmi_status = BMI_FAILURE
  end if

end function test_initialize

! BMI finalizer.
function test_finalize(this) result (bmi_status)
  class (bmi_test_bmi), intent(inout) :: this
  integer :: bmi_status

  bmi_status = BMI_SUCCESS
end function test_finalize

  ! Get the name of the model.
  function test_component_name(this, name) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), pointer, intent(out) :: name
    integer :: bmi_status

    name => component_name
    bmi_status = BMI_SUCCESS
  end function test_component_name

  ! Model time units.
  function test_time_units(this, units) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(out) :: units
    integer :: bmi_status

    units = "s"
    bmi_status = BMI_SUCCESS
  end function test_time_units

  ! Count the output variables.
  function test_output_item_count(this, count) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(out) :: count
    integer :: bmi_status

    count = size(output_items)
    bmi_status = BMI_SUCCESS
  end function test_output_item_count

  ! List output variables.
  function test_output_var_names(this, names) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (*), pointer, intent(out) :: names(:)
    integer :: bmi_status

    names => output_items
    bmi_status = BMI_SUCCESS
  end function test_output_var_names

  ! Count the input variables.
  function test_input_item_count(this, count) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(out) :: count
    integer :: bmi_status

    count = size(input_items)
    bmi_status = BMI_SUCCESS
  end function test_input_item_count

  ! List input variables.
  function test_input_var_names(this, names) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (*), pointer, intent(out) :: names(:)
    integer :: bmi_status

    names => input_items
    bmi_status = BMI_SUCCESS
  end function test_input_var_names

  ! The data type of the variable, as a string.
  function test_var_type(this, name, type) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    character (len=*), intent(out) :: type
    integer :: bmi_status, i

    !checkout output vars
    do  i = 1, size(output_items)
      if(output_items(i) .eq. trim(name) ) then
        type = output_type(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do

    !checkout input vars
    do  i = 1, size(input_items)
      if(input_items(i) .eq. trim(name) ) then
        type = input_type(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do

    !check grid meta vars
    do i = 1, size(grid_meta_vars)
      if( grid_meta_vars(i) .eq. trim(name) ) then
        type = grid_meta_vars_types(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do
  
    !check any other vars???

    !no matches
    type = "-"
    bmi_status = BMI_FAILURE
  end function test_var_type

  ! The units of the variable, as a string.
  function test_var_units(this, name, units) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    character (len=*), intent(out) :: units
    integer :: bmi_status, i

    !checkout output vars
    do  i = 1, size(output_items)
      if(output_items(i) .eq. trim(name) ) then
        units = output_units(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do

    !checkout input vars
    do  i = 1, size(input_items)
      if(input_items(i) .eq. trim(name) ) then
        units = input_units(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do
  
    !check any other vars???

    !no matches
    units = "-"
    bmi_status = BMI_FAILURE
  end function test_var_units

  ! The units of the variable, as a string.
  function test_var_location(this, name, location) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    character (len=*), intent(out) :: location
    integer :: bmi_status, i

    !checkout output vars
    do  i = 1, size(output_items)
      if(output_items(i) .eq. trim(name) ) then
        location = output_location(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do

    !checkout input vars
    do  i = 1, size(input_items)
      if(input_items(i) .eq. trim(name) ) then
        location = input_location(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do
  
    !check any other vars???

    !no matches
    location = "-"
    bmi_status = BMI_FAILURE
  end function test_var_location

  ! Get the grid id for a particular variable.
  function test_var_grid(this, name, grid) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    integer, intent(out) :: grid
    integer :: bmi_status, i

    !checkout output vars
    do  i = 1, size(output_items)
      if(output_items(i) .eq. trim(name) ) then
        grid = output_grid(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do

    !checkout input vars
    do  i = 1, size(input_items)
      if(input_items(i) .eq. trim(name) ) then
        grid = input_grid(i)
        bmi_status = BMI_SUCCESS
        return
      endif
    end do
  
    !check any other vars???

    !no matches
    grid = -1
    bmi_status = BMI_FAILURE
  end function test_var_grid

  ! The number of dimensions of a grid.
  function test_grid_rank(this, grid, rank) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, intent(out) :: rank
    integer :: bmi_status
    integer :: i
    ! Failure unless we find what we are looking for...
    bmi_status = BMI_FAILURE
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        rank = grids(i)%rank
        bmi_status = BMI_SUCCESS
        return
      end if
    end do

  end function test_grid_rank

  ! The total number of elements in a grid.
  function test_grid_size(this, grid, size) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, intent(out) :: size
    integer :: bmi_status
    
    integer :: i, upper
    block
      ! This is problematic given that BMI uses intrinsic names for dummy arguements
      ! which leads to a shadowing issue.  If this wasn't in the block like this, 
      ! size(grids) would attempt to treat size as an array indexed by grids which cause all kinds
      ! of compiler errors that are not intuitive.  Also, this kind of shadowing prevents the ability
      ! to do some thing like call size on the size variable...e.g. `size(size)` couldn't be done even
      ! with this block method of exlicitly defining size to be the intrinsic
      ! TODO open an issue with CSDMS fortran BMI
      intrinsic :: size
      upper = size(grids)
    end block
    do i = 1, upper
      if ( grids(i)%id .eq. grid ) then
        size = product( grids(i)%shape )
        bmi_status = BMI_SUCCESS
        return
      end if
    end do

  end function test_grid_size

  ! The dimensions of a grid.
  function test_grid_shape(this, grid, shape) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, dimension(:), intent(out) :: shape
    integer :: bmi_status, i

    bmi_status = BMI_FAILURE
    ! FIXME using shape as an arg shadows the intrinsic SHAPE function and makes it unsuable!!!!!!
    ! TODO add to CSDMS issue related
    
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        shape = grids(i)%shape
        bmi_status = BMI_SUCCESS
        return
      end if
    end do
  end function test_grid_shape

  ! The distance between nodes of a grid.
  function test_grid_spacing(this, grid, spacing) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    double precision, dimension(:), intent(out) :: spacing
    integer :: bmi_status, i

    bmi_status = BMI_FAILURE
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        spacing = grids(i)%spacing
        bmi_status = BMI_SUCCESS
        return
      end if
    end do

  end function test_grid_spacing
!
  ! Coordinates of grid origin.
  function test_grid_origin(this, grid, origin) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    double precision, dimension(:), intent(out) :: origin
    integer :: bmi_status, i

    bmi_status = BMI_FAILURE
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        origin = grids(i)%origin
        bmi_status = BMI_SUCCESS
        return
      end if
    end do
  end function test_grid_origin

  ! X-coordinates of grid nodes.
  function test_grid_x(this, grid, x) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    double precision, dimension(:), intent(out) :: x
    integer :: bmi_status, i

    bmi_status = BMI_FAILURE
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        call grids(i)%grid_x(x)
        bmi_status = BMI_SUCCESS
      end if
    end do
  end function test_grid_x

  ! Y-coordinates of grid nodes.
  function test_grid_y(this, grid, y) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    double precision, dimension(:), intent(out) :: y
    integer :: bmi_status, i

    bmi_status = BMI_FAILURE
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        call grids(i)%grid_y(y)
        bmi_status = BMI_SUCCESS
      end if
    end do
  end function test_grid_y

  ! Z-coordinates of grid nodes.
  function test_grid_z(this, grid, z) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    double precision, dimension(:), intent(out) :: z
    integer :: bmi_status, i

    bmi_status = BMI_FAILURE
    do i = 1, size(grids)
      if ( grids(i)%id .eq. grid ) then
        call grids(i)%grid_z(z)
        bmi_status = BMI_SUCCESS
      end if
    end do
  end function test_grid_z

  ! Get the number of nodes in an unstructured grid.
  function test_grid_node_count(this, grid, count) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, intent(out) :: count
    integer :: bmi_status

    select case(grid)
    case(0:1) ! Scalars are single "node"
       count = 1
       bmi_status = BMI_SUCCESS
    case default
       count = -1
       bmi_status = BMI_FAILURE
    end select
  end function test_grid_node_count

  ! Get the number of edges in an unstructured grid.
  function test_grid_edge_count(this, grid, count) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, intent(out) :: count
    integer :: bmi_status

    count = -1
    bmi_status = BMI_FAILURE
  end function test_grid_edge_count

  ! Get the number of faces in an unstructured grid.
  function test_grid_face_count(this, grid, count) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, intent(out) :: count
    integer :: bmi_status

    count = -1
    bmi_status = BMI_FAILURE
  end function test_grid_face_count

    ! Get the edge-node connectivity.
  function test_grid_edge_nodes(this, grid, edge_nodes) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, dimension(:), intent(out) :: edge_nodes
    integer :: bmi_status

    edge_nodes(:) = -1
    bmi_status = BMI_FAILURE
  end function test_grid_edge_nodes

  ! Get the face-edge connectivity.
  function test_grid_face_edges(this, grid, face_edges) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, dimension(:), intent(out) :: face_edges
    integer :: bmi_status

    face_edges(:) = -1
    bmi_status = BMI_FAILURE
  end function test_grid_face_edges

  ! Get the face-node connectivity.
  function test_grid_face_nodes(this, grid, face_nodes) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, dimension(:), intent(out) :: face_nodes
    integer :: bmi_status

    face_nodes(:) = -1
    bmi_status = BMI_FAILURE
  end function test_grid_face_nodes

  ! Get the number of nodes for each face.
  function test_grid_nodes_per_face(this, grid, nodes_per_face) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    integer, dimension(:), intent(out) :: nodes_per_face
    integer :: bmi_status

    nodes_per_face(:) = -1
    bmi_status = BMI_FAILURE
  end function test_grid_nodes_per_face

  ! The type of a variable's grid.
  function test_grid_type(this, grid, type) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    integer, intent(in) :: grid
    character (len=*), intent(out) :: type
    integer :: bmi_status

    select case(grid)
    case(0)
       type = "scalar"
       bmi_status = BMI_SUCCESS
    case default
       type = "-"
       bmi_status = BMI_FAILURE
    end select
  end function test_grid_type

  ! Memory use per array element.
  function test_var_itemsize(this, name, size) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    integer, intent(out) :: size
    integer :: bmi_status

    !TODO think of a better way to do this
    !Use 'sizeof' in gcc & ifort
    select case(name)
    case("grid_1_shape")
      size = sizeof(grids(2)%shape(1)) !FIXME this does NOT work the same as C sizeof...it gives total bytes of the array, not a single element
      bmi_status = BMI_SUCCESS
    case("grid_1_spacing")
      size = sizeof(grids(2)%spacing(1))
      bmi_status = BMI_SUCCESS
    case("grid_1_units")
      size = sizeof(grids(2)%units(1))
      bmi_status = BMI_SUCCESS
    case("grid_1_origin")
      size = sizeof(grids(2)%units(1))
      bmi_status = BMI_SUCCESS
    case("grid_2_shape")
      size = sizeof(grids(3)%shape(1))
      bmi_status = BMI_SUCCESS
    case("grid_2_spacing")
      size = sizeof(grids(3)%spacing(1))
      bmi_status = BMI_SUCCESS
    case("grid_2_units")
      size = sizeof(grids(3)%units(1))
      bmi_status = BMI_SUCCESS
    case("grid_2_origin")
      size = sizeof(grids(3)%units(1))
      bmi_status = BMI_SUCCESS
    case("INPUT_VAR_1")
       size = sizeof(this%model%input_var_1)
       bmi_status = BMI_SUCCESS
    case("INPUT_VAR_2")
       size = sizeof(this%model%input_var_2)
       bmi_status = BMI_SUCCESS
    case("INPUT_VAR_3")
       size = sizeof(this%model%input_var_3)
       bmi_status = BMI_SUCCESS
    case("GRID_VAR_1")
        size = sizeof(this%model%grid_var_1(1,1))
        bmi_status = BMI_SUCCESS
    case("GRID_VAR_2")
       size = sizeof(this%model%grid_var_2(1,1))
       bmi_status = BMI_SUCCESS
    case("GRID_VAR_3")
        size = sizeof(this%model%grid_var_3(1,1,1))
        bmi_status = BMI_SUCCESS
    case("GRID_VAR_4")
        size = sizeof(this%model%grid_var_4(1,1,1))
        bmi_status = BMI_SUCCESS
    case("OUTPUT_VAR_1")
       size = sizeof(this%model%output_var_1)
       bmi_status = BMI_SUCCESS
    case("OUTPUT_VAR_2")
       size = sizeof(this%model%output_var_2)
       bmi_status = BMI_SUCCESS
    case("OUTPUT_VAR_3")
       size = sizeof(this%model%output_var_3)
       bmi_status = BMI_SUCCESS
    case default
       size = -1
       bmi_status = BMI_FAILURE
    end select
  end function test_var_itemsize

  ! The size of the given variable.
  function test_var_nbytes(this, name, nbytes) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    integer, intent(out) :: nbytes
    integer :: bmi_status
    integer :: s1, s2, s3, grid, grid_size, item_size

    s3 = this%get_var_itemsize(name, item_size)

    select case(name)
    case("grid_1_shape", "grid_1_spacing", "grid_1_units", "grid_1_origin")
      nbytes = item_size*grids(2)%rank
      bmi_status = BMI_SUCCESS
      return !FIXME refactor the rest of this function
    case("grid_2_shape", "grid_2_spacing", "grid_2_units", "grid_2_origin")
      nbytes = item_size*grids(3)%rank
      bmi_status = BMI_SUCCESS
      return !FIXME refactor the rest of this function
    end select
  
    s1 = this%get_var_grid(name, grid)
    s2 = this%get_grid_size(grid, grid_size)

    if( grid .eq. 0) then
      !these are the scalar values wrapped in an array
      nbytes = item_size
      bmi_status = BMI_SUCCESS
    else if ((s1 == BMI_SUCCESS).and.(s2 == BMI_SUCCESS).and.(s3 == BMI_SUCCESS)) then
       nbytes = item_size * grid_size
       bmi_status = BMI_SUCCESS
    else
       nbytes = -1
       bmi_status = BMI_FAILURE
    end if
  end function test_var_nbytes

  ! Set new integer values.
  function test_set_int(this, name, src) result (bmi_status)
    class (bmi_test_bmi), intent(inout) :: this
    character (len=*), intent(in) :: name
    integer, intent(in) :: src(:)
    integer :: bmi_status
  
    select case(name)
    case("grid_1_shape")
      if( allocated(this%model%grid_var_2) ) deallocate( this%model%grid_var_2 )
      ! src is in y, x order (last dimension first)
      ! make this variable be x, y
      allocate( this%model%grid_var_2(src(2), src(1)) )
      grids(2)%shape = src
      bmi_status = BMI_SUCCESS
    case("grid_1_units")
      grids(2)%units = src
      bmi_status = BMI_SUCCESS
    case("grid_2_shape")
      grids(3)%shape = src
      !OUTPUT vars must be allocated.  If they have already, deallocate and allocate the correct size
      if( allocated(this%model%grid_var_3) ) deallocate( this%model%grid_var_3 )
      ! src is in z, y, x order (last dimension first)
      ! make this variable be x, y, z
      allocate( this%model%grid_var_3(src(3), src(2), src(1)) )
      if( allocated(this%model%grid_var_4) ) deallocate( this%model%grid_var_4 )
      ! src is in z, y, x order (last dimension first)
      ! make this variable be z, y, x as well
      allocate( this%model%grid_var_4(src(1), src(2), src(3)) )
      bmi_status = BMI_SUCCESS
    case("grid_2_units")
      grids(3)%units = src
      bmi_status = BMI_SUCCESS
    case("INPUT_VAR_3")
       this%model%input_var_3 = src(1)
       bmi_status = BMI_SUCCESS
    case default
       bmi_status = BMI_FAILURE
    end select
    ! NOTE, if vars are gridded, then use:
    ! this%model%var= reshape(src, [this%model%n_y, this%model%n_x])
  end function test_set_int

  ! Set new real values.
  function test_set_float(this, name, src) result (bmi_status)
    class (bmi_test_bmi), intent(inout) :: this
    character (len=*), intent(in) :: name
    real, intent(in) :: src(:)
    integer :: bmi_status

    select case(name)
    case("INPUT_VAR_2")
       this%model%input_var_2 = src(1)
       bmi_status = BMI_SUCCESS
    case default
       bmi_status = BMI_FAILURE
    end select
    ! NOTE, if vars are gridded, then use:
    ! this%model%temperature = reshape(src, [this%model%n_y, this%model%n_x])
  end function test_set_float

  ! Set new double values.
  function test_set_double(this, name, src) result (bmi_status)
    class (bmi_test_bmi), intent(inout) :: this
    character (len=*), intent(in) :: name
    double precision, intent(in) :: src(:)
    integer :: bmi_status
    !Default return
    bmi_status = BMI_FAILURE

    select case(name)
    case("grid_1_spacing")
      grids(2)%spacing = src
      bmi_status = BMI_SUCCESS
    case("grid_1_origin")
      grids(2)%origin = src
      bmi_status = BMI_SUCCESS
    case("grid_2_spacing")
      grids(3)%spacing = src
      bmi_status = BMI_SUCCESS
    case("grid_2_origin")
      grids(3)%origin = src
      bmi_status = BMI_SUCCESS
    case("GRID_VAR_1")
      ! shape is in y, x
      ! make this var x, y
      this%model%grid_var_1 = reshape(src, [grids(2)%shape(2), grids(2)%shape(1)])
      bmi_status = BMI_SUCCESS
    case("INPUT_VAR_1")
      this%model%input_var_1 = src(1)
      bmi_status=BMI_SUCCESS
    case default
       bmi_status = BMI_FAILURE
    end select
    ! NOTE, if vars are gridded, then use:
    ! this%model%var = reshape(src, [this%model%n_y, this%model%n_x])
  end function test_set_double

  ! Test setting integer values at particular (one-dimensional) indices.
  function test_set_at_indices_int(this, name, inds, src) result(bmi_status)
    class(bmi_test_bmi), intent(inout) :: this
    character(len=*), intent(in) :: name
    integer, intent(in) :: inds(:)
    integer, intent(in) :: src(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_set_at_indices_int

  ! Test setting real values at particular (one-dimensional) indices.
  function test_set_at_indices_float(this, name, inds, src) result(bmi_status)
    class(bmi_test_bmi), intent(inout) :: this
    character(len=*), intent(in) :: name
    integer, intent(in) :: inds(:)
    real, intent(in) :: src(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_set_at_indices_float

  ! Test setting double precision values at particular (one-dimensional) indices.
  function test_set_at_indices_double(this, name, inds, src) result(bmi_status)
    class(bmi_test_bmi), intent(inout) :: this
    character(len=*), intent(in) :: name
    integer, intent(in) :: inds(:)
    double precision, intent(in) :: src(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_set_at_indices_double

  ! Get a copy of a integer variable's values, flattened.
  function test_get_int(this, name, dest) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    integer, intent(inout) :: dest(:)
    integer :: bmi_status

    select case(name)
    case("grid_1_units")
      dest = grids(2)%units
      bmi_status = BMI_SUCCESS
    case("grid_2_units")
      dest = grids(3)%units
      bmi_status = BMI_SUCCESS
    case("INPUT_VAR_3")
       dest = [this%model%input_var_3]
       bmi_status = BMI_SUCCESS
    case("OUTPUT_VAR_3")
      dest = [this%model%output_var_3]
      bmi_status = BMI_SUCCESS
    case default
       dest(:) = -1
       bmi_status = BMI_FAILURE
    end select
    ! NOTE, if vars are gridded, then use:
    ! dest = reshape(this%model%var, [this%model%n_x*this%model%n_y])
  end function test_get_int

  ! Get a copy of a real variable's values, flattened.
  function test_get_float(this, name, dest) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    real, intent(inout) :: dest(:)
    integer :: bmi_status, size

    select case(name)
    case("INPUT_VAR_2")
       dest = [this%model%input_var_2]
       bmi_status = BMI_SUCCESS
    case("OUTPUT_VAR_2")
      dest = [this%model%output_var_2]
      bmi_status = BMI_SUCCESS
    case("GRID_VAR_2")
      bmi_status = this%get_grid_size(1, size)
      if( bmi_status == BMI_SUCCESS ) then
        dest = reshape(this%model%grid_var_2, [size])
    end if
    case default
       dest(:) = -1.0
       bmi_status = BMI_FAILURE
    end select
    ! NOTE, if vars are gridded, then use:
    ! dest = reshape(this%model%temperature, [this%model%n_x*this%model%n_y]) 
  end function test_get_float

  ! Get a copy of a double variable's values, flattened.
  function test_get_double(this, name, dest) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character (len=*), intent(in) :: name
    double precision, intent(inout) :: dest(:)
    integer :: bmi_status, size
    !==================== UPDATE IMPLEMENTATION IF NECESSARY FOR DOUBLE VARS =================

    select case(name)
    case("INPUT_VAR_1")
      dest = [this%model%input_var_1]
      bmi_status = BMI_SUCCESS
    case("GRID_VAR_1")
      bmi_status = this%get_grid_size(1, size)
      if( bmi_status == BMI_SUCCESS .and. allocated(this%model%grid_var_1) ) then
        ! Have to ensure grid_var_1 as been set by something before we try to get it...
        dest = reshape(this%model%grid_var_1, [size])
      else
        bmi_status = BMI_FAILURE
      end if
    case("GRID_VAR_3")
      bmi_status = this%get_grid_size(2, size)
      if( bmi_status == BMI_SUCCESS ) then
        dest = reshape(this%model%grid_var_3, [size])
      end if
    case("GRID_VAR_4")
      bmi_status = this%get_grid_size(2, size)
      if( bmi_status == BMI_SUCCESS ) then
        dest = reshape(this%model%grid_var_4, [size])
      end if
    case("OUTPUT_VAR_1")
      dest = [this%model%output_var_1]
      bmi_status = BMI_SUCCESS
    case default
       dest(:) = -1.d0
       bmi_status = BMI_FAILURE
    end select
    ! NOTE, if vars are gridded, then use:
    ! dest = reshape(this%model%var, [this%model%n_x*this%model%n_y])
  end function test_get_double

  ! Test getting a reference to the given integer variable.
  function test_get_ptr_int(this, name, dest_ptr) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character(len=*), intent(in) :: name
    integer, pointer, intent(inout) :: dest_ptr(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_get_ptr_int

  ! Test getting a reference to the given real variable.
  function test_get_ptr_float(this, name, dest_ptr) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character(len=*), intent(in) :: name
    real, pointer, intent(inout) :: dest_ptr(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_get_ptr_float

  ! Test getting a reference to the given double variable.
  function test_get_ptr_double(this, name, dest_ptr) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    character(len=*), intent(in) :: name
    double precision, pointer, intent(inout) :: dest_ptr(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_get_ptr_double

  ! Test getting integer values at particular (one-dimensional) indices.
  function test_get_at_indices_int(this, name, dest, inds) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    character(len=*), intent(in) :: name
    integer, intent(inout) :: dest(:)
    integer, intent(in) :: inds(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_get_at_indices_int

  ! Test getting real values at particular (one-dimensional) indices.
  function test_get_at_indices_float(this, name, dest, inds) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    character(len=*), intent(in) :: name
    real, intent(inout) :: dest(:)
    integer, intent(in) :: inds(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_get_at_indices_float

  ! Test getting double precision values at particular (one-dimensional) indices.
  function test_get_at_indices_double(this, name, dest, inds) result(bmi_status)
    class(bmi_test_bmi), intent(in) :: this
    character(len=*), intent(in) :: name
    double precision, intent(inout) :: dest(:)
    integer, intent(in) :: inds(:)
    integer :: bmi_status

    bmi_status = BMI_FAILURE
  end function test_get_at_indices_double

  ! Model start time.
  function test_start_time(this, time) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    double precision, intent(out) :: time
    integer :: bmi_status

    time = 0.0
    bmi_status = BMI_SUCCESS
  end function test_start_time
  
  ! Model end time.
  function test_end_time(this, time) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    double precision, intent(out) :: time
    integer :: bmi_status

    time = this%model%num_time_steps * this%model%time_step_size
    bmi_status = BMI_SUCCESS
  end function test_end_time

  ! Model current time.
  function test_current_time(this, time) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    double precision, intent(out) :: time
    integer :: bmi_status

    time = this%model%current_model_time
    bmi_status = BMI_SUCCESS
  end function test_current_time

  ! Model current time.
  function test_time_step(this, time_step) result (bmi_status)
    class (bmi_test_bmi), intent(in) :: this
    double precision, intent(out) :: time_step
    integer :: bmi_status

    time_step = this%model%time_step_size
    bmi_status = BMI_SUCCESS
  end function test_time_step

  ! Advance the model until the given time.
  function test_update_until(this, time) result (bmi_status)
    use test_model, only: run
    class (bmi_test_bmi), intent(inout) :: this
    double precision, intent(in) :: time
    integer :: bmi_status, run_status

    call run(this%model, time - this%model%current_model_time )
    !really this if isn't required...
    if(this%model%current_model_time /= time ) then
      this%model%current_model_time = time
    endif

    bmi_status = BMI_SUCCESS
  end function test_update_until
  
  ! Advance model by one time step.
  function test_update(this) result (bmi_status)
    class (bmi_test_bmi), intent(inout) :: this
    integer :: bmi_status

    bmi_status = this%update_until(this%model%current_model_time + this%model%time_step_size)
  end function test_update

#ifdef NGEN_ACTIVE
  function register_bmi(this) result(bmi_status) bind(C, name="register_bmi")
   use, intrinsic:: iso_c_binding, only: c_ptr, c_loc, c_int
   use iso_c_bmif_2_0
   implicit none
   type(c_ptr) :: this ! If not value, then from the C perspective `this` is a void**
   integer(kind=c_int) :: bmi_status
   !Create the model instance to use
   type(bmi_test_bmi), pointer :: bmi_model
   !Create a simple pointer wrapper
   type(box), pointer :: bmi_box

   !allocate model
   allocate(bmi_test_bmi::bmi_model)
   !allocate the pointer box
   allocate(bmi_box)

   !associate the wrapper pointer the created model instance
   bmi_box%ptr => bmi_model

   if( .not. associated( bmi_box ) .or. .not. associated( bmi_box%ptr ) ) then
    bmi_status = BMI_FAILURE
   else
    !Return the pointer to box
    this = c_loc(bmi_box)
    bmi_status = BMI_SUCCESS
   endif
 end function register_bmi
#endif
end module bmitestbmi
