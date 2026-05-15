#ifndef _LIBCSV_
#define _LIBCSV_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// this datatype is needed only as return value of function parse_row
struct CSV_ROW {
	int width;
	char** cells;
};

struct CSV_DATA {
	int width;
	int height;
	char* cell00;  // the very first cell. Not needed, but should be kept
	char** column_names;
	unsigned long long* column_hashes; // for quick comparison
	int* row_numbers;     // rows are numbers, and numbers only
	// struct CSV_ROW** rows;
	char*** cells;  // matrix of strings
};

struct POINT {
	int row_index;
	int column_index;
};

typedef struct CSV_ROW CSV_row;
typedef struct CSV_DATA CSV_data;
typedef struct POINT point;

// file => inner sturcture CSV_data
// returns NULL in case csv file is incorrect
CSV_data* parse_csv(char* filename);

// working with expressions (=A1*B2 and similar)
CSV_data* process_csv(CSV_data* csv_table);

void print_csv(CSV_data* csv_table);

// freeing
void delete_csv(CSV_data* data);


#endif
