/*Chido Nguyen
931506965
Project 2: FTP Operations using Client-Server (2 connections) model and Socket API
Citations: Majority of code is reused from project 1 or from CS344 Socket Block with brewster.
*/
#include <dirent.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

int BIGNUMBER = 300000000; // buffer size 


/*Setup a socket and return the i/o stream int value for socket*/
/*return the file descriptor number*/
int new_connection_setup(int portNum){
	int listenSocketFD;
	struct sockaddr_in serverAddress;
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clears everything out.
	serverAddress.sin_family = AF_INET; // generate socket with network capability 
	serverAddress.sin_port = htons(portNum); // store port #
	serverAddress.sin_addr.s_addr = INADDR_ANY; // allow for any address to connect
	listenSocketFD = socket(AF_INET,SOCK_STREAM, 0); // init socket
	//BEEJS!
	int yes=1;
	// lose the pesky "Address already in use" error message
	if (setsockopt(listenSocketFD,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		perror("setsockopt");
		exit(1);
	} 
	if(bind(listenSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		perror("Binding of socket and port failed");
		exit(1);
	}
	return listenSocketFD;
}
/* parse out arguments from client command via string tokens*/
/* pulled from old 344 usage in smallsh assignment*/
void Tokenize( char * command, char ** args){
	char *token; // storage
	token = strtok(command, " ");
	int i = 0;
	while(token != NULL){
		args[i] = token;
		i++;
		token = strtok(NULL, " " );
	}
}

/*Prints out items in directory*/
//https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
//https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
void get_dir(char** list_store,int* x){
	DIR *dir;
	struct dirent *ent;
	dir = opendir(".");
	while((ent = readdir(dir)) != NULL){
		if(strcmp(ent->d_name,".") != 0 && strcmp(ent->d_name, "..") !=0){
			list_store[(*x)] = ent->d_name;
			(*x)++;
		}
	}
	
	closedir(dir);
}

/*Pulls all items in current working directory and sends it to the client*/
void direct_me(int socket){
	char* listing[500];
	int k = 0;
	get_dir(listing, &k);
	char fusion[2048];
	memset(fusion, '\0', sizeof(fusion));
	const char* nLine = "\n";
	int i;
	for(i = 0; i < k;i++){
		strcat(fusion,listing[i]);
		strcat(fusion,nLine);
	}
	int x = strlen(fusion);
	send(socket, &x, sizeof(x),0);
	
	send(socket,fusion, sizeof(fusion), 0);
	
}
/*transfer_me: opens file via name provided and transfer file content*/
void transfer_me(int socket, char* fileName){
	FILE *inFile;
	inFile = fopen(fileName, "r");
	if(!inFile){
		perror("Failed to Open\n");
		fflush(stderr);
		int error = 0;
		send(socket , &error, sizeof(error),0);
		return;
	}
	else{
		int x = 0;
		char *file_buffer;
		file_buffer = (char *) malloc( sizeof(char) * BIGNUMBER);
		memset(file_buffer, '\0',sizeof(file_buffer));
		char c = getc(inFile);
		while(c != EOF){
			file_buffer[x] = c;
			x++;
			c=getc(inFile);
		}
		//OTP PROGRAM FROM 344 HELPS WITH THIS SECTION//
		send(socket, &x, sizeof(x), 0); // https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c
		int sent = 0;
		char *ptr;
		ptr = file_buffer;
		while(sent < x ){
			sent += send(socket,ptr, x, 0);
			ptr += sent;
		}
	}
	fclose(inFile);
	free(file_buffer);
}

//./ftserver [port#]
int main(int argc, char* argv[]){
	////////////	SOCKET SETUP	////////////////////
	//Code from CS344 Reused//
	int portNumber;
	int establishedConnectionFD;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in clientAddress;
	
	
	// Check to make sure only 2 arguments are used total
	if(argc != 2){
		fprintf(stderr, "%s","Not enough arguments.\nUSAGE: ./ftserver [portnumber] \n");
		exit(1);
	}
	portNumber = atoi(argv[1]); // convert string to int from our user directed port#
	int listenSocketFD = new_connection_setup(portNumber); //setup our socket 
	if ( listenSocketFD > 0){
		printf("Server Started, Port: %i\n",portNumber);
	}
	listen(listenSocketFD, 5); // turns socket on , 5 connections max
	//////////////////////////////////////////////////////////
	
	//Server "alive" setup//

	while(1){
		char command[1024];
		memset(command, '\0', sizeof(command));
		sizeOfClientInfo = sizeof(clientAddress);
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		recv(establishedConnectionFD, command, 1024, 0); // tested 
		char* arg[32];
		memset(arg, 0 , sizeof(char)*32);
		Tokenize(command,arg); // parse out arguments 
		/*
		./program server port command port
		*/
		
		//data port//
		int dataport;
		int dataConFD;
		int estCon;
		socklen_t sizeOfClientInfo2;
		struct sockaddr_in clientAddress2;
		if(strcmp(arg[3], "-l") == 0){
			//handles the "get" directory command
			printf("Get Directory Command\n");
			fflush(stdout);
			dataport = atoi(arg[4]);
			dataConFD = new_connection_setup(dataport);
			listen(dataConFD,1);
			sizeOfClientInfo2 = sizeof(clientAddress2);
			estCon = accept(dataConFD, (struct sockaddr *)&clientAddress2, &sizeOfClientInfo2);
			if(estCon > 0){
				printf("Sending Directory, Port: %i\n",dataport);
				fflush(stdout);
			}
			direct_me(estCon);
 
			close(estCon);
			close(dataConFD);
		}
		else if(strcmp(arg[3],"-g") == 0){
			printf("Get File Command\n");
			fflush(stdout);
			dataport = atoi(arg[5]);
			dataConFD = new_connection_setup(dataport);
			listen(dataConFD,1);
			sizeOfClientInfo2 = sizeof(clientAddress2);
			estCon = accept(dataConFD, (struct sockaddr *)&clientAddress2, &sizeOfClientInfo2);
			if(estCon > 0){
				printf("Sending File, Port: %i\n",dataport);
				fflush(stdout);
			}
			transfer_me(estCon, arg[4]);
			close(estCon);
			close(dataConFD);
		}
		else
			printf("Invalid Commands: format should be 'ftclient host port command port'\n");
		close(establishedConnectionFD);
	}
	close(listenSocketFD);

	return 0;
}