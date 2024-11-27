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
#include <unordered_map>
#include <deque>

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
        NOT_ARRIVED,
        NEW,
        READY,
        RUNNING,
        WAITING,
        TERMINATED
    };

    //This structure represents a single PCB entry.
    struct PcbEntry {
        uint pid;
        uint memorySize;
        uint arrivalTime;
        uint totalCPUTime;
        uint ioFrequency;
        uint ioDuration;
        part_t* memoryAllocated;
        uint waitedTime;
    } typedef pcb_t;

    //This structure represents an execution order
    //It is responsible for stating what process should be executed and for how long
    struct ExecutionOrder {
        pcb_t* process;
        int time;
        ExecutionOrder() : process(nullptr), time(0) {}
    };

    std::string stateName(ProcessState state) {
        switch (state) {
            case NEW:
                return "NEW";
            case READY:
                return "READY";
            case WAITING:
                return "WAITING";
            case RUNNING:
                return "RUNNING";
            case TERMINATED:
                return "TERMINATED";
            case NOT_ARRIVED:
                return "NOT_ARRIVED";
            default:
                return "UNKNOWN";
        }
    }
}

//These functions and structures are responsible for getting input for the program and parsing it.
namespace Parsing {
    const int ARGUMENT_NUM = 3; // The number of arguments allowed in the program + 1

    /**
     * This function reads from a given input data text file and returns a pcb table.
     * @param fileName - a fileName to read the input from
     * @return a pcbEntry vector containing the entire pcb.
    */
    std::deque<MemoryStructures::PcbEntry*> loadPCBTable(std::string fileName);

    /**
     * This method takes the filename of the input file given, and grabs all student ids. it does this so that
     * the output files can have the same ids and can be correlated with each other.
     * @param fileName - the filename that was passed to the script before running
     * @return A list of student numbers to be put into the output files.
    */
    std::deque<std::string> grabStudentNumbers(std::string fileName);

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
    int strategyUsed = 0; //The strategy used for the scheduler
    const int QUANTUM = 100; //The time quantum for the round robin scheduler
    const int NUM_STATES = 6; //The number of states in the program

    using namespace MemoryStructures;

    /**
     * This function reserves the memory. by best fit
     * @param memory - pointer to the memory object
     * @param partitionNum - parition number
     * @param programName - what the partition is reserved for.
     * @return - a boolean stating whether or not the memory was reserved.
     * @
    */
    bool reserveMemory(Partition *memory, uint size, pcb_t* process);

    /**
     * This function evaluates the memory and decides what processes to load into main memory. 
     * It also is responsible for changing state from NOT_ARRIVED to NEW and NEW to ready.
     * @param pcb - the pcb table
     * @param memory - the memory array
    */
    void loadMemory(std::deque<pcb_t*>* pcb, Partition* memory);

    /**
     * This function is responsible for returning an execution order. It states what process should run and for how long.
     * @param pcb - the pcb table containing all processes
     * @loadMem - a boolean stating whether or not to load memory
     * @return an execution order.
    */
    ExecutionOrder getExecutionOrder(std::deque<pcb_t*>& pcb, bool loadMem);

    /**
     * This function is responsible for returning true if there are still processes to run.
     * @param pcb - the pcb table
     * @param memory - the memory array
     * @return true if there are processes to run, false otherwise.
     */
    bool processesRemain(std::deque<pcb_t*>* pcb, Partition* memory);

    /**
     * This function is responsible for executing the first come first serve
     * scheduling algorithm
     * @param pcb - the pcb table
    */
    ExecutionOrder schedulerFCFS(std::deque<pcb_t*>& pcb);

    /**
     * This function is responsible for executing the external priority scheduling algorithm
     * @param pcb - the pcb table
     * @param loadMem - a boolean stating whether or not to load memory
    */
    ExecutionOrder schedulerEP(std::deque<pcb_t*>& pcb, bool loadMem);

    /**
     * This function is responsible for executing the round robin scheduling algorithm
     * @param pcb - the pcb table
    */
    ExecutionOrder schedulerRR(std::deque<pcb_t*>& pcb);

    /**
     * This method sets the output file for execution
     * @param executionFileName - the name of the primary output file
     * @param memoryStatusFileName - the name of the secondary output file
    */
    void setOutputFiles(std::string executionFileName, std::string memoryStatusFileName);

    /**
     * This method sets the strategy used for the scheduler 
     * @param strategy - the strategy used
    */
    void setStrategyUsed(std::string strategy);

    /**
     * This method writes the memory status to the output file
     * @param memAllocated - the memory allocated
     * @param pcb - the pcb table
    */
    void writeMemoryStatus(int memAllocated, std::deque<pcb_t*>* pcb, MemoryStructures::Partition *memory);

    /**
     * This method writes the execution step to the output file
     * @param process - the process to execute
     * @param currentState - the current state of the process
     * @param nextState - the next state of the process
    */
    void writeExecutionStep(pcb_t* process, ProcessState currentState ,ProcessState nextState);

    /**
     * This method is intended to do one execution cycle of a process.
     * @param pcb - the pcb table
    */
    void doExecution(std::deque<pcb_t*>* pcb, MemoryStructures::Partition* memory);

    /**
     * This function is responsible for changing the state of a process
     * @param process - the process to change the state of
     * @param initialIndex - the initial index representing the initial state
     * @param finalIndex - the final index representing the final state
     * @return true if the state was changed, false otherwise.
    */
    bool changeState(pcb_t* process, ProcessState initialState, ProcessState finalState, std::deque<MemoryStructures::PcbEntry *>* pcb);

    /**
     * This function is responsible for doing checking if any processes have arrived
     * if they have, move them to new state
     * @param pcb - the pcb table
     * @param memory - the memory array
    */
    void checkArrived(std::deque<pcb_t*>* pcb, Partition* memory);

    /**
     * This function is responsible for checking if a process is done waiting
     * if so, it changes to ready state
     * @param pcb - the pcb table
    */
    void doIO(std::deque<pcb_t*>* pcb);
};
#endif