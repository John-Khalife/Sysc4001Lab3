/**
 * This file contains code for the assistant simulator
 * @date November 11th, 2024
 * @author John Khalife, Stavros Karamalis
*/

#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
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
        if (shm_id < 0) {perror("failed shared memory allocation.");}
        shmSet.insert(shm_id);
        return shm_id;
    }

    void* getSharedMemory(int shm_id) {
        if (shm_id < 0) {perror("failed to aquire.");}
        void* ptr = shmat(shm_id,NULL,0);
        return ptr;
    }


    void deallocateSharedMemory(int shmid) {
        shmctl(shmid, IPC_RMID, NULL);
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

    void cleanup() {

    }

}

