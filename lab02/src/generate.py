import random

n = int(5 * 1e8)
a = int(1e7)

with open("/home/npabwa/Desktop/MAI_OS/lab02/src/test.txt", 'w') as file:
    file.write(f"{n}\n")

    for i in range(n):
        file.write(f"{random.randint(-a, a)} ")
    
    file.write("\n")
