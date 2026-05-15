#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "libcsvparse.h"

const size_t BASIC_CELL_LENGTH = 8;
const size_t BASIC_LINE_LENGTH = 32;
const size_t MAX_LINE_LENGTH = 1024;
const size_t MAX_CELL_LENGTH = 256;
const size_t MODULUS = 10e+9 + 7;

#define MAX_CELL_RECURSION (3)

// global variable, needed to store point the program has already been to
point cell_buffer[MAX_CELL_RECURSION];


CSV_row* parse_row(char* row_string) {
	if (row_string == NULL)
		return NULL;
	
	CSV_row* return_row = (CSV_row*)malloc(sizeof(CSV_row));
	size_t cell_data_length = BASIC_CELL_LENGTH;
	char* current_cell_data = (char*)malloc(sizeof(char) * cell_data_length);

	int current_cell_index = 0;

	// width calculation (according to CSV standard, it is equal to amount of commas + 1)
	int row_width = 0;
	for (size_t i = 0; i < strlen(row_string); ++i)
		row_width += row_string[i] == ',';
	row_width += 1;

	return_row->width = row_width;
	return_row->cells = (char**)malloc(sizeof(char*) * return_row->width);

	int previous_cell_end = -1; // index of previous ',' symbol
	int column_index = 0;
	for (size_t i = 0; i < strlen(row_string) + 1; ++i) {  // strlen() + 1 is used to include terminating null byte as a marker of string end
		// reallocation in case of upcoming overflow
		if (i - previous_cell_end > cell_data_length) {
			cell_data_length *= 2;
			current_cell_data = (char*)realloc(current_cell_data, sizeof(char) * cell_data_length);

			if (cell_data_length > MAX_CELL_LENGTH) {
				fprintf(stderr, "Cell size too big (column: %d)\n", column_index + 1);
				free(return_row->cells);
				return NULL;
			}
		}

		if (row_string[i] == ',' || row_string[i] == '\x00') {
			size_t cell_end = current_cell_index;

			return_row->cells[column_index] = (char*)malloc(sizeof(char) * (cell_data_length + 1));

			strncpy((return_row->cells)[column_index], current_cell_data, cell_end);
			return_row->cells[column_index][cell_end] = '\x00';
			previous_cell_end = i;
			current_cell_index = 0;
			column_index++;

			cell_data_length = BASIC_CELL_LENGTH;
			current_cell_data = (char*)realloc(current_cell_data, sizeof(char) * cell_data_length);
			memset(current_cell_data, '\x00', cell_data_length);

			// printf("This is the cell that was created: %s\n", return_row->cells[column_index - 1]);
		} else {
			current_cell_data[current_cell_index++] = row_string[i];
		}
	}

	free(current_cell_data);
	return return_row;
}

void delete_row(CSV_row* row) {
	for (int i = 0; i < row->width; ++i) {
		free(row->cells[i]);
	}
	free(row);
}

/*
char** parse_row(char* row_string) {
	// calculation of width
	int row_width = 0;
	for (int i = 0; i < strlen(row_string); ++i)
		row_width += row_string[i] == ',';
	row_width += 1;

	char** row_cells = (char**)malloc(sizeof(char*) * row_width);
	printf("width %d\n", row_width);
	

}
*/

// util funcitons (2) (vibe-coded)
int count_non_empty_lines(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		return -1; // Error opening file
	}
	
	int count = 0;
	int c;
	int has_content = 0;

	while ((c = fgetc(file)) != EOF)
	{
		if (c == '\n') 
		{
			if (has_content)
			{
				count++;
				has_content = 0;
			}
		}
		else if (!isspace(c))
			has_content = 1;
	}

	// Handle last line if file doesn't end with '\n'
	if (has_content)
	{
		count++;
	}
	fclose(file);
	return count;
}

char* read_line(FILE* file) {
	if (file == NULL)
		return NULL;
	size_t line_size = 128;
	size_t len = 0;
	char *line = malloc(line_size * sizeof(char));
	
	if (line == NULL)
		return NULL;
	
	line[0] = '\0';

	while (fgets(line + len, (int)(line_size - len), file)) {
		len += strlen(line + len);
		if (len > 0 && line[len - 1] == '\n')
			break;
		
		line_size *= 2;
		if (line_size > MAX_LINE_LENGTH) {
			fputs("Line length too big\n", stderr);
			free(line);
			exit(1);  // creation of error marker (to differ too long line and end of file) would be better, probably.
			return NULL;
		}
		
		// char *tmp = realloc(line, line_size);
		char *tmp = realloc(line, line_size);
		if (tmp == NULL) {
			free(line);
			return NULL;
		}
		line = tmp;
	}

	if (len == 0 && feof(file)) {
		free(line);
		return NULL;
	}

	if (line[len - 1] == '\n')
		line[len - 1] = '\x00';

	return line;
}

// hash for quick string comparison
unsigned long long rabin_karp_hash(const char* s, int n) {
	// printf("The string %s, its length: %d\n", s, n);
	unsigned long long hash = 0;

	// may cause collusions
	const unsigned long p = 31;
	for (int i = 0; i < n; ++i)
		hash = (hash * p + s[i]) % MODULUS;

	return hash;
}

CSV_data* parse_csv(char* filename) {
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		perror("Incorrect filename was given");
		return NULL;
	}

	CSV_data* return_data = malloc(sizeof(CSV_data));
	int height = count_non_empty_lines(filename) - 1;

	if (height < 0) {
		free(return_data);
		return NULL;
	}

	return_data->height = height;
	return_data->cells = (char***)malloc(sizeof(char**) * height);
	return_data->row_numbers = (int*)malloc(sizeof(int) * height);
	
	bool is_first = true;
	char* line;
	int row_index = -1;  // starting from -1 because first line is columns
	while ((line = read_line(file)) != NULL) {
		if (strlen(line) == 0) {  // empty line
			fputs("Empty line", stderr);
			fclose(file);
			delete_csv(return_data);
			free(line);
			return NULL;
		}

		// saving columns
		if (is_first) {
			CSV_row* header_row = parse_row(line);

			if (header_row == NULL) {
				fprintf(stderr, "Error occured when parsing row\n");
				fclose(file);
				delete_csv(return_data);
				free(line);
				return NULL;
			}

			// setting width once;
			return_data->width = header_row->width - 1;
			return_data->column_names = (char**)malloc(sizeof(char*) * return_data->width);
			return_data->column_hashes = (unsigned long long*)malloc(sizeof(unsigned long long) * return_data->width);

			return_data->cell00 = (char*)malloc(sizeof(char) * strlen(header_row->cells[0]));
			strcpy(return_data->cell00, header_row->cells[0]);

			for (int i = 0; i < return_data->width; ++i) {
				return_data->column_names[i] = (char*)malloc(sizeof(char) * strlen(header_row->cells[i + 1]));
				strcpy(return_data->column_names[i], header_row->cells[i + 1]);
				return_data->column_hashes[i] = rabin_karp_hash(return_data->column_names[i], strlen(return_data->column_names[i]));
			}

			for (int i = 0; i < return_data->height; ++i) {
				return_data->cells[i] = (char**)malloc(sizeof(char*) * return_data->width);
			}
			// printf("Initialized %d rows of length %d\n", return_data->height, return_data->width);
			
			delete_row(header_row);	
		} else {
			CSV_row* data_row = parse_row(line);

			if (data_row == NULL) {
				fprintf(stderr, "Row %d parsing failed", row_index);
				fclose(file);
				free(return_data);
				free(line);
				return NULL;
			}

			if (data_row->width != return_data->width + 1) {
				fprintf(stderr, "Row %d width incorrect (%d expected)", row_index, return_data->width);
				delete_row(data_row);
				free(return_data);
				free(line);
				fclose(file);
				return NULL;
			}
			
			/*printf("Line initial data: %s\n", line);
			for (int i = 0; i < data_row->width; ++i) {
				printf("cell %s\n", data_row->cells[i]);
			}*/
			
			// working with row number
			for (size_t i = 0; i < strlen(data_row->cells[0]); ++i) {
				if (!isdigit(data_row->cells[0][i])) {
					fprintf(stderr, "Row cells must be digits (row %d)", row_index);
					delete_row(data_row);
					free(return_data);
					free(line);
					fclose(file);
					return NULL;
				}
			}
			return_data->row_numbers[row_index] = atoi(data_row->cells[0]);

			for (int i = 1; i < data_row->width; ++i) {
				return_data->cells[row_index][i - 1] = (char*)malloc(sizeof(char) * (strlen(data_row->cells[i]) + 1));
				// return_data->cells[row_index][i - 1][strlen(data_row->cells[i])] = '\x00';
				// printf("(new address is %p)\n", ((return_data->cells)[row_index])[i - 1]);
				strcpy(return_data->cells[row_index][i - 1], data_row->cells[i]);

				// printf("New data: %s\n", return_data->cells[row_index][i - 1]);
			}
		}
		is_first = false;
		row_index++;

		free(line);
	}

	fclose(file);
	return return_data;
}

int is_numeric(const char *str) {
	if (str == NULL || *str == '\0')
		return 0;

	// Optional leading minus
	if (*str == '-') {
		str++;
		// String was only "-"
		if (*str == '\0')
			return 0;
	}
	while (*str) {
		if (!isdigit((unsigned char)*str))
			return 0;
		str++;
	}

	return 1;
}

size_t search_for_op_sign(char* string) {
	for (size_t i = 0; i < strlen(string); ++i) {
		if (string[i] == '+' || string[i] == '-' || string[i] == '/' || string[i] == '*')
			return i;
	}
	return -1;
}

point search_for_indexes(char* string_cell, size_t length, CSV_data* data) {
	point result;
	result.row_index = -1;
	result.column_index = -1;
	
	size_t num_start;	
	for (num_start = 0; num_start < length; ++num_start) {
		if (isdigit(string_cell[num_start]))
			break;
	}

	if (num_start == length) {
		fprintf(stderr, "Incorrect operating cell: no number");
		return result;
	}

	// starting from here: column is string_cell[0..num_start), row is string_cell[num_start..strlen())
	
	// check if row is just a number
	for (size_t i = num_start; i < length; ++i)
		if (!isdigit(string_cell[i])) {
			fprintf(stderr, "Incorrect cell: non-digit characters were found in row name");
			return result;
		}
	
	unsigned long long hash = rabin_karp_hash(string_cell, num_start);

	for (int i = 0; i < data->width; ++i) {
		if (data->column_hashes[i] == hash) {
			result.column_index = i;
			break;
		}
	}

	for (int j = 0; j < data->height; ++j) {
		if (atoi(string_cell + num_start) == data->row_numbers[j]) {
			result.row_index = j;
			break;
		}
	}
	return result;
}

// resolving one cell
// There are 3 general cases:
// 1) Cell is a number. In this case, cell is unchanged
// 2) Cell is a correct string that links to other 2 cells. In this case there are 2 possible ends:
// 	2.1) Cell is resolvable (no circular dependencies, no errors in referred cells). In this case *correct* answer is returned
// 	2.2) Cell is not resolvable. warning is printed and ERROR is returned
// 3) Cell is incorrect string. warning is printed and ERROR is returned
char* process_cell(CSV_data* raw_csv_data, int row, int column) {
	char* cell = raw_csv_data->cells[row][column];
	if (cell == NULL)
		return NULL;
	int length = strlen(cell);
	if (length == 0)
		return NULL;

	if (cell[0] == '=') {
		// char* ptr_to_plus = strchr(cell, '+');	
		// char* ptr_to_minus = strchr(cell, '+');	
		// char* ptr_to_div = strchr(cell, '+');	
		// char* ptr_to_mul = strchr(cell, '+');	

		int op_sign = search_for_op_sign(cell);
		if (op_sign == -1) {
			fprintf(stderr, "Incorrect cell, row %d, column %d", row, column);
			return NULL;
		}

		// left part: cell[1..op_sign), right part: cell[op_sign + 1..strlen(cell))
		point left_part = search_for_indexes(&cell[1], op_sign - 1, raw_csv_data);
		point right_part = search_for_indexes(&cell[op_sign + 1], strlen(cell) - op_sign - 1, raw_csv_data);

		int left_expr = 0;
		int right_expr = 0;

		// TODO: make recursive, add more checks
		if (left_part.row_index != -1 && left_part.column_index != -1) {
			left_expr = atoi(raw_csv_data->cells[left_part.row_index][left_part.column_index]);
		} else {
			// trying to interpretate part as a number in case its not linked to other cells 
			// (trash at the end is ignored, what should not happen)
			left_expr = atoi(&cell[1]);
		}


		if (right_part.row_index != -1 && right_part.row_index != -1) {
			right_expr = atoi(raw_csv_data->cells[right_part.row_index][right_part.column_index]);
		} else {
			right_expr = atoi(&cell[op_sign + 1]);
		}
		
		int result = 0;
		switch (cell[op_sign]) {
			case '+':
				result = left_expr + right_expr;	
				break;
			case '-':
				result = left_expr - right_expr;	
				break;
			case '*':
				result = left_expr * right_expr;	
				break;
			case '/':
				if (right_expr != 0) {
					result = left_expr / right_expr;
				} else {
					fprintf(stderr, "Division by zero; %d %d is set to 0", row, column);
					result = 0;
				}
				break;
		}
		
		// 16 chars is enough to store any int
		free(raw_csv_data->cells[row][column]);
		raw_csv_data->cells[row][column] = (char*)malloc(sizeof(char) * 16);
		sprintf(raw_csv_data->cells[row][column], "%d", result);
	}
	else if (is_numeric(cell)) {  // numeric cells not changed
		return cell;
	} else {
		
	}

	return cell;
}

// this function calculates everything needed
CSV_data* process_csv(CSV_data* csv_data) {
	if (csv_data == NULL)
		return NULL;

	for (int i = 0; i < csv_data->height; ++i) {
		for (int j = 0; j < csv_data->width; ++j) {
			process_cell(csv_data, i, j);
		}
	}

	return csv_data;	
}

void print_csv(CSV_data* csv_table) {
	if (csv_table == NULL)
		return;

	printf("%s,", csv_table->cell00);
	for (int i = 0; i < csv_table->width - 1; ++i) {
		printf("%s,", csv_table->column_names[i]);
	}
	puts(csv_table->column_names[csv_table->width - 1]);
	
	for (int i = 0; i < csv_table->height; ++i) {
		printf("%d,", csv_table->row_numbers[i]);
		for (int j = 0; j < csv_table->width - 1; ++j) {
			printf("%s,", csv_table->cells[i][j]);
		}
		puts(csv_table->cells[i][csv_table->width - 1]);
	}
}

void delete_csv(CSV_data* data) {
	free(data->cell00);
	free(data->column_hashes);
	for (int i = 0; i < data->width; ++i) {
		free(data->column_names[i]);
	}
	free(data->column_names);

	free(data->row_numbers);
	for (int i = 0; i < data->height; ++i) {
		for (int j = 0; j < data->width; ++j)
			free(data->cells[i][j]);
	}
	free(data);
}

