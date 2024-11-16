/**
 * This file contains code for the assistand instructor simulator
 * @date November 11th, 2024
 * @author John Khalife, Stavros Karamalis
*/


//Imports
#include <unordered_set>

/**
 * This namespace is intended to be used for process management.
*/
namespace ProcessManagement {
    //This set will hold all the ids of every process created by the running process
    std::unordered_set<pid_t> processSet;
    //This set will hold every shared memory segment key created by the running process
    std::unordered_set<int> shmSet;

    /**
     * This method is responsible for creating processes
     * @param processNum - the number of processes to be created.
    */
    void createProcesses(int processNum);

    /**
     * This method allocates shared memory and returns an id. It does the error handling
     * @param size: the size of the shared memory
     * @param key: the key of the shared memory
    */
    int createSharedMemory(int key, int size);

    /**
     * This method returns attatches the current process to shared memory and returns a pointer
     * @param shm_id the id of the shared memory.
    */
    void* getSharedMemory(int shm_id);

    /**
     * This method queues shared memory for deletion
     * @param shmid - the id of the shared memory
    */
    void deallocateSharedMemory(int shmid);

    /**
     * This method detatches shared memory pointer from the current process
     * @param ptr - a pointer to the shared memory
    */
    void detachSharedMemory(void* ptr);

    /**
     * This method is responsible for terminating any given process
     * @param pid - the process id of the process to terminate
    */
    void terminateProcess(pid_t pid);

    /**
     * This method is intended to clean up all shared memory instances and processes. 
     * It checks all a threads children.
     * However, any pointers to memory should be deallocated before this method is called.
    */
   void cleanup();
}

