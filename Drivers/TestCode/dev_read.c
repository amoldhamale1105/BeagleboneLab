#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char buffer[2048];

#define TRY_READ 10

int main(int argc, char *argv[])
{
	int fd;
	
	/*this variable holds remaining data bytes to be read */
	int remaining = TRY_READ;
	
	/*Holds count of total data bytes read so far */
	int total_read=0;

	/* Offset into the file */
	int offset = 10;

	/* Seek whence */
	int whence = SEEK_SET;
	
	int n =0,ret=0;

	if (argc < 2){
		printf("usage: %s <file> [readcount] [offset] [set|curr|end]\n", argv[0]);
		return 0;
	}

	if (argc > 2){
		remaining = atoi(argv[2]);
		if (argc > 3){
			offset = atoi(argv[3]);
			if (argc > 4)
				whence = !strcmp(argv[4], "set") ? SEEK_SET : !strcmp(argv[4], "curr") ? SEEK_CUR : SEEK_END;
		}
	}

	printf("read requested = %d\n",remaining);

	fd = open(argv[1], O_RDONLY);

	if(fd < 0){
		/*perror decodes user space errno variable and prints cause of failure*/
		perror("open");
		return fd;
	}

	printf("open was successful\n");

#if  1 
	/*activate this for lseek testing */
	ret = lseek(fd, offset, whence);
	if(ret < 0){
		perror("lseek");
		close(fd);
		return ret;
	}
#endif
	/*Lets attempt reading twice */
	
	while(n != 2 && remaining)
	{
		/*read data from 'fd' */
		ret = read(fd, &buffer[total_read], remaining);

		if(!ret){
			/*There is nothing to read */
			printf("end of file \n");
			break;
		}else if(ret <= remaining){
			printf("read %d bytes of data \n",ret );
			/*'ret' contains count of data bytes successfully read , so add it to 'total_read' */
		        total_read += ret;
			/*We read some data, so decrement 'remaining'*/
			remaining -= ret;
		}else if(ret < 0){
			printf("something went wrong\n");
			break;
		}else
			break;

		n++;
	}

	printf("total_read = %d\n", total_read);

	//dump buffer
	for(int i=0 ; i < total_read; i++)
		printf("%c", buffer[i]);

	close(fd);
	
	return 0;
}
