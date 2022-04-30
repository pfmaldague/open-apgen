//
// Inspired by the example in the kernel required reading from the cspice
// documentation
//

#include <stdio.h>	// for printf
#include <stdlib.h>	// for malloc.h
#include <string.h>	// for strdup
#include "SpiceUsr.h"

#ifdef NOT_NEEDED_HERE

int usage(char* s) {
	fprintf(stderr, "Usage: %s <first file to load> [<second file> [...]]\n", s);
	return -1;
}

void set_error_params() {

	//
	// set the level of error reporting
	//

	errprt_c("SET", 10, "ALL");

	//
	// set the file in which errors will be collected
	//

	errdev_c("SET", 10, "spice-errors.txt");

	//
	// set the behavior when an error is found
	//

	erract_c("SET", 10, "ABORT");
}

#endif /* NOT_NEEDED_HERE */

void report_kernel_types() {

	//
	// figure out which kernels were loaded
	//

	SpiceInt	spk_count, ck_count, pck_count, ek_count, text_count, meta_count;
	ktotal_c("spk", &spk_count);
	ktotal_c("ck", &ck_count);
	ktotal_c("pck", &pck_count);
	ktotal_c("ek", &ek_count);
	ktotal_c("text", &text_count);
	ktotal_c("meta", &meta_count);

	printf("SPICE kernels loaded:\n\tspk\t%d\n\tck\t%d\n\tpck\t%d\n\tek\t%d\n\ttext\t%d\n\tmeta\t%d\n",
			spk_count, ck_count, pck_count, ek_count, text_count, meta_count);
}

int call_furnsh_c(const char* argv) {
	int i;

	//
	// load the kernel(s) specified on the command line
	//
	furnsh_c(argv);

	report_kernel_types();

	SpiceInt	total_count;

	ktotal_c("all", &total_count);

	printf("total # of kernels: %d\n", total_count);

	int FILLEN = 256;
	int TYPLEN = 256;
	int SRCLEN = 256;

	char		file_name[FILLEN];
	char		kernel_type[TYPLEN];
	char		source[SRCLEN];
	SpiceInt	handle;
	SpiceBoolean	found;

	char**		file_names = (char**) malloc(sizeof(char**) * total_count);
	char**		kernel_types = (char**) malloc(sizeof(char**) * total_count);
	char**		sources = (char**) malloc(sizeof(char**) * total_count);

	printf("\n");
	for(i = 0; i < total_count; i++) {
		kdata_c(
				i,
				"all",
				FILLEN,		TYPLEN,		SRCLEN,
				file_name,	kernel_type,	source,
				&handle, &found);
		file_names[i] = strdup(file_name);
		kernel_types[i] = strdup(kernel_type);
		sources[i] = strdup(source);
		printf("kernel # %d: file name = %s\n", i, file_name);
	}
	printf("\n");
	for(i = 0; i < total_count; i++) {
		printf("kernel # %d: source = %s\n", i, sources[i]);
	}
	printf("\n");
	for(i = 0; i < total_count; i++) {
		printf("kernel # %d: type = %s\n", i, kernel_types[i]);
	}

	//
	// we could also use kinfo_c to access kernels based on file name
	//

	//
	// For SPK files, find out as much as possible:
	//	- time coverage
	//	- bodies described
	//

	// spkobj_c, spkcov_c

#	define MAXOBJ 10000
#	define MAXIV  10000
#	define WINSIZ (2 * MAXIV)
	SPICEINT_CELL	( ids, MAXOBJ );
	SPICEDOUBLE_CELL( cover, WINSIZ );
	printf("\n");
	for(i = 0; i < total_count; i++) {

		//
		// 1. dig into SPK kernels
		//

		if(!strcmp(kernel_types[i], "SPK")) {
			int j;
			SpiceInt Card = 0;
			SpiceInt obj;

			printf("coverage for SPK file %s:\n", file_names[i]);

			spkobj_c(file_names[i], &ids);

			Card = card_c(&ids);

			printf("\t%d\t object(s) found\n", Card);
			for(j = 0; j < Card; j++) {
				SpiceInt number_of_intervals;
				int k;
				SpiceDouble begin_time, end_time;

				//
				// Do not print too much stuff...
				//
				if(j > 20) {
					printf("\t\t...\n");
					break;
				}

				//
				// NOTE: it is possible to dimension arrays
				// dynamically now...
				//

				int time_length = 256;
				SpiceChar time_string[time_length];

				//
				// get object code
				//

				obj = SPICE_CELL_ELEM_I(&ids, j);

				//
				// find a text description for it
				//

				int object_length = 100;
				SpiceChar object_name[object_length];
				bodc2n_c(obj, object_length, object_name, &found);

				printf("\t\t%d:\t%d (%s)\n", j, obj, object_name);

				//
				// get coverage window for each object
				//

				// initialize cell to 'empty':
				scard_c(0, &cover);

				// get coverage windows:
				spkcov_c(file_names[i], obj, &cover);

				// get number of intervals:
				number_of_intervals = wncard_c(&cover);

				for(k = 0; k < number_of_intervals; k++) {


					//
					// Do not print too much stuff...
					//
					if(k > 20) {
						printf("\t\t\t...\n");
						break;
					}

					//
					// get endpoints of interval
					//
					wnfetd_c(&cover, k, &begin_time, &end_time);

					//
					// output in human-friendly format
					//
					timout_c(begin_time,
						// Barycentric Dynamical Time:
						"YYYY MON DD HR:MN:SC ::TDB",
						time_length,
						time_string);
					printf("\t\t\tinterval %d:\tbegin %s\n", k, time_string);
					timout_c(end_time,
						// Barycentric Dynamical Time:
						"YYYY MON DD HR:MN:SC ::TDB",
						time_length,
						time_string);
					printf("\t\t\t\t\tend %s\n", time_string);
				}
			}
		}
	}

	//
	// 2. dig into TEXT kernels -- slightly different methodology: query
	// functions are global to the kernel pool, not specific to a given
	// text kernel
	//

	int		ROOM = 1000;
	int		LENOUT = 40;

			//
			// CAUTION: SPICE 1.0 expects 2-dim. array, not an
			// array of pointers
			//

	SpiceChar	kernel_variables[ROOM][LENOUT];
	SpiceInt	number_returned;

	gnpool_c("*", 0, ROOM, LENOUT,
			&number_returned,
			kernel_variables,
			&found);

	printf("\n%d variable(s) in kernel pool:\n", number_returned);
	for(i = 0; i < number_returned; i++) {
		SpiceInt	number_of_values;
		SpiceChar	variable_type;
		if(i > 20) {
			printf("\t...\n");
			break;
		}
		printf("\t%d:\t%s ", i, kernel_variables[i]);

		//
		// learn more about this variable
		//

		dtpool_c(kernel_variables[i], &found, &number_of_values, &variable_type);

		printf("\t(type %c, %d value(s))\n", variable_type, number_of_values);

		if(variable_type == 'N') {

			//
			// get double-precision data for this variable
			//

			int number_provided;
			int m;
			SpiceDouble*	variable_values
				= (SpiceDouble*) malloc(sizeof(SpiceDouble*) * number_of_values);
			gdpool_c(	kernel_variables[i],
					0,
					number_of_values,
					&number_provided,
					variable_values,
					&found);
			if(number_provided != number_of_values) {
				fprintf(stderr, "%s: number of variable values = "
						"%d disagrees with number provided = %d\n",
						argv[0], number_of_values, number_provided);
				return -1;
			}
			for(m = 0; m < number_provided; m++) {
				if(m > 20) {
					printf("\t\t\t...\n");
					break;
				}
				printf("\t\t\tvalue[%d] = %lf\n", m, variable_values[m]);
			}

			//
			// cleanup
			//

			free((char*) variable_values);
		} else if(variable_type == 'C') {
			SpiceInt	number_provided;
			int		n;

			//
			// get character data for this variable
			//

			SpiceChar	variable_values[number_of_values][100];
			gcpool_c(	kernel_variables[i],
					0,
					number_of_values,
					100,
					&number_provided,
					variable_values,
					&found);
			if(number_provided != number_of_values) {
				fprintf(stderr, "%s: number of variable values = "
						"%d disagrees with number provided = %d\n",
						argv[0], number_of_values, number_provided);
				return -1;
			}
			for(n = 0; n < number_provided; n++) {
				if(n > 20) {
					printf("\t\t\t...\n");
					break;
				}
				printf("\t\t\tvalue[%d] = %s\n", n, variable_values[n]);
			}
		}
	}

	//
	// clean up
	//

	for(i = 0; i < total_count; i++) {
		free(file_names[i]);
		free(kernel_types[i]);
		free(sources[i]);
	}
	free((char*) file_names);
	free((char*) kernel_types);
	free((char*) sources);
	return 0;
}
