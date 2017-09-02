#include "stdafx.h"
#include "curl\curl.h"
#include <stdio.h>


struct FtpFile {
	const char *filename;
	FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct FtpFile *out = (struct FtpFile *)stream;
	if (out && !out->stream) {
		/* open file for writing */
		fopen_s(&(out->stream), out->filename, "wb");
		if (!out->stream)
			return -1; /* failure, can't open file to write */
	}
	return fwrite(buffer, size, nmemb, out->stream);
}


int progress_func(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUpload)
{
	double download_percentage = 0.0;
	(TotalToDownload != 0)?(download_percentage = NowDownloaded / TotalToDownload):(download_percentage = 0);

	printf("\b\b\b\b\b\b\b%3.1f%%", download_percentage * 100);
	fflush(stdout);  
	return 0;
}


int main(void)
{
	CURL *curl;
	CURLcode res;
	struct FtpFile ftpfile = {
		"test.jpg", /* name to store the file as if successful */
		NULL
	};

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl) 
	{
		curl_easy_setopt(curl, CURLOPT_URL, "ftp://robotics/test.jpg");

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_func);

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		res = curl_easy_perform(curl);

		curl_easy_cleanup(curl);
	}

	if (ftpfile.stream)
	{
		fclose(ftpfile.stream);
	}

	curl_global_cleanup();
	getchar();
	return 0;
}