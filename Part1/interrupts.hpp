/**
 * This file contains code for the interrupt simulator
 * @date September 30th, 2024
 * @author John Khalife, Stavros Karamalis
*/

#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

//dependencies
#include <iostream>
#include <fstream>
#include <string>

//This holds all of the memory structures used in this program
namespace MemoryStructures {
    const int PARTITION_SIZES[] = {40,25,15,10,8,2};
    const int PARTITION_NUM = 6; 

    //This structure represents a single partition
    struct Partition {
        uint partitionNum;
        uint size;
        int code; //holds the PID
    } typedef part_t;

    //These hold the current process state
    enum ProcessState {
        NEW,
        READY,
        WAITING,
        RUNNING,
        TERMINATED,
        NOT_ARRIVED
    };

    //This structure represents a single PCB entry.
    struct PcbEntry {
        uint pid;
        uint priority;
        uint memorySize;
        uint arrivalTime;
        uint totalCPUTime;
        uint ioFrequency;
        uint ioDuration;
        part_t* memoryAllocated;
        ProcessState currentState;
        uint currentTime;

    } typedef pcb_t;

    //This structure represents an execution order
    //It is responsible for stating what process should be executed and for how long
    struct ExecutionOrder {
        pcb_t* process;
        int time;
        ProcessState nextState;
    };
}

//These functions and structures are responsible for getting input for the program and parsing it.
namespace Parsing {
    const int ARGUMENT_NUM = 2; // The number of arguments allowed in the program

    /**
     * This function reads from a given input data text file and returns a pcb table.
     * @param fileName - a fileName to read the input from
     * @return a pcbEntry vector containing the entire pcb.
    */
    std::vector<MemoryStructures::PcbEntry*> loadPCBTable(std::string fileName);

    /**
     * This method takes the filename of the input file given, and grabs all student ids. it does this so that
     * the output files can have the same ids and can be correlated with each other.
     * @param fileName - the filename that was passed to the script before running
     * @return A list of student numbers to be put into the output files.
    */
    std::vector<std::string> grabStudentNumbers(std::string fileName);

    /**
     * This method takes a filename and returns the name of an output file
     * @param prefix - the prefix to add before the student names
     * @param fileName - the name of the input file
     * @return a string containing the name of the execution file
    */
    std::string getOutputFilename(std::string prefix, std::string fileName);
};

//All functions in this namespace are responsible for execution
namespace Execution {
    int timer = 0; //Necessary for keeping track of the program time over multiple functions within execution
    std::ofstream executionOutput; //output object for the main output file
    std::ofstream memoryStatusOutput; //output object for the memoryStatus.
    const int STRAGEGY_USED = 0; //The strategy used for the scheduler
    const int QUANTUM = 100; //The time quantum for the round robin scheduler

    using namespace MemoryStructures;

    /**
     * This function reserves the memory. by best fit
     * @param memory - pointer to the memory object
     * @param partitionNum - parition number
     * @param programName - what the partition is reserved for.
     * @return - a partition pointer
     * @
    */
    Partition* reserveMemory(Partition *memory, uint size, pcb_t* process);

    /**
     * This function evaluates the memory and decides what processes to load into main memory. 
     * It also is responsible for changing state from NOT_ARRIVED to NEW and NEW to ready.
     * @param pcb - the pcb table
     * @param memory - the memory array
    */
    void evaluateMemory(std::vector<pcb_t*>& pcb, Partition* memory);

    /**
     * This function is intended for checking when a process should be terminated, as well as deloading it from memory.evaluateMemory
     * @param pcb
     * @param memory - the memory array
    */
    void processCleanup(std::vector<pcb_t*>& pcb, Partition* memory);

    /**
     * This function is responsible for returning an execution order. It states what process should run and for how long.
     * @param pcb - the pcb table containing all processes
     * @loadMem - a boolean stating whether or not to load memory
     * @return an execution order.
    */
    ExecutionOrder getExecutionOrder(std::vector<pcb_t*>& pcb, bool loadMem);

    /**
     * This function is responsible for simulating IO. It transitions out of and into the waiting state
     * @param pcb - the pcb table
     * @param order - the execution order
    */
    void doIO(std::vector<pcb_t*>& pcb, ExecutionOrder* order);

    /**
     * This function is responsible for returning true if there are still processes to run.
     * @param pcb - the pcb table
     * @return true if there are processes to run, false otherwise.
     */
    bool processesRemain(std::vector<pcb_t*>& pcb);

    /**
     * This function is responsible for executing the first come first serve
     * scheduling algorithm
     * @param pcb - the pcb table
     * @param loadMem - a boolean stating whether or not to load memory
    */
    ExecutionOrder schedulerFCFS(std::vector<pcb_t*>& pcb, bool loadMem);

    /**
     * This function is responsible for executing the external priority scheduling algorithm
     * @param pcb - the pcb table
     * @param loadMem - a boolean stating whether or not to load memory
    */
    ExecutionOrder schedulerEP(std::vector<pcb_t*>& pcb, bool loadMem);

    /**
     * This function is responsible for executing the round robin scheduling algorithm
     * @param pcb - the pcb table
     * @param loadMem - a boolean stating whether or not to load memory
    */
    ExecutionOrder schedulerRR(std::vector<pcb_t*>& pcb, bool loadMem);

    /**
     * This method sets the output file for execution
     * @param executionFileName - the name of the primary output file
     * @param memoryStatusFileName - the name of the secondary output file
    */
    void setOutputFiles(std::string executionFileName, std::string memoryStatusFileName);

    /**
     * This method writes the memory status to the output file
     * @param memAllocated - the memory allocated
     * @param pcb - the pcb table
    */
    void writeMemoryStatus(int memAllocated, std::vector<pcb_t*>& pcb, MemoryStructures::Partition *memory);

    /**
     * This method writes the execution step to the output file
     * @param order - the execution order
    */
    void writeExecutionStep(ExecutionOrder order);

    /**
     * This method is intended to do one execution cycle of a process.
    */
    void doExecute();
};
#endif