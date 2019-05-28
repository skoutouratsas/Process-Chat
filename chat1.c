#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include "error.h"
#define NAME_SIZE 32
#include <fcntl.h>

//stergios koutouratsas 2162 stathis tsitsopoulos 2283
//programma chat antistoixo me xrisi named pipes gia blockarisma-sigxronismo
static volatile sig_atomic_t gotsig=0;

static void handler(int sig) {
	gotsig=1;
}

int main(int argc, char *argv[]) {
	int key,shmid,first=1,relogged=0,check;
	int fd[8];
	char *offset,user1[NAME_SIZE],user2[NAME_SIZE],message[512];
	
	struct sigaction act={ {0} };
	
	act.sa_handler=handler;
	sigaction(SIGINT,&act,NULL);

	strcpy(user1,argv[1]);
	strcpy(user2,argv[3]);
	
	key=ftok(".",'a');
	error(key);
	shmid=shmget(key, (130+2*NAME_SIZE) , IPC_CREAT | IPC_EXCL | S_IRWXU);
	
	
	if ((shmid==-1)&&(errno==EEXIST)) {
		first=0;
		shmid=shmget(key, (130+2*NAME_SIZE) , IPC_CREAT | S_IRWXU);
		error(shmid);
	}
	else {
		error(shmid);
	}

	offset=(char*)shmat(shmid,NULL,0);
	error((int)*offset);
	
	if(!strcmp(argv[1],offset+1)&&*offset==0){
		first=1;
		relogged=1;
	}

	*(offset+NAME_SIZE+1)='0';
	*(offset+2*NAME_SIZE+67)='0';

	
	if (first==1) {
		strcpy(offset+1,user1);
		printf("Logged in as: %s.\n", user1);
		if(relogged==0){
			//dimiourgia named pipes, mono mia fora mias kai apotigxanei stin epanalipsi
			error(mkfifo("pipe1", S_IRWXU));
			error(mkfifo("pipe2", S_IRWXU));
			error(mkfifo("pipe3", S_IRWXU));
			error(mkfifo("pipe4", S_IRWXU));
			error(mkfifo("pipe5", S_IRWXU));
			printf("Waiting for %s.\n", user2);
			strcpy(offset+NAME_SIZE+67,user2);
			fd[0]=open("pipe5", O_RDONLY);//xristis 1 perimenei ton deutero blockarismenos sthn open 
			error(fd[0]);
		}
		*offset='1';
	}
	else {
		
		if ((strcmp(argv[1],(offset+NAME_SIZE+67)))) {
			printf("%s is trying to communicate with another person!\n", argv[3]);
			exit(1);
		}
		if ((strcmp(argv[3],(offset+1)))) {
			char answer;
			printf("%s is not available.%s is trying to communicate with you.Would you like to connect anyway(y/n)?\n",argv[3], (offset+1) );
			scanf(" %c",&answer);
			getchar();
			if (answer=='n') {
				exit(0);
			}
			strcpy(user2,offset+1);
		}
		
		strcpy((offset+NAME_SIZE+67),user1);
		printf("Logged in as: %s.\n",user1);
		*(offset+NAME_SIZE+66)='1';
		fd[1]=open("pipe5", O_WRONLY); //xristis2 syndeetai kai anoigei to write end tou pipe ksemplokarontas ton prwto
		error(fd[1]);
		check=(close(fd[1]));
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		

	}
	if (first) {
		check=fork();
		error(check);
		if (!check) {//grapsimo xristi1
			printf("Enter message: \n");
			while (gotsig!=1) {
				int len;
				char *enter;
				fgets(message,511,stdin);
				if (gotsig==1) {
					break;
				}
				enter=strrchr(message, '\n');
				*enter='\0';
				if(!strcmp(message,"quit")){
					error(kill(0,SIGINT));
					exit(0);
				}
				
				len=strlen(message);
				if (len<64) {
					fd[0]=open("pipe2", O_RDONLY);		//mporw na grapsw sthn shm?
					
					error(fd[0]);
					
					strcpy((offset+NAME_SIZE*2+68),message);
					
					check=(close(fd[0])); 
					if(check==-1&&errno!=EBADF){
						error(check);
					}
					
					fd[1]=open("pipe1", O_WRONLY);	//egrapsa!
					
					error(fd[1]);
					
					check=(close(fd[1]));
					if(check==-1&&errno!=EBADF){
						error(check);
					}
				}
				else {
					int repzZz=len/64+1,i;
					for (i=0; i<repzZz; i++) {
						fd[0]=open("pipe2", O_RDONLY);		//mporw na grapsw sthn shm?
					
						error(fd[0]);
						
						strncpy((offset+NAME_SIZE*2+68),(message+i*64),63);
						*(offset+NAME_SIZE*2+68+63)='\0';
						check=(close(fd[0])); 
						if(check==-1&&errno!=EBADF){
							error(check);
						}
						
						fd[1]=open("pipe1", O_WRONLY);	//egrapsa!
						
						error(fd[1]);
						
						check=(close(fd[1]));
						if(check==-1&&errno!=EBADF){
							error(check);
						}
					}
				}
			}
			exit (0);
		}
		if(gotsig!=1){
			fd[7]=open("pipe4",O_WRONLY);  //proti fora mporei na grapsei etsi ki alliws
			error(fd[7]);
			check=(close(fd[7]));
			if(check==-1&&errno!=EBADF){
				error(check);
			}
		}
		while (gotsig!=1) {//diabasma xristi1
			
			
			if(!gotsig){
				fd[6]=open("pipe3", O_RDONLY); // mporw na diabasw?
				
				error(fd[6]);
				
				printf("%s: %s\n",user2,(offset+NAME_SIZE+2));
				
				check=(close(fd[6]));
				if(check==-1&&errno!=EBADF){
					error(check);
				}
				
				fd[7]=open("pipe4",O_WRONLY); //diabasa
				
				error(fd[7]);
				
				check=(close(fd[7]));
				if(check==-1&&errno!=EBADF){
					error(check);
				}
			}
		}
	}
	else {
		if (!fork()) {//grapsimo xristi2 
			printf("Enter message: \n");
			while(gotsig!=1) {
				int len;
				char *enter;
				fgets(message,511,stdin);
				if (gotsig==1) {
					break;
				}
				enter=strrchr(message, '\n');
				*enter='\0';
				if(!strcmp(message,"quit")){
					error(kill(0,SIGINT));
					exit(0);
				}
				len=strlen(message);
				if (len<64) {
					fd[4]=open("pipe4",O_RDONLY);
					error(fd[4]);
					strcpy((offset+NAME_SIZE+2),message);
					check=(close(fd[4]));
					if(check==-1&&errno!=EBADF){
						error(check);
					}
					fd[5]=open("pipe3", O_WRONLY);
					error(fd[5]);
					check=(close(fd[5]));
					if(check==-1&&errno!=EBADF){
						error(check);
					}
				}
				else {
					int repZzZ=len/64+1 ,i;
					for (i=0; i<repZzZ; i++) {
						fd[4]=open("pipe4",O_RDONLY);
						error(fd[4]);
						strncpy((offset+NAME_SIZE+2),(message+i*64),63);
						*(offset+NAME_SIZE+2+63)='\0';
						check=(close(fd[4]));
						if(check==-1&&errno!=EBADF){
							error(check);
						}
						fd[5]=open("pipe3", O_WRONLY);
						error(fd[5]);
						check=(close(fd[5]));
						if(check==-1&&errno!=EBADF){
							error(check);
						}
					}
				}
			}
			exit (0);
		}
		if(gotsig!=1){
			fd[2]=open("pipe2", O_WRONLY);
			error(fd[2]);
			check=(close(fd[2]));
			if(check==-1&&errno!=EBADF){
				error(check);
			}
		}
		while (gotsig!=1) {//diabasma xristi2
			
			if (!gotsig) {
				
				fd[3]=open("pipe1", O_RDONLY);
				error(fd[3]);
				printf("%s: %s\n",user2,(offset+NAME_SIZE*2+68));
				
				error(close(fd[3]));
				
				fd[2]=open("pipe2", O_WRONLY);
				error(fd[2]);
				check=(close(fd[2]));
				if(check==-1&&errno!=EBADF){
					error(check);
				}
			}
		}
	}
	if (first==1) {
		*offset=0; //aposindesi xristi 1 kai kleisimo ton antistoixwn fd
		
		check=close(fd[0]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		
		check=close(fd[1]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		check=close(fd[6]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		check=close(fd[7]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		
	}
	else {
		*(offset+NAME_SIZE+66)=0; //aposindesi xristi 2 kai kleisimo ton antistoixwn fd
		
		check=close(fd[2]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		check=close(fd[3]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		check=close(fd[4]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		check=close(fd[5]);
		if(check==-1&&errno!=EBADF){
			error(check);
		}
		
	}
	check=waitpid(-1,NULL,WNOHANG);
	if (errno!=ECHILD&&check==-1) 
		error(waitpid(-1,NULL,WNOHANG));
	
	
	if (*(offset+NAME_SIZE+66)==0 && *offset==0) {
		//ekka8arisi
		shmdt(offset);
		shmctl(shmid,IPC_RMID,0);
		error(unlink("pipe1"));
		error(unlink("pipe2"));
		error(unlink("pipe3"));
		error(unlink("pipe4"));
		error(unlink("pipe5"));
	}
	return 0;
}