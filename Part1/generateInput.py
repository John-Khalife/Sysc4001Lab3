import sys
import random

filename = sys.argv[1]
pNum = int(sys.argv[2])

pidList = []
process_info = {}
for i in range(pNum):
    random_number = random.randint(0, 100 * pNum)
    while random_number in pidList:
        random_number = random.randint(0, 100 * pNum)
    pidList.append(random_number)
    process_info[random_number] = {
        'pid': random_number,
        'memory_size': random.randint(1, 40),
        'arrival_time': random.randint(0, 100),
        'total_cpu_time': random.randint(1, 40),
        'io_frequency': random.randint(1, 20),
        'io_duration': random.randint(1, 20),
    }

# Write the input data to a file
with open(filename, 'w') as file:
    for pid, info in process_info.items():
        file.write(f"{pid}, {info['memory_size']}, {info['arrival_time']}, {info['total_cpu_time']}, {info['io_frequency']}, {info['io_duration']}\n")