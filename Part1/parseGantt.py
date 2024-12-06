import sys
import Gantt
import plotly.graph_objects as go

# Constants for state names
NEW = "NEW"
READY = "READY"
RUNNING = "RUNNING"
WAITING = "WAITING"
TERMINATED = "TERMINATED"

# Get the filename passed when executing the script
filename = sys.argv[1]

# Remove 'input_data_' from the filename
filename = filename[11:]
print(filename)
newfilename = 'execution_' + filename

# Read the file (skip the first three lines) and process data
data = []
with open(newfilename, 'r') as file:
    data = file.readlines()[3:]
    # Remove punctuation and split columns
    data = [x.strip(" ").replace(" ", "").split("|") for x in data]

# Remove the last index of data
data.pop()

# Now print data
for i in data:
    print(i.__len__())
    print(i, end=" ")

# Create a list of events and divisions
events = []
divisions = []
process_info = {}  # Dictionary to store process-specific info
last_stop = 0
# Now create the lists of events and calculate metrics
for i in data:
    timestamp = int(i[1])
    process_name = i[2]
    nextState = i[4]
    state = i[3]
    

    # Record event for Gantt chart
    if nextState == RUNNING:
        print("timestamp: ", timestamp , "last_stop: ", last_stop)
        if (last_stop != timestamp):
            divisions.append(last_stop)
            events.append({'Event': ''})
            print("IO event added at" , last_stop)

        events.append({'Event': process_name})
        divisions.append(timestamp)
   
    if (nextState == WAITING or nextState == READY or nextState == TERMINATED) and state == RUNNING:
        print("called with timestamp: ", timestamp)
        last_stop = timestamp

    # Initialize process info if not present
    if process_name not in process_info:
        process_info[process_name] = {
            'arrival_time': None,
            'start_time': None,
            'completion_time': None,
            'waiting_time': 0,
            'response_time': None,
            'is_waiting': False,
            'current_wait': None
        }

    # Record arrival time
    if state == NEW and process_info[process_name]['arrival_time'] is None:
        process_info[process_name]['arrival_time'] = timestamp

    # Record start time (first time in RUNNING state)
    if nextState == RUNNING and process_info[process_name]['start_time'] is None:
        process_info[process_name]['start_time'] = timestamp

    # Record completion time (first time in TERMINATED state)
    if nextState == TERMINATED:
        process_info[process_name]['completion_time'] = timestamp

    if nextState == READY:
        process_info[process_name]['current_wait'] = timestamp

    # Increment waiting time if process is READY
    if nextState == RUNNING:
        process_info[process_name]['waiting_time'] += timestamp - process_info[process_name]['current_wait']

    #Set the response time
    if process_info[process_name]['response_time'] is None and process_info[process_name]['start_time'] is not None and process_info[process_name]['arrival_time'] is not None:
        process_info[process_name]['response_time'] = process_info[process_name]['start_time'] - process_info[process_name]['arrival_time']

# Append the last division
divisions.append(int(data[-1][1]))

print("Events:", events, "\nDivisions:", divisions)

#print all process info
for process_name, info in process_info.items():
    print(f"Process {process_name}:")
    print(f"Arrival Time: {info['arrival_time']}")
    print(f"Start Time: {info['start_time']}")
    print(f"Completion Time: {info['completion_time']}")
    print(f"Waiting Time: {info['waiting_time']}")
    print(f"Response Time: {info['response_time']}\n\n")


# Check for errors
if (len(events) + 1) != len(divisions):
    print("Error: Events and divisions are not the same length")
    print("Events:", len(events), "Divisions:", len(divisions))



# Calculate metrics
total_processes = len(process_info)
total_wait_time = 0
total_turnaround_time = 0
total_response_time = 0
valid_processes = 0  # Count processes with complete data

for process_name, info in process_info.items():
    # Skip processes with incomplete data
    if info['arrival_time'] is None or info['completion_time'] is None:
        print(f"Skipping process {process_name} due to incomplete data.")
        continue

    # Calculate turnaround time
    turnaround_time = info['completion_time'] - info['arrival_time']
    total_turnaround_time += turnaround_time

    # Calculate response time
    response_time = info['start_time'] - info['arrival_time'] if info['start_time'] is not None else 0
    total_response_time += response_time

    # Calculate wait time
    total_wait_time += info['waiting_time']

    # Increment valid process count
    valid_processes += 1

# Throughput is the number of completed processes divided by total time
total_time = divisions[-1]
throughput = valid_processes / total_time if total_time > 0 else 0
print(f"Total time: {total_time}")

# Calculate averages
avg_wait_time = total_wait_time / valid_processes if valid_processes > 0 else 0
avg_turnaround_time = total_turnaround_time / valid_processes if valid_processes > 0 else 0
avg_response_time = total_response_time / valid_processes if valid_processes > 0 else 0

# Create a metrics dictionary
metrics = {
    'throughput': throughput,
    'avg_wait_time': avg_wait_time,
    'avg_turnaround_time': avg_turnaround_time,
    'avg_response_time': avg_response_time
}

# Print metrics for debugging
print("Metrics:")
print(f"Throughput: {throughput}")
print(f"Average Wait Time: {avg_wait_time}")
print(f"Average Turnaround Time: {avg_turnaround_time}")
print(f"Average Response Time: {avg_response_time}")


# Write metrics to file
with open("metrics.txt", "a") as file:
    file.write(f"File: {filename} Strategy: {sys.argv[2]} Throughput: {throughput}, Average Wait Time: {avg_wait_time}, Average Turnaround Time: {avg_turnaround_time}, Average Response Time: {avg_response_time}\n")

# Call the Gantt chart function
Gantt.drawGantt(events, divisions, "Part1 CPU execution " + sys.argv[2], metrics).write_image("Chart_" + newfilename + ".png")