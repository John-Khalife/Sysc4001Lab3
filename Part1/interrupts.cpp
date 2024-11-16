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

namespace MemoryStructures
{
    int reserveMemory(Partition *memory, __uint8_t size, char *programName)
    {
        //*NOTE Partition sizes are ordered from largest to smallest - so best fit will be easy.
        //*That means however, this method may need to be updated in the future if different partitions are given.
        for (int i = PARTITION_NUM - 1; i >= 0; i--)
        {
            if (memory[i].code == "free")
            {
                if (size <= memory[i].size)
                {
                    memory[i].code = programName;
                    return i;
                }
            }
        }
        return -1;
    }

    __uint8_t getFileSize(vector<extFile> &files, char *programName)
    {
        for (int i = 0; i < files.size(); i++)
        {
            if (strcmp(programName, files[i].programName) == 0)
            {
                return files[i].size;
            }
        }
        return 0;
    }

    pcb_t *getRunningProcess(vector<pcb_t> &pcb)
    {
        for (int i = pcb.size(); i >= 0; i--)
        {
            if (pcb[i].isRunning)
            {
                return &pcb[i];
            }
        }
        return NULL;
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
        if (output.fail())
        {
            return;
        }
        output << timer << ", " << duration << ", " << eventType << endl; // Write the Execution message in the proper format
        timer += duration;                                                // Add the amount of timer to CPU timer for the next write
    }

    void writePcbTable(vector<MemoryStructures::pcb_t> pcb)
    {
        static ofstream pcbOutput(PCB_OUTPUT_FILE_NAME);

        pcbOutput << "!-----------------------------------------------------------!" << endl;
        pcbOutput << "Save Time: " << timer << " ms" << endl;
        pcbOutput << "+----------------------------------------------+" << endl;
        pcbOutput << "| PID | Program Name | Partition Number | Size |" << endl;
        pcbOutput << "+----------------------------------------------+" << endl;
        for (int i = 0; i < pcb.size(); i++)
        {
            pcbOutput << "| " << to_string((int)pcb[i].pid);
            pcbOutput << string(max(3 - Parsing::numDigits((int)pcb[i].pid), 0), ' ') << " | ";
            pcbOutput << pcb[i].programName << string((int)(12 - pcb[i].programName.length()), ' ') << " | ";
            pcbOutput << to_string((int)pcb[i].partitionNum);
            pcbOutput << string(16 - Parsing::numDigits((int)pcb[i].partitionNum), ' ');
            pcbOutput << " | " << to_string((int)pcb[i].memoryAllocated);
            pcbOutput << string(4 - Parsing::numDigits((int)pcb[i].memoryAllocated), ' ');
            pcbOutput << " |" << endl;
        }
        pcbOutput << "+----------------------------------------------+" << endl;
        pcbOutput << "!-----------------------------------------------------------!" << endl;
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

    void executeInstruction(
        Parsing::instr *instruction,
        vector<MemoryStructures::pcb_t> &pcb,
        vector<MemoryStructures::extFile> &files,
        MemoryStructures::Partition *memory,
        int index)
    {

        if (!Parsing::orders::CPU.compare(instruction->commandName))
        {
            executeCPU(instruction->args[0].number);
        }
        else if (!Parsing::orders::SYSCALL.compare(instruction->commandName))
        {
            systemCall(instruction->args[1].number, instruction->args[0].number);
        }
        else if (!Parsing::orders::END_IO.compare(instruction->commandName))
        {
            interrupt(instruction->args[1].number, instruction->args[0].number);
        }
        else if (!Parsing::orders::FORK.compare(instruction->commandName))
        {
            fork(instruction->args[0].number, pcb, index);
        }
        else if (!Parsing::orders::EXEC.compare(instruction->commandName))
        {
            exec(instruction->args[0].word, instruction->args[1].number, pcb, files, memory, index);
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
    for (int i = 0; i < PARTITION_NUM; i++)
    {
        memory[i] = (Partition){.partitionNum = __uint8_t(i + 1), .size = __uint8_t(PARTITION_SIZES[i]), .code = -1};
    }

    vector<pcb_t> pcb = Parsing::loadPCBTable(argv[1]);  // Initialize pcb entry

    // Loop continues while there are still processes to run in the pcb.
    // Get the process that should be run right now. from the pcb
    // Start executing that and continue to check for the best process to run.
    PcbEntry *currentProcess = getRunningProcess(pcb);
    while (currentProcess != NULL)
    {
        Execution::executeInstruction(operation, pcb, files, memory, (currentProcess->pid - MemoryStructures::SMALLEST_PID));
        PcbEntry* newProcess = getRunningProcess(pcb); // check to see if a new process needs to be run
        if (newProcess != currentProcess)
        { //TODO: Do whatever needs to be done for a context switch
            currentProcess = newProcess;
        }
    }

    // Cleanup
    delete[] memory;
    // All other structs are deallocated when vector activates their deconstructors.
    return 0;
}
