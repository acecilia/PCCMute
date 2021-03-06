/*
 * ADC controller through SPI  (using spidev driver)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 *
 * Developed by Miguel Chavarrías in the GDEM-UPM laboratory
 *
 * TI_ADC control through SPI interface with OMAP35XX. This program
 * establishes the communication between both devices, and configures
 * ADC in Automatic Mode 1. Then it starts working and sends the info
 * to the OMAP. Resulting info is presented on screen and saved into
 * a .txt file.
 *
 * This ADC control has been designed in order to obtain power consumption
 * information, and resulting data are presented as mA, mW and energy
 * information of the different power consumption dominions.
 *
 * Most important input parameters can be performance by command line
 * before program execution.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <sys/time.h>

#define MAXSAMPLES 10000

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev4.0";
static const char *filesave = "Measures.txt";

/*
 * Default SPI working values
 */
uint8_t mode = 0;
uint8_t bits = 8;
uint32_t speed = 50000;
uint16_t delay = 100;


uint8_t tx_aux_1 = 0x00, tx_aux_2 = 0x00;

int config_ADC = 0;
int chan = 0;
int valor = 0;
int fd;
int num_channels = 16;
int chs_read[16];
int num_samples = 10;
int count_samples = 0;
int elapsed_time_0 = 0, elapsed_time_1 = 0;

/*
 * Transmission data structures
 */
uint8_t intr_word[2];
uint8_t tx[] = {0x00,0x00,};
uint8_t rx[sizeof(tx)] = {0, };

struct spi_ioc_transfer tr;

/*
 * Timing data structures
 */
struct timestrc {
	struct timeval aux_timeval;
	struct tm* ptm_aux;
} aux_time;

/*
 * Main registering data structures
 * Will be a measure_reg structure for each sample
 * this structure register the measure value and the time
 * in which this sample was taken
 */
struct measure_reg {
	float measure;
	struct timestrc timems;
};

struct dom_registro {
	char  dominio[7];
	float   Rsense;
	float   voltage;
	float   avg_measure;
	struct  measure_reg measures[MAXSAMPLES];
};

/*
 * Fixed values and identifiers of the different measuring domains
 * @.dominio: Name of the voltage domains
 * @.Rsense: Sensing resistor value in mOhms
 * @.voltage: Domain power source voltage
 * @ avg_measure: average current consumption resulting in the dominium
 */
struct dom_registro dominios[16] = {
{.dominio = "DVI", 		.Rsense = 68,	.voltage = 5.3, },
{.dominio = "USBS", 	.Rsense = 8.25,	.voltage = 5.3, }, //4 RESISTORES DE 33MOHM EN PARALELO
{.dominio = "MPU_IVA", 	.Rsense = 430,	.voltage = 5.3, },
{.dominio = "OTHER", 	.Rsense = 910,	.voltage = 5.3, },
{.dominio = "CORE", 	.Rsense = 750,	.voltage = 5.3, },
{.dominio = "IO", 		.Rsense = 620,	.voltage = 5.3, },
{.dominio = "TPS", 		.Rsense = 470,	.voltage = 5.3, },
{.dominio = "SRAM",		.Rsense = 560,	.voltage = 1.8, },
{.dominio = "UARTS", 	.Rsense = 100,	.voltage = 5.3, },
{.dominio = "MEM", 		.Rsense = 620,	.voltage = 1.8, },
{.dominio = "IMON", 	.Rsense = 4300,	.voltage = 5.3, },};

/* DOMINIOS FINALMENTE NO INCLUÍDOS EN LA TARJETA */
//{.dominio = "CAM", 		.Rsense = 120,	.voltage = 5.3, },
//{.dominio = "SCREEN", 	.Rsense = 68,	.voltage = 5.3, },
//{.dominio = "LAN", 		.Rsense = 200, .voltage = 5.3, 	},
//{.dominio = "WIFI", 	.Rsense = 68,	.voltage = 5.3, },

/* DOMINIO MMC1 NO INCLUÍDO EN LA TARJETA POR ERROR (FALTA HARDWARE)*/
//{.dominio = "MMC1", 	.Rsense = 110,	.voltage = 5.3, },


/* timmer(void) function returns a timestrc struct with
 * the second and useconds information. This timing information
 * is taken of the clock system.
 */
struct timestrc timmer(void)
{
     struct timezone tz;
     gettimeofday(&aux_time.aux_timeval, &tz);
     aux_time.ptm_aux = localtime(&aux_time.aux_timeval.tv_sec);

     return(aux_time);
}

/* writeToFile()
 * Writes the information (mA, mW, time-stamp and energy consumed
 * in a .txt FILE
 */
void writeToFile()
{
	FILE *File = fopen(filesave, "a");
	int k=0, i=0;
	char str1[16], str2[16];

	if(File == NULL)
	{
	   printf("\nThere was a problem writing the data to the disk (.txt file)");
	}
	else
	{
		for( k = 0; k < num_channels; k++ )
		{
			fwrite(&dominios[chs_read[k]].dominio, sizeof(dominios[chs_read[k]].dominio), 1, File);

			for ( i = 0; i < num_samples; i++)
			{
				//CAUSING SEGMENTATION FAULT
				//sprintf(str1,"\n%d:%02d:%02d %d",dominios[chs_read[k]].measures[i].timems.ptm_aux->tm_hour, dominios[chs_read[k]].measures[i].timems.ptm_aux->tm_min, dominios[chs_read[k]].measures[i].timems.ptm_aux->tm_sec, dominios[chs_read[k]].measures[i].timems.aux_timeval.tv_usec);
				fwrite(str1,sizeof(str1), 1, File);
				sprintf(str2,":::%f mA",dominios[chs_read[k]].measures[i].measure);
				fwrite(str2, sizeof(str2), 1, File);
			}
		}

		fclose(File);
	}
    return;
}

/* transfer(void) function send and receipt data through SPI interface
 * communication information and data transmission buffers are available
 * in tr structure << struct spi_ioc_transfer tr >>
 * @ tr.rx_buf: pointer of reception data buffer
 * @ tr.tx_buf: pointer of sending data buffer
 *
 * See also declaration_tx(void) function
 */
void transfer(void)
{
	int ret = 0;

	tx[0] = tx_aux_1;
	tx[1] = tx_aux_2;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

	if (ret == 1)
		pabort("can't send spi message");

	for (ret = 0; ret < sizeof(tx); ret++)
	{
		aux_time = timmer();

		if (!ret)
			intr_word[0] = rx[ret];

		if (ret)
			intr_word[1] = rx[ret];
	}
}

/* tx_config(void) function establishes first communication with ADC through SPI
 * and configure the device in Mode_Auto 1, also indicates to the ADC desired
 * channels to be measured. Once do it ADC will continues working as this configuration
 * changing automatically the channels to be measured.
 */
void tx_config(void)
{
	uint8_t k=0, j=0;
	int g=0;
	uint8_t mask_1 = 0x00, mask_2 = 0x00;

	do{
		transfer();

		if (k==0)
		{
			tx_aux_1 = 0x80;
			tx_aux_2 = 0x00;
		}
		else if(k==1)
		{
			if (num_channels == 16)
			{
				tx_aux_1 = 0xFF;
				tx_aux_2 = 0xFF;
			}
			else
			{
				tx_aux_1 = 0x00;
				for (j=0; j<num_channels;j++)
				{
					g = chs_read[j];

					if(g==0)	mask_2=0x01;
					if(g==1)	mask_2=0x02;
					if(g==2)	mask_2=0x04;
					if(g==3)	mask_2=0x08;
					if(g==4)	mask_2=0x10;
					if(g==5)	mask_2=0x20;
					if(g==6)	mask_2=0x40;
					if(g==7)	mask_2=0x80;
					if(g==8)	mask_1=0x01;
					if(g==9)	mask_1=0x02;
					if(g==10)	mask_1=0x04;
					if(g==11)	mask_1=0x08;
					if(g==12)	mask_1=0x10;
					if(g==13)	mask_1=0x20;
					if(g==14)	mask_1=0x40;
					if(g==15)	mask_1=0x80;

					tx_aux_1 |= mask_1;
					tx_aux_2 |= mask_2;
				}
			}
		}
		else if(k==2)
		{
			tx_aux_1 = 0x20;
			tx_aux_2 = 0x00;
		}else if(k==3)
		{
			printf("ADC automatic mode 1 has been configured\n\n");
			tx_aux_1 = 0x20;
			tx_aux_2 = 0x00;
			config_ADC = 1;
		}

		k++;

	}while (k<5);
}

/*declaration_tx(void) initializes tr << struct spi_ioc_transfer tr >> data transmission structure
 */
void declaration_tx (void)
{
	tr.tx_buf = (unsigned long)tx;
	tr.rx_buf = (unsigned long)rx;
	tr.len = sizeof(tx);
	tr.delay_usecs = delay;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;
}

/* print_usage(const char *prog) prints by screen user input commands
 */
static void print_usage(const char *prog)
{
	printf("Usage: %s [-dsebmnpt123456789101112131415]\n", prog);
	puts("  -d --device   device to use (default /dev/spidev4.0)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -e --delay    delay (usec)\n"
	     "  -b --bpw      bits per word \n"
	     "  -m --mode     SPI working mode\n"
	     "  -n --num_chs  number of channels\n"
		 "  -p --num_s    number_of_samples\n"
		 "  -t --time     time of measuring\n"
	     "  -1 --ch_1     channel_1_measure\n"
	     "  -2 --ch_2     channel_2_measure\n"
	     "  -3 --ch_3     channel_3_measure\n"
	     "  -4 --ch_4     channel_4_measure\n"
		 "  -5 --ch_5     channel_5_measure\n"
		 "  -6 --ch_6     channel_6_measure\n"
		 "  -7 --ch_7     channel_7_measure\n"
		 "  -8 --ch_8     channel_8_measure\n"
		 "  -9 --ch_9     channel_9_measure\n"
		 "  -A --ch_10   channel_10_measure\n"
		 "  -B --ch_11   channel_11_measure\n"
		 "  -C --ch_12   channel_12_measure\n"
	     "  -D --ch_13   channel_13_measure\n"
		 "  -E --ch_14   channel_14_measure\n"
		 "  -F --ch_15   channel_15_measure\n"
		 );
	exit(1);
}

/* int compare_channels(char *id_channel) function returns the number of the channel corresponding
 * to the string channel name passed. *
 */
static int compare_channels(char *id_channel)
{
	int k=0;
	for(k=0;k<16;k++)
	{
		if (!strcmp(id_channel, dominios[k].dominio))
			return(k);
	}
	return(16);
}

/* void parse_opts(int argc, char *argv[]) function evaluates and register the commands
 * introduced by command line by user at the program call execution
 *
 */
static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "device",  1, 0, 'd' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'e' },
			{ "bpw",     1, 0, 'b' },
			{ "mode",    1, 0, 'm' },
			{ "num_chs", 1, 0, 'n' },
			{ "num_s",   1, 0, 'p' },
			{ "time",    1, 0, 't' },
			{ "ch_0",    1, 0, '0' },
			{ "ch_1",    1, 0, '1' },
			{ "ch_2",    1, 0, '2' },
			{ "ch_3", 	 1, 0, '3' },
			{ "ch_4",    1, 0, '4' },
			{ "ch_5",    1, 0, '5' },
			{ "ch_6", 	 1, 0, '6' },
			{ "ch_7",    1, 0, '7' },
			{ "ch_8",    1, 0, '8' },
			{ "ch_9",    1, 0, '9' },
			{ "ch_10",    1, 0, 'A' },
			{ "ch_11",    1, 0, 'B' },
			{ "ch_12",    1, 0, 'C' },
			{ "ch_13",    1, 0, 'D' },
			{ "ch_14",    1, 0, 'E' },
			{ "ch_15",    1, 0, 'F' },
			{ NULL, 0, 0, 0 },
		};

		int c;

		c = getopt_long(argc, argv, "d:s:e:b:m:n:p:0:1:2:3:4:5:6:7:8:9:A:B:C:D:E:F", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'd':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'e':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'm':
			mode = atoi(optarg);
			break;
		case 'n':
			num_channels = atoi(optarg);
			break;
		case 'p':
			num_samples = atoi(optarg);
			break;
		case '0':
			chs_read[0] = compare_channels(optarg);
			break;
		case '1':
			chs_read[1] = compare_channels(optarg);
			break;
		case '2':
			chs_read[2] = compare_channels(optarg);
			break;
		case '3':
			chs_read[3] = compare_channels(optarg);
			break;
		case '4':
			chs_read[4] = compare_channels(optarg);
			break;
		case '5':
			chs_read[5] = compare_channels(optarg);
			break;
		case '6':
			chs_read[6] = compare_channels(optarg);
			break;
		case '7':
			chs_read[7] = compare_channels(optarg);
			break;
		case '8':
			chs_read[8] = compare_channels(optarg);
			break;
		case '9':
			chs_read[9] = compare_channels(optarg);
			break;
		case 'A':
			chs_read[10] = compare_channels(optarg);
			break;
		case 'B':
			chs_read[11] = compare_channels(optarg);
			break;
		case 'C':
			chs_read[12] = compare_channels(optarg);
			break;
		case 'D':
			chs_read[13] = compare_channels(optarg);
			break;
		case 'E':
			chs_read[14] = compare_channels(optarg);
			break;
		case 'F':
			chs_read[15] = compare_channels(optarg);
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

/* calc_operations (void) function is executed once all measures had been taken and calculates the real
 * values of the current and power consumption of the different measuring domains, and the average
 * values desired (averge current and power consumption)
 * Also calculates the elapsed time since the first to the last measure.
 */
void calc_operations (void)
{
	int k=0, i=0;

	for (k=0; k < num_channels;k++)
		{
			for (i = 0; i < num_samples; i++)
			{
				dominios[chs_read[k]].measures[i].measure = 25000*dominios[chs_read[k]].measures[i].measure;
				dominios[chs_read[k]].measures[i].measure = dominios[chs_read[k]].measures[i].measure/4095;
				dominios[chs_read[k]].measures[i].measure = dominios[chs_read[k]].measures[i].measure/dominios[chs_read[k]].Rsense;
				dominios[chs_read[k]].avg_measure = dominios[chs_read[k]].avg_measure + dominios[chs_read[k]].measures[i].measure;
			}
		}

		for(k=0; k< 16; k++)
		{
		dominios[k].avg_measure = dominios[k].avg_measure / count_samples;
		}

		for (k = 0; k < num_channels; k++)
		{
			printf("%s\tConsumption:\t%f mA %f mW \n", dominios[chs_read[k]].dominio, dominios[chs_read[k]].avg_measure, (dominios[chs_read[k]].voltage)*(dominios[chs_read[k]].avg_measure));
		}

		elapsed_time_1 = dominios[chs_read[num_channels-1]].measures[num_samples-1].timems.aux_timeval.tv_sec - dominios[chs_read[0]].measures[0].timems.aux_timeval.tv_sec;
		elapsed_time_0 = dominios[chs_read[num_channels-1]].measures[num_samples-1].timems.aux_timeval.tv_usec - dominios[chs_read[0]].measures[0].timems.aux_timeval.tv_usec;

		if(elapsed_time_0 < 0)
		{
			elapsed_time_1 = elapsed_time_1 - 1;
			elapsed_time_0 = 1000000 - elapsed_time_0;
		}

		printf("\n Elapsed time: %d sec ::%d usec", elapsed_time_1,elapsed_time_0);
}

int main(int argc,char *argv[])
{
	int ret = 0;
	int k = 0, i = 0;

	parse_opts(argc, argv);

	if (num_channels == 16)
	{
		num_channels = 16;
		for (k=0; k < 16; k++)
		{
			chs_read[k]=k;
		}
	}

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	// SPI mode
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	// Bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	// Max speed Hz
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	printf("number of channels: %d \n", num_channels);

	count_samples = 0;
	declaration_tx();

	if (!config_ADC)
		tx_config();

	if (config_ADC == 1)
	{
		for (k = 0; k < num_samples; k++)
		{
			for(i=0; i<num_channels;i++)
			{
			transfer();
			dominios[(intr_word[0]>>4)].measures[count_samples].measure = (0x0F&intr_word[0])<<8 | intr_word[1];
			dominios[(intr_word[0]>>4)].measures[count_samples].timems = aux_time;

			}
			count_samples++;
		}
	}

	calc_operations();

	close(fd);

	writeToFile();

	printf("\n*-----End of the program-----*");
	return ret;
}



