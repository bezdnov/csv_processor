#include <stdio.h>
#include "libcsvparse.h"

int main(int argc, char* argv[]) {
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc != 2) {
		puts("Incorrect number of arguments");
		return 1;
	}
	char* filename = argv[1];
	
	CSV_data* data = parse_csv(filename);
	// print_csv(data);

	process_csv(data);

	print_csv(data);
	
	if (data != NULL)	
		delete_csv(data);
	return 0;
}
