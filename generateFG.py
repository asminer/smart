
forbidden_cell=[]
sampling_cell=[]
with open('grid20_f_20_g_10.txt', 'r')as file1:
    Lines = file1.readlines()
    count=0
    count2=0
    count3=0
    for line in Lines:
        
        
        #print(type(line[2:]))
        if 'X' in line[2:]:
            #xx= line[2:].index('X')
            for n in range(len(line[2:])):
                if line[2:].find('X',n)==n:
                    xy=(count,n-1)
                    forbidden_cell.append(xy)
        if 's' in line[2:]:
            #xx= line[2:].index('X')
            for n in range(len(line[2:])):
                if line[2:].find('s',n)==n:
                    xy=(count,n-1)
                    sampling_cell.append(xy)

            #print([n for n in range(len(line[2:])) if line[2:].find('X', n) == n])
            count2+=1
        #print("Line{}: {}".format(count, line[2:].strip()))
        #print("line", line[2:])
        count += 1
print("count2: ",count2,"\n")
print(count,"\n")
print(forbidden_cell)
print(len(forbidden_cell))
print("\n\n")
print(sampling_cell)
print(len(sampling_cell))