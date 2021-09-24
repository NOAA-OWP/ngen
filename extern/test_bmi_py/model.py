class ngen_model():
    def __init__(self, model):
        super(ngen_model, self).__init__()
        self.model = model

    def run(self, model, dt):
        if dt == model['time_step_size']:
            model['output_var_1'] = model['input_var_1']
            model['output_var_2'] = 2.0 * model['input_var_2']
            model['output_var_3'] = 0
        else:
            model['output_var_1'] = model['input_var_1'] * dt / model['time_step_size']
            model['output_var_2'] = 2.0 * model['input_var_2'] * dt / model['time_step_size']
            model['output_var_3'] = 0

        model['current_model_time'] = model['current_model_time'] + dt