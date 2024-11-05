with open("/home/npabwa/Desktop/MAI_OS/lab02/src/test_out.txt", 'r') as file:
    prev = int(file.readline().strip(" \n"))
    for i in range(int(5 * 1e8) - 1):
        n = int(file.readline().strip(" \n"))
        if (n < prev):
            print(f"not sorted, line {i}, prev = {prev}, n = {n}")
            break
        prev = n
    else:
        print("sorted")