#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void error(int status){
	char ans;
	if(status==-1){
		if(errno!=EINTR){
			fprintf(stderr,"Error occured: %s\n",strerror(errno));
			printf("Kill?(y/n)\n");
			scanf(" %c", &ans);
			if(ans=='y'){
				exit (0);
			}
		}
	}
}