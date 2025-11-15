#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
    printf("Usage: %s -n -f <database file>\n",argv[0]);
    printf("\t -n create new database file\n");
    printf("\t -f (required) path to database file\n");
    return;
}

int main(int argc, char *argv[]) { 
    char *filepath = NULL;
    bool newfile = false;
	int c;
    int dbfd = -1;
    struct dbheader_t *dbhdr = NULL;

	while ((c = getopt(argc, argv, "nf:") != -1)) {
		switch(c) {
			case 'f':
				newfile = true;
				break;
			case 'n':
                // point to file in command line
				filepath = optarg;
				break;
            case '?':
                // catch all
                printf("Unknown option -%c\n",c);
                break;
            default:
                return -1;
		};
	}

    if (filepath == NULL){
        printf("Filepath is a required variable!\n");
        print_usage(argv);
        return 0;
    }

    if (newfile){
        dbfd = create_db_file(filepath);
        if(dbfd == STATUS_ERROR){
            printf("Unable to create database file\n");
            return -1;
        }
        if (create_db_header(dbfd,&dbhdr) == STATUS_ERROR){
            printf("Unable to create database header\n");
            return -1;
        };
    }else{
        dbfd = open_db_file(filepath);
        if(dbfd == STATUS_ERROR){
            printf("Unable to open database file\n");
            return -1;
        }

        if(validate_db_header(dbfd, &dbhdr) == STATUS_ERROR){
            printf("Failed to validate database header\n");
            return -1;
        }
    }


    printf("Newfile %d\n",newfile);
    printf("Filepath %s\n",filepath);

    return 0;
}
