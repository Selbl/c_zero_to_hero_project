#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

int list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (dbhdr == NULL || employees == NULL) {
        return STATUS_ERROR;
    }
	int i = 0;
    unsigned short count = dbhdr->count;

    for (; i < count; i++) {
		printf("Employee %d\n",i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %u\n", employees[i].hours);
    }

    return STATUS_SUCCESS;
}


int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring){
    if (dbhdr == NULL || employees == NULL || addstring == NULL) {
        return STATUS_ERROR;
    }

    /* Parse "name,address,hours" from addstring (strtok modifies addstring in place) */
    char *name = strtok(addstring, ",");
    char *addr = name ? strtok(NULL, ",") : NULL;
    char *hours_str = addr ? strtok(NULL, ",") : NULL;

    if (name == NULL || addr == NULL || hours_str == NULL) {
        return STATUS_ERROR;
    }

    unsigned long hours_val = strtoul(hours_str, NULL, 10);

    /* Grow the employees array by one */
    unsigned short old_count = dbhdr->count;
    size_t new_count = (size_t)old_count + 1;

    struct employee_t *new_employees;
    if (*employees == NULL) {
        /* first employee */
        new_employees = malloc(new_count * sizeof(struct employee_t));
    } else {
        new_employees = realloc(*employees, new_count * sizeof(struct employee_t));
    }
    if (new_employees == NULL) {
        return STATUS_ERROR;
    }
    *employees = new_employees;

    struct employee_t *e = &(*employees)[old_count];

    /* Copy fields with proper bounds checking */
    strncpy(e->name, name, sizeof(e->name) - 1);
    e->name[sizeof(e->name) - 1] = '\0';

    strncpy(e->address, addr, sizeof(e->address) - 1);
    e->address[sizeof(e->address) - 1] = '\0';

    e->hours = (unsigned int)hours_val;

    dbhdr->count = (unsigned short)new_count;

    return STATUS_SUCCESS;
}


int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}


	int count = dbhdr->count;

	struct employee_t *employees = calloc(count, sizeof(struct employee_t));
	if (employees == (void*)-1) {
		printf("Malloc failed\n");
		return STATUS_ERROR;
	}

	read(fd, employees, count*sizeof(struct employee_t));

	int i = 0;
	for (; i < count; i++) {
		employees[i].hours = ntohl(employees[i].hours);
	}

	*employeesOut = employees;
	return STATUS_SUCCESS;

}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	int realcount = dbhdr->count;

	dbhdr->magic = htonl(dbhdr->magic);
	dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
	dbhdr->count = htons(dbhdr->count);
	dbhdr->version = htons(dbhdr->version);

	lseek(fd, 0, SEEK_SET);

	write(fd, dbhdr, sizeof(struct dbheader_t));

	int i = 0;
	for (; i < realcount; i++) {
		employees[i].hours = htonl(employees[i].hours);
		write(fd, &employees[i], sizeof(struct employee_t));
	}

	return STATUS_SUCCESS;

}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
	if (fd < 0) {
		printf("Got a bad FD from the user\n");
		return STATUS_ERROR;
	}

	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
	if (header == (void*)-1) {
		printf("Malloc failed create a db header\n");
		return STATUS_ERROR;
	}

	if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
		perror("read");
		free(header);
		return STATUS_ERROR;
	}

	header->version = ntohs(header->version);
	header->count = ntohs(header->count);
	header->magic = ntohl(header->magic);
	header->filesize = ntohl(header->filesize);

	if (header->magic != HEADER_MAGIC) {
		printf("Impromper header magic\n");
		free(header);
		return -1;
	}


	if (header->version != 1) {
		printf("Impromper header version\n");
		free(header);
		return -1;
	}

	struct stat dbstat = {0};
	fstat(fd, &dbstat);
	if (header->filesize != dbstat.st_size) {
		printf("Corrupted database\n");
		free(header);
		return -1;
	}

	*headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut) {
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}



