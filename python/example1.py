import numpy as np

# Steps:
# 1 - Calculate min, max, mean and expected value for the data
# 2 - Normalize the data
# 3 - Chi Square test the data with normal, uniform, exponential and poisson distributions


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
    

def min_max_mean(data):
    return np.min(data), np.max(data), np.mean(data)


def normalize_data(data, min, max):
    return (data - min) / (max - min)
    


