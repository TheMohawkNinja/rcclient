#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdlib>
#include <pthread.h>
#include <curses.h>

using namespace std;

char buf[4096];
int bytesReceived,sendRes,sock;
int connectRes=INT16_MAX;
bool LButtonPressed,RButtonPressed;
string input,curX,curY,oldCurX,oldCurY;
string LButtonState,oldLButtonState,RButtonState,oldRButtonState;

string getCmdOut(string cmd)
{
        string data;
        FILE* stream;
        const int max_buffer = 256;
        char buffer[max_buffer];
        cmd.append(" 2>&1");
        stream=popen(cmd.c_str(), "r");

        if(stream)
        {
                while (!feof(stream))
                if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
                pclose(stream);
        }

        return data;
}
void sendData(string d, int s)
{
	sendRes=send(s,d.c_str(),d.size()+1,0);

        if(sendRes==-1)
        {
                cout<<"Could not send data to server!\r\n";
		sleep(1000);
        }

        //Wait for response
        memset(buf,0,4096);
	bytesReceived=recv(s,buf,4096,0);

        //Display response
	if(bytesReceived>0)
	{
       		cout<<"SERVER> "<<string(buf,bytesReceived)<<"\r\n";
	}
}
void *getMouseState(void* tid)
{
	do
        {
                //Mouse position
                curX=getCmdOut("xinput --query-state 13 | grep -E \"valuator\\[0\\]\" | sed -e 's/^[[:space:]]*//'");	//X
                curY=getCmdOut("xinput --query-state 13 | grep -E \"valuator\\[1\\]\" | sed -e 's/^[[:space:]]*//'");	//Y
                if(curX!=oldCurX)
                {
                        sendData(curX,sock);
                        oldCurX=curX;
                }
                if(curY!=oldCurY)
                {
                        sendData(curY,sock);
                        oldCurY=curY;
                }

                //Mouse buttons
                LButtonState=getCmdOut("xinput --query-state 13 | grep -E \"button\\[1\\]\" | sed -e 's/^[[:space:]]*//'");	//LMB
                RButtonState=getCmdOut("xinput --query-state 13 | grep -E \"button\\[3\\]\" | sed -e 's/^[[:space:]]*//'");	//RMB

                if(LButtonState!=oldLButtonState)
                {
                        cout<<LButtonState<<endl;
                        sendData(LButtonState,sock);
                        oldLButtonState=LButtonState;
                }
                if(RButtonState!=oldRButtonState)
                {
                        cout<<RButtonState<<endl;
                        sendData(RButtonState,sock);
                        oldRButtonState=RButtonState;
                }
        }while(true);

	close(sock);

	pthread_exit(NULL);
}
void *getKeyboardState(void* tid)
{
	int ch;

	initscr();
        cbreak();
	noecho();

	while(true)
	{
		ch=getch();
		sendData(to_string(ch),sock);
		cout<<"Key: "<<ch<<endl;
	}

	close(sock);

        pthread_exit(NULL);
}
int main()
{
	int mouseThreadErr,keyboardThreadErr;
	bool LButtonPressed,RButtonPressed;
	string LButtonState,oldLButtonState,RButtonState,oldRButtonState;
	pthread_t threads[2];

	//Create a hint structure for the server we're connecting with
	sock=socket(AF_INET,SOCK_STREAM,0);
	int port=54000;
	string ipAddress="192.168.1.10";
	sockaddr_in hint;
	hint.sin_family=AF_INET;
	hint.sin_port=htons(port);
	inet_pton(AF_INET,ipAddress.c_str(),&hint.sin_addr);

	curX="";
	curY="";
	oldCurX="";
	oldCurY="";
	oldLButtonState="";
	oldRButtonState="";

	if(sock==-1)
	{
		cout<<"Unable to create socket!"<<endl;
	}

	//Connect to the server on the socket
	connectRes=connect(sock,(sockaddr*)&hint,sizeof(hint));

	if(connectRes==-1)
	{
		cout<<"Could not connect to server!"<<endl;
		return -1;
	}

	//Start threads for input listeners
	mouseThreadErr=pthread_create(&threads[0],NULL,getMouseState,(void *)0);
	keyboardThreadErr=pthread_create(&threads[1],NULL,getKeyboardState,(void *)0);

	if(mouseThreadErr)
	{
        	cout<<"Unable to create thread:"<<mouseThreadErr<<endl;
        	return -2;
	}
	if(keyboardThreadErr)
        {
                cout<<"Unable to create thread:"<<mouseThreadErr<<endl;
                return -3;
        }

	pthread_exit(NULL);
}
