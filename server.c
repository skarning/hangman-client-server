#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <string.h>
#include <time.h>
#include <pthread.h>


struct loginData{
    char username[256];
    char password[256];
};

int port;
void startGame(int clientSocketFd,struct loginData *loginDataPTR,int threadnumber);
void displayLeaderboard(int clientSocketFD,int threadnumber);
void onQuitHandler(int signal);
int setupConnection();
int checkLoginData(struct loginData *loginDataPTR);
void removeSpaces();
void removeTrailingBreak();
int sendToClient();
void receiveMenuData();
int recieveGameData(int clientSocketFD);
void *runner(void *param);
void recieveGuessedLetters(int clientSocketFD,char* str);

struct userstatistics{
    char username[256];
    int gameswon;
    int gamesplayed;
};
struct userstatistics userstats[100];

struct word{
    char thing[256];
    char word[256];
};

struct sockaddr_in server;
struct sockaddr_in client;
socklen_t sizeOfStruct;

pthread_mutex_t idMutex;
pthread_mutex_t lgMutex;

int id=0;

int main(int argv,char *argc[]){
    signal(SIGINT,onQuitHandler);

    if(argv<2){
        printf("Invalid arguments...");
        exit(1);
    }
    //Setting portnumber
    port=atoi(argc[1]);
    
    int clientSocketFD;
    int socketFD;
    pthread_t tid[10];
    //Starting connection and authentication
    socketFD=setupConnection();
    int byteCount=0;
    socklen_t sin_size=sizeof(struct sockaddr_in);
    
    for(int i=0;i<10;i++){
        //Setting thread data
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        clientSocketFD=accept(socketFD,(struct sockaddr *)&client,&sin_size);
        pthread_create(&tid[i],&attr,runner,&clientSocketFD);
    }
    //Closing network socket
    for(int i=0;i<10;i++){
        pthread_join(tid[i],NULL);
    }
    printf("Closing...");
    close(socketFD);
    return 1;
}

void displayLeaderboard(int clientSocketFD,int threadnumber){
    char emptyArray[256]="";
    char sendingArray[256];
    int counter=0;
    for(int i=0;i<100;i++){
        if(counter==0){
            send(clientSocketFD,userstats[i].username,256,0);
        }
        counter++;
        if(strcmp(emptyArray,userstats[i].username)!=0){
                sprintf(sendingArray,"%d",userstats[i].gameswon);
                send(clientSocketFD,sendingArray,256,0);
                sprintf(sendingArray,"%d",userstats[i].gamesplayed);
                send(clientSocketFD,sendingArray,256,0);
        }
    }
}

void startGame(int clientSocketFD, struct loginData *loginDataPTR,int threadnumber){
    strcpy(userstats[0].username,loginDataPTR->username);
    struct word word;
    //Reading words from file and selecting an random one
    FILE *fs;
    char textbuffer[256];
    fs=fopen("hangman.txt","r");
    int lineCounter=1;
    while(fgets(textbuffer,256,(FILE*)fs)!=NULL){
        lineCounter++;
    }
    fclose(fs);
    FILE *fp;
    fp=fopen("hangman.txt","r");
    srand(time(NULL));
    int r=rand()%lineCounter+1;
    int setWord=0; 
    while(fgets(textbuffer,256,(FILE*)fp)!=NULL){
        setWord++;
        char *token;
	token=strtok(textbuffer,",");
        int counter=0;
        while(token!=NULL){
            if(counter==0){
	        strcpy(word.thing,token);
	        token=strtok(NULL,",");
	    }
		       
	    if(counter==1){
                removeTrailingBreak(token);
	        strcpy(word.word,token);
	        break;
	    }
	counter++;
        }
        if(r==setWord){
            break;
        }
    }
    fclose(fp);
    int lI=strlen(word.word);
    int lC=strlen(word.thing);
    int guesses=lI+lC+10;
    int guessCounter=0;
    char guessedLetters[256]="";

    char word1[256];
    char word2[256];  
    for(int i=0;i<256;i++){
        if(i<lI){
            word1[i]='_';
        }
        else{
            word1[i]='\0';
        }
    }
    for(int i=0;i<256;i++){
        if(i<lC){
             word2[i]='_';
        }
        else{
            word2[i]='\0';
        }
    }
    char numberOfGuesses[256];
    char closeGame[256]="close";
    char wonGame[256]="won";
    char empty[256];
    int isGameWon=0;
    while(guessCounter<=guesses){
        if(guesses==0){
            userstats[threadnumber].gamesplayed++;
            sendToClient(clientSocketFD,closeGame);
            break;
        }
        else if(isGameWon==1){
            userstats[threadnumber].gameswon++;
            userstats[threadnumber].gamesplayed++;
            sendToClient(clientSocketFD,wonGame);
            sendToClient(clientSocketFD,word1);
            sendToClient(clientSocketFD,word2);
            break;
        }
        else{sendToClient(clientSocketFD,empty);}
        sprintf(numberOfGuesses,"%d",guesses);
        sendToClient(clientSocketFD,guessedLetters);
        sendToClient(clientSocketFD,numberOfGuesses);
        sendToClient(clientSocketFD,word1);
        sendToClient(clientSocketFD,word2);
        char guessedBuff[256];
        recieveGuessedLetters(clientSocketFD,guessedBuff);
        strcat(guessedLetters,guessedBuff);
        guesses--;

        isGameWon=1;

        for(int i=0;i<256;i++){
            if(guessedBuff[0]==word.word[i]) {
                word1[i]=guessedBuff[0];
            }
            if(guessedBuff[0]==word.thing[i]){
                word2[i]=guessedBuff[0];
            }
            if(word1[i]=='_'){
                isGameWon=0;
            }
            if(word2[i]=='_'){
                isGameWon=0;
            }
        }
        if(isGameWon==1){
            guesses++;
        }
    }
}

int setupConnection(){
    int socketBind=0;
    int socketFD;
    //Creating a socket
    socketFD=socket(AF_INET,SOCK_STREAM,0);
    if(socketFD==-1){
	printf("Could not create a socket");
        exit(1);
    }
    
    //Setting server connection data
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=INADDR_ANY;

    //Binding socket to endpoint
    socketBind=bind(socketFD,(struct sockaddr *)&server,sizeof(server));
    if(socketBind==-1){
        printf("Could not bind socket to endpoint\n");
        perror("bind");
        exit(1);
    }

    if(listen(socketFD,3)==-1){
        printf("Could not listen");
        exit(1);
    }
   return socketFD;
}

//Handles ctrl-c
void onQuitHandler(int signal){
    int closeStatus;//=close(socketFD);
    if(closeStatus==-1){
        printf("\nSocket was not properly closed\n");
        exit(1);
    }
    printf("\nSocket was properly closed\n");
    exit(1);   
}

//Checking if the login data form the client is correct
    int checkLoginData(struct loginData *loginDataPTR){
    struct loginData loginDataBuffer;
    FILE *fp;
    char textbuffer[256];
    fp=fopen("authentication.txt","r");
    while(fgets(textbuffer,256,(FILE*)fp)!=NULL){
        char *token;
	token=strtok(textbuffer,"\t");
        int counter=0;
        while(token!=NULL){
            if(counter==0){
	        strcpy(loginDataBuffer.username,token);
	        token=strtok(NULL,"\t");
	    }
		       
	    if(counter==1){
	        strcpy(loginDataBuffer.password,token);
	        break;
	    }
	counter++;
        }
        removeSpaces(loginDataBuffer.username);
        removeTrailingBreak(loginDataBuffer.password);
        if(strcmp(loginDataBuffer.username,loginDataPTR->username)==0&&strcmp(loginDataBuffer.password,loginDataPTR->password)==0){
            return 1;
        }
    }	    
    fclose(fp);
    return 0;
}

//Sending data to client
int sendToClient(int sckFD,char* inp){
    int byteSent;
    resend:
    byteSent=send(sckFD,inp,256,0);
    if(byteSent==-1){
        perror("send");
        exit(1);
    }
    if(byteSent!=256){
        goto resend;
    }
    return 1;
}

//Getting client menu choice
int recieveGameData(int clientSocketFD){
    char resbuff[256];
    int byteCount=0;
    while(1){
        byteCount=recv(clientSocketFD,resbuff,256,0);
        if(byteCount==256){
            break;
        }
    }
    int choice=atoi(resbuff);
    return choice;
}

void recieveGuessedLetters(int clientSocketFD,char* str){
    char resbuff[256];
    int byteCount=0;
    while(1){
        byteCount=recv(clientSocketFD,resbuff,256,0);
        if(byteCount==256){
            break;
        }
    }
    strcpy(str,resbuff);
}

//Removing the spaces from word in file
void removeSpaces(char* str){
    char *p1=str,*p2=str;
    do{
        while (*p2==' '){
        p2++;
        }
    }
    while (*p1++=*p2++);
}

//Removing the linebreak from word in file
void removeTrailingBreak(char* str){
    char *p3=str,*p4=str;
    do{
        while(*p4=='\n'){
        p4++;
        }
    }
    while (*p3++=*p4++);
}

void *runner(void *clientSocketFD){
    //The threads are running this
    int threadnumber;
    threadnumber=id;
    id++;
    int counter=0;
    int byteCount=0;
    struct loginData *logindataPTR,logindata;
    logindataPTR=&logindata;
    char loginBuff[256];
    while(1){
        byteCount=recv(*((int*)clientSocketFD),loginBuff,256,0);
        if(counter==0){
            strcpy(logindata.username,loginBuff);
        }
        if(counter==1){
            strcpy(logindata.password,loginBuff);
            break;
        }
        counter++;
    }
    int csckID = *((int*)clientSocketFD);
    int loginstatus=checkLoginData(logindataPTR);
    if(loginstatus==0){
        char sendingDataBuffer[256]="0";
        sendToClient(*((int*)clientSocketFD),sendingDataBuffer);
    }
    else if(loginstatus==1){
        char sendingDataBuffer[256]="1";
        sendToClient(*((int*)clientSocketFD),sendingDataBuffer);

        while(1){
            pthread_mutex_lock(&lgMutex);
            int gameChoice=recieveGameData(csckID);
            pthread_mutex_unlock(&lgMutex);
            if(gameChoice==1){
                //client starts a game
                startGame(csckID,logindataPTR,threadnumber);
            }
            else if(gameChoice==2){
                //client presses leaderboard
                displayLeaderboard(csckID,threadnumber);
            }
           else{
                //When client quits a game
                pthread_exit(0);
            }
        }
    }
}
