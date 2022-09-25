import os
import random
import numpy as np
from numpy import random as nprandom

# All possible distributions:
# gauss, poisson, exp, uniform
distribution = 'gauss'
file_size_mb = 128
file_path = f'../data/{distribution}_{file_size_mb}M.bin'
batch_size = 10_000

os.makedirs('../data', exist_ok=True)

file_size_bytes = file_size_mb * 1024 * 1024
n_doubles = file_size_bytes // 8

# File might be a bit longer but whatever
n_batches = n_doubles // batch_size + 1

# Create generator fn dict
random_fns = {
    'gauss': lambda: nprandom.normal(loc=.0, scale=100_000_000, size=batch_size).astype(float),
    'poisson': lambda: nprandom.poisson(lam=1000, size=batch_size).astype(float),
    'exp': lambda: nprandom.exponential(scale=1., size=batch_size).astype(float),
    'uniform': lambda: nprandom.uniform(low=-10_000_000, high=10_000_000, size=batch_size).astype(float),
}

with open(file_path, 'wb') as f:
    for _ in range(n_batches):
        bytes = random_fns[distribution]().tobytes()
        f.write(bytes)
        
