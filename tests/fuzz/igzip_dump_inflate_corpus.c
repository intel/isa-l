#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inflate_std_vects.h"

#define DNAME_MAX 256
#define FNAME_MAX (DNAME_MAX + 81)

int main(int argc, char *argv[])
{
	uint8_t *buf;
	int i, len, err;
	FILE *fout = NULL;
	char fname[FNAME_MAX];
	char dname[DNAME_MAX];

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <outdir>\n", argv[0]);
		exit(1);
	}
	strncpy(dname, argv[1], DNAME_MAX - 1);

	for (i = 0; i < sizeof(std_vect_array) / sizeof(struct vect_result); i++) {
		buf = std_vect_array[i].vector;
		len = std_vect_array[i].vector_length;
		err = std_vect_array[i].expected_error;

		snprintf(fname, FNAME_MAX, "%s/inflate_corp_n%04d_e%04d", dname, i, err);
		printf(" writing %s\n", fname);
		fout = fopen(fname, "w+");
		if (!fout) {
			fprintf(stderr, "Can't open %s for writing\n", fname);
			exit(1);
		}
		fwrite(buf, len, 1, fout);
		fclose(fout);
	}
}
