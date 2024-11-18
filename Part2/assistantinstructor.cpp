/**
 * This file contains code for the assistant simulator
 * @date November 11th, 2024
 * @author John Khalife, Stavros Karamalis
*/

#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <csignal>
#include <sys/shm.h>
#include "assistantinstructor.hpp"
#include <sys/sem.h>



namespace ProcessManagement {

    //This union is used for passing values to the semctl function
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };

    void createProcesses(int processNum) {
        //create a boolean
        int bool_id = createSharedMemory(1234,sizeof(bool));
        bool* isFinished = (bool*) getSharedMemory(bool_id);
        *isFinished = false;

        int pid; //for checking the process id
        for (int i = 0 ; i < processNum ; i++) {
            pid = fork(); //create the new process

            //Make sure the child process exits the loop so there is no exponential growth of processes
            if (pid == 0) {
                //wipe the shared memory and process sets so that the created process can have its own children and shared memory
                processSet.clear();
                shmSet.clear();
                isFinished = (bool*) getSharedMemory(bool_id); //attach this value to the process
                break;
            } else if (pid < 0) {
                //This means the process was unsuccessful in being created.
                std::cout<< "The " << i + 1 << "th process failed to be created." << std::endl;
                exit(1);
            } else {
                std::cout << "Created process " << i << " with pid " << pid << "." << std::endl;
                processSet.insert(pid);
                //Now check if the loop is over
                if (i == processNum - 1) {
                    (*isFinished) = true;
                }
            }
        }

        //All created processes will wait until every process has been created. Otherwise, terminate all of them.
        while(!(*isFinished)) {}
        detachSharedMemory(isFinished);
        deallocateSharedMemory(bool_id);
    }

    int createSharedMemory(int key, int size) {
        int shm_id = shmget(key,size, IPC_CREAT | 0666); 
        if (shm_id < 0) {perror("Failed shared memory allocation.");}
        shmSet.insert(shm_id);
        return shm_id;
    }

    void* getSharedMemory(int shm_id) {
        void* ptr = shmat(shm_id,NULL,0);
        //error handling
        if (ptr == NULL) {
            perror("Failed to attach memory");
        }
        return ptr;
    }


    void deallocateSharedMemory(int shmid) {
        shmctl(shmid, IPC_RMID, NULL); //queue for deallocation
        //Make sure that the id specified is in the shmset before attempting to remove
        if (shmSet.find(shmid) != shmSet.end()) {
            shmSet.erase(shmid);
        }
    }

    void detachSharedMemory(void* ptr) {
        //unmap the memory from it
        if (shmdt(ptr) == -1) {
        perror("shmdt failed");
        } 
    }

    void terminateProcess(pid_t pid) {
        if (kill(pid, SIGTERM) == -1) {
            perror("Failed to terminate process");
        } else {
            std::cout << "Process " << pid << " terminated successfully." << std::endl;
        }
    }

    int createSemaphore(int key, int initialValue) {
        int sem_id = semget(key, 1, IPC_CREAT | 0666); //actually makes the sesmaphore
        if (sem_id < 0) {
            perror("Failed to create semaphore");
        } else {
            union semun arg;
            arg.val = initialValue;
            if (semctl(sem_id, 0, SETVAL, arg) < 0) {
                perror("Failed to set semaphore value");
            }
        }
        semSet.insert(sem_id); //add the semaphore to the set
        return sem_id;
    }

    void incrementSemaphore(int sem_id) {
        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = 1;
        sb.sem_flg = 0;
        if (semop(sem_id, &sb, 1) < 0) {
            perror("Failed to increment semaphore");
        }
    }

    void decrementSemaphore(int sem_id) {
        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = -1;
        sb.sem_flg = 0;
        if (semop(sem_id, &sb, 1) < 0) {
            perror("Failed to decrement semaphore");
        }
    }

    void removeSemaphore(int sem_id) {
        semctl(sem_id, 0, IPC_RMID);
        if (semSet.find(sem_id) != semSet.end()) {
            semSet.erase(sem_id);
        }
    }


    void cleanup() {
        //Start by iterating through the process set.
        //This set should contain all of the children processes created by the calling process.
        for (pid_t i : processSet) {
            terminateProcess(i);
        }
        //Then deallocate all shared memory
        for (int i : shmSet) {
            deallocateSharedMemory(i);
        }
        //Finally remove all semaphores
        for (int i : semSet) {
            semctl(i, 0, IPC_RMID);
        }
    }



}


int main(int argc, char* argv[]) {
    /*
     * My Idea for the structure
     - The original process acts as the manager for all the TA processes.
     - It will be responsible for ending the program.
    */

    //Save the controller process id
    const pid_t ORIGINAL_PID = getpid();

    //Create all the processes
    ProcessManagement::createProcesses(5);

    //Test by printing
    std::cout<< "I am process " << getpid() << "." << std::endl;

    //Cleanup all the processes - called by OG process
    if (getpid() == ORIGINAL_PID) {
        ProcessManagement::cleanup();
        exit(0);
    }

    //Only the created processes will end up calling this
    //They should be terminated when the OG process calls cleanup.
    //Then the OG process goes to the exit call and everything ends.
    while(true) {
        std::cout << "I am still alive." << std::endl;
    }
}

