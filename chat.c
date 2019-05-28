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

//TSITSOPOYLOS EUSTATHIOS(2283) KAI KOUTOURATSAS STERGIOS(2162)

//Chat metaksi 2 xristwn  me ti boi8eia simatoforwn gia sigxronismo kai xrisi koinis mnimis gia ti metafora minimatwn


static volatile sig_atomic_t gotsig=0;

static void handler(int sig) {
	gotsig=1;					//flag gia termatismo kai eka8arisi/ksemplokarisma meta apo SIGINT. H shmaia auth xrisimopoieitai ws sin8iki se if kai while gia na yparksei omalos termatismos
}

int main(int argc, char *argv[]) {
	if (argc<4) {
		printf("Not enought arguments.\n (User1 to User2).\n");
		exit(0);
	}
	int key,shmid,first=1,relogged=0,semid,check;
	char *offset,user1[NAME_SIZE],user2[NAME_SIZE],message[512];
	
	struct sembuf sops;
	struct sigaction act={ {0} };
	
	act.sa_handler=handler;
	error(sigaction(SIGINT,&act,NULL));				

	strcpy(user1,argv[1]);
	strcpy(user2,argv[3]);
	
	key=ftok(".",'a');
	error(key);
	shmid=shmget(key, (130+2*NAME_SIZE) , IPC_CREAT | IPC_EXCL | S_IRWXU);
	if ((shmid==-1)&&(errno==EEXIST)) {    //Anixneusi an o xristis pou sindeetai einai o 1os i oxi.
		first=0;
		shmid=shmget(key, (130+2*NAME_SIZE) , IPC_CREAT | S_IRWXU);
		error(shmid);
	}
	else {
		error(shmid);
	}

	offset=(char*)shmat(shmid,NULL,0);
	error((int)*offset);
	
	if(!strcmp(argv[1],offset+1)&&*offset==0){		//Anixneusi an o prwtos xristis ksanasindeetai 
		first=1;
		relogged=1;
	}

	*(offset+NAME_SIZE+1)='0';
	*(offset+2*NAME_SIZE+67)='0';
	
	//Arxizei i arxikopoiisi simatoforwn
	semid=semget(key, 5, IPC_CREAT | S_IRWXU);
	error(semid);
	
	if (first==1) {
		
		strcpy(offset+1,user1);			//to onoma tou xristi 1 stin shm
		printf("Logged in as: %s.\n", user1);
		if(relogged==0){					//Ginetai mono tin 1i fora pou mpainei o prwtos xristis
			error(semctl(semid, 0, SETVAL, 1)); 
			error(semctl(semid, 1, SETVAL, 0)); 
			error(semctl(semid, 2, SETVAL, 0)); 
			error(semctl(semid, 3, SETVAL, 1)); 
			error(semctl(semid, 4, SETVAL, 0)); 
			printf("Waiting for %s.\n", user2);		//anamoni xristi 1 gia tin sindesi tou xristi2 me simatoforous
			strcpy(offset+NAME_SIZE+67,user2);
			sops.sem_num=4; sops.sem_op=-1; sops.sem_flg=0;
			error(semop(semid,&sops,1));
		}
		*offset='1';					//Byte sthn shm gia tin endeiksi oti o xristis(1) einai online
	}
	
	else {
		//Eidikes periptwseis la8ous me ta onomata san orismata
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
		strcpy((offset+NAME_SIZE+67),user1);	//Onoma xristi 2
		printf("Logged in as: %s.\n",user1);
		*(offset+NAME_SIZE+66)='1';   			//endeiksi online tou xristi 2
		
		sops.sem_num=4; sops.sem_op=1; sops.sem_flg=0;
		error(semop(semid,&sops,1));
	}
	if (first) {
		check= fork();
		error(check);
		if (!check) {//grapsimo xristi1
			printf("Enter message: \n");
			while (gotsig!=1) {
				char *enter;
				int len;
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
				if(len<=63){
					sops.sem_num=0; sops.sem_op=-1; sops.sem_flg=0;			//Mporw na to steilw?
					error(semop(semid,&sops,1));
					strcpy((offset+NAME_SIZE*2+68),message);
					sops.sem_num=1; sops.sem_op=1; sops.sem_flg=0;			//Esteila! (+1)
					error(semop(semid,&sops,1));
				}
				else{
					int reps=(len/64+1),i;
					for(i=0;i<reps;i++){
						sops.sem_num=0; sops.sem_op=-1; sops.sem_flg=0;			//Mporw na to steilw?
						error(semop(semid,&sops,1));
						strncpy((offset+NAME_SIZE*2+68),(message+i*64),63);
						*(offset+NAME_SIZE*2+68+63)='\0';
						sops.sem_num=1; sops.sem_op=1; sops.sem_flg=0;			//Esteila! (+1)
						error(semop(semid,&sops,1));
					}
				}

			}
			exit (0);
		}
		while (gotsig!=1) {//diabasma xristi1
			sops.sem_num=2; sops.sem_op=-1; sops.sem_flg=0;				//iparxei n diabasw?
			error(semop(semid,&sops,1)); 
			if(!gotsig){
				printf("%s: %s\n",user2,(offset+NAME_SIZE+2));
				sops.sem_num=3; sops.sem_op=1; sops.sem_flg=0;			//Diabasa(mporeis na steileis)
				error(semop(semid,&sops,1));
			}
		}
	}
	else {
		check= fork();
		error(check);
		if (!check) {//grapsimo xristi2 
			printf("Enter message: \n");
			while(gotsig!=1) {
				char *enter;
				int len;
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
				if(len<=63){
					sops.sem_num=3; sops.sem_op=-1; sops.sem_flg=0;
					error(semop(semid,&sops,1));
					strcpy((offset+NAME_SIZE+2),message);
					sops.sem_num=2; sops.sem_op=1; sops.sem_flg=0;
					error(semop(semid,&sops,1));
				}
				else{
					int repz=(len/64+1),i;
					for(i=0;i<repz;i++){
						sops.sem_num=3; sops.sem_op=-1; sops.sem_flg=0;
						error(semop(semid,&sops,1));
						strncpy((offset+NAME_SIZE+2),message+i*64,63);
						*(offset+NAME_SIZE+2+63)='\0';
						sops.sem_num=2; sops.sem_op=1; sops.sem_flg=0;
						error(semop(semid,&sops,1));
					}
				}
				
			}
			exit (0);
		}
		while (gotsig!=1) {//diabasma xristi2
			sops.sem_num=1; sops.sem_op=-1; sops.sem_flg=0;
			error(semop(semid,&sops,1));
			if (!gotsig) {
				printf("%s: %s\n",user2,(offset+NAME_SIZE*2+68));
				sops.sem_num=0; sops.sem_op=1; sops.sem_flg=0;
				error(semop(semid,&sops,1));
			}
		}
	}
	if (first==1) {
		*offset=0;		//Aposidesi xristi 1
	}
	else {
		*(offset+NAME_SIZE+66)=0;			//Aposindesi xristi 2
	}
	check=waitpid(-1,NULL,WNOHANG);
	if (errno!=ECHILD&&check==-1) 
		error(waitpid(-1,NULL,WNOHANG));
	
	if (*(offset+NAME_SIZE+66)==0 && *offset==0) {
		error(shmdt(offset));			//Ka8arismos koinis mnimis kai simatoforwn(mono otan bgoun kai oi 2!)
		error(shmctl(shmid,IPC_RMID,0));
		error(semctl(semid,0,IPC_RMID));
	}
	return 0;
}