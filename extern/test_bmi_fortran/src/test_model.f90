module test_model
    type :: test_bmi_model
        private
        integer(kind=4) :: epoch_start_time
        integer :: num_time_steps
        double precision :: current_model_time
        double precision :: model_end_time
        integer :: time_step_size

        double precision :: input_var_1
        double precision :: input_var_2

        double precision :: output_var_1
        double precision :: output_var_2
    end type test_bmi_model

    contains

    subroutine run(model, dt)
        type(test_bmi_model), intent(inout) :: model
        double precision, intent(in) :: dt

        if( dt == model%time_step_size) then
            model%output_var_1 = model%input_var_1
            model%output_var_2 = 2.0 * model%input_var_2
        else
            model%output_var_1 = model%input_var_1 * dt / model%time_step_size
            model%output_var_2 = 2.0 * model%input_var_2 * dt / model%time_step_size
        end if

        model%current_model_time = model%current_model_time + dt


    end subroutine run

end module test_model