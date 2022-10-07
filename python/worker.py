
from matplotlib.style import available
from job_builder import Job, mini_chunk_size


class Worker:
    
    def __init__(self, file_path):
        self.file_path = file_path
        self.available = True
        
    def assign_job(self, job: Job):
        available = False
        
        bytes = []  # read bytes from file
        with open(self.file_path, 'rb') as file:
            for mini_chunk_id in job.mini_chunk_ids:
                file.seek(mini_chunk_id * mini_chunk_size)
                mini_chunk = file.read(mini_chunk_size)
                bytes.append(mini_chunk)
            
            bytes = b''.join(bytes)
        
        
        
