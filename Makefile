# q1: q1.o	
# 	g++ q1.o

# q1.o: q1.cpp
# 	g++ -c q1.cpp

create: q3
	rm *.o

q3: q3.o	
	g++ q3.o -o output3

q3.o: q3.cpp
	g++ -c q3.cpp 

