#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>

#include <string>
#include <iostream>

using namespace std;

#define PORT_NO 51717

/* baudrate settings are defined in <asm/termbits.h>, which is
   included by <termios.h> */
#define BAUDRATE B38400
//#define BAUDRATE B9600
/* change this definition for the correct port */
#define MODEMDEVICE_0 "/dev/ttyUSB0"
#define MODEMDEVICE_1 "/dev/ttyUSB1"

#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int init_serial(char *MODEMDEVICE, struct termios *p_oldtio)
{
    int fd;
    struct termios newtio;
    /* 
       Open modem device for reading and writing and not as controlling tty
       because we don't want to get killed if linenoise sends CTRL-C.
    */
    fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(MODEMDEVICE); exit(-1); }

    tcgetattr(fd, p_oldtio); /* save current serial port settings */
    bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

    /* 
       BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
       CRTSCTS : output hardware flow control (only used if the cable has
       all necessary lines. See sect. 7 of Serial-HOWTO)
       CS8     : 8n1 (8bit,no parity,1 stopbit)
       CLOCAL  : local connection, no modem contol
       CREAD   : enable receiving characters
    */
    newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;

    /*
      IGNPAR  : ignore bytes with parity errors
      ICRNL   : map CR to NL (otherwise a CR input on the other computer
      will not terminate input)
      otherwise make device raw (no other input processing)
    */
    newtio.c_iflag = IGNPAR | ICRNL;

    /*
      Raw output.
    */
    newtio.c_oflag = 0;

    /*
      ICANON  : enable canonical input
      disable all echo functionality, and don't send signals to calling program
    */
    newtio.c_lflag = ICANON;

    /* 
       initialize all control characters 
       default values can be found in /usr/include/termios.h, and are given
       in the comments, but we don't need them here
    */
    newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
    newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
    newtio.c_cc[VERASE]   = 0;     /* del */
    newtio.c_cc[VKILL]    = 0;     /* @ */
    newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
    newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
    newtio.c_cc[VSWTC]    = 0;     /* '\0' */
    newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
    newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
    newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
    newtio.c_cc[VEOL]     = 0;     /* '\0' */
    newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
    newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
    newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
    newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
    newtio.c_cc[VEOL2]    = 0;     /* '\0' */

    /* 
       now clean the modem line and activate the settings for the port
    */
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    return fd;
}

int main(int argc, char *argv[])
{
    int n;
    char buffer[256];
     
    int sockfd, newsockfd, portno = PORT_NO;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
     
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    if (argc > 1)
        portno = atoi(argv[1]);
     
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
        error("ERROR on binding");
     
    listen(sockfd, 1);
    clilen = sizeof(cli_addr);

    struct termios oldtio_0;
    struct termios oldtio_1;
    int sfd_0 = init_serial((char *)MODEMDEVICE_0, &oldtio_0);
    int sfd_1 = init_serial((char *)MODEMDEVICE_1, &oldtio_1);

    while (1) {
        printf("Waiting for new connection in port (%d) ...\n", portno);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        printf("Connected in socket(%d)\n", newsockfd);
        STOP = FALSE;

        /*
          send commands to arduinoes for rotating cube
          needs to send both of arduinoes
        */
     
        while (STOP == FALSE) {
            bzero(buffer, 256);
            n = read(newsockfd, buffer, 255);
            if (n < 0) error("ERROR reading from socket");
            else if (n == 0) STOP = TRUE;

            char tbuf[256];
            string cmd_str(buffer);
            string str_0 = "";
            string str_1 = "";
            cout << "Cube command: " << cmd_str << endl;
            for (unsigned int i = 0; i < cmd_str.length(); i++) {
                if (cmd_str[i] == 'R') {
                    if (cmd_str[i+1] == '\'') {
                        str_0 += "r";
                    }
                    else {
                        str_0 += "R";
                    }
                }
                else if (cmd_str[i] == 'F') {
                    if (cmd_str[i+1] == '\'') {
                        str_0 += "f";
                    }
                    else
                        str_0 +="F" ;
                }
                else if (cmd_str[i] == 'U') {
                    if (cmd_str[i+1] == '\'') {
                        cmd_str[i] = 'u';
                        str_0 += cmd_str[i];
                    }
                    else
                        str_0 += cmd_str[i];
                }
    

                if (str_0.length() > 0) {
                    //cout << str_0 << endl;
                    int n = write(sfd_0, str_0.c_str(), str_0.length());
                    if (n < 0) error("ERROR writing to arduino #1");
                    str_0 = "";
                    while (tbuf[0] != '1') {
                        read(sfd_0, tbuf, 1);
                        printf("from Arduino_1: %c\n", tbuf[0]);
                    }
                    bzero(tbuf, 256);
                }

                if (cmd_str[i] == 'B') {
                    if (cmd_str[i+1] == '\'') {
                        str_1 += "b";
                    }
                    else
                        str_1 += "B";
                }
      
                if (cmd_str[i] == 'D') {
                    if (cmd_str[i+1] == '\'') {
                        str_1 += "d";
                    }
                    else
                        str_1 += "D";
                }

                if (cmd_str[i] == 'L') {
                    if (cmd_str[i+1] == '\'') {
                        str_1 += "l";
                    }
                    else
                        str_1 += "L";
                }
     
                if (str_1.length() > 0) {
                    //cout << str_1 << endl;
                    int n = write(sfd_1, str_1.c_str(), str_1.length());
                    if (n < 0) error("ERROR writing to arduino #2");
                    str_1 = "";
                    while (tbuf[0] != '2') {
                        read(sfd_1, tbuf, 1);
                        printf("from Arduino_2: %c\n", tbuf[0]);
                    }
                    bzero(tbuf, 256);
                }
            }
        }
        close(newsockfd);
        printf("Socket(%d) is closed\n");
    }
    
    /* restore the old port settings */
    tcsetattr(sfd_0, TCSANOW, &oldtio_0);
    tcsetattr(sfd_1, TCSANOW, &oldtio_1);

    close(newsockfd);
    close(sockfd);
    return 0;
}
