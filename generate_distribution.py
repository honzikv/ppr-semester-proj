import argparse
import numpy as np


distributions = {
    'gauss': lambda x: np.random.normal(size=x, loc=0, scale=500),
    'uniform': lambda x: np.random.uniform(size=x, low=-300, high=300),
    'poisson': lambda x: np.random.poisson(size=x, lam=100),
    'exp': lambda x: np.random.exponential(size=x, scale=100),
}

if __name__ == '__main__':
    argparser = argparse.ArgumentParser()
    argparser.add_argument('--distribution_type', type=str,
                           required=True, choices=['gauss', 'uniform', 'poisson', 'exp'])
    argparser.add_argument('--num_samples', type=int,
                          required=True, help='Number of samples to generate')
    argparser.add_argument('--output_file', type=str,
                           required=True, help='Path to output file')
    argparser.add_argument('--noise', type=float, default=0.0, max=0.5, min=0.0, help='Noise to add to the distribution')
    
    args = argparser.parse_args()
    
    generator = distributions[args.distribution_type]
    samples = generator(args.num_samples)
    
    # Write to file in binary
    with open(args.output_file, 'wb') as f:
        f.write(samples.tobytes('C'))
        
