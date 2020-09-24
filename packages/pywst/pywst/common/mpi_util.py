import imp
import logging

logger = logging.getLogger(__name__)

found_mpi = False
try:
    imp.find_module('mpi4py')
    found_mpi = True
except ImportError:
    found_mpi = False

if found_mpi:
    from mpi4py import MPI
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()
    host = MPI.Get_processor_name().split('.')[0]
else:
    comm = None
    rank = 0
    size = 1

def run_once(f):
    def wrapper(*args, **kwargs):
        if not wrapper.has_run:
            wrapper.has_run = True
            return f(*args, **kwargs)
    wrapper.has_run = False
    return wrapper

# just runs once in each processor. dont want to print every time
@run_once
def setup_mpi_logger(comm_size, host, rank):
    """
    Sets up the logger to add [`{hostname}`:`rank`] to the log lines.
    """
    ncpu = len(str(comm_size))
    fmt = r'[%6s:%-'+str(ncpu)+r's]'
    cpuid = fmt%(host,rank)
    for handler in logger.handlers:
        if isinstance(handler,logging.FileHandler):
            format = logging.Formatter(cpuid+' %(asctime)s %(levelname)8s: %(name)s - %(message)s')
            handler.setFormatter(format)
        elif isinstance(handler,logging.StreamHandler):
            format = logging.Formatter(cpuid+' %(levelname)8s: %(message)s')
            handler.setFormatter(format)
    logger.debug('I am running on host %s, rank %d',host,rank)

def distributeN(comm,N):
    """
    Distribute N consecutive things (rows of a matrix , blocks of a 1D array) 
    as evenly as possible over a given communicator.
    Uneven workload (differs by 1 at most) is on the initial ranks.

    Parameters
    ----------
    comm: MPI communicator
    N:  int
    Total number of things to be distributed.

    Returns
    ----------
    rstart: index of first local row
    rend: 1 + index of last row

    Notes
    ----------
    Index is zero based.
    """

    P      = comm.size
    rank   = comm.rank
    rstart = 0
    rend   = 0
    if P >= N:
        if rank < N:
            rstart = rank
            rend   = rank + 1
        else:
            rstart = rank
            rend   = rank
    else:
        n = N/P
        remainder = N%P
        rstart    = n * rank
        rend      = n * (rank+1)
        if remainder:
            if rank >= remainder:
                rstart += remainder
                rend   += remainder
            else: 
                rstart += rank
                rend   += rank + 1    
    return rstart, rend
