
import os

from job_builder import JobBuilder
from worker import Worker


class JobScheduler:
    
    def __init__(self, file_path: os.path, n_workers: int) -> None:
        self.file_path = file_path
        self.workers = [Worker(self.file_path) for _ in range(n_workers)]
        self.job_builder = JobBuilder(self.file_path)
        
    def run(self):
        while job := self.job_builder.create_job() is not None:
            # Find a worker that is available
            pass
