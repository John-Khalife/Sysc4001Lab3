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



namespace ProcessManagement {
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
    }

}


int main(int argc, char* argv[]) {
    //Save the controller process id
    const pid_t ORIGINAL_PID = getpid();

    //Create all the processes
    ProcessManagement::createProcesses(5);

    //Test by printing
    std::cout<< "I am process " << getpid() << "." << std::endl;

    //Cleanup all the processes
    if (getpid() == ORIGINAL_PID) {
        ProcessManagement::cleanup();
        exit(0);
    }

    //Only the created processes will end up calling this
    //They should be terminated when the OG process calls cleanup.
    //Then the OG process goes to the exit command and everything ends.
    while(true) {
        std::cout << "I am still alive." << std::endl;
    }
}

