/**
 * This is the file used to create the test case trace files.
 * @date October 3rd, 2024
 * @author John Khalife, Stavros Karamalis
*/

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <time.h>
#include <random>
#include "../interrupts.hpp"

int main(int argc, char* argv[]){

    if (argc <= 1 || argc > 3) {
        std::cout << "There are too many or too few arguments." << std::endl;
        return 1;
    }

    int numTraces = atoi(argv[1]) + 2; //Represents how many trace files we want to make.
    int loopNum = atoi(argv[2]); //Represents how many instructions each trace file will have.
    std::default_random_engine generator; // generates uniformly distributed numbers
    generator.seed(0);
    std::uniform_int_distribution<int> traceLineDistribution(1,loopNum); //Create a distribution for calculating the number of trace lines
    std::uniform_int_distribution<int> vectorTableDistribution(1,26); //Distribution for vector_table
    std::uniform_int_distribution<int> interruptDurationDistribution(100,400); //Distribution for interrupts and systemcall duration
    std::uniform_int_distribution<int> cpuDurationDistribution(1,100); //Distribution for interrupts and systemcall duration
    std::string nameFile;

    for(int i = 3; i <= numTraces; i++) //Create a set amount of trace files determined by the value of numTraces.
    {
        nameFile = "testTrace" + std::to_string(i) + ".txt"; //Creates a new file name for each trace file.
        std::ofstream outfile (nameFile); //Creates a new empty file for our trace.
        
        int lineNum = traceLineDistribution(generator); //Randomizes how many instructions will be in this trace file.

        for(int j = 0; j < lineNum/2; j++) //Writes the instructions in the trace file
        {
            outfile << Parsing::orders::CPU << ", " << cpuDurationDistribution(generator) << std::endl;
            

            if (j % 2 == 1){
                outfile << Parsing::orders::END_IO << " " << vectorTableDistribution(generator) << ", " << interruptDurationDistribution(generator) << std::endl;
            } else {
                outfile << Parsing::orders::SYSCALL << " " << vectorTableDistribution(generator) << ", " << interruptDurationDistribution(generator) << std::endl;
            }
        }

        outfile << Parsing::orders::CPU << ", " << cpuDurationDistribution(generator) << std::endl; //Last instruction

        outfile.close(); //Closes the trace file.
    }

    return 0;
        
        
}