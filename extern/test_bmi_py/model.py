class ngen_model():
    # TODO: refactor the bmi_model.py file and this to have this type maintain its own state.
    #def __init__(self):
    #    super(ngen_model, self).__init__()
    #    #self._model = model

    def run(self, model: dict, dt: int):
        """
        Run this model into the future.

        Run this model into the future, updating the state stored in the provided model dict appropriately.

        Note that the model assumes the current values set for input variables are appropriately for the time
        duration of this update (i.e., ``dt``) and do not need to be interpolated any here.

        Parameters
        ----------
        model: dict
            The model state data structure.
        dt: int
            The number of seconds into the future to advance the model.

        Returns
        -------

        """
        #Since VAR_1 and 2 are scalars wrapped in a (0,) sized numpy array, need to index with either `...` or `()`
        model['OUTPUT_VAR_1'][...] = model['INPUT_VAR_1']
        model['OUTPUT_VAR_2'][...] = 2.0 * model['INPUT_VAR_2']
        model['OUTPUT_VAR_3'] += 1
        #Update the grid data
        model['GRID_VAR_2'] = model['GRID_VAR_1'] + 2
        model['GRID_VAR_3'] = model['GRID_VAR_1'] + 3
        model['current_model_time'] = model['current_model_time'] + dt