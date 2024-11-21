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

    void* createSharedMemory(int key, int size)
    {
        int shm_id = shmget(key, size, IPC_CREAT | 0666);
        if (shm_id < 0)
        {
            perror("Failed shared memory allocation.");
        }
        shmSet.insert(std::pair<int, int>(SHM_VALUE, shm_id));
        void *ptr = shmat(shm_id, NULL, 0);
        return ptr;
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
        //Go through all IPC objects and remove them
        for (auto it = shmSet.begin(); it != shmSet.end(); ++it)
        {
            switch (it->first)
            {
                case PROCESS_VALUE:
                    kill(it->second, SIGTERM);
                    std::cout << "Process " << it->second << " terminated successfully." << std::endl;
                    break;
                case SEMAPHORE_VALUE:
                    semctl(it->second, 0, IPC_RMID);
                    break;
                case SHM_VALUE:
                    shmctl(it->second, IPC_RMID, NULL);
                    break;
            }
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
        int* sharedDatabase = (int*) ProcessManagement::createSharedMemory(2222, database.size() * sizeof(int));
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
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    using namespace ProcessManagement;
    using namespace TAManagement;

    // Save the controller process id
    const pid_t MANAGER_PID = getpid();
    std::cout << "Manager process has pid " << MANAGER_PID << std::endl;
    atexit(normalCleanup);
    // Add a signal handler for when the manager process is terminated (to automatically call cleanup)
    signal(SIGINT, signalCleanup);
    signal(SIGTERM, signalCleanup);
    signal(SIGQUIT, signalCleanup);
    
    std::cout << "Creating semaphore..." << std::endl;
    int safety_sem = createSemaphore(4444, 1, 1);

    std::cout << "Loading database..." << std::endl;
    // First the student database needs to be loaded into shared memory.
    int* database = TAManagement::loadDatabase("student_database.txt");
        
    //Next the TA semaphores are created
    std::cout << "Creating semaphores..." << std::endl;
    int ta_sem = createSemaphore(7878, 1, TAManagement::NUM_TA);
    //Then the TAs are created
    std::cout << "Creating TAs..." << std::endl;
    TAState* TAStates = (TAState*) createSharedMemory(123, NUM_TA * sizeof(TAState));

    for (int i = 0 ; i < NUM_TA; i++) {
        //Create a new process for each TA
        createProcess();
        srand(time(NULL) + getpid());
        if (getpid() != MANAGER_PID) {
            semaphoreOperation(safety_sem, 0, -1);
            if (!TAStates[i].pid) {
                TAStates[i] = (TAState){0, getpid(), 1};
            }
            semaphoreOperation(safety_sem, 0, 1);

            //The main line of execution starts here.
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
        // Each TA continues marking until it loops through the database 3 times.
        while (true) {
            //Access the database and choose a student to mark.
            // Decrement the semaphore to prevent more than 2 TAs from database access at once.
            std::cout << "TA " << taNum << " is queued for access to the database." << std::endl;
            semaphoreOperation(ta_sem, taNum, -1);
            if (semctl(ta_sem, nextTaNum, GETVAL) == 0) {
                if (TAStates[taNum].pid < TAStates[nextTaNum].pid) {
                    semaphoreOperation(ta_sem, taNum, 1);
                    std::cout << "TA " << taNum << " is waiting for TA " << nextTaNum << " to finish marking." << std::endl;
                    sleep(rand() % 4 + 1);
                    continue;
                }                
            }
            semaphoreOperation(ta_sem, nextTaNum, -1);
            std::cout << "TA " << taNum << " is has gained access to the database." << std::endl;
            sleep(rand() % 4 + 1);
            //increment the index
            if (database[TAStates[taNum].index] == 9999) {
                TAStates[taNum].index = 1;
                TAStates[taNum].loopNum++;
                std::cout << "TA " << taNum << " has looped through the database " << TAStates[taNum].loopNum << " times." << std::endl;
                if (TAStates[taNum].loopNum == LOOP_NUM) {
                    semaphoreOperation(ta_sem, nextTaNum, 1);
                    semaphoreOperation(ta_sem, taNum, 1);
                    break;
                }
            }
            std::cout << "TA " << taNum << " decided to mark student " << database[TAStates[taNum].index] << std::endl;
            semaphoreOperation(ta_sem, nextTaNum, 1);
            semaphoreOperation(ta_sem, taNum, 1);
            markStudent(database[TAStates[taNum].index], rand() % 100, ta_sem, taNum);
            TAStates[taNum].index++;
        }
        exit(0);
        }
    }
    return 0;
}
