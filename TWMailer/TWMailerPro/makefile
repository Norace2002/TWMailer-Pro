all: TWClient TWServer

Message.o: Message.cpp Message.h
		g++ -std=c++14 -Wall -Werror -c -o Message.o Message.cpp

myldap.o: myldap.cpp mypw.h
		g++ -std=c++14 -Wall -Werror -c -o myldap.o myldap.cpp

TWClient: TWClient.cpp Message.o
		g++ -std=c++14 -Wall -Werror -o twmailer-client TWClient.cpp Message.o

TWServer: TWServer.cpp Message.o myldap.o
		g++ -std=c++14 -Wall -Werror -o twmailer-server TWServer.cpp Message.o myldap.o

clean:
		rm -f twmailer-server twmailer-client Message.o myldap.o