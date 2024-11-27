# Description: This file is used to generate the Gantt chart based on information created in execution.txt

import sys
import Gantt

# Constants for state names
NEW = "NEW"
READY = "READY"
RUNNING = "RUNNING"
WAITING = "WAITING"
TERMINATED = "TERMINATED"

# Get the filename passed when executing the script
filename = sys.argv[1]

# remove input_data_ from the string filename
filename = filename[11:]
print(filename) 
newfilename = 'execution_'  + filename

# Read the file (skip the first three lines) and remove all spaces
data = []
with open(newfilename, 'r') as file:
    data = file.readlines()[3:]
    #remove punctuation
    data = [x.strip(" ").replace(" ", "").split("|") for x in data]

#remove last index of data
data.pop()

# Now print data
for i in data: 
    print(i.__len__())
    print(i, end = " ")

# Create a list of events and divisitionz
events = []
divisions = []

# Now create the lists of events
for i in data:
    if (i[4] == RUNNING):
        events.append({'Event': i[2]})
        divisions.append(int(i[1]))

divisions.append(int(data[-1][1]))

print(events)
print(divisions)
if (events.__len__() != divisions.__len__()):
    print("Error: Events and divisions are not the same length")

Gantt.drawGantt(events,divisions,"Part1 CPU execution")

