#!/bin/bash

# Check if the correct number of parameters is provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <filename> <name>"
    exit 1
fi

filename=$1
name=$2

echo $filename
# Check if the file exists
if [ ! -f $filename ]; then
    echo "File not found!"
    exit 1
fi


#compile interrupts
g++  interrupts.cpp -I interrupts.hpp -o sim
./sim $filename $name
#Run the gantt chart bullcrap
python3 parseGantt.py $filename

