#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <cstdlib>
#include <pthread.h>
#include <curses.h> //-lcurses
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <tgmath.h>

using namespace std;

char buf[4096];
int bytesReceived,sendRes,sock;
int connectRes=INT16_MAX;
bool LButtonPressed,RButtonPressed;
string input;

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
int HexStrToInt(string str)
{
	int out=0;

	if(str.substr(0,1)=="-")
	{
		return stoi(str);
	}

	for(int i=1; i<=str.length(); i++)
        {
        	if(str.substr(i-1,1)=="a")
                {
                	out+=pow(16,str.length()-i)*10;
                }
                else if(str.substr(i-1,1)=="b")
                {
                        out+=pow(16,str.length()-i)*11;
                }
                else if(str.substr(i-1,1)=="c")
                {
                        out+=pow(16,str.length()-i)*12;
                }
                else if(str.substr(i-1,1)=="d")
                {
                        out+=pow(16,str.length()-i)*13;
                }
                else if(str.substr(i-1,1)=="e")
                {
                        out+=pow(16,str.length()-i)*14;
                }
                else if(str.substr(i-1,1)=="f")
                {
                        out+=pow(16,str.length()-i)*15;
                }
                else
                {
			out+=pow(16,str.length()-i)*stoi(str.substr(i-1,1));
                }
	}
	return out;
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
	int fd, code, x, y;
        int mbuf[72], oldmbuf[72];
        string codeStr;

        fd=open("/dev/input/by-id/usb-PixArt_USB_Optical_Mouse-event-mouse", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);
	//fd=open("/dev/input/mice", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);

        if(fd==-1)
        {
                perror("open_port: Unable to open mouse input file - ");
        }

        //Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
        fcntl(fd, F_SETFL, 0);

        while(true)
        {
                int n=read(fd, (void*)mbuf, 255);

                cout<<"\nn="<<n<<endl;

                if(n<0)
                {
                        perror("Read failed - ");
                }
                else if(n==0)
                {
                        perror("No data on port");
                }
		else
                {
                        //Get mouse state
			x=0;
			y=0;
                        for(int i=0; i<n; i++)//4,6
                        {
				//if(mbuf[i]!=oldmbuf[i]){cout<<"Buf["<<i<<"]="<<std::hex<<mbuf[i]<<"-"<<std::dec<<endl;oldmbuf[i]=mbuf[i];}
				if(i==29)
				{
					cout<<"Buf[4]="<<std::hex<<mbuf[4]<<"-"<<std::dec<<endl;
					cout<<"Buf[5]="<<std::hex<<mbuf[5]<<"-"<<std::dec<<endl;
                        	        cout<<"Buf[10]="<<std::hex<<mbuf[10]<<"-"<<std::dec<<endl;
					cout<<"Buf[11]="<<std::hex<<mbuf[11]<<"-"<<std::dec<<endl;
				}

				if(mbuf[4]< 10000&&HexStrToInt(to_string(mbuf[5]))>10000)//Left
				{
					x=HexStrToInt("FFFFFFFF")-HexStrToInt(to_string(mbuf[5]))+1;
				}
				else if(mbuf[4]<10000&&HexStrToInt(to_string(mbuf[5]))<10000)//Right
				{
					x=HexStrToInt(to_string(mbuf[5]));
				}
				else if(mbuf[4]>10000&&HexStrToInt(to_string(mbuf[5]))>10000)//Up
				{
					y=HexStrToInt("FFFFFFFF")-HexStrToInt(to_string(mbuf[5]))+1;
				}
				else if(mbuf[4]>10000&&HexStrToInt(to_string(mbuf[5]))<10000)//Down
                                {
					y=HexStrToInt(to_string(mbuf[5]));
                                }

				//Second set for diagonal movement
				if(mbuf[10]>0)
				{
					if(mbuf[10]< 10000&&HexStrToInt(to_string(mbuf[11]))>10000)//Left
                	                {
        	                                x=HexStrToInt("FFFFFFFF")-HexStrToInt(to_string(mbuf[11]))+1;
	                                }
	                                else if(mbuf[10]<10000&&HexStrToInt(to_string(mbuf[11]))<10000)//Right
	                                {
	                                        x=HexStrToInt(to_string(mbuf[11]));
	                                }
	                                else if(mbuf[10]>10000&&HexStrToInt(to_string(mbuf[11]))>10000)//Up
                                	{
                                        	y=HexStrToInt("FFFFFFFF")-HexStrToInt(to_string(mbuf[11]))+1;
                                	}
                                	else if(mbuf[10]>10000&&HexStrToInt(to_string(mbuf[11]))<10000)//Down
               	                	{
        	                                y=HexStrToInt(to_string(mbuf[11]));
	                                }
				}
			}

			if(x<0)
			{
				sendData(("MOUSE:x-"+to_string(-1*x)),sock);
			}
			else if(x>0)
                        {
                                sendData(("MOUSE:x+"+to_string(x)),sock);
                        }
			if(y<0)
                        {
                                sendData(("MOUSE:y-"+to_string(-1*y)),sock);
                        }
                        else if(y>0)
                        {
                                sendData(("MOUSE:y+"+to_string(y)),sock);
                        }
			cout<<"Mouse change: ("<<x<<","<<y<<")"<<endl;
		}
	}
}
void *getKeyboardState(void* tid)
{
 	int fd, code;
        int kbuf[72];
        time_t time;
        string timestamp, codeStr;
        unsigned short type;
        unsigned short state;

 	fd=open("/dev/input/by-id/usb-DELL_Dell_USB_Entry_Keyboard-event-kbd", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK);

        if(fd==-1)
        {
                perror("open_port: Unable to open keyboard input file - ");
        }

        //Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
        fcntl(fd, F_SETFL, 0);

        while(true)
        {
                int n=read(fd, (void*)kbuf, 255);

                cout<<"\nn="<<n<<endl;

                if(n<0)
                {
                        perror("Read failed - ");
                }
                else if(n==0)
                {
                        perror("No data on port");
                }
                else
                {
                        //Timestamp
                        time=kbuf[0];
                        timestamp=string(ctime(&time));
                        cout<<"\nTime="<<timestamp;
                        cout<<"Usec="<<kbuf[2]<<endl;

                        //Keycode
                        code=kbuf[10];

                        //Convert keycode integer to hex string
                        std::stringstream stream;
                        stream<<std::hex<<code;
                        codeStr=stream.str();
                        stream.str("");

                        cout<<"Pre-cut Code="<<codeStr<<endl;

                        //Truncate
                        codeStr=codeStr.substr(0,(codeStr.length()-4));

                        //Convert to dec for integer convesion
                        code=0;
			code=HexStrToInt(codeStr);
                        cout<<"Code (dec)="<<code<<endl;
                        cout<<"Code (hex)="<<std::hex<<code<<std::dec<<endl;

                        //State
                        state=kbuf[11];
                        if(state==0)
                        {
                                cout<<"State=Up"<<endl;
                        }
                        else
                        {
                                cout<<"State=Down"<<endl;
                        }
                        cout<<endl;

			sendData(("KEY:"+to_string(code)+"-"+to_string(state)),sock);
                }
        }

	close(fd);
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
