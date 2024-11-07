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

#include "interrupts.hpp"

using namespace std;

namespace MemoryStructures {
    void copyPCBEntry(vector<pcb_t>& pcb, int index)    
    {
            if (pcb.empty()) {
                pcb_t newProcess(SMALLEST_PID + pcb.size(),"init",6,1,0,true,true);
                pcb.push_back(newProcess);
            } else {
                pcb_t newProcess(SMALLEST_PID + pcb.size(),
                    pcb[index].programName,
                    pcb[index].partitionNum,
                    pcb[index].memoryAllocated,
                    pcb[index].fpos,
                    true,
                    true);
                //Tell the parent process not to take the next exec command
                pcb[index].doExec = false;
                pcb.push_back(newProcess);
            }
    }

    void modifyPCBEntry(
        vector<pcb_t>& pcb,
        int index,
        char programName[20],
        __uint8_t partitionNum,
        __uint128_t memoryAllocated) {
        pcb[index].programName = programName;
        pcb[index].partitionNum = partitionNum;
        pcb[index].memoryAllocated = memoryAllocated;
        pcb[index].fpos = 0;  
        if (pcb[index].doExec == false) {
            pcb[index].doExec = true;
        }
    }

    int reserveMemory(Partition* memory, __uint8_t size, char* programName) {
        //*NOTE Partition sizes are ordered from largest to smallest - so best fit will be easy.
        //*That means however, this method may need to be updated in the future if different partitions are given.
        for (int i = PARTITION_NUM - 1; i >= 0 ; i--) {
            if (memory[i].code == "free") {
                if (size <= memory[i].size) {
                    memory[i].code = programName;
                    return i;
                }
            }
        }
        return -1;
    }

    __uint8_t getFileSize(vector<extFile>& files, char* programName) {
        for (int i = 0 ; i < files.size() ; i++) {
            if (strcmp(programName,files[i].programName) == 0) {
                return files[i].size;
            }
        }
        return 0;
    }

    pcb_t* getRunningProcess(vector<pcb_t>& pcb) {
        for (int i = pcb.size(); i >= 0 ; i--) {
            if(pcb[i].isRunning) {
                return &pcb[i];
            }
        }
        return NULL;
    }
}

namespace Parsing {

    void readExtFiles(vector<MemoryStructures::extFile>& files)
    {
        ifstream file("./external_files.txt");
        while (!file.eof()) {
                MemoryStructures::extFile newNode;
                string text;
                getline(file,text);
                for(int i = 0, len = text.size(); i < len; i++){
                    if (text[i] == ','){
                        text.erase(i--, 1);
                        len = text.size();
                    }
                }
                stringstream ss(text);
                string temp;
                getline(ss, temp,' ');// First parameter always program name
                for (int i = 0; i < temp.length(); i++){
                    newNode.programName[i] = temp[i];
                }
                newNode.programName[temp.length()] = '\0';
                getline(ss,temp,' ');
                getline(ss,temp,' ');
                newNode.size = stoi(temp);
                files.push_back(newNode);
        }
        file.close();
    }

    instr* readFromTrace()
        {
            instr* operation = new instr();
            if (!input.eof()) {
                string text;
                getline(input,text);
                for(int i = 0, len = text.size(); i < len; i++){
                    if (text[i] == ','){
                        text.erase(i--, 1);
                        len = text.size();
                    }
                }
                
                stringstream ss(text);
                int i = 0;
                string s;
                getline(ss,operation->commandName,' ');// First parameter always command
                while (getline(ss,s,' ')) { //while for any args
                    //Check if the argument is a string or an integer
                    for (int i = 0 ; i < strlen(s.c_str()) ; i++) {
                        if (!isdigit(s[i])) {
                            operation->args[i].isString = true;
                            break;
                        }
                    }
                    if (operation->args[i].isString) {
                        strncpy(operation->args[i].word, s.c_str(),20);
                    } else {
                        operation->args[i].number = stoi(s);
                    }
                    i++;
                    
                }
            } else {
                input.close();
            }
            return operation;
        }

    string integerToHexString(int number) {
        stringstream ss;
        ss << setfill('0') << setw(4) << hex << number;
        return "0x" + ss.str();
    }

    //helper method
    int numDigits(int number) {
        if (number == 0) return 1;
        return static_cast<int>(log10(abs(number)) + 1);
    }

    /**
     * function to set the input file
    */
    void setInput(string fileName) {
       input.close();
       input.open(fileName);
    }
}

namespace Execution {

    bool setOutputFile(string filename) {
        output.open(filename);
        if (output.fail()) {return false;}
        return true;
    }

    void writeExecutionStep(int duration, string eventType) {
        if (output.fail()) {return;}
        output << timer << ", " << duration << ", " << eventType << endl; // Write the Execution message in the proper format
        timer += duration; //Add the amount of timer to CPU timer for the next write
    }

    void writePcbTable(vector<MemoryStructures::pcb_t> pcb) {
        static ofstream pcbOutput(PCB_OUTPUT_FILE_NAME);

        pcbOutput << "!-----------------------------------------------------------!" << endl;
        pcbOutput << "Save Time: " << timer << " ms" << endl;
        pcbOutput << "+----------------------------------------------+" << endl;
        pcbOutput << "| PID | Program Name | Partition Number | Size |" << endl;
        pcbOutput << "+----------------------------------------------+" << endl;
        for (int i = 0 ; i < pcb.size() ; i++) {
            pcbOutput << "| " << to_string((int) pcb[i].pid); 
            pcbOutput << string(max(3 - Parsing::numDigits((int) pcb[i].pid), 0),' ') << " | ";
            pcbOutput << pcb[i].programName << string((int)(12 - pcb[i].programName.length()),' ') << " | ";
            pcbOutput << to_string((int) pcb[i].partitionNum);
            pcbOutput << string(16 - Parsing::numDigits((int) pcb[i].partitionNum) ,' ');
            pcbOutput << " | " << to_string((int) pcb[i].memoryAllocated);
            pcbOutput << string(4 - Parsing::numDigits((int) pcb[i].memoryAllocated),' ');
            pcbOutput << " |" << endl;
        }
        pcbOutput << "+----------------------------------------------+" << endl;
        pcbOutput << "!-----------------------------------------------------------!" << endl;
    }
        
    void systemCall(int duration, int isrAddress) {
        accessVectorTable(isrAddress);

        default_random_engine generator; // generates uniformly distributed numbers
        generator.seed(time(0)); //Give the generator a seed
        uniform_real_distribution<> isrDistribution(0, 0.25); //probability distribution for 0-25% of the duration.
        int dataTransferTime = (int) duration*isrDistribution(generator) + 1; //time for data transfer
        int errorCheckingTime = (int) duration*isrDistribution(generator) + 1; //generate time for error checking

        //Call the device driver
        writeExecutionStep(duration - dataTransferTime - errorCheckingTime, "SYSCALL: Execute ISR.");
        writeExecutionStep(dataTransferTime,"Transfer data."); //transfer data
        writeExecutionStep(errorCheckingTime,"Check for errors."); // Error check
        writeExecutionStep(1,"IRET"); // Interrupt return.
    }

    void executeCPU(int duration) {
        writeExecutionStep(duration,"CPU Execution.");
    }

    void interrupt(int duration, int isrAddress) {
        writeExecutionStep(1,"Check priority of interrupt.");
        writeExecutionStep(1,"Check if interrupt is masked.");
        accessVectorTable(isrAddress);
        writeExecutionStep(duration,"END_IO"); 
        writeExecutionStep(1,"IRET"); // Interrupt return.
    }

    void accessVectorTable(int isrAddress) {
        default_random_engine generator; // generates uniformly distributed numbers
        generator.seed(time(0)); //Give the generator a seed
        uniform_int_distribution<int> contextSaveTimeDistribution(1,3); //Create a distribution

        //First switch to kernel mode
        writeExecutionStep(1,"Switch CPU to Kernel mode.");
        
        using namespace std;
        writeExecutionStep(contextSaveTimeDistribution(generator),"Context saved."); //Save context

        //Check vector table and call ISR
        ifstream file;
        string text;
        
        writeExecutionStep(1,"Find vector " + to_string(isrAddress) + " in memory position " + Parsing::integerToHexString(isrAddress) + ".");
        file.open("vector_table.txt");
        for (int i = 0 ; i <= isrAddress ; i++) {
            getline(file,text);
        }
        file.close(); // Now the text string should contain the ISR.
        writeExecutionStep(1, "Load address " + text + " into the PC."); //output the address being loaded
    }

void fork(int duration, vector<MemoryStructures::pcb_t>& pcb, int index) {
        default_random_engine generator; // generates uniformly distributed numbers
        generator.seed(time(0)); //Give the generator a seed
        uniform_int_distribution<int> forkTimeDistribution(1,9); //Create a distribution

        //a. simulates system call - access vector table
        accessVectorTable(2);
        //b. Copy parent PCB to child process
        //*NOTE: The assignment stated a random time of 1-10 but also must sum to duration.
        //*Assumption: a fork call will not take less than 10ms.
        int forkTime = floor((forkTimeDistribution(generator)/100)*duration);
        writeExecutionStep(forkTime, "FORK: copy parent PCB to child PCB");
        //Need to increment once before forking
        pcb[index].fpos = Parsing::input.tellg();
        MemoryStructures::copyPCBEntry(pcb,index);
        //c. Call the routing scheduler (all it should display for now is 'scheduler called')
        writeExecutionStep(duration - forkTime,"Scheduler called.");
        //d. return from the ISR
        writeExecutionStep(1,"IRET"); // Interrupt return.
        writePcbTable(pcb);
    }

    void exec(char* filename, int duration, 
        vector<MemoryStructures::pcb_t>& pcb,
        vector<MemoryStructures::extFile>& files,
        MemoryStructures::Partition* memory,
        int index) {
        if (!pcb[index].doExec) {
            pcb[index].doExec = true;
            return;
        }
        default_random_engine generator; // generates uniformly distributed numbers
        generator.seed(time(0)); //Give the generator a seed
        uniform_real_distribution<float> execTimeDistribution(0,0.2); //Create a distribution

        //a. simulate a system call - access the vector table
        accessVectorTable(3);
        //b. find the file in the list, and set the size in the PCB
        __uint8_t size = MemoryStructures::getFileSize(files,filename);
        if (size == 0) {
            writeExecutionStep(0,"ERROR: No file of name " + string(filename)  + " exists");
            return;
        }
        int execTime = floor((0.2)*duration);
        writeExecutionStep(execTime,"EXEC: load " + string(filename) + " of size " + to_string(size) + "Mb.");
        //c. find an empty partition where the program fits, with the best fit policy
        int partitionNum = MemoryStructures::reserveMemory(memory,size,filename);
        if (partitionNum < 0) {
            writeExecutionStep(0,"ERROR: Unable to allocate memory for process");
            return;
        }
        int findTime = floor((execTimeDistribution(generator))*duration);
        writeExecutionStep(findTime,"Found partition " + to_string(partitionNum + 1) + " with " + to_string(memory[partitionNum].size) + "Mb of space." );
        //d. mark the partition as occupied
        int occupiedTime = floor((execTimeDistribution(generator))*duration);
        writeExecutionStep(occupiedTime,"Marked partition " + to_string(partitionNum + 1) + " as occupied.");
        //e. update PCB
        int updateTime = floor((execTimeDistribution(generator))*duration);
        writeExecutionStep(updateTime,"Updating PCB with new information.");
        MemoryStructures::modifyPCBEntry(pcb,index,filename,partitionNum + 1,size);
        if (pcb[index].programName == "init") {
                Parsing::setInput(Parsing::traceName);
            } else {
                Parsing::setInput(pcb[index].programName + ".txt");
            }
            Parsing::input.seekg(pcb[index].fpos); //set PC
        //f. do the routing scheduler thing again
        writeExecutionStep(duration - updateTime - occupiedTime - findTime - execTime,"Scheduler called.");
        //g. return normally
        writeExecutionStep(1,"IRET"); // Interrupt return.
        writePcbTable(pcb);
    }

    void executeInstruction(
        Parsing::instr* instruction,
        vector<MemoryStructures::pcb_t>& pcb,
       vector<MemoryStructures::extFile>& files,
        MemoryStructures::Partition* memory,
        int index) {

        if (!Parsing::orders::CPU.compare(instruction->commandName)) {
            executeCPU(instruction->args[0].number);
        } else if (!Parsing::orders::SYSCALL.compare(instruction->commandName)) {
            systemCall(instruction->args[1].number,instruction->args[0].number);
        } else if (!Parsing::orders::END_IO.compare(instruction->commandName)) {
            interrupt(instruction->args[1].number,instruction->args[0].number);
        } else if (!Parsing::orders::FORK.compare(instruction->commandName)) {
            fork(instruction->args[0].number,pcb,index);
        } else if (!Parsing::orders::EXEC.compare(instruction->commandName)) {
            exec(instruction->args[0].word,instruction->args[1].number,pcb, files, memory, index);
        }
    }
}

int main(int argc, char* argv[]) {
    //Check to make sure there are arguments
    if (argc <= 1 || argc > 2) {
        cout << "Improper argument number." << endl;
        return 1;
    }
    Parsing::traceName = argv[1];
    
    //Get the file number in the title of trace (if it is there)
    string fileNum = ((string) argv[1]).substr(((string) argv[1]).find_first_of("0123456789"),((string) argv[1]).find_last_of("0123456789") - ((string) argv[1]).find_first_of("0123456789") + 1); //Grab the value before .txt
    ofstream output;
    //Create input and output file objects
    if (isdigit(fileNum[0])) { //If the value is a number, add it to the end of Execution.
        Execution::setOutputFile("Execution" + fileNum + ".txt");
    } else {
        Execution::setOutputFile("Execution.txt"); //otherwise just open Execution.txt as the output file.
    }

    //Initialize memory partitions with the proper sizes.
    using namespace MemoryStructures;
    Partition* memory = new Partition[6];
    for (int i = 0; i < PARTITION_NUM; i++) {
        memory[i] = (Partition) {.partitionNum = __uint8_t(i + 1), .size = __uint8_t(PARTITION_SIZES[i]), .code = "free"};
    } 
    //Initialize partition 6 with the PCB
    memory[5].code = "init";
    //Initialize pcb entry
    vector<pcb_t> pcb;
    MemoryStructures::copyPCBEntry(pcb,0);
    
    //Initialize the file list.
    vector<extFile> files;
    Parsing::readExtFiles(files);

    //Loop continues while there are still processes with open files in the pcb.
    //Get the process that should be run right now. from the pcb
    //Start executing that line by line (continue to check for new processes each line)
    PcbEntry* currentProcess = getRunningProcess(pcb);
    Parsing::setInput(argv[1]);
    Execution::writePcbTable(pcb);
    while(true){
        Parsing::instr* operation = Parsing::readFromTrace();
        Execution::executeInstruction(operation,pcb,files,memory,(currentProcess->pid - MemoryStructures::SMALLEST_PID));
        //Switch context if needed
        if (!Parsing::input.is_open()) {currentProcess->isRunning = false;}
        //cout << "bug" << endl;
        PcbEntry* newProcess = getRunningProcess(pcb);
        if (newProcess == NULL) {break;}
        if (newProcess != currentProcess) { //compare pointers
            //context switch, save file position
            if (newProcess->programName == "init") {
                Parsing::setInput(argv[1]);
            } else {
                Parsing::setInput(newProcess->programName + ".txt");
            }
            Parsing::input.seekg(newProcess->fpos); //set PC
            //Execution::writeExecutionStep(0,"Process switch from " + to_string(currentProcess->index) + " to " + to_string(newProcess->index));
            currentProcess = newProcess;
        }
    }

    //Cleanup 
    delete[] memory;
    //All other structs are deallocated when vector activates their deconstructors.
    return 0;
}
