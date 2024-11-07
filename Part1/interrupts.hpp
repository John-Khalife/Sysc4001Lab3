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
    const int SMALLEST_PID = 11; //This holds the smallest pid

    //This structure represents a single partition
    struct Partition {
        __uint8_t partitionNum;
        __uint8_t size;
        std::string code;
    } typedef part_t;

    //This structure represents a single PCB entry.
    struct PcbEntry {
        __uint64_t pid;
        std::string programName;
        __uint8_t partitionNum; 
        __uint8_t memoryAllocated;
        std::streampos fpos; //This is essentially the PC
        bool doExec; //hidden variable
        bool isRunning; //hidden variable
        
        PcbEntry(__uint64_t pid_v,
            std::string programName_v,
            __uint8_t partitionNum_v,
            __uint8_t memoryAllocated_v,
            std::streampos fpos_v,
            bool doExec_v,
            bool isRunning_v)
            :
            pid(pid_v),
            programName(programName_v),
            partitionNum(partitionNum_v),
            memoryAllocated(memoryAllocated_v),
            fpos(fpos_v),
            doExec(doExec_v),
            isRunning(isRunning_v){}
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

    const int MAX_PARAMETERS = 2; // Constant that holds the maximum number of parameters a command can have.
    const char* PROGRAMS_LIST_FILE_NAME = "external_files.txt";
    std::ifstream input; //This is the object to be used for intput.
    std::string traceName; //name of the trace given.

    // If ever a new instruction needs to be added - add the equivalent string here
    namespace orders {
        using namespace std;
        const string CPU = "CPU";
        const string SYSCALL = "SYSCALL";
        const string END_IO = "END_IO";
        const string FORK = "FORK";
        const string EXEC = "EXEC";
    }

    //This structure holds parameters
    struct Parameter {
        bool isString; //This is a type tag
        union {
            int number;
            char word[20];
        };

        Parameter() {isString = false;}
    };

    //This struct holds the contents of an instruction including command and arguments
    struct Instruction{
        Parameter args[MAX_PARAMETERS];
        std::string commandName;
    } typedef instr;

    /**
     * This function reads from a trace given and returns an instruction each time it is called.
     * @param file - an ifstream object that provides access to the trace.
     * @return an instruction object containing the neccessary information for the CPU to execute a command.
    */
    instr* readFromTrace();

    /**
     * This function converts an integer to a hexidecimal string.
     * @param number - an integer that will be converted.
     * @return a hexidecimal string.
    */
    std::string integerToHexString(int number);

    /**
     * This function takes a extFile empty node and initializes a linked list of files in memory.
     * @param head - this is a null node that gets initialized by the function.
    */
    void readExtFiles(std::vector<MemoryStructures::extFile>& files);

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
    std::ofstream output; //keep track of the output file - otherwise would have to pass to every single function in execution.
    const char* PCB_OUTPUT_FILE_NAME = "system_status.txt";

    /**
     * This method sets the output file for execution
     * @param filename - the name of the primary output file
    */
    bool setOutputFile(std::string filename);

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