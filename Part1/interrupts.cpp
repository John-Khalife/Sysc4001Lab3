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

namespace Parsing
{

    vector<MemoryStructures::PcbEntry*> loadPCBTable(string fileName)
    {
        vector<MemoryStructures::PcbEntry*> pcbTable;
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
            MemoryStructures::PcbEntry* newProcess = new MemoryStructures::PcbEntry();
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
                newProcess->pid = stoi(s);
                getline(ss, s, ' ');
                newProcess->memorySize = stoi(s);
                getline(ss, s, ' ');
                newProcess->arrivalTime = stoi(s);
                getline(ss, s, ' ');
                newProcess->totalCPUTime = stoi(s);
                getline(ss, s, ' ');
                newProcess->ioFrequency = stoi(s);
                getline(ss, s, ' ');
                newProcess->ioDuration = stoi(s);
                newProcess->currentState = MemoryStructures::ProcessState::NOT_ARRIVED;
                newProcess->currentTime = 0;
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
}

namespace Execution
{
    using namespace MemoryStructures;

    Partition* reserveMemory(Partition *memory, uint size, pcb_t* process)
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
                    return &memory[i];
                }
            }
        }
        return nullptr;
    } 

    void evaluateMemory(vector<pcb_t*>& pcb, Partition* memory) {
        processCleanup(pcb,memory); //clean up the processes that have finished execution

        vector<pcb_t*> loadableProcesses; //holds all loadable processes.
        //Iterate through every single process
        for (pcb_t* p : pcb) {
            //Check to see if it has not arrived and if the time is ready for arrival
            if (p->currentState == NOT_ARRIVED && p->arrivalTime <= Execution::timer) {
                cout << "Process " << p->pid << " has arrived." << endl;
                p->currentState = NEW; //If so, set the process to new
                loadableProcesses.push_back(p);
            } else if (p->currentState == NEW) {
                loadableProcesses.push_back(p);
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
            ExecutionOrder order = getExecutionOrder(loadableProcesses, true);
            //Load that process into memory reservememory()
            Partition* partition = reserveMemory(memory, order.process->memorySize, order.process);
            if (partition) {
                order.process->memoryAllocated = partition;
            } 
            //Remove that process from the list - doesn't matter if the loading succeeded or failed
            loadableProcesses.erase(std::remove(loadableProcesses.begin(),loadableProcesses.end(), *(order.process)));
        }
    }

    void processCleanup(vector<pcb_t*>& pcb, Partition* memory) {
        //find the processes who have satisfied their execution time.
        for (pcb_t* p : pcb) {
            if (p->currentState == TERMINATED) {
                if (p->memoryAllocated) {
                    p->memoryAllocated->code = -1;
                    p->memoryAllocated = nullptr;
                }
            }
        }
    }

    ExecutionOrder getExecutionOrder(std::vector<pcb_t*>& pcb, bool loadMem) {
        vector<pcb_t*> readyProcesses; //fetch all the ready processes
        for (pcb_t* p : pcb) {
            if (p->currentState == READY) {
                readyProcesses.push_back(p);
            }
        }

        switch (STRAGEGY_USED) {
            case 0:
                return schedulerFCFS(pcb, loadMem);
                break;
            case 1:
                return schedulerEP(pcb, loadMem);
                break;
            case 2:
                return schedulerRR(pcb, loadMem);
                break;
            default:
                return schedulerFCFS(pcb, loadMem);
        }
    }

    void doIO(std::vector<pcb_t*>& pcb, ExecutionOrder* order) {
        if (order->nextState == WAITING) {
            order->process->currentTime = 0;
        }
        order->process->currentState = order->nextState;
        
        //Increment waiting time for all proceses in the waiting state. Move to ready if their time has completed.
        for (pcb_t* p : pcb) {
            if (p->currentState == WAITING) {
                p->currentTime += order->time;
                if (p->currentTime >= p->ioDuration) {
                    p->currentState = READY;
                    p->currentTime = 0;
                }
            }
        }
    }

    bool processesRemain(vector<pcb_t*>& pcb) {
        for (pcb_t* p : pcb) {
            if (p->currentState != TERMINATED) {
                return true;
            }
        }
        return false;
    }

    ExecutionOrder schedulerFCFS(vector<pcb_t*>& pcb, bool loadMem) {
        ProcessState currentState = READY;
        ExecutionOrder order;
        
        if (loadMem) {
            currentState = NEW;
        }
        
        pcb_t* firstProcess = pcb.front();
        bool processFound = false;
        for (pcb_t* i : pcb) {
            if (i->currentState == currentState) {
                processFound = true;
                if (i->arrivalTime < firstProcess->arrivalTime) {
                    firstProcess = i;
                }
            }
        }
        if (processFound)
        {
            order.process = firstProcess;
            order.time = firstProcess->ioFrequency;
            return order;
        }
        return order;
        
    }


    ExecutionOrder schedulerEP(vector<pcb_t*>& pcb, bool loadMem) {
        ProcessState currentState = READY;
        ExecutionOrder order;
        
        if (loadMem) {
            return schedulerFCFS(pcb, loadMem);
        }

        pcb_t* firstProcess = pcb.front();
        bool processFound = false;
        for (pcb_t* i : pcb) {
            if (i->currentState == currentState) {
                if (i->priority < firstProcess->priority) {
                    firstProcess = i;
                }
            }
        }
        if (processFound)
        {
            order.process = firstProcess;
            order.time = firstProcess->ioFrequency;
            return order;
        }
        return order;
    }


    ExecutionOrder schedulerRR(vector<pcb_t*>& pcb, bool loadMem) {       
        ExecutionOrder order;
        if (loadMem) {
            return schedulerFCFS(pcb, loadMem);
        }
        for (pcb_t* i : pcb) {
            if (i->currentState == READY) {
                pcb.push_back(i);
                for (int j = 0 ; j < pcb.size() ; j++) {
                    if (pcb[j]->pid == i->pid) {
                        pcb.erase(pcb.begin() + j);
                        break;
                    }
                }
                return order;
                
            }
        }
        return order;
    }


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

    void writeMemoryStatus(MemoryStructures::Partition *memory)
    {
        if (memoryStatusOutput.fail())
        {
            return;
        }
        
    }

    void doExecution(MemoryStructures::ExecutionOrder* order)
    {   
        //Set to running
        order->process->currentState = MemoryStructures::RUNNING;
        cout << "Process " << order->process->pid << " is now running." << endl;

        //do the execution
        order->process->currentTime += order->time;
        //Check the next state
        if (order->nextState == MemoryStructures::WAITING) {
            cout << "Process " << order->process->pid << " is now waiting." << endl;
        }
        order->process->currentState = order->nextState;
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
    //Print the headers of both files
    //Execution output header
    Execution::executionOutput << "+------------------------------------------------+" << std::endl;
    Execution::executionOutput << "|Time of Transition |PID | Old State | New State |" << std::endl;
    Execution::executionOutput << "+------------------------------------------------+" << std::endl;
    //Memory Status output header
    Execution::memoryStatusOutput << "+------------------------------------------------------------------------------------------+" << std::endl; 
    Execution::memoryStatusOutput << "| Time of Event | Memory Used | Partitions State | Total Free Memory | Usable Free Memory |" << std::endl;
    Execution::memoryStatusOutput << "+------------------------------------------------------------------------------------------+" << std::endl;

    // Initialize memory partitions with the proper sizes.
    using namespace MemoryStructures;
    
    cout << "Initializing memory partitions" << endl;
    Partition *memory = new Partition[PARTITION_NUM];
    if (memory == nullptr) {
        cout << "Failed to allocate memory" << endl;
        return 1;
    }
    for (int i = 0; i < PARTITION_NUM; i++)
    {
        memory[i] = (Partition){.partitionNum = __uint8_t(i + 1), .size = __uint8_t(PARTITION_SIZES[i]), .code = -1};
    }

    vector<pcb_t*> pcb = Parsing::loadPCBTable(argv[1]);  // Initialize pcb entry
    cout << "Loaded PCB Table: " << endl;
    for (pcb_t* p : pcb) {
        cout << "PID: " << p->pid << " Memory Size: " << p->memorySize << " Arrival Time: " << p->arrivalTime << " Total CPU Time: " << p->totalCPUTime << " IO Frequency: " << p->ioFrequency << " IO Duration: " << p->ioDuration << endl;
    }
    // Loop continues while there are still processes to run in the pcb.
    // Get the process that should be run right now. from the pcb
    // Start executing that and continue to check for the best process to run.
    while (Execution::processesRemain(pcb))
    {
       Execution::evaluateMemory(pcb,memory); //handles Memory loading and unloading
       //Execution::doExecution(); //handles the CPU
    }

    //End the output files
    Execution::memoryStatusOutput << "+------------------------------------------------------------------------------------------+" << std::endl;
    Execution::executionOutput << "+------------------------------------------------+" << std::endl;

    // Cleanup
    delete[] memory;
    // All other structs are deallocated when vector activates their deconstructors.
    return 0;
}
