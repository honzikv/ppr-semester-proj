import sys
import os

def parse_args():
    args = sys.argv
    if len(args) < 1:
        print('Error, no file path provided. Exiting...')
        exit(1)
    
    file_path = args[0]
    if not os.path.exists(file_path):
        print('Error, file does not exist. Exiting...')
        exit(1)
        
    return file_path

def main():
    file_path = parse_args()
    
        
    
    
