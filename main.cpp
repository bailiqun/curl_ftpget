#include "stdafx.h"
#include "curl\curl.h"
#include <stdio.h>
#include <string>

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
		const char* error;
		ftp_file_info()
		{
			_ip = NULL;
			_port = 22;
			_username = "Anonymous";
			_password = "";
			_file_name = NULL;
		}

		int ip_address_set(char* ip)
		{
			if (ip_valid(ip))
			{
				_ip = ip;
				return true;
			}
			printf("[ERROR] IP address is invalid.\r\n");
			fflush(stdout);
			return false;
		}

		int port_set(int port)
		{
			if (port <= 0)
				return false;

			_port = port;
			return true;
		}

		int user_pass_set(char* user, char* pass)
		{
			if (user == NULL || pass == NULL)
				return false;

			_username = user;
			_password = pass;
			return true;
		}

		int file_name_set(char* file_name)
		{
			if (file_name == NULL)
				return false;

			_file_name = file_name;
			return true;
		}

		inline int   port_get()  const { return _port; }
		inline char* ip_get()    const { return _ip; }
		inline char* user_get()  const { return _username; }
		inline char* pass_get()  const { return _password; }
		inline char* file_name_get() const { return _file_name; }
	private:
		char* _ip;
		int   _port;
		char* _username;
		char* _password;
		char* _file_name;

		int ip_valid(const char* ip)
		{
			int sub[4] = { 0,0,0,0 };
			if (sscanf_s(ip, "%d.%d.%d.%d", sub, sub + 1, sub + 2, sub + 3) != 4)
			{
				return false;
			}
			for (int i = 0; i < 4; ++i)
			{
				if (sub[i] < 0 || sub[i] > 255)
				{
					return false;
				}
			}
			return true;
		}
	};

	int download(ftp_file_info* profile,std::string location = "./")
	{
		struct FtpFile ftpfile;

		ftpfile.filename = profile->file_name_get();
		ftpfile.stream = NULL;

		char taget_file[100] = { '\0' };
		sprintf_s(taget_file, 99, "ftp://%s/%s", profile->ip_get(), profile->file_name_get());

		char user_info[100] = { '\0' };
		sprintf_s(user_info, 99, "%s:%s", profile->user_get(), profile->pass_get());

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

			(const char*)profile->error = curl_easy_strerror(res);

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
		printf("\b\b\b\b\b\b\b\b\b\b\b\b%d  %d", up, down);
		fflush(stdout);
	}
};




int main(void)
{
	FTPHelper ftp;
	FTPHelper::ftp_file_info info;
	info.ip_address_set("192.168.3.8");
	info.user_pass_set("bai", "a");
	info.file_name_set("map.pgm");
	ftp.download(&info);
	getchar();
	return 0;
}