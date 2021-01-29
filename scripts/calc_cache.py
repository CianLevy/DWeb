# Using readline() 
file1 = open('./req50.txt', 'r') 
count = 0
hit_count=0
miss_count=0
from_sec=0
while True: 
    count += 1
  
    # Get next line from file 
    line = file1.readline() 
  
    # if line is empty 
    # end of file is reached 
    if not line: 
        break
    str1=("Line{}: {}".format(count, line.strip())) 
    str2=str1.split() 
    str3=str2[6]
    if from_sec>0:
     str4=float(str3)
     print(str3)
     if str4==0:
      hit_count=hit_count+1
     else:
      miss_count=miss_count+1

    from_sec=from_sec+1  
file1.close() 
hit_count=hit_count/2
miss_count=miss_count/2
hit_ratio=((hit_count)/(hit_count+miss_count))*100
miss_ratio=((miss_count)/(hit_count+miss_count))*100

print (hit_ratio)
print (miss_ratio)

