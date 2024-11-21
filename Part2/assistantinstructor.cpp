/**
 * This file contains code for the assistant simulator
 * @date November 11th, 2024
 * @author John Khalife, Stavros Karamalis
 */

#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <csignal>
#include <sys/shm.h>
#include "assistantinstructor.hpp"
#include <sys/sem.h>
#include <sstream>
#include <functional>

namespace ProcessManagement
{

    void createProcess()
    {
        int pid = fork(); // create the new process
        if (pid == 0)
            {
                shmSet.clear();
            }
            else if (pid < 0)
            {
                // This means the process was unsuccessful in being created.
                std::cout << "A process failed to be created." << std::endl;
                exit(1);
            }
            else
            {
                std::cout << "Created  a process with pid " << pid << "." << std::endl;
                shmSet.insert(std::pair<int, int>(PROCESS_VALUE, pid));
            }
    }

    int createSharedMemory(int key, int size)
    {
        int shm_id = shmget(key, size, IPC_CREAT | 0666);
        if (shm_id < 0)
        {
            perror("Failed shared memory allocation.");
        }
        shmSet.insert(std::pair<int, int>(SHM_VALUE, shm_id));
        return shm_id;
    }

    void *getSharedMemory(int shm_id)
    {
        void *ptr = shmat(shm_id, NULL, 0);
        // error handling
        if (ptr == NULL)
        {
            perror("Failed to attach memory");
        }
        return ptr;
    }

    void deallocateSharedMemory(int shmid)
    {
        shmctl(shmid, IPC_RMID, NULL); // queue for deallocation
        // Make sure that the id specified is in the shmset before attempting to remove
        if (shmSet.find(std::pair<int, int>(SHM_VALUE, shmid)) != shmSet.end())
        {
            shmSet.erase(std::pair<int, int>(SHM_VALUE, shmid));
        }
    }

    void detachSharedMemory(void *ptr)
    {
        // unmap the memory from it
        if (shmdt(ptr) == -1)
        {
            perror("shmdt failed");
        }
    }

    void terminateProcess(pid_t pid)
    {
        if (kill(pid, SIGTERM) == -1)
        {
            perror("Failed to terminate process");
        }
        else
        {
            std::cout << "Process " << pid << " terminated successfully." << std::endl;
        }
    }

    int createSemaphore(int key, int initialValue, int length)
    {
        semun sem_union;
        sem_union.val = initialValue;
        int sem_id = semget(key, length, IPC_CREAT | 0666); // actually makes the semaphore
        if (sem_id == -1) {
            perror("semget failed");
            exit(1);
        }
        for (int i = 0; i < length; i++) {
            if (semctl(sem_id, i, SETVAL, sem_union) == -1) {
                perror("semctl failed");
                exit(1);
            }
        }
        return sem_id;
    }

    void semaphoreOperation(int sem_id, int index, int operation)
    {
        struct sembuf sb;
        sb.sem_num = index;
        sb.sem_op = operation;
        sb.sem_flg = 0;
        if (semop(sem_id, &sb, 1) < 0)
        {
            perror("Failed to perform semaphore operation");
        }
    }
    
    void throwError(std::string message)
    {
        std::cout << "Error in process " << getpid() << ": " << message << std::endl;
        exit(1);
    }

    void cleanup(int signalNumber)
    {
        std::cout << "cleanup called by pid " << getpid() << std::endl;
        // Start by iterating through the process set.
        // This set should contain all of the children processes created by the calling process.
        while (!shmSet.empty())
        {
            auto i = shmSet.begin();
            switch (i->first)
            {
            case PROCESS_VALUE:
                terminateProcess(i->second);
                break;
            case SEMAPHORE_VALUE:
                if (semctl(i->second, 0, IPC_RMID) == -1) {
                    perror("Failed to remove semaphore");
                }
            case SHM_VALUE:
                if (shmctl(i->second, IPC_RMID, NULL) == -1) {
                    perror("Failed to remove shared memory");
                }
            }
            shmSet.erase(i);
        }
    }

    // Function to be called at normal program termination
    void normalCleanup()
    {
        cleanup(0);
    }

    // Signal handler function
    void signalCleanup(int signum)
    {
        cleanup(signum);
        exit(signum);
    }

    void handleChildProcessError(int signalNumber)
    {
        std::cout << "Child process failed with signal number " << signalNumber << "." << std::endl;
        exit(1);
    }

}

namespace TAManagement
{
    int* loadDatabase(std::string fileName)
    {
        // Create the shared memory - 0 - 9999 student numbers can be stored
        //Attach the shared memory (for now)
        std::vector<int> database;
        std::ifstream file;
        try
        {
            file.open(fileName);
        }
        catch (std::exception e)
        {
            ProcessManagement::throwError("Failed to open file.");
        }
        while (!file.eof())
        {
            // Read the file line by line
            std::string line;
            std::getline(file, line);
            database.push_back(stoi(line));
        }
        int shm_id = ProcessManagement::createSharedMemory(2222, database.size() * sizeof(int));
        int* sharedDatabase = (int*) ProcessManagement::getSharedMemory(shm_id);
        for (int i = 0 ; i < database.size() ; i++) {
            sharedDatabase[i] = database.at(i);
        }
        file.close();
        return sharedDatabase;
    }

    void markStudent(int studentNumber, int mark, int sem_id, int index)
    {
        sleep((rand() % 10) + 1); // sleep for 1-10 seconds to represent marking
        // Create the output stream
        std::ofstream file;
        try
        {
            file.open("TA" + std::to_string(index + 1) + ".txt", std::ios::app); // open the file in append mode
        }
        catch (const std::exception &e)
        {
            ProcessManagement::throwError("Failed to open TA.txt");
        }

        if (file.is_open())
        {
            file << "Student " << studentNumber << " given grade " << mark << std::endl;
            file.close();
        }
        else
        {
            ProcessManagement::throwError("Failed to open TA.txt");
        }
        std::cout << "TA " << index << " marked student " << studentNumber << " with mark " << mark << std::endl;
    }

    bool loopCompleted(TAState* TAStates)
    {
        int taNum = getTANumber(TAStates);
        if (TAStates[taNum].loopNum >= LOOP_NUM) {
            return true;
        }
        return false;
    }
    
    void incrementLoopNum(TAState* TAStates) {
        TAStates[getTANumber(TAStates)].loopNum++;
    }

    int getTANumber(TAState* TAStates) {
        for (int i = 0 ; i < NUM_TA; i++) {
            if (getpid() == TAStates[i].pid) {
                return i;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    /*
     * My Idea for the structure
     - The original process acts as the manager for all the TA processes.
     - It will be responsible for ending the program.
    */
    using namespace ProcessManagement;
    //! program pseudocode/not really pseudocode
    // Save the controller process id
    const pid_t MANAGER_PID = getpid();
    std::cout << "Manager process has pid " << MANAGER_PID << std::endl;
    atexit(normalCleanup);
    // Add a signal handler for when the manager process is terminated (to automatically call cleanup)
    signal(SIGINT, signalCleanup);
    signal(SIGTERM, signalCleanup);
    signal(SIGQUIT, signalCleanup);
    signal(SIGCHLD, handleChildProcessError); // signal for child process error
    
    std::cout << "Creating semaphore..." << std::endl;
    int safety_sem = createSemaphore(4444, 1, 1);

    std::cout << "loading database..." << std::endl;
    // First the student database needs to be loaded into shared memory.
    int* database = TAManagement::loadDatabase("student_database.txt");
        
    //Next the TA semaphores are created
    std::cout << "Creating semaphores..." << std::endl;
    int ta_sem = createSemaphore(7777, 1, TAManagement::NUM_TA);
    //Then the TAs are created
    using namespace TAManagement;
    std::cout << "Creating TAs..." << std::endl;
    int shm_id = createSharedMemory(123, NUM_TA * sizeof(TAState));
    TAState* TAStates = (TAState*) getSharedMemory(shm_id);
    
    
    for (int i = 0 ; i < NUM_TA; i++) {
        createProcess();
        if (getpid() != MANAGER_PID) {
            semaphoreOperation(safety_sem, 0, -1);
            std::cout << "semaphore decremented by " << getpid() << std::endl;
            if (!TAStates[i].pid) {
                TAStates[i] = (TAState){0, getpid(), 0};
                semaphoreOperation(safety_sem, 0, 1);
                std::cout << "semaphore incremented by " << getpid() << std::endl;
                break;
            }
            semaphoreOperation(safety_sem, 0, 1);
            std::cout << "semaphore incremented by " << getpid() << std::endl;
        }
    }
    
    
    

    //Now we have the semaphores, and our processes. The simulation can begin
    if (getpid() == MANAGER_PID)
    {
        // The manager process will wait for all the TAs to finish their marking
       while (true) {}
    } else {
        //The main line of exection starts here.
        int taNum;
        semaphoreOperation(safety_sem, 0, -1);
        for (int i = 0 ; i < TAManagement::NUM_TA; i++) {
            if (getpid() == TAStates[i].pid) {
                taNum = i;
                break;
            }
        }
        semaphoreOperation(safety_sem, 0, 1);
        int nextTaNum = (taNum + 1) % TAManagement::NUM_TA;
        std::cout << "TA " << taNum << " has been assigned its TA number." << std::endl;
        // Each TA continues marking until it loops through the database 3 times.
        while (TAStates[taNum].loopNum < TAManagement::LOOP_NUM) {
            //Access the database and choose a student to mark.
            // Decrement the semaphore to prevent more than 2 TAs from database access at once.
            sleep(1);
            std::cout << "TA " << taNum << " is queued for access to the database." << std::endl;
            semaphoreOperation(ta_sem, taNum, -1);
            if (semctl(ta_sem, nextTaNum, GETVAL) == 0) {
                semaphoreOperation(ta_sem, taNum, 1);
                std::cout << "TA " << taNum << " is waiting for TA " << nextTaNum << " to finish marking." << std::endl;
                continue;
            }
            semaphoreOperation(ta_sem, nextTaNum, -1);
            std::cout << "TA " << taNum << " is has gained access to the database." << std::endl;
            sleep(rand() % 4 + 1);
            //increment the index
            if (TAStates[taNum].index >= 9999) {
                TAStates[taNum].index = 1;
                TAStates[taNum].loopNum++;
                std::cout << "TA " << taNum << " has looped through the database " << TAStates[taNum].loopNum << " times." << std::endl;
            } else {
                TAStates[taNum].index++;
            }
            std::cout << "TA " << taNum << " decided to mark student " << database[TAStates[taNum].index] << std::endl;
            semaphoreOperation(ta_sem, nextTaNum, 1);
            semaphoreOperation(ta_sem, taNum, 1);
            markStudent(database[TAStates[taNum].index], rand() % 100, ta_sem, taNum);
        }

    }

    return 0;
}
