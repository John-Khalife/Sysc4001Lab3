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


//This namespace contains simulated memory structures used by the CPU.
namespace MemoryStructures {

    const int PARTITION_SIZES[] = {40,25,15,10,8,2};
    const int PARTITION_NUM = 6; 
    const int QUANTUM = 100; //The time quantum for the round robin scheduler

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
        Partition* memoryAllocated;
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

    /**
     * This function reserves the memory. by best fit
     * @param memory - pointer to the memory object
     * @param partitionNum - parition number
     * @param programName - what the partition is reserved for.
     * @return - a partition number
     * @
    */
    int reserveMemory(Partition *memory, uint size, pcb_t process);

    /**
     * This function evaluates the memory and decides what processes to load into main memory. 
     * It also is responsible for changing state from NOT_ARRIVED to NEW and NEW to ready.
     * @param pcb - the pcb table
     * @param memory - the memory array
    */
    void evaluateMemory(std::vector<pcb_t>& pcb, Partition* memory);

    /**
     * This function is intended for checking when a process should be terminated, as well as deloading it from memory.evaluateMemory
     * @param pcb
     * @param memory - the memory array
    */
    void processCleanup(std::vector<pcb_t>& pcb, Partition* memory);

    /**
     * This function is responsible for returning an execution order. It states what process should run and for how long.
     * @param pcb - the pcb table containing all processes
     * @return an execution order.
    */
    ExecutionOrder getExecutionOrder(std::vector<pcb_t>& pcb);

    /**
     * This function serves to get the process that needs to be executed.
     * @param pcb a pointer to the PCB
     * @return a pointer to the running process
    */
    pcb_t* getRunningProcess(std::vector<pcb_t>& pcb);

    /**
     * This function is responsible for simulating IO. It transitions out of and into the waiting state
     * @param pcb - the pcb table
     * @param order - the execution order
    */
    void doIO(std::vector<pcb_t>& pcb, ExecutionOrder* order);

    /**
     * This function is responsible for returning true if there are still processes to run.
     * @param pcb - the pcb table
     * @return true if there are processes to run, false otherwise.
     */
    bool processesRemain(std::vector<pcb_t>& pcb);



}


//These functions and structures are responsible for Parsing.
namespace Parsing {
    const int ARGUMENT_NUM = 1; // The number of arguments allowed in the program

    // If ever a new instruction needs to be added - add the equivalent string here
    namespace orders {
        using namespace std;
        const string CPU = "CPU";
        const string SYSCALL = "SYSCALL";
        const string END_IO = "END_IO";
        const string FORK = "FORK";
        const string EXEC = "EXEC";
    }

    /**
     * This function reads from a given input data text file and returns a pcb table.
     * @param fileName - a fileName to read the input from
     * @return a pcbEntry vector containing the entire pcb.
    */
    std::vector<MemoryStructures::PcbEntry> loadPCBTable(std::string fileName);

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

    /**
     * This function converts an integer to a hexidecimal string.
     * @param number - an integer that will be converted.
     * @return a hexidecimal string.
    */
    std::string integerToHexString(int number);

    /**
     * helper method to get the number of digits in a number
     * @param number - the number to get the # of digits from
     * @return the number of digits
    */
    int numDigits(int number);
};

//All functions in this namespace are responsible for execution
namespace Execution {
    int timer = 0; //Necessary for keeping track of the program time over multiple functions within execution
    std::ofstream executionOutput; //output object for the main output file
    std::ofstream memoryStatusOutput; //output object for the memoryStatus.

    /**
     * This method sets the output file for execution
     * @param executionFileName - the name of the primary output file
     * @param memoryStatusFileName - the name of the secondary output file
    */
    void setOutputFiles(std::string executionFileName, std::string memoryStatusFileName);

    /**
     * This method prints the toString method of the PCB to a file
     * @param pcb the pcb structure
    */
    void writePcbTable(std::vector<MemoryStructures::pcb_t> pcb);

    /**
     * This method represents the CPU instruction that can be given from the trace.
     * @param duration - an integer representing the duration of the command.
    */
    void executeCPU(int duration);

    /**
     * This method is intended to process interrupts given by an I/O device connected to the CPU.
     * @param duration - an integer representing the duration of the command.
     * @param isrAddress - an integer representing the address of the ISR address in the vector table
    */
    void interrupt(int duration,int isrAddress);

    /**
     * This method is meant to be used to access the vector table given an address and will output the ISR address it found.
     * @param isrAddress - an integer representing the address of the ISR address in the vector table
    */
    void accessVectorTable(int isrAddress);

    /**
     * Method used to write CPU events to the output file
     * @param duration - An integer stating the timer taken for the CPU to complete the action
     * @param eventType - A string dictating the action of the CPU
     * 
    */
    void writeExecutionStep(int duration, std::string eventType);

    /**
     * This method is intended to be used for a system call - it checks the input device.
     * @param duration - An integer stating the timer taken for the CPU to complete the action
     * @param isrAddress - An integer stating the memory address in the vector table for the ISR.
    */
    void systemCall(int duration, int isrAddress);

    /**
     * This method is intended to do one execution cycle of a process.
    */
    void doExecute();
};
#endif