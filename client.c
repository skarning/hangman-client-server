#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int port;
int socketFD;
int serverSocketFD;
int connection;
int socketBind;
int socketListen;
int byteCount;
char ipAdress[32];
char input[256];
char *newLine;
char buff[256];
char userName[256];

void sendDataToServer();
void getDataFromUser(char message[256]);
void launchMenu();
void recieveData();
int getMenuChoice();
void startGame();
void sendGameDataToServer(char *str);
void displayLeaderboard();

struct sockaddr_in client;
struct sockaddr_in server;

int main(int argv,char *argc[]){


    if(argv!=3){
        printf("You will need to provide server IP and portnumber");
        exit(1);
    }
    //Setting portnumber
    port=atoi(argc[2]);
    //setting IP
    strcpy(ipAdress, argc[1]);
    
    //Creating a socket
    socketFD=socket(AF_INET,SOCK_STREAM,0);
    if(socketFD==-1){
	printf("Could not create a socket");
        exit(1);
    }
    //Setting server connection data
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ipAdress);
    
    //Connecting client to server
    connection=connect(socketFD,(struct sockaddr *)&server, sizeof(struct sockaddr));
    if(connection==-1){
        printf("Could not create a connection\n");
        exit(1);
    }
    
    launchMenu();
    getDataFromUser("Enter Username-->");
    sendDataToServer();
    strcpy(userName,input);
    getDataFromUser("Enter Password-->");
    sendDataToServer();
    
    recieveData();
    int answer=atoi(buff);
    if(answer==1){
       while(1){
           int choice=getMenuChoice();
            if(choice==1){
                startGame();
            }
            if(choice==2){
                displayLeaderboard();
            }
            if(choice==3){
                printf("Dette er meget sært");
                sendGameDataToServer("3");
                exit(1);
            }
        }
    }
    else if(answer==0){
        printf("You entered either a incorrect password or username-disconnecting");
    }
    close(socketFD);
    return 1;
}

void sendDataToServer(){
    int bytesSent;
    bytesSent=0;
    while(1){
        bytesSent=send(socketFD,input,256,0);
        if(bytesSent==-1){printf("Could not be sent");}
        if(bytesSent>=256){
            break;
        }
    }
}

void sendGameDataToServer(char *str){
    int bytesSent;
    bytesSent=0;
    bytesSent=send(socketFD,str,256,0);
    if(bytesSent==-1){printf("Could not be sent");}
}

void getDataFromUser(char message[256]){
    printf("%s", message);
    scanf("%s",input);
}

void launchMenu(){
    printf("\n================================================================================\n\n\n");
    printf("                Welcome to the online Hangman gaming system\n\n\n");
    printf("================================================================================\n");
}

void recieveData(){
    int byteCount;
    reres:
    byteCount=recv(socketFD,buff,256,0);
    if(byteCount==-1){
        printf("could not recieve data");
        perror("recv");
        exit(1);
    }
    if(byteCount!=256){
        goto reres;
    }
}

void startGame(){
    char myGuess[256];
    char emptyChar[256];
    char closeGame[256]="close";
    char wonGame[256]="won";
    sendGameDataToServer("1");
    while(1){
        recieveData();
        if(strcmp(buff,closeGame)==0){
            printf("\nGame Over\n");
            printf("\nBad luck %s! You have run out of guesses. The Hangman got you!\n",userName);
            break;
        }
        else if(strcmp(buff,wonGame)==0){
            printf("\nCongrats you won the game of Hangman!\n");
            recieveData();
            printf("\nFinal Word:%s",buff);
            recieveData();
            printf(" %s\n",buff);
            break;
        }
        recieveData();
        printf("\nGuessed letters: %s\n",buff);
        recieveData();
        printf("\nGuesses left: %s\n",buff);
        recieveData();
        printf("\nWord:%s",buff);
        recieveData();
        printf(" %s\n",buff);
        retry:
        printf("\nEnter your guess - ");
        scanf("%s",myGuess);
        if(strlen(myGuess)!=1){
            printf("\nOnly one character please...\n");
            strcpy(myGuess,emptyChar);
            goto retry;
        }
        if(strcmp(myGuess,emptyChar)==0){
            printf("\nNo character recorded\n");
            strcpy(myGuess,emptyChar);
            goto retry;
        }
        strcpy(input,myGuess);
        sendDataToServer();
        printf("\n\n-----------------------------------------------------------------------------\n\n");
    }
}

int getMenuChoice(){
    printf("\n\nPlease enter a selection\n<1>Play Hangman\n<2>Show Leaderboard\n<3>Quit\nSelection option 1-3 ->");
    char charChoice[256];
    scanf("%s",charChoice);
    int intChoice=atoi(charChoice);
    return intChoice;
}

void displayLeaderboard(){
        char empty[256]="";
        printf("\n\n================================================================================================================\n\n");
        sendGameDataToServer("2");
        for(int i=0;i<10;i++){
        recv(socketFD,buff,256,0);
        printf("Buff:%s",buff);
        if(strcmp(buff,empty)==0){
            printf("There is currently no information currently stored in the leaderboard");
        }
        else{
            recv(socketFD,buff,256,0);
            printf("Number of games won - %s\n",buff);
            recv(socketFD,buff,256,0);
            printf("Number of games played - %s\n",buff);
        }
    }
    printf("\n\n================================================================================================================\n");
}
