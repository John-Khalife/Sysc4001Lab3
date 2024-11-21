/**
 * This file contains code for the assistand instructor simulator
 * @date November 11th, 2024
 * @author John Khalife, Stavros Karamalis
*/




#ifndef __ASSISTANTINSTRUCTOR_HPP__
#define __ASSISTANTINSTRUCTOR_HPP__

//Imports
#include <unordered_set>
#include <functional> 
#include <vector>
#include <string>

/**
 * This namespace is intended to be used for process management.
*/
namespace ProcessManagement {

    // This union is used for passing values to the semctl function
    union semun
    {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };

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
    */
    void createProcess();

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
     * This method is responsible for performing semaphore operations
     * @param sem_id - the id of the semaphore
     * @param index - the index of the semaphore
     * @param operation - the operation to perform
    */
    void semaphoreOperation(int sem_id, int index, int operation);

    /**
     * This method is responsible for throwing an error and exiting the process
     * @param message - the message to print before exiting
    */
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
    const int LOOP_NUM = 3; //The number of times a TA should loop through the database
    const int NUM_TA = 5; //The number of TA's to create

    //This structure is responsible for holding information about a TA's state
    struct TAState{
        int loopNum; // The number of times this TA has looped through the database
        pid_t pid; //The id of the process associated with this TA.
        int index; //The index that the TA is currently at in the student database
    };

    /**
     * This method is responsible for loading the database from a file into shared memory
     * *This function should be called by the manager process
     * @return a pointer to the integer array
    */
    int* loadDatabase(std::string fileName);

    /**
     * This method is responsible for marking a student in the database and printing it out to the file
     * *To be called by TA processes
     * @param studentNumber - the student number to mark
     * @param mark - the mark to give the student
    */
    void markStudent(int studentNumber, int mark, int sem_id);

    /**
     * This method is responsible for creating processes and associating them with
     * TA objects.
     * *To be called by the manager process.
    */
    int createAssistantInstructors(int numTAs);

    /**
     * This function returns true if the looping is completed
     * *To be called by a TA process
     * @return a boolean that states whether or not the loop is completed
    */
    bool loopCompleted(TAState* TAStates);

    /**
     * This function increments the loop number of the TA
     * *To be called by a TA process
    */
    void incrementLoopNum(TAState* TAStates);

    /**
     * This function returns the number of the TA
     * *To be called by a TA process
    */
    int getTANumber(TAState* TAStates);
}

#endif