#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "byteswap.h"
#include "mifareClassicLog.h"

int main (int argc, char * argv[] ) {
	int binlog_fd;
	int i;
	logheader_t log_header;

	uint8_t binBuffer[24];
	//char strBuffer[128];
	uint32_t logFlashLen;
	uint32_t logFlashRecordAdrr = MFCLASSIC_LOG_MEM_LOG_HEADER_LEN;
	//uint8_t logBuffLenAddr = MFCLASSIC_LOG_MEM_LOG_HEADER_LEN;
	uint8_t logRecordLen;
	//uint8_t logRecordIdx;
	uint16_t logtimestamp;

	
	printf("Chameleon RevE Rebooted log parser v. 0.01 by NiTRo THe DeMoN\n\n");
	
	if (argc != 2) {
		printf("Usage: %s [filename]\n", argv[0]);
		exit(1);
	}
	binlog_fd = open(argv[1], O_RDONLY);
	if (binlog_fd <= 0) {
		printf("File not found: %s\n", argv[1]);
		exit(1);
	}


	printf("Binary log filename: %s\n", argv[1]);

	read(binlog_fd, &log_header, MFCLASSIC_LOG_MEM_LOG_HEADER_LEN);

	printf("Log enabled: %s\n", ( log_header.isLogEnabled == MFCLASSIC_LOG_MEM_STATUS_CANARY) ? "true" : "false");
	logFlashLen = swap_uint32(log_header.LogBytesWrote);
	printf("Log lenght: %u\n", logFlashLen);
	logFlashLen += MFCLASSIC_LOG_MEM_LOG_HEADER_LEN;
	printf("Total bytes wrote: %u\n\n", logFlashLen);

        while ( logFlashRecordAdrr < logFlashLen ){

		read(binlog_fd, &logRecordLen, 1);
		lseek(binlog_fd, -1 , SEEK_CUR);

        	if (logRecordLen > MFCLASSIC_LOG_MAX_RECORD_LENGHT) return 1;

		read(binlog_fd, &binBuffer, logRecordLen);

		memcpy(&logtimestamp, &binBuffer[1], MFCLASSIC_LOG_MEM_RECORD_TIMESTAMP_LEN);

        	printf("%05u/%05u: [%05u] %c | %s | ", logFlashRecordAdrr, logFlashLen, swap_uint16(logtimestamp), binBuffer[3], estate_str[binBuffer[4]]);

        	for (i = MFCLASSIC_LOG_RECORD_HEADER_LEN; i < logRecordLen; i++)
        	{
			if (i > MFCLASSIC_LOG_RECORD_HEADER_LEN) printf(" ");
			printf("%02X", binBuffer[i]);
        	}
        	printf("\n");

        	logFlashRecordAdrr += logRecordLen;

	}

	
	close(binlog_fd);
	exit(0);
}
