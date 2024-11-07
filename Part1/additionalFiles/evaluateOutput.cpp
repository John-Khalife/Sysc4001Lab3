/**
 * This file holds code responsible for ensuring that the output is correct
 * @author John Khalife, Stavros Karamalis
 */

#include <sstream>
#include <fstream>
#include <iostream>

struct outputLine
{
    int time;
    int duration;
    std::string message;
} typedef Output;

// If ever a new instruction needs to be added - add the equivalent string here

const int MAX_PARAMETERS = 2;
namespace orders
{
    using namespace std;
    const string CPU = "CPU";
    const string SYSCALL = "SYSCALL";
    const string END_IO = "END_IO";
}

// This struct holds the contents of an instruction including command and arguments
struct instruction
{
    int args[MAX_PARAMETERS];
    std::string argName;
} typedef instr;

instr *readFromTrace(std::ifstream *file)
{
    instr *operation = new instr();
    if (!file->eof())
    {
        std::string text;
        getline(*(file), text);
        for (int i = 0, len = text.size(); i < len; i++)
        {
            if (text[i] == ',')
            {
                text.erase(i--, 1);
                len = text.size();
            }
        }

        std::stringstream ss(text);
        int i = 0;
        std::string s;
        getline(ss, operation->argName, ' '); // First parameter always command
        while (getline(ss, s, ' '))
        { // while for any args
            operation->args[i] = atoi(s.c_str());
            i++;
        }
    }
    else
    {
        file->close();
    }
    return operation;
}

Output parseExecution(std::ifstream *executionFile)
{
    Output output;
    if (!executionFile->eof())
    {
        std::string text;
        getline(*(executionFile), text);
        output.message = text.substr(text.find_last_of(',') + 1);
        for (int i = 0, len = text.size(); i < len; i++)
        {
            if (text[i] == ',')
            {
                text.erase(i--, 1);
                len = text.size();
            }
        }

        std::stringstream ss(text);
        int i = 0;
        std::string s;
        getline(ss, s, ' '); // First output always time
        output.time = atoi(s.c_str());
        getline(ss, s, ' '); // Second parameter is always duration
        output.duration = atoi(s.c_str());
    }
    else
    {
        executionFile->close();
    }
    return output;
}

bool checkVectorTableConsultation(std::ifstream *output, instr *operation, int (&ratio)[3], int (&totalTime)[3])
{
    // Get the first line - CPU to kernel mode
    Output line = parseExecution(output);
    // Verify the range of the CPU duration
    if (line.duration != 1)
    {
        std::cout << "Error: CPU to Kernel mode has incorrect duration: " << line.duration << ":" << operation->args[1] << "." << std::endl;
        return false;
    }
    // Verify the string that was outputed
    if (line.message.compare(" Switch CPU to Kernel mode."))
    {
        std::cout << "Error: VectorTable search message is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // Get the next line - context saved
    line = parseExecution(output);
    // Verify the range of the CPU duration
    if (line.duration < 1 || line.duration > 3)
    {
        std::cout << "Error: context saved incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    // Verify the string that was outputed
    if (line.message.compare(" Context saved."))
    {
        std::cout << "Error: VectorTable search message is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // Get the next line - find vector x
    // Get the next line - context saved
    line = parseExecution(output);
    // Verify the range of the CPU duration
    if (line.duration != 1)
    {
        std::cout << "Error: find vector x incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    // Verify the string that was outputed - point in verifying the hexadecimal because it uses stringstream functions.
    if (line.message.find("Find vector") == 0)
    {
        std::cout << "Error: Find vector search message is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // Get the next line - Load address
    line = parseExecution(output);
    // Verify the range of the CPU duration
    if (line.duration != 1)
    {
        std::cout << "Error: load address incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    // Verify the string that was outputed - point in verifying the hexadecimal because it uses stringstream functions.
    if (line.message.find("Load address") == 0)
    {
        std::cout << "Error: Load address message is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // All tests have passed
    return true;
}

bool checkExecuteCPU(std::ifstream *output, instr *operation, int (&ratio)[3], int (&totalTime)[3])
{
    // This is an easy one, just get one line
    Output line = parseExecution(output);

    // Verify the range of the CPU duration
    if (line.duration < 1 || line.duration > 100)
    {
        std::cout << "Error: CPU command duration not within allowable duration: " << line.duration << "." << std::endl;
        return false;
    }

    // Verify the string that was outputed
    if (line.message.compare(" CPU execution."))
    {
        std::cout << "Error: CPU command message is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[0]++;
    totalTime[0] += line.duration;

    // All tests passed.
    return true;
}

bool checkSystemCall(std::ifstream *output, instr *operation, int (&ratio)[3], int (&totalTime)[3])
{

    // Check the vector table
    if (!checkVectorTableConsultation(output, operation,ratio,totalTime))
    {
        return false;
    }

    // This is an easy one, just get one line
    Output line = parseExecution(output);
    int durations[3] = {0, 0, 0};
    durations[0] = line.duration;

    // Verify the string that was outputed
    if (line.message.compare(" SYSCALL: Execute ISR."))
    {
        std::cout << "Error: SYSCALL command output is faulty: " << line.message << std::endl;
        return false;
    }    
    ratio[2]++;
    totalTime[2] += line.duration;

    line = parseExecution(output);
    durations[1] = line.duration;

    // Verify the string that was outputed
    if (line.message.compare(" Transfer data."))
    {
        std::cout << "Error: SYSCALL command output is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[1]++;
    totalTime[1] += line.duration;

    line = parseExecution(output);
    durations[2] = line.duration;

    // Verify the string that was outputed
    if (line.message.compare(" Check for errors."))
    {
        std::cout << "Error: SYSCALL command output is faulty: " << line.message << std::endl;
        return false;
    }
    ratio[0]++;
    totalTime[0] += line.duration;

    int sum = durations[0] + durations[1] + durations[2];
    // Verify the range of the CPU duration
    if (sum != operation->args[1])
    {
        std::cout << "Error: SYSCALL command duration not matching correct duration: " << line.duration << ":" << operation->args[1] << "." << std::endl;
        return false;
    }
    if (sum < 100 || sum > 400)
    {
        std::cout << "Error: SYSCALL command duration within allowable duration: " << line.duration << "." << std::endl;
        return false;
    }

    line = parseExecution(output);

    // Verify the string that was outputed
    if (line.message.compare(" IRET"))
    {
        std::cout << "Error: SYSCALL command output is faulty: " << line.message << std::endl;
        return false;
    }
    if (line.duration != 1)
    {
        std::cout << "Error: SYSCALL command output incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // All tests passed
    return true;
}

bool checkInterrupt(std::ifstream *output, instr *operation, int (&ratio)[3], int (&totalTime)[3])
{
    // Get the first line
    Output line = parseExecution(output);

    // Verify the string that was outputed
    if (line.message.compare(" Check priority of interrupt."))
    {
        std::cout << "Error: END_IO command output is faulty: " << line.message << std::endl;
        return false;
    }
    if (line.duration != 1)
    {
        std::cout << "Error: END_IO command output incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;


    // Get the first line
    line = parseExecution(output);

    // Verify the string that was outputed
    if (line.message.compare(" Check if interrupt is masked."))
    {
        std::cout << "Error: END_IO command output is faulty: " << line.message << std::endl;
        return false;
    }
    if (line.duration != 1)
    {
        std::cout << "Error: END_IO command output incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // Check the vector table
    if (!checkVectorTableConsultation(output, operation, ratio, totalTime))
    {
        return false;
    }

    // Get the next line
    line = parseExecution(output);

    // Verify the string that was outputed
    if (line.message.compare(" END_IO"))
    {
        std::cout << "Error: END_IO command output is faulty: " << line.message << std::endl;
        return false;
    }
    if (line.duration != operation->args[1])
    {
        std::cout << "Error: END_IO command output incorrect duration: " << line.duration << ":" << operation->args[1] << "." << std::endl;
        return false;
    }
    ratio[1]++;
    totalTime[1] += line.duration;

    line = parseExecution(output);

    // Verify the string that was outputed
    if (line.message.compare(" IRET"))
    {
        std::cout << "Error: SYSCALL command output is faulty: " << line.message << std::endl;
        return false;
    }
    if (line.duration != 1)
    {
        std::cout << "Error: SYSCALL command output incorrect duration: " << line.duration << "." << std::endl;
        return false;
    }
    ratio[2]++;
    totalTime[2] += line.duration;

    // All tests passed
    return true;
}

int main(int argc, char *argv[])
{
    // Check to make sure there is one argument
    if (argc <= 1 || argc > 3)
    {
        std::cout << "Too many or too few arguments" << std::endl;
        return 1;
    }

    // Get the file number in the title of trace (if it is there)
    // Create input and output file objects
    
    int traceNum = atoi(argv[2]);
    int startFileNum = atoi(argv[1]);

    int ratio[3] = {0,0,0};
    int totalTime[3] = {0,0,0};
    for (int i = 0 ; i < traceNum ; i++) {
        std::ifstream output("execution" + std::to_string(startFileNum) + ".txt");
        std::ifstream input("testTrace" + std::to_string(startFileNum)  + ".txt");
        std::cout<< "Input: " << "testTrace" + std::to_string(startFileNum)  + ".txt" << " Output: " << "execution" + std::to_string(startFileNum) + ".txt" << std::endl;
        startFileNum++;
        int line = 0;
        int tempRatio[3] = {0,0,0};
        int tempTotalTime[3] = {0,0,0};;
        while (input.is_open())
        {
            instr *operation = readFromTrace(&input);
            bool properOutput;
            // Check the right instruction based off of this
            if (!orders::CPU.compare(operation->argName))
            {
                properOutput = checkExecuteCPU(&output, operation, tempRatio, tempTotalTime);
            }
            else if (!orders::SYSCALL.compare(operation->argName))
            {
                properOutput = checkSystemCall(&output, operation, tempRatio, tempTotalTime);
            }
            else if (!orders::END_IO.compare(operation->argName))
            {
                properOutput = checkInterrupt(&output, operation, tempRatio, tempTotalTime);
            }
            if (!properOutput)
            {
                std::cout << "The tests have failed due to a failed " << operation->argName << " instruction on line " << line << "." << std::endl;
                std::cout << "Parameters: " << operation->args[0] << ", " << operation->args[1] << std::endl;
                return 1;
            }
            line = line + 1;
        }
        std::cout << "Ratio - CPU: " << tempRatio[0] << ", I/O: " << tempRatio[1] << ", Overhead: " << tempRatio[2] << "." << std::endl;
        std::cout << "Time - CPU: " << tempTotalTime[0] << ", I/O: " << tempTotalTime[1] << ", Overhead: " << tempTotalTime[2] << "." << std::endl;
        for (int j = 0 ; j < 3 ;j++) {
            ratio[j] += tempRatio[j];
            totalTime[j] += tempTotalTime[j];
        }
        output.close();
        input.close();
    }

    std::cout << "Overall Stats:" << std::endl;
    std::cout << "Ratio - CPU: " << ratio[0] << ", I/O: " << ratio[1] << ", Overhead: " << ratio[2] << "." << std::endl;
    std::cout << "Time - CPU: " << totalTime[0] << ", I/O: " << totalTime[1] << ", Overhead: " << totalTime[2] << "." << std::endl;

    return 0;
}