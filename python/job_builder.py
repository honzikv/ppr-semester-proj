from typing import List
import bitmap
import os
import random

# Memory avaiable - 1GB

memory_for_chunks = 768 * 1024 * 1024 # 768 MB
mini_chunk_size = 4096 # 4 kB
mini_chunks_per_job = (8 * 1024 * 1024) // mini_chunk_size  # 8 MB / 4 kB = 2048


class Job:
    def __init__(self, mini_chunk_ids: List[int]):
        self.mini_chunk_ids = mini_chunk_ids
        self.result = None             
        

class SequenceGenerator:
    """
    Class used for generating permutations for mini-chunks
    """
    
    def __init__(self, max: int):
        self._next = 0  # pointer to the next "generated" value
        # essentially just create a list with indices from 0 to max
        # and shuffle it using random.shuffle
        self.sequence = [x for x in range(max)] 
        random.shuffle(self.sequence)
        
        
    def get_next_sequence(self, sequence_size: int) -> List[int]:
        """
        Returns the next "amount" of values from the sequence
        """
        
        if self._next + sequence_size > len(self.sequence):
            raise IndexError('SequenceGenerator: out of bounds')
        
        result = self.sequence[self._next:self._next+sequence_size]
        self._next += sequence_size
        return result
              

class JobBuilder:
    def __init__(self, file_path: os.path, seed=42):
        self.file_path = file_path
        self.file_size = os.stat(file_path).st_size
        self.chunk_count = self.file_size // mini_chunk_size
        self.bitmap = bitmap.Bitmap(self.chunk_count)
        random.seed(seed)
        self.sequence_generator = SequenceGenerator(self.chunk_count)

    def create_job(self) -> Job:
        """
        Creates a new job to be processed or returns None indicating that there are no more jobs to be created
        Returns:
            Job: instance of job to be processed
        """
        try:
            job_items = self.sequence_generator.get_next_sequence(mini_chunks_per_job)
            return Job(job_items)
            
        except IndexError:
            return None

    @property
    def file_path(self) -> os.path:
        return self.file_path
