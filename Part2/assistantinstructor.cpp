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

namespace ProcessManagement
{

    // This union is used for passing values to the semctl function
    union semun
    {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };

    void createProcesses(int processNum)
    {
        // create a boolean
        int bool_id = createSharedMemory(1234, sizeof(bool));
        bool *isFinished = (bool *)getSharedMemory(bool_id);
        *isFinished = false;

        int pid; // for checking the process id
        for (int i = 0; i < processNum; i++)
        {
            pid = fork(); // create the new process

            // Make sure the child process exits the loop so there is no exponential growth of processes
            if (pid == 0)
            {
                shmSet.clear();
                // Now check if the loop is over
                if (i == processNum - 1)
                {
                    (*isFinished) = true;
                }
                break;
            }
            else if (pid < 0)
            {
                // This means the process was unsuccessful in being created.
                std::cout << "The " << i + 1 << "th process failed to be created." << std::endl;
                exit(1);
            }
            else
            {
                std::cout << "Created process " << i << " with pid " << pid << "." << std::endl;
                shmSet.insert(std::pair<int, int>(PROCESS_VALUE, pid));
            }
        }

        // All created processes will wait until every process has been created. Otherwise, terminate all of them.
        while (!(*isFinished))
        {
        }
        detachSharedMemory(isFinished);
        deallocateSharedMemory(bool_id);
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
        int sem_id = semget(key, length, IPC_CREAT | 0666); // actually makes the sesmaphore
        if (sem_id < 0)
        {
            perror("Failed to create semaphore");
        }
        else
        {
            union semun arg;
            arg.val = initialValue;
            if (semctl(sem_id, 0, SETVAL, arg) < 0)
            {
                perror("Failed to set semaphore value");
            }
        }
        shmSet.insert(std::pair<int, int>(SEMAPHORE_VALUE, sem_id)); // add the semaphore to the set
        return sem_id;
    }

    void incrementSemaphore(int sem_id, int index)
    {
        struct sembuf sb;
        sb.sem_num = index;
        sb.sem_op = 1;
        sb.sem_flg = 0;
        if (semop(sem_id, &sb, 1) < 0)
        {
            perror("Failed to increment semaphore");
        }
    }

    void decrementSemaphore(int sem_id, int index)
    {
        struct sembuf sb;
        sb.sem_num = index;
        sb.sem_op = -1;
        sb.sem_flg = 0;
        if (semop(sem_id, &sb, 1) < 0)
        {
            perror("Failed to decrement semaphore");
        }
    }

    void removeSemaphore(int sem_id)
    {
        semctl(sem_id, 0, IPC_RMID);
        if (shmSet.find(std::pair<int, int>(SEMAPHORE_VALUE, sem_id)) != shmSet.end())
        {
            shmSet.erase(std::pair<int, int>(SEMAPHORE_VALUE, sem_id));
        }
    }

    void throwError(std::string message)
    {
        std::cout << "Error in process " << getpid() << ": " << message << std::endl;
        exit(1);
    }

    void cleanup(int signalNumber)
    {
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
                deallocateSharedMemory(i->second);
                break;
            case SHM_VALUE:
                semctl(i->second, 0, IPC_RMID);
                break;
            }
            shmSet.erase(i);
        }
    }


    // Function to be called at normal program termination
    void normalCleanup() {
        cleanup(0);
    }

    // Signal handler function
    void signalCleanup(int signum) {
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
    int loadDatabase(std::string fileName)
    {
        // Create the shared memory - 0 - 9999 student numbers can be stored
        int shm_id = ProcessManagement::createSharedMemory(1234, 10000 * sizeof(int));
        // Attach the shared memory (for now)
        std::vector<int> *database = (std::vector<int> *)ProcessManagement::getSharedMemory(shm_id);
        // open the file
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

            // Split the line by commas
            std::string s;
            std::stringstream ss(line);

            while (!ss.eof())
            {
                getline(ss, s, ',');
                if (stoi(s) != 0 && stoi(s) != 9999)
                {
                    database->push_back(std::stoi(s));
                }
            }
        }
        return shm_id;
    }

    void markStudent(int studentNumber, int mark)
    {
        sleep((rand() % 10) + 1); //sleep for 1-10 seconds to represent marking
        // Create the output stream
        std::ofstream file;
        try
        {
            file.open("TA.txt", std::ios::app); //open the file in append mode
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
    atexit(normalCleanup);
    // Add a signal handler for when the manager process is terminated (to automatically call cleanup)
    signal(SIGINT, signalCleanup);
    signal(SIGTERM, signalCleanup);
    signal(SIGQUIT, signalCleanup);
    signal(SIGCHLD, handleChildProcessError); // signal for child process error

    // First the student database needs to be loaded into shared memory.
    //*int database_id = TAManagement::loadDatabase("student_database.txt");

    //! Test starts here
    // Create all the processes
    createProcesses(5);
    // Cleanup all the processes - called by OG process
    if (getpid() == MANAGER_PID)
    {
        exit(0);
    }
    sleep(3);
    std::cout << "I am still alive." << std::endl;
}
