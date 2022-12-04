import argparse
import numpy as np


distributions = {
    'gauss': lambda x: np.random.normal(size=x, loc=-69, scale=1_000_000),
    'uniform': lambda x: np.random.uniform(size=x, low=-300, high=300),
    'poisson': lambda x: np.random.poisson(size=x, lam=100),
    'exp': lambda x: np.random.exponential(size=x, scale=100),
}

noise_distributions = {
    'gauss': lambda x: np.random.uniform(size=x, low=-300, high=300),
    'uniform': lambda x:  np.random.normal(size=x, loc=0, scale=500),
    'exp': lambda x: np.random.normal(size=x, loc=0, scale=500),
    'poisson': lambda x: np.random.uniform(size=x, low=-300, high=300),
}


def add_noise(x: np.array, distribution_type: str, percent_noise_items: float, percent_nans=.1, percent_infs=.1):
    """
    Replaces percentage of the items in array x with noise. This isn't particularly optimized but will suffice

    Args:
        x (np.array): array containing "valid" distribution. X must be flattened
        distribution_type (str): type of the distribution of x
        percent_noise_items (float): % of items to replace with noise
        percent_nans (float, optional): percentage of NaNs in the noise
        percent_infs (float, optional): percentage of Infs in the noise
    """
    
    # Sample indices to replace with noise
    x_noise_indices = np.random.choice(x.shape[0], int(x.shape[0] * percent_noise_items), replace=False)
    
    # Choose a noise distribution
    noise_distribution_fn = noise_distributions[distribution_type]
    
    # Generate noise
    x[x_noise_indices] = noise_distribution_fn(x_noise_indices.shape[0])
    
    # Pick float_percent_nans + percent_infs to be NaNs and Infs
    x_percent_nans_infs_indices = np.random.choice(x_noise_indices, int(x_noise_indices.shape[0] * (percent_nans + percent_infs)), replace=False)
    
    # Set the indices
    x[x_percent_nans_infs_indices[:int(x_percent_nans_infs_indices.shape[0] * percent_nans)]] = np.nan
    x[x_percent_nans_infs_indices[int(x_percent_nans_infs_indices.shape[0] * percent_nans):]] = np.inf
    
    return x
    

if __name__ == '__main__':
    argparser = argparse.ArgumentParser()
    argparser.add_argument('--distribution_type', type=str,
                           required=True, choices=['gauss', 'uniform', 'poisson', 'exp'])
    argparser.add_argument('--num_samples', type=int,
                          required=True, help='Number of samples to generate')
    argparser.add_argument('--output_file', type=str,
                           required=True, help='Path to output file')
    argparser.add_argument('--noise_prob', type=float, default=0.0, help='Noise to add to the distribution')
    
    args = argparser.parse_args()
    
    # Chunk size is 8 MB
    chunk_size = 8 * 1024 * 1024
    
    n_chunks = args.num_samples // chunk_size
    
    with open(args.output_file, 'wb') as file:
        for chunk_id in range(n_chunks):
            # Generate values and add noise
            x = distributions[args.distribution_type](chunk_size)
            x = add_noise(x, args.distribution_type, args.noise_prob)
            
            # Write to file
            file.write(x.tobytes('C'))         
    
    print(f'Generated {args.num_samples} samples of {args.distribution_type} distribution and saved to {args.output_file}')    
    
