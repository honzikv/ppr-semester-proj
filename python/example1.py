import numpy as np

def generate_distribution(distribution_type, **kwargs):
    if distribution_type == 'normal':
        return np.random.normal(**kwargs)
    elif distribution_type == 'uniform':
        return np.random.uniform(**kwargs)
    elif distribution_type == 'exponential':
        return np.random.exponential(**kwargs)
    elif distribution_type == 'poisson':
        return np.random.poisson(**kwargs)
    
    raise ValueError('Invalid distribution type')
    
    
def perform_test(expected_distribution, )

