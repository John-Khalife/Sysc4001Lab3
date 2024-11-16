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

    //This structure represents a single partition
    struct Partition {
        uint partitionNum;
        uint size;
        int code; //holds the PID
    } typedef part_t;

    //This structure represents a single PCB entry.
    struct PcbEntry {
        uint pid;
        uint memorySize;
        uint arrivalTime;
        uint totalCPUTime;
        uint ioFrequency;
        uint ioDuration;
        std::string programName;
        __uint8_t partitionNum; 
        __uint8_t memoryAllocated;
    } typedef pcb_t;

    //This structure represents a file in persistent memory
    struct extFile {
        char programName[21];
        __uint128_t size;
    };

    /**
     * This method is intended to be used to add on a new process to the pcb.
     * @param pcb - a reference to the vector
     * @param index - the index of the entry to be copied.
    */
    void copyPCBEntry(std::vector<pcb_t>& pcb ,int index);

    /**
     * This method modifies a pcb entry, used for the exec command.
     * @param head
     * @param pid
     * @param programName
     * @param partitionNum
     * @param memoryAllocated
     * @param filename
    */
    void modifyPCBEntry(
        std::vector<pcb_t>& pcb,
        int index,
        char programName[20],
        __uint8_t partitionNum,
        __uint128_t memoryAllocated);

    /**
     * This function reserves the memory.
     * @param memory - pointer to the memory object
     * @param partitionNum - parition number
     * @param programName - what the partition is reserved for.
     * @return - a partition number
     * @
    */
    int reserveMemory(Partition* memory, __uint8_t size, std::string programName);

    /**
     * This function searches the file linked list for the size of a file
     * @param head - the structure containing all files
     * @param programName - the name of the program
     * @return an integer representing the size of the program.
    */
   __uint8_t getFileSize(std::shared_ptr<extFile>& head, char* programName);

    /**
     * This function serves to get the process that needs to be executed.
     * @param pcb a pointer to the PCB
     * @return a pointer to the running process
    */
    pcb_t* getRunningProcess(std::vector<pcb_t>& pcb);
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
     * This method is intended to handle the fork instruction 
     * @param duration - An integer stating the time taken for the CPu to complete the action
    */
    void fork(int duration, std::vector<MemoryStructures::pcb_t>& pcb, int index);

   /**
    * This method handles the execute instruction
    * @param filename - a string representing the file name
    * @param duration - An integer stating the time taken for the CPU to complete the action
   */
    void exec(char* filename, int duration, 
        std::vector<MemoryStructures::pcb_t>& pcb,
        std::vector<MemoryStructures::extFile>& files,
        MemoryStructures::Partition* memory,
        int index);

    /**
     * This method is used to call the appropriate function based on the instrcution given.
     * @param instruction - a instr struct that contains the command and any parameters it may have
    */
    void executeInstruction(
        Parsing::instr* instruction,
        std::vector<MemoryStructures::pcb_t>& pcb,
       std::vector<MemoryStructures::extFile>& files,
        MemoryStructures::Partition* memory,
        int index);
};
#endif