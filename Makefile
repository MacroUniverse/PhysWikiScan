goal: main.cpp lib/PhysWikiScan.h
	g++ -fmax-errors=1 -O3 --std=c++11 -Wall -Wno-sign-compare -Wno-reorder -Wno-misleading-indentation main.cpp -l:libSQLiteCpp.a  -l:libSQLiteCpp-sqlite3.a -l pthread -l dl -o PhysWikiScan

clean:
	rm -f PhysWikiScan *.o *.gch
