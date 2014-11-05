#include <common.h>
#include <command.h>
#include <linux/compiler.h>

int do_cpuhardtest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	if ((argc != 2))
	{
		printf("\nERROR: You must specify ONE argument indicatting the duration of the test in seconds.\n");
		printf("\n=====TEST ENDED WITH ERRORS=====\n");
		return 0;
	}

	int duracion = simple_strtoul(argv[1], NULL, 10);
	int number = 0;
	int multiplier = 0;
	int i = 0;
	int e = 0;

	number = 1;
	multiplier = 5;
	
	for (i = 0; i < duracion; ++i) {
		for (e = 0; e < 6500000; ++e) {
			number = number * multiplier;
			multiplier = number/2;
			number = number * multiplier;
			multiplier = number/2;
			number = number * multiplier;
			multiplier = number/2;
			number = number * multiplier;
			multiplier = number/2;
			number = number * multiplier;
			multiplier = number/2;
		}
	printf("\nLoop run %i times, duration of the test: %i\n", i, duracion);
	}
	printf("\nFinal value: number=%i multiplier=%i\n",number, multiplier);
	printf("\n=====TEST ENDED SUCCESFULLY=====\n");
	return 0;
}

U_BOOT_CMD(
	cpuhardtest, 2, 0, do_cpuhardtest,
	"Makes math operations to test the CPU consumption",
	"cpuhardtest [duration of the test] Ex: cpuhardtest 5 (the test will run for 5 seconds)"
);
