class ngen_model():
    def __init__(self, m, b):
        super(ngen_model, self).__init__()
        self.m = m
        self.b = b

    def run_model(self, x):
        return self.m * x + self.b