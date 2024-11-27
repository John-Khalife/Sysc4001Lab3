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
#include <deque>
#include <algorithm>

#include "interrupts.hpp"

using namespace std;

namespace Parsing
{

    deque<MemoryStructures::PcbEntry*> loadPCBTable(string fileName)
    {
        deque<MemoryStructures::PcbEntry*> pcbTable;
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
                newProcess->waitedTime = 0;
            }
            catch (const exception &e)
            {
                cerr << e.what() << '\n';
                exit(1);
            }
            pcbTable.push_front(newProcess);

        }
    
        input.close();
        return pcbTable;
    }

    deque<string> grabStudentNumbers(string fileName)
    {
        deque<string> ids;
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
        deque<string> studentNums = grabStudentNumbers(fileName);
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

    bool reserveMemory(Partition *memory, uint size, pcb_t* process)
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
                    process->memoryAllocated = &memory[i];
                    return true;
                }
            }
        }
        return false;
    } 

    void loadMemory(deque<pcb_t*>* pcb, Partition* memory) {
        //Iterate through every single process in the new state
        while (!pcb[1].empty()) {
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
            ExecutionOrder order = getExecutionOrder(pcb[NEW], true);
            if (order.process == nullptr) {
                break;
            }
            //Load that process into memory reservememory()
            if (reserveMemory(memory, order.process->memorySize, order.process)) {
                changeState(order.process, NEW, READY, pcb);
                writeMemoryStatus(order.process->memorySize,pcb,memory);
            } 
        }
    }

    ExecutionOrder getExecutionOrder(deque<pcb_t*>& pcb, bool loadMem) {
        switch (strategyUsed) {
            case 0:
                return schedulerFCFS(pcb);
            case 1:
                return schedulerEP(pcb, loadMem);
            case 2:
                return schedulerRR(pcb);
            default:
                return schedulerFCFS(pcb);
        }
    }

    bool processesRemain(deque<pcb_t*>* pcb, Partition* memory) {
        bool allNotTerminated = false;
        for (int i = NOT_ARRIVED ; i < TERMINATED ; i++) {
            if (!pcb[i].empty()) {
                allNotTerminated = true;
                break;
            }
        }
        bool memoryNotDeallocated = false;
        for (int i = 0 ; i < MemoryStructures::PARTITION_NUM ; i++) {
            if (memory[i].code != -1) {
                memoryNotDeallocated = true;
                break;
            }
        }
        return (allNotTerminated) || (memoryNotDeallocated); ;
    }

    ExecutionOrder schedulerFCFS(deque<pcb_t*>& pcb) {
        ExecutionOrder order;   
        if (!pcb.empty()) {
            order.process = pcb.front();
            order.time = pcb.front()->ioDuration;
        }
        return order;  
    }



    ExecutionOrder schedulerEP(deque<pcb_t*>& pcb, bool loadMem) {
        ExecutionOrder order;
        if (!pcb.empty()) {
            if (loadMem) {
                return schedulerFCFS(pcb);
            }
            // Sort the pcb by ioFrequency from smallest to largest
            for (int i = 0; i < pcb.size() - 1; i++) {
                for (int j = 0; j < pcb.size() - i - 1; j++) {
                    if (pcb[j]->ioFrequency > pcb[j + 1]->ioFrequency) {
                        pcb_t* temp = pcb[j];
                        pcb[j] = pcb[j + 1];
                        pcb[j + 1] = temp;
                    }
                }
            }
            order.process = pcb.front();
            order.time = pcb.front()->ioDuration;
        }
        return order;
    }

    ExecutionOrder schedulerRR(deque<pcb_t*>& pcb) {       
        ExecutionOrder order;   
        if (!pcb.empty()) {
            order.process = pcb.front();
            order.time = QUANTUM;
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

    void writeExecutionStep(pcb_t* process, ProcessState currentState ,ProcessState nextState)
    {
        if (executionOutput.fail())
        {
            return;
        }
        executionOutput << "| " << std::setw(18) << std::right << timer;
        executionOutput << " | " << std::setw(2) << std::right << process->pid << " | " << std::setw(9) << std::right;
        executionOutput<<  MemoryStructures::stateName(currentState) << " | " << std::setw(9) << std::right << MemoryStructures::stateName(nextState) << " |" << std::endl;
    }

    void writeMemoryStatus(int memAllocated, deque<pcb_t*>* pcb, MemoryStructures::Partition *memory)
    {
        if (memoryStatusOutput.fail())
        {
            return;
        }
        //Get the memory state, total free memory, and usable free memory
        string memoryState = "";
        int totalFreeMemory = 0;
        int usableFreeMemory = 0;
        for (int i = 0; i < PARTITION_NUM; i++)
        {
            if (memory[i].code == -1)
            {
                totalFreeMemory += memory[i].size;
                usableFreeMemory += memory[i].size;
            } else {
                for (int z = READY ; z < TERMINATED ; z++) {
                    for (int j = 0; j < pcb[z].size(); j++)
                    {
                        if (pcb[z].at(j)->pid == memory[i].code)
                        {
                            totalFreeMemory += memory[i].size - pcb[z].at(j)->memorySize;
                            break;
                        }
                    }
                }
            }
            memoryState +=  to_string(memory[i].code);
            if (i != PARTITION_NUM - 1)
            {
                memoryState += ",";
            }
        }
        //| Time of Event | Memory Used | Partitions State | Total Free Memory | Usable Free Memory |
        memoryStatusOutput << "| " << std::setw(13) << std::right << timer;
        memoryStatusOutput << " | " << std::setw(11) << std::right << memAllocated;
        memoryStatusOutput << " | " << std::setw(16) << std::right << memoryState;
        memoryStatusOutput << " | " << std::setw(17) << std::right << totalFreeMemory;
        memoryStatusOutput << " | " << std::setw(18) << std::right << usableFreeMemory;
        memoryStatusOutput << " | " << std::endl;
    }

    void checkArrived(deque<pcb_t*>* pcb, Partition* memory) {
        //Iterate through every single process not arrived process
        for (int i = 0 ; i < pcb[NOT_ARRIVED].size() ; i++) {
            //Check to see if it has not arrived and if the time is ready for arrival
            if (pcb[NOT_ARRIVED].at(i)->arrivalTime <= Execution::timer) {
                //change state without printing
                pcb[NEW].push_back(pcb[NOT_ARRIVED].at(i));
                pcb[NOT_ARRIVED].erase(pcb[NOT_ARRIVED].begin() + i);
                loadMemory(pcb, memory);
            }
        }
    }

    void doIO(deque<pcb_t*>* pcb) {
        //Iterate through every single process in the waiting state
        //Do the IO for any processes that are waiting during this time
        //Increment waiting time for all proceses in the waiting state. Move to ready if their time has completed.
        for (int i = 0 ; i < pcb[WAITING].size() ; i++) {
            pcb[WAITING].at(i)->waitedTime++;
            //Check to see if the process is ready to leave the waiting state
            if (pcb[WAITING].at(i)->ioDuration <= pcb[WAITING].at(i)->waitedTime) {
                pcb[WAITING].at(i)->waitedTime = 0;
                changeState(pcb[WAITING].at(i), WAITING, READY, pcb);
            }
        }
    }

    void doExecution(deque<pcb_t*>* pcb, Partition* memory)
    {   
        // We need to choose a process to run.
        ExecutionOrder order = getExecutionOrder(pcb[READY], false);

        //Check if there is a process to run
        if (order.process != nullptr) {
            //Determine the next state of the process
            ProcessState nextState;
            if (((int) order.process->totalCPUTime - order.time) <= 0) {
                nextState = TERMINATED;
                order.time = order.process->totalCPUTime;
            } else if (order.time >= order.process->ioFrequency) {
                nextState = WAITING;
                order.time = order.process->ioFrequency;
            } else {
                nextState = READY;
            }
            changeState(order.process, READY, RUNNING, pcb);
            //Increment the timer while checking for any processes that have arrived or finished IO
            for (int i = 0 ; i < order.time ; i++) {
                timer += 1;
                order.process->totalCPUTime -= 1;
                checkArrived(pcb,memory);
                doIO(pcb);
            }
            changeState(order.process, RUNNING, nextState, pcb);
            if (nextState == TERMINATED) {
                order.process->memoryAllocated->code = -1;
                order.process->memoryAllocated = nullptr;
                writeMemoryStatus(order.process->memorySize,pcb,memory);
                loadMemory(pcb, memory);
            }
        } else {
            timer += 1;
            checkArrived(pcb,memory);
            doIO(pcb);
        }
    }

    bool changeState(pcb_t* process, ProcessState initialState, ProcessState finalState, deque<pcb_t*>* pcb) {
        //Get the process
        for (int i = 0; i < pcb[initialState].size(); i++) {
            if (pcb[initialState].at(i) == process) {
                pcb[initialState].erase((pcb[initialState].begin() + i));
                pcb[finalState].push_back(process);
                writeExecutionStep(process, initialState, finalState);
                return true;
            }
        }
        return false;
    }

    void setStrategyUsed(std::string strategy) {
        if (strategy == "FCFS") {
            strategyUsed = 0;
        } else if (strategy == "EP") {
            strategyUsed = 1;
        } else if (strategy == "RR") {
            strategyUsed = 2;
        }
    }
}


int main(int argc, char *argv[])
{
    // Check to make sure there are arguments
    if (argc != Parsing::ARGUMENT_NUM)
    {
        cout << "There must be " << Parsing::ARGUMENT_NUM << " argument." << endl;
        return 1;
    }
    //Set the output
    Execution::setOutputFiles(Parsing::getOutputFilename("execution",argv[1]),Parsing::getOutputFilename("memory_status",argv[1]));
    Execution::setStrategyUsed(argv[2]);
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

    // Create the PCB table
    deque<pcb_t*> pcb[Execution::NUM_STATES];
    pcb[0] = Parsing::loadPCBTable(argv[1]);  // Initialize pcb entry
    cout << "Loaded PCB Table: " << endl;
    for (pcb_t* p : pcb[0]) {
        cout << "PID: " << p->pid << " Memory Size: " << p->memorySize << " Arrival Time: " << p->arrivalTime << " Total CPU Time: " << p->totalCPUTime << " IO Frequency: " << p->ioFrequency << " IO Duration: " << p->ioDuration << endl;
    }
    //Print initial state of memory
    Execution::writeMemoryStatus(0,pcb,memory);
    //Check for any processes arriving at t=0
    Execution::checkArrived(pcb,memory);
    //Now begin the execution and memory loading until there are no procesess left
    while (Execution::processesRemain(pcb,memory))
    {
       Execution::doExecution(pcb,memory); //handles the CPU
    }

    //End the output files
    Execution::memoryStatusOutput << "+------------------------------------------------------------------------------------------+" << std::endl;
    Execution::executionOutput << "+------------------------------------------------+" << std::endl;
    //Close files
    Execution::executionOutput.close();
    Execution::memoryStatusOutput.close();
    // Cleanup
    delete[] memory;
    for (int i = 0 ; i < Execution::NUM_STATES ; i++) {
        for (int j = 0 ; j < pcb[i].size() ; j++) {
            delete pcb[i].at(j);
        }
    }
    // All other structs are deallocated when vector activates their deconstructors.
    return 0;
}
