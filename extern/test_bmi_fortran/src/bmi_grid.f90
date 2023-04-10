module bmi_grid
    use, intrinsic :: iso_c_binding, only: c_int, c_double
    implicit none
    public

    enum, bind(c) ! Grid Type "enumeration"
    ! BMI GRID TYPE enumeration, should be C compat with the BMI_GRID_TYPE enum
    enumerator :: scalar = 0 !0 dim
    enumerator points !1 dim
    enumerator vector !1 dim
    enumerator unstructured !1-N dim
    enumerator structured_quadrilateral !2-N dim
    enumerator rectilinear ! 2-N dim
    enumerator uniform_rectilinear !2-N dim
    end enum

    enum, bind(c) ! Grid Unit "enumeration"
    ! BMI GRID UNIT enumeration
    enumerator :: none = -1
    enumerator :: m = 0
    enumerator :: km = 1
    end enum

    type :: GridType
        integer(kind=c_int) :: id
        integer(kind=c_int) :: rank
        integer(kind=c_int) :: size
        ! here kind(scalar) just referers to the kind of all things in the enum
        integer(kind(scalar)) :: type
        ! These allocatable dynamic size arrays make this type impossible to directly bind(c)
        integer(kind=c_int), allocatable :: shape(:)
        real(kind=c_double), allocatable :: spacing(:)
        real(kind=c_double), allocatable :: origin(:)
        ! TODO possible to keep a pointer/handle to all vars referencing this grid?
        integer(kind(none)), allocatable :: units(:)
        ! Allocation guard
        logical, private :: allocated = .false.

        contains
            procedure :: init => grid_init
            ! TODO implement grid destruction to deallocate
            procedure :: grid_x => grid_x
            procedure :: grid_y => grid_y
            procedure :: grid_z => grid_z
    end type GridType

    contains

    ! Initialize the grid data structure
    ! 
    ! Parameters
    ! this: GridType the instance to initialize
    ! id: integer the BMI grid identifier
    ! rank: integer the number of dimensions of the grid
    ! type: integer Enumerated type of grid from the GridType Enumeration
    ! units: integer (Optional) Enumerated typer of units from the GridUnit Enumeration. Defaults to none (-1)
    subroutine grid_init(this, id, rank, type, units)
        class(GridType), intent(inout) :: this
        integer, intent(in) :: id
        integer, intent(in) :: rank
        integer(kind(none)), intent(in) :: type
        integer, intent(in), optional :: units
        integer :: rank_
        ! optional, defaults to none
        integer :: units_
        units_ = none
        if (present(units)) units_ = units

        this%id = id
        this%rank = rank
        this%type = type
        rank_ = rank
        ! allocate the meta data arrays based on the rank of the grid we are supporting
        if (rank .eq. 0 ) then
            rank_ = 1 ! use a 1 element 1D array for scalar representation
        end if

        ! TODO should we allow re-allocation/construction?  This seeems to be a bit dangerous
        ! if things are dependent on the grid to not change without explicit reference changing...
        if( .not. this%allocated ) then 
            allocate( this%shape(rank_), source=0_c_int )
            allocate( this%spacing(rank_), source=0.0_c_double )
            allocate( this%origin(rank_), source=0.0_c_double )
            allocate( this%units(rank_), source=units_ )
            this%allocated = .true.
        end if
        
    end subroutine grid_init

    ! Compute the x coordinates of the grid for a rectilinear or uniform rectilinear grid with rank > 0
    ! 
    ! If none of these conditions are true, then the result is undefined.
    ! 
    ! Parameters
    ! this: GridType the instance to compute the coordinates for
    ! xs: 1D double precision array to put the coordinates in, the length of the resulting array
    !     is determined by the shape of the grid
    subroutine grid_x(this, xs)
        class(GridType), intent(in) :: this
        double precision, intent(out) :: xs(:) ! deferred shape, 1D
        integer :: idx, i

        idx = this%rank ! indexing [z,y,x]
        if( (this%type .eq. rectilinear .or. this%type .eq. uniform_rectilinear) .and. this%rank .gt. 0) then
            do i = 1, this%shape(idx)
                xs(i) = this%origin(idx) + this%spacing(idx)*(i-1) ! have to offset the 1 based index
            end do
        end if
        ! TODO should this set a sentinal value for scalars, e.g.
        ! if( this%rank .eq. 0 ) xs(1) = -1
    end subroutine

    ! Compute the y coordinates of the grid for a rectilinear or uniform rectilinear grid with rank > 0
    !
    ! If none of these conditions are true, then the result is undefined.
    ! 
    ! Parameters
    ! this: GridType the instance to compute the coordinates for
    ! ys: 1D double precision array to put the coordinates in, the length of the resulting array
    !     is determined by the shape of the grid
    subroutine grid_y(this, ys)
        class(GridType), intent(in) :: this
        double precision, intent(out) :: ys(:) ! deferred shape, 1D
        integer :: idx, i

        idx = this%rank-1 ! indexing [z,y,x]
        if( (this%type .eq. rectilinear .or. this%type .eq. uniform_rectilinear) .and. this%rank .gt. 1) then
            do i = 1, this%shape(idx)
                ys(i) = this%origin(idx) + this%spacing(idx)*(i-1) ! have to offset the 1 based index
            end do
        end if
    end subroutine

    ! Compute the z coordinates of the grid for a rectilinear or uniform rectilinear grid with rank > 0
    !
    ! If none of these conditions are true, then the result is undefined.
    ! 
    ! Parameters
    ! this: GridType the instance to compute the coordinates for
    ! zs: 1D double precision array to put the coordinates in, the length of the resulting array
    !     is determined by the shape of the grid
    subroutine grid_z(this, zs)
        class(GridType), intent(in) :: this
        double precision, intent(out) :: zs(:) ! deferred shape, 1D
        integer :: idx, i

        idx = this%rank-2 ! indexing [z,y,x]
        if( (this%type .eq. rectilinear .or. this%type .eq. uniform_rectilinear) .and. this%rank .gt. 2) then
            do i = 1, this%shape(idx)
                zs(i) = this%origin(idx) + this%spacing(idx)*(i-1) ! have to offset the 1 based index
            end do
        end if
    end subroutine

end module bmi_grid