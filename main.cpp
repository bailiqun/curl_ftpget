#include "stdafx.h"
#ifndef FTPHELPER_H
#define FTPHELPER_H
#include "iostream"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "curl/curl.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

class FTPHelper
{
public:
	class ftp_file_info
	{
	public:
		ftp_file_info(char* ip = NULL,
			int port = 0,
			char* username = NULL,
			char* password = NULL,
			char* serve_file_path = NULL,
			char* client_file_path = NULL)
		{
			ip_ = ip;
			port_ = port;
			username_ = username;
			password_ = password;
			serve_file_path_ = serve_file_path;
			client_file_path_ = client_file_path;
		}
	private:
		char* ip_;
		int   port_;
		char* username_;
		char* password_;
		char* serve_file_path_;
		char* client_file_path_;
		const char* error_;
	public:
		inline int ip(char* ip)
		{
			if (ip_valid(ip))
			{
				ip_ = ip;
				return true;
			}
			printf("[ERROR] IP address is invalid.\r\n");
			fflush(stdout);
			return false;
		}
		inline int port(int port)
		{
			if (port <= 0)
				return false;
			port_ = port;
			return true;
		}
		inline int username(char* user)
		{
			if (user == NULL)
				return false;
			username_ = user;
			return true;
		}
		inline int password(char* pass)
		{
			password_ = pass;
			return true;
		}
		inline int server_file_path(char* file_name)
		{
			if (file_name == NULL)
				return false;
			serve_file_path_ = file_name;
			return true;
		}
		inline int client_file_path(char* file_name)
		{
			if (file_name == NULL)
				return false;
			client_file_path_ = file_name;
			return true;
		}
		inline int error(const char* error)
		{
			error_ = error;
			return true;
		}
		inline const char *ip() const { return ip_; }
		inline const int  &port() { return port_; }
		inline const char *username() const { return username_; }
		inline const char *password() const { return password_; }
		inline const char *serve_file_path() const { return serve_file_path_; }
		inline const char *client_file_path() const { return client_file_path_; }
		inline const char *error() const { return error_; }
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
private:
	struct ftp_file
	{
		const char *filename;
		FILE *stream;
	};
public:
	FTPHelper() :download_percentage_(0), upload_percentage_(0)
	{

	}
	virtual ~FTPHelper()
	{

	}

	int download(ftp_file_info& profile)
	{
		struct ftp_file ftp_file;
		char taget_file[100] = { '\0' };
		char user_info[100] = { '\0' };
		if ((profile.serve_file_path() == NULL) ||
			(profile.ip() == NULL) ||
			(profile.client_file_path() == NULL))
			return -1; 

		ftp_file.filename = profile.client_file_path();                     /*set the ftp_file information*/
		ftp_file.stream = NULL;
		sprintf_s(taget_file, 99, "ftp://%s/%s", profile.ip(), profile.serve_file_path());
		sprintf_s(user_info, 99, "%s:%s", profile.username(), profile.password());
		
		curl_global_init(CURL_GLOBAL_DEFAULT);                             /* In windows, this will init the winsock stuff */
		curl_ = curl_easy_init();                                           /* get a curl handle */
		if (curl_)
		{
			curl_easy_setopt(curl_, CURLOPT_URL, taget_file);                  /*set the target file*/
			curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);     /*write data into file*/
			curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &ftp_file);                /*set the ftp_file information*/
			curl_easy_setopt(curl_, CURLOPT_USERPWD, user_info);                /*set the user information*/
			curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl_, CURLOPT_XFERINFOFUNCTION, progress_func);
			curl_easy_setopt(curl_, CURLOPT_XFERINFODATA, this);
			curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
			curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10);
			curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10);
			res_ = curl_easy_perform(curl_);
			profile.error(curl_easy_strerror(res_));
			curl_easy_cleanup(curl_);
		}
		else
		{
			profile.error(" init the curl error");
			return -2;
		}
		if (ftp_file.stream)
		{
			fclose(ftp_file.stream);
		}
		curl_global_cleanup();
		return is_download_success(profile.error());
	}

	int upload(ftp_file_info& profile)
	{
		FILE *hd_src;
		struct stat file_info;
		curl_off_t fsize;
		const char * local_file = profile.client_file_path();
		struct curl_slist *headerlist = NULL;
		char taget_file[100] = { '\0' };
		char user_info[100] = { '\0' };
		char buf_1[100] = { '\0' };
		char buf_2[100] = { '\0' };

		sprintf_s(taget_file, 99, "ftp://%s/%s", profile.ip(), profile.serve_file_path());  /*translate the text formate*/
		sprintf_s(user_info, 99, "%s:%s", profile.username(), profile.password());
		sprintf_s(buf_1, 99, "RNFR%s", profile.client_file_path());
		sprintf_s(buf_2, 99, "RNTO%s", profile.client_file_path());
		if (stat(local_file, &file_info))                              /* get the file size of the local file */
		{
			return -1;
		}

		fsize = (curl_off_t)file_info.st_size;
		fopen_s(&hd_src,local_file, "rb");                            /* get a FILE * of the same file */
		curl_global_init(CURL_GLOBAL_ALL);                           /* In windows, this will init the winsock stuff */
		curl_ = curl_easy_init();                                    /* get a curl handle */
		if (curl_)
		{
			headerlist = curl_slist_append(headerlist, buf_1);            /* build a list of commands to pass to libcurl */
			headerlist = curl_slist_append(headerlist, buf_2);
			
			curl_easy_setopt(curl_, CURLOPT_READFUNCTION, read_callback); /* we want to use our own read function */
			curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);                 /* enable uploading */
			curl_easy_setopt(curl_, CURLOPT_URL, taget_file);              /* specify target */
			curl_easy_setopt(curl_, CURLOPT_USERPWD, user_info);
			curl_easy_setopt(curl_, CURLOPT_POSTQUOTE, headerlist);        /* pass in that last of FTP commands to run after the transfer */
			curl_easy_setopt(curl_, CURLOPT_READDATA, hd_src);            /* now specify which file to upload */
			curl_easy_setopt(curl_, CURLOPT_INFILESIZE_LARGE,(curl_off_t)fsize);
			curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl_, CURLOPT_XFERINFOFUNCTION, progress_func);
			curl_easy_setopt(curl_, CURLOPT_XFERINFODATA, this);
			curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
			curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 10);
			curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10);
			
			res_ = curl_easy_perform(curl_);

			curl_slist_free_all(headerlist);
			curl_easy_cleanup(curl_);
		}
		curl_global_cleanup();
		return  is_upload_success(profile.error());
	}

	virtual int progress_callback(int upload, int download)
	{
		return  1;
	}

private:
	CURL* curl_;
	CURLcode res_;
	int download_percentage_;
	int upload_percentage_;

	static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
	{
		curl_off_t nread;
		size_t ret = fread(ptr, size, nmemb, (FILE *)stream);
		nread = (curl_off_t)ret;
		return ret;
	}

	static size_t write_callback(void *buffer, size_t size, size_t nmemb, void *stream)
	{
		struct ftp_file *out = (struct ftp_file *)stream;
		if (out && !out->stream)
		{
			fopen_s(&(out->stream), out->filename, "wb");   /* open file for writing */
			if (!out->stream)
			{
				return -1;                              /* failure, can't open file to write */
			}
		}
		return fwrite(buffer, size, nmemb, out->stream);
	}

	static int progress_func(void* ptr,
		double TotalToDownload,
		double NowDownloaded,
		double TotalToUpload,
		double NowUpload)              /* use to get download percentage */
	{
		FTPHelper *instance = (FTPHelper*)ptr;
		double download_percentage, upload_percentage;
		
		(TotalToDownload != 0) ? (download_percentage = NowDownloaded * 100 / TotalToDownload) : (download_percentage = 0);
		(TotalToUpload   != 0) ? (upload_percentage = NowUpload * 100 / TotalToUpload) : (upload_percentage = 0);
		
		instance->percentage_set(upload_percentage, download_percentage);
		instance->progress_callback(upload_percentage, download_percentage);
		return 0;
	}

	int is_upload_success(const char* error)
	{
		if (upload_percentage_ == 100)
		{
			std::printf("\r\n[OK] Success\r\n");
			std::fflush(stdout);
			return 1;
		}
		else
		{
			std::printf("\r\n[ERROR] %s\r\n", error);
			std::fflush(stdout);
			return -1;
		}
	}

	int is_download_success(const char* error)
	{
		if (download_percentage_ == 100)
		{
			std::printf("\r\n[OK] Success\r\n");
			std::fflush(stdout);
			return 1;
		}
		else
		{
			std::printf("\r\n[ERROR] %s\r\n", error);
			std::fflush(stdout);
			return -1;
		}
	}

	void percentage_set(int up, int down)
	{
		download_percentage_ = down;
		upload_percentage_ = up;
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\bupload=%d download=%d", up, down);
		fflush(stdout);
	}
};
#endif //  FTPHELPER_H


int main(int argc, char *argv[])
{
	std::cout << "begin" << std::endl;
	FTPHelper ftp;
	FTPHelper::ftp_file_info info;
	info.ip("192.168.3.8");
	info.username("bai");
	info.password("a");
	info.client_file_path("map.pgm");
	info.server_file_path("map.pgm");
	std::cout << ftp.download(info) << std::endl;;

	info.client_file_path("stdafx.h");
	info.server_file_path("stdafx.h");
	std::cout << ftp.upload(info) << std::endl;
	std::getchar();
	return 0;

}