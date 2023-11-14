all: TWClient TWServer

Message.o: Message.cpp Message.h
	g++ -std=c++14 -Wall -Werror -c -o Message.o Message.cpp

mypw.o: mypw.cpp mypw.h
	g++ -std=c++14 -Wall -Werror -c -o mypw.o mypw.cpp

myldap.o: myldap.cpp mypw.h
	g++ -std=c++14 -Wall -Werror -c -o myldap.o myldap.cpp -I/path/to/ldap/include

TWClient: TWClient.cpp Message.o mypw.o
	g++ -std=c++14 -Wall -Werror -o twmailer-client TWClient.cpp Message.o mypw.o

TWServer: TWServer.cpp Message.o myldap.o
	g++ -std=c++14 -Wall -Werror -o twmailer-server TWServer.cpp Message.o myldap.o -lldap -llber

clean:
	rm -f twmailer-server twmailer-client Message.o myldap.o mypw.o