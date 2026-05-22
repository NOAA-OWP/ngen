module test_model
    type :: test_bmi_model
        integer(kind=4) :: epoch_start_time
        integer :: num_time_steps
        double precision :: current_model_time
        double precision :: model_end_time
        double precision :: time_step_size

        double precision :: input_var_1
        real :: input_var_2
        integer :: input_var_3

        double precision :: output_var_1
        real :: output_var_2
        integer :: output_var_3

        ! some gridded vars to play with
        ! a rank 2 (2 dimensions) dyamically allocatable array of doubles (used as an input gridded var)
        double precision, dimension(:,:), allocatable :: grid_var_1
        ! a rank 2 (2 dimensions) dyamically allocatable array of reals (used as an output gridded var)
        real, dimension(:,:), allocatable :: grid_var_2
        ! a rank 3 (3 dimensions) dyamically allocatable array in (x,y,z) order
        double precision, dimension(:,:,:), allocatable :: grid_var_3
        ! a rank 3 (3 dimensions) dyamically allocatable array in (z,y,x) order
        double precision, dimension(:,:,:), allocatable :: grid_var_4

    end type test_bmi_model

    contains

    subroutine run(model, dt)
        type(test_bmi_model), intent(inout) :: model
        double precision, intent(in) :: dt

        if( dt == model%time_step_size) then
            model%output_var_1 = model%input_var_1
            model%output_var_2 = 2.0 * model%input_var_2
            model%output_var_3 = 0
        else
            model%output_var_1 = model%input_var_1 * dt / model%time_step_size
            model%output_var_2 = 2.0 * model%input_var_2 * dt / model%time_step_size
            model%output_var_3 = 0
        end if
        ! #Update the grid data
        ! Note, the if( allocated(...) ) statements are only needed
        ! because we test grid and regular variables seperately, so we don't always
        ! initialize the grid variables in the test setup.  These if's ensure
        ! we don't use unallocated arrays
        if( allocated(model%grid_var_2) .and. allocated(model%grid_var_1) ) then
            model%grid_var_2 = model%grid_var_1 + 2
        endif
        ! Update x,y,z
        if( allocated(model%grid_var_3) .and. allocated(model%grid_var_1) ) then
            model%grid_var_3(:,:,1) = model%grid_var_1 + 10
            model%grid_var_3(:,:,2) = model%grid_var_1 + 100
        endif
        ! Update z,y,x
        if( allocated(model%grid_var_4) .and. allocated(model%grid_var_1) ) then
            model%grid_var_4(1,:,:) = transpose(model%grid_var_1) + 10
            model%grid_var_4(2,:,:) = transpose(model%grid_var_1) + 100
        endif
        ! a little debug loop, leaving commented out for the next time...
        ! do k = 1, size(model%grid_var_3, 3)
        !     do j = 1, size(model%grid_var_3, 2)
        !         do i = 1, size(model%grid_var_3, 1)
        !             write(0, *) "i,j,k: ",i,j,k, model%grid_var_3(i,j,k)
        !         end do
        !     end do
        ! end do
        model%current_model_time = model%current_model_time + dt


    end subroutine run

end module test_model