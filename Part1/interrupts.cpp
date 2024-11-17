/**
 * This is the definition file for the methods belonging to the CPU class.
 * @date September 30th, 2024
 * @author John Khalife, Stavros Karamalis
 */

#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <string.h>
#include <iomanip>
#include <memory>
#include <cstdint>

#include "interrupts.hpp"

using namespace std;

namespace MemoryStructures
{
    int reserveMemory(Partition *memory, uint size, pcb_t* process)
    {
        //*NOTE Partition sizes are ordered from largest to smallest - so best fit will be easy.
        //*That means however, this method may need to be updated in the future if different partitions are given.
        for (int i = PARTITION_NUM - 1; i >= 0; i--)
        {
            if (memory[i].code == -1)
            {
                if (size <= memory[i].size)
                {
                    memory[i].code = process->pid;
                    process->currentState = READY; // now that it is loaded, set the state to ready.
                    return i;
                }
            }
        }
        return -1;
    }

    pcb_t *getRunningProcess(vector<pcb_t> &pcb)
    {
        for (int i = pcb.size(); i >= 0; i--)
        {
            if (pcb[i].currentState == RUNNING)
            {
                return &pcb[i];
            }
        }
        return NULL;
    }

    void evaluateMemory(vector<pcb_t>& pcb, Partition* memory) {
        vector<pcb_t*> loadableProcesses; //holds all loadable processes.
        //Iterate through every single process
        for (pcb_t p : pcb) {
            //Check to see if it has not arrived and if the time is ready for arrival
            if (p.currentState == NOT_ARRIVED && p.arrivalTime <= Execution::timer) {
                p.currentState = NEW; //If so, set the process to new
                loadableProcesses.push_back(&p);
            } else if (p.currentState == NEW) {
                loadableProcesses.push_back(&p);
            }
        }

        while (!loadableProcesses.empty()) {
            //First check if there is space
            bool isSpace = false;
            for (int i = 0 ; i < PARTITION_NUM ; i++) {
                if (memory[i].code == -1) {
                    isSpace = true;
                }
            }

            if (!isSpace) {
                break; //leave the loop
            }

            //Next decide what processes to load into memory - this is where scheduling strategy comes into play
            //TODO: implement a way to choose what process to load into memory
            
            //Load that process into memory reservememory()
            //Remove that process from the list - doesn't matter if the loading succeeded or failed
        }
    }

    void processCleanup(vector<pcb_t>& pcb, Partition* memory) {
        //find the processes who have satisfied their execution time.
        for (pcb_t b : pcb) {
            if (p.currentState == TERMINATED) {
                if (p.memoryAllocated) {
                    memory[p.memoryPartition].code = -1;
                    p.memoryAllocated = nullptr;
                }
            }
        }
    }

    ExecutionOrder getExecutionOrder(std::vector<pcb_t>& pcb) {
        vector<pcb_t*> readyProcesses; //fetch all the ready processes
        for (pcb_t p : pcb) {
            if (p.currentState == READY) {
                readyProcesses.push_back(&p);
            }
        }
        //TODO: Insert scheduling strategy here - should decide on a process and a time.
        return ExecutionOrder();
    }

    void doIO(std::vector<pcb_t>& pcb, ExecutionOrder* order) {
        if (order->nextState == WAITING) {
            order->process->currentTime = 0;
        }
        order->process->currentState = order->nextState;
        
        //Increment waiting time for all proceses in the waiting state. Move to ready if their time has completed.
        for (pcb_t p : pcb) {
            if (p.currentState == WAITING) {
                p.currentTime += order->time;
                if (p.currentTime >= p.ioDuration) {
                    p.currentState = READY;
                    p.currentTime = 0;
                }
            }
        }
    }

    bool processesRemain(std::vector<pcb_t>& pcb) {
        for (pcb_t p : pcb) {
            if (p.currentState != TERMINATED) {
                return true;
            }
        }
        return false;
    }
}

namespace Parsing
{

    vector<MemoryStructures::PcbEntry> loadPCBTable(string fileName)
    {
        vector<MemoryStructures::PcbEntry> pcbTable;
        ifstream input;
        try {
            input.open(fileName);
        } catch (const exception &e) {
            cerr << e.what() << '\n';
            exit(1);
        }
        while(!input.eof()) 
        {
            // Create a new PcbEntry
            MemoryStructures::PcbEntry newProcess;
            string text;
            getline(input, text);
            for (int i = 0, len = text.size(); i < len; i++)
            {
                if (text[i] == ',')
                {
                    text.erase(i--, 1);
                    len = text.size();
                }
            }

            stringstream ss(text);
            string s;
            try
            {
                getline(ss, s, ' '); // First parameter Pid
                newProcess.pid = stoi(s);
                getline(ss, s, ' ');
                newProcess.memorySize = stoi(s);
                getline(ss, s, ' ');
                newProcess.arrivalTime = stoi(s);
                getline(ss, s, ' ');
                newProcess.totalCPUTime = stoi(s);
                getline(ss, s, ' ');
                newProcess.ioFrequency = stoi(s);
                getline(ss, s, ' ');
                newProcess.ioDuration = stoi(s);
                newProcess.currentState = MemoryStructures::ProcessState::NOT_ARRIVED;
                newProcess.currentTime = 0;
            }
            catch (const exception &e)
            {
                cerr << e.what() << '\n';
                exit(1);
            }
            pcbTable.push_back(newProcess);
        }
        input.close();
        return pcbTable;
    }

    vector<string> grabStudentNumbers(string fileName)
    {
        vector<string> ids;
        stringstream ss(fileName.substr(0, fileName.find_first_of('.')));
        string s;
        while (getline(ss, s, '_'))
        { // while for any args
            bool isStudentId = true;
            // Check if the argument is a string or an integer
            for (int i = 0; i < strlen(s.c_str()); i++)
            {
                if (!isdigit(s[i]))
                {
                    isStudentId = false;
                    break;
                }
            }
            if (isStudentId) {
                ids.push_back(s);
            }
        }
        return ids;
    }

    string getOutputFilename(string prefix, string fileName) {
        vector<string> studentNums = grabStudentNumbers(fileName);
        string outputName = prefix;
        for (string s : studentNums) {
            outputName += ("_" + s);
        }
        outputName += ".txt";
        return outputName;
    }


    string integerToHexString(int number)
    {
        stringstream ss;
        ss << setfill('0') << setw(4) << hex << number;
        return "0x" + ss.str();
    }

    // helper method
    int numDigits(int number)
    {
        if (number == 0)
            return 1;
        return static_cast<int>(log10(abs(number)) + 1);
    }

    /**
     * function to set the input file
     */
    void setInput(string fileName)
    {
        input.close();
        input.open(fileName);
    }
}

namespace Execution
{
    void setOutputFiles(std::string executionFileName, std::string memoryStatusFileName)
    {
        memoryStatusOutput.open(memoryStatusFileName);
        if (memoryStatusOutput.fail())
        {
            cout << "Unable to open memory status output file." << std::endl;
            exit(1);
        }
        executionOutput.open(executionFileName);
        if (executionOutput.fail())
        {
            cout << "Unable to open execution output file." << std::endl;
            exit(1);
        }
    }

    void writeExecutionStep(int duration, string eventType)
    {
        if (executionOutput.fail())
        {
            return;
        }
        executionOutput << timer << ", " << duration << ", " << eventType << endl; // Write the Execution message in the proper format
        timer += duration;                                                // Add the amount of timer to CPU timer for the next write
    }

    void systemCall(int duration, int isrAddress)
    {
        accessVectorTable(isrAddress);

        default_random_engine generator;                                        // generates uniformly distributed numbers
        generator.seed(time(0));                                                // Give the generator a seed
        uniform_real_distribution<> isrDistribution(0, 0.25);                   // probability distribution for 0-25% of the duration.
        int dataTransferTime = (int)duration * isrDistribution(generator) + 1;  // time for data transfer
        int errorCheckingTime = (int)duration * isrDistribution(generator) + 1; // generate time for error checking

        // Call the device driver
        writeExecutionStep(duration - dataTransferTime - errorCheckingTime, "SYSCALL: Execute ISR.");
        writeExecutionStep(dataTransferTime, "Transfer data.");     // transfer data
        writeExecutionStep(errorCheckingTime, "Check for errors."); // Error check
        writeExecutionStep(1, "IRET");                              // Interrupt return.
    }

    void executeCPU(int duration)
    {
        writeExecutionStep(duration, "CPU Execution.");
    }

    void interrupt(int duration, int isrAddress)
    {
        writeExecutionStep(1, "Check priority of interrupt.");
        writeExecutionStep(1, "Check if interrupt is masked.");
        accessVectorTable(isrAddress);
        writeExecutionStep(duration, "END_IO");
        writeExecutionStep(1, "IRET"); // Interrupt return.
    }

    void accessVectorTable(int isrAddress)
    {
        default_random_engine generator;                                 // generates uniformly distributed numbers
        generator.seed(time(0));                                         // Give the generator a seed
        uniform_int_distribution<int> contextSaveTimeDistribution(1, 3); // Create a distribution

        // First switch to kernel mode
        writeExecutionStep(1, "Switch CPU to Kernel mode.");

        using namespace std;
        writeExecutionStep(contextSaveTimeDistribution(generator), "Context saved."); // Save context

        // Check vector table and call ISR
        ifstream file;
        string text;

        writeExecutionStep(1, "Find vector " + to_string(isrAddress) + " in memory position " + Parsing::integerToHexString(isrAddress) + ".");
        file.open("vector_table.txt");
        for (int i = 0; i <= isrAddress; i++)
        {
            getline(file, text);
        }
        file.close();                                                    // Now the text string should contain the ISR.
        writeExecutionStep(1, "Load address " + text + " into the PC."); // output the address being loaded
    }

    void doExecution(MemoryStructures::ExecutionOrder* order)
    {   
        //Set to running
        order->process->currentState = MemoryStructures::RUNNING;
        //TODO: add the change state output here
        //do the execution
        executeCPU(order->time);
        order->process->currentTime += order->time;
        //Check the next state
        if (order->nextState == MemoryStructures::WAITING) {
            interrupt(order->time, 5); //TODO: Figure out what to do here
        }
    }
}

int main(int argc, char *argv[])
{
    // Check to make sure there are arguments
    if (argc != Parsing::ARGUMENT_NUM)
    {
        cout << "There should only be " << Parsing::ARGUMENT_NUM << " argument." << endl;
        return 1;
    }
    //Set the output
    Execution::setOutputFiles(Parsing::getOutputFilename("execution",argv[1]),Parsing::getOutputFilename("memory_status",argv[1]));

    // Initialize memory partitions with the proper sizes.
    using namespace MemoryStructures;
    Partition *memory = new Partition[PARTITION_NUM];
    if (memory == nullptr) {
        cout << "Failed to allocate memory" << endl;
        return 1;
    }
    for (int i = 0; i < PARTITION_NUM; i++)
    {
        memory[i] = (Partition){.partitionNum = __uint8_t(i + 1), .size = __uint8_t(PARTITION_SIZES[i]), .code = -1};
    }

    vector<pcb_t> pcb = Parsing::loadPCBTable(argv[1]);  // Initialize pcb entry

    // Loop continues while there are still processes to run in the pcb.
    // Get the process that should be run right now. from the pcb
    // Start executing that and continue to check for the best process to run.
    ExecutionOrder order = getExecutionOrder(pcb);
    while (processesRemain(pcb))
    {
        evaluateMemory(pcb,memory); //handles NOT_ARRIVED and NEW to READY (loading to memory)
        ExecutionOrder order = getExecutionOrder(pcb);
        Execution::doExecution(&order); //handles READY to RUNNING
        doIO(pcb,&order); //handles RUNNING to WAITING or READY
        processCleanup(pcb,memory); // handles TERMINATION and deloading from memory
    }

    // Cleanup
    delete[] memory;
    // All other structs are deallocated when vector activates their deconstructors.
    return 0;
}
