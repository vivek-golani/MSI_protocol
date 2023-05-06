#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <linux/userfaultfd.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <pthread.h>

#define BUFF_SIZE 4096
#define MAX_TRACES 5
#define PROCESS 5
#define MAX_PAGES 100

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE);    \
    } while (0)

static char msi_array[PROCESS][MAX_TRACES];
static char addresses[MAX_TRACES];

char *filename; 

/*Hash Map creation, insert and delete*/

struct Node {
    char* key;
    int value;
    struct Node* next;
};

struct Node* hashTable[MAX_TRACES] = {NULL};

int hashFunction(char* key) {
    int sum = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        sum += (int) key[i];
    }
    return sum % MAX_TRACES;
}

void insert(char* key, int value) {
    int index = hashFunction(key);
    struct Node* node = hashTable[index];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            return;
        }
        node = node->next;
    }
    node = (struct Node*) malloc(sizeof(struct Node));
    node->key = key;
    node->value = value;
    node->next = hashTable[index];
    hashTable[index] = node;
}

int lookup(char* key) {
    int index = hashFunction(key);
    struct Node* node = hashTable[index];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return -1;
}

/* Helper print function */
void print_helper() {	
    printf("|\tOperation\t|");
    for(int j=0; j<MAX_TRACES; j++)
    	printf(" %d\t|", j);

    printf("\n");
}

/* cache write function */

void inputwrite(char *addr, char id) 
{
    int current_page = lookup(addr);
    int curr = id - 'A';
    msi_array[curr][current_page] = 'M';
    for (int i = 0; i < PROCESS; ++i) {
        if(i != curr)
            msi_array[i][current_page] = 'I';            
    }

    for (int i=0; i<PROCESS; i++) {
    	printf("Process %c:\n\n", 'A'+i);
	print_helper();
    	printf("|W on %s by %c|", addr, id);
	for (int j=0; j<MAX_TRACES; j++)
            printf(" %c\t|", msi_array[i][j]);

	printf("\n");
    } 
    printf("\n");
}

/* cache read function */

void inputread(char *addr, char id) 
{
    int current_page = lookup(addr);
    int curr = id - 'A';
    msi_array[curr][current_page] = 'S';
    for (int j = 0; j < PROCESS; ++j)
	if(msi_array[j][current_page]) == 'M')
             msi_array[j][current_page] = 'S';


    for (int i=0; i<PROCESS; i++) {
    	printf("Process %c:\n\n", 'A'+i);
	print_helper();
    	printf("|R on %s by %c|", addr, id);
        for (int j=0; j<MAX_TRACES; j++)
            printf(" %c\t|", msi_array[i][j]);

	printf("\n");
    }
    printf("\n");
}



/* Main Function */

int main(int argc, char *argv[]) {    

    for(int i=0; i<PROCESS; i++)
		for(int j=0; j<MAX_TRACES; j++)
			msi_array[i][j] = 'N';

	FILE* file;
	char *file_path = "data.txt";
	char line[100];
	char* token;
	char data[MAX_PAGES][2][100];
	int row = 0, col = 0, len = 0;

	file = fopen(file_path, "r");
	if (file == NULL) {
		printf("Error: Cannot open file.\n");
		return 1;
	}

	while (fgets(line, 100, file) != NULL && row < MAX_PAGES) {            
		len = strlen(line);
		if(line[len-1] == '\n')
		   line[len-1] = '\0';	
		token = strtok(line, ":");
		token = strtok(NULL, " ");

		strcpy(data[row][col++], token);

		token = strtok(NULL, " ");
		if (token != NULL) 
			strcpy(data[row][col++], token);
		else
			strcpy(data[row][col++], "");       
		col = 0;
		row++;
	}


	fclose(file);

	int i = 0;
	char id;
	while(i < MAX_PAGES) {  
		printf("Please give Process id (A/B/C/D/E): ");
		scanf(" %c", &id);
		if(id != 'A' && id != 'B' && id != 'C' && id != 'D' && id != 'E') {
			printf("Enter valid process\n");
			continue;
		}
		char *address = data[i][1];
		insert(address,hashFunction(address));
		int page = lookup(address);
		char *read_write = data[i][0];
		
		for(int i=0;i<PROCESS;i++) {
			if(msi_array[i][page] == 'N')
				msi_array[i][page] = 'I';
		}
		

		//read logic          
		if (*read_write == 'R') 
			inputread(address, id);
		//write logic
		else if (*read_write == 'W') 
			inputwrite(address, id);
		else 
			printf("Invalid Trace : please enter r (read), w (write)");
		i++;
	} 
}
    


