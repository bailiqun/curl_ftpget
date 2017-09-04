#include "stdafx.h"
#include "curl\curl.h"
#include <stdio.h>
#include <functional>



class FTPHelper
{
public:
	FTPHelper()
	{
		download_percentage = 0;
		upload_percentage = 0;
	}

	class ftp_file_info 
	{
	public:
		ftp_file_info()
		{
			ip = NULL;
			port = 22;
			username = "Anonymous";
			password = "";
			file_name = NULL;
		}
		int ip_valid(const char* ip)
		{
			return 1;
		}

		char* ip;
		int   port;
		char* username;
		char* password;
		char* file_name;
		const char* error;
	};

	int download(ftp_file_info* profile)
	{
		using namespace std::placeholders;
		struct FtpFile ftpfile;

		if (profile->file_name == NULL)
			return -1;

		ftpfile.filename = profile->file_name;
		ftpfile.stream = NULL;

		char taget_file[100] = { '\0' };
		sprintf_s(taget_file, 99, "ftp://%s/%s", profile->ip, profile->file_name);

		char user_info[100] = { '\0' };
		sprintf_s(user_info, 99, "%s:%s", profile->username, profile->password);

		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();

		if (curl)
		{
			curl_easy_setopt(curl, CURLOPT_URL, taget_file);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,my_fwrite);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

			curl_easy_setopt(curl, CURLOPT_USERPWD, user_info);//set "user:pass"
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,progress_func);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);

			curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
			res = curl_easy_perform(curl);

			profile->error = curl_easy_strerror(res);

			curl_easy_cleanup(curl);
		}
		if (ftpfile.stream)
		{
			fclose(ftpfile.stream);
		}
		curl_global_cleanup();

		return is_download_success(profile->error);
	}

	int upload()
	{
	
	}

	virtual int progress_callback(int upload, int download)
	{
		return  1;
	}
private:
	CURL* curl;
	CURLcode res;
	int download_percentage;
	int upload_percentage;

	struct FtpFile {
		const char *filename;
		FILE *stream;
	};

	/* use to write data into file */
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

	/* use to get download percentage */
	static int progress_func(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUpload)
	{
		FTPHelper *instance = (FTPHelper*)ptr;
		double download_percentage, upload_percentage;
		(TotalToDownload != 0) ? (download_percentage = NowDownloaded * 100 / TotalToDownload) : (download_percentage = 0);
		(TotalToUpload != 0) ? (upload_percentage = NowDownloaded * 100 / TotalToUpload) : (upload_percentage = 0);
		instance->percentage_set(upload_percentage, download_percentage);
		instance->progress_callback(upload_percentage, download_percentage);
		return 0;
	}

	int is_download_success(const char* error)
	{
		if (download_percentage == 100)
		{
			printf("\r\n[OK] Success\r\n");
			fflush(stdout);
			return 1;
		}
		else
		{
			printf("[ERROR] %s\r\n", error);
			fflush(stdout);
			return -1;
		}
	}

	void percentage_set(int up, int down)
	{
		download_percentage = down;
		upload_percentage = up;
		std::printf("\b\b\b\b\b\b\b\b\b\b\b\b%d  %d", up, down);
		fflush(stdout);
	}
};




int main(void)
{
	FTPHelper ftp;
	FTPHelper::ftp_file_info info;
	info.ip = "192.168.3.8";
	info.file_name = "map.pgm";
	info.username = "bai";
	info.password = "a";

	ftp.download(&info);
	getchar();
	return 0;
}