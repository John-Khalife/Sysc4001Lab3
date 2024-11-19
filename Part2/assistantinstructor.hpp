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

    //This hash function is used for hashing pairs
    struct pair_hash {
        //The hash can contain any class
        template <class T1, class T2>
        //The hash function - hash the indivisual elements, then xor them together
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            std::size_t hash1 = std::hash<T1>()(p.first);
            std::size_t hash2 = std::hash<T2>()(p.second);
            return (hash1 << 4) ^ (hash2 << 2);
        }
    };

    //This set will hold all the ids of every process,semaphore, and shared memory created by the running process
    std::unordered_set<std::pair<int, int>,pair_hash> shmSet;
    //These are constants for marking types of objects used in the shmSet.
    const int PROCESS_VALUE = 0;
    const int SHM_VALUE = 1;
    const int SEMAPHORE_VALUE = 2;

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
     * This method creates a semaphore and returns its id.
     * @param key - the key of the semaphore
     * @param initialValue - the initial value of the semaphore
     * @return int - the id of the created semaphore
     */
    int createSemaphore(int key, int initialValue, int length);

    /**
     * This method is responsible for telling a semaphore to wait
     * @param sem_id - the id of the semaphore to decrement
    */
    void decrementSemaphore(int sem_id, int index);

    /**
     * This method is responsible for incrementing a semaphore - telling processes to continuess
     * @param sem_id - the id of the semaphore to increment
    */
    void incrementSemaphore(int sem_id, int index);


    /**
     * This method is resonpsible for removing a semaphore
     * @param sem_id - the id of the semaphore to remove
    */
    void removeSemaphore(int sem_id);

    void throwError(std::string message);

    /**
     * This method is intended to clean up all shared memory instances and processes. 
     * It checks all a threads children.
     * However, any pointers to memory should be deallocated before this method is called.
    */
   void signalCleanup(int signalNumber);

    /**
     * This method is responsible for cleaning up all shared memory and processes,
     * when the program shuts down normally
    */
   void normalCleanup();

    /**
     * This method is responsible for cleaning up all shared memory and processes,
     * when the program shuts down.
    */
   void cleanup(int signalNumber);

    /**
     * This method is responsible for checking to see if errors have occured in a child process. If so, call the cleanup.
    */
   void handleChildProcessError();
}

//This namespace is responsible for the implementation of the TA management system
namespace TAManagement{

    /**
     * This method is responsible for loading the database from a file into shared memory
     * *This function should be called by the manager process
    */
    int loadDatabase(std::string fileName);

    /**
     * This method is responsible for marking a student in the database and printing it out to the file
     * *To be called by TA processes
     * @param studentNumber - the student number to mark
     * @param mark - the mark to give the student
    */
    void markStudent(int studentNumber, int mark);
}

