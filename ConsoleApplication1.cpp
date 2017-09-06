#include "stdafx.h"
#include "curl\curl.h"
#include <stdio.h>
#include <functional>

#include "curl/curl.h"
#include <stdio.h>
#include <functional>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include"iostream"


class FTPHelper
{
public:
    struct ftp_file
    {
        const char *filename;
        FILE *stream;
    };
    class ftp_folder_info
    {
        public:
            ftp_folder_info():ip(NULL),port(22),username("Anonymous"),password(""),
                        serve_folder_path(NULL),client_folder_path(NULL)
            {
            }
            int ip_valid(const char* ip){return 1;}
        public:
            char* ip;
            int   port;
            char* username;
            char* password;
            char* serve_folder_path;
            char* client_folder_path;
            const char* error;
    };
    class ftp_file_info
    {
        public:
            ftp_file_info():ip(NULL),port(22),username("Anonymous"),password(""),
                        serve_file_path(NULL),client_file_path(NULL)
            {
            }
            int ip_valid(const char* ip){return 1;}
        public:
            char* ip;
            int   port;
            char* username;
            char* password;
            char* serve_file_path;
            char* client_file_path;
            const char* error;
    };
public:
    FTPHelper():download_percentage(0),upload_percentage(0)
    {

    }
    virtual ~FTPHelper()
    {

    }
    int download( ftp_file_info& profile)
    {
        using namespace std::placeholders;
        struct ftp_file ftp_file;
        char taget_file[100] = { '\0' };
        char user_info[100] = { '\0' };
        /* input check*/
        if (profile.serve_file_path == NULL)
            return -1;
        if (profile.ip == NULL)
            return -2;
        if (profile.client_file_path == NULL)
            return -3;
        ftp_file.filename = profile.client_file_path;    /*set the ftp_file information*/
        ftp_file.stream = NULL;
        try                                     /*translate the text formate*/
        {
            sprintf_s(taget_file, 99, "ftp://%s/%s", profile.ip, profile.serve_file_path);
            sprintf_s(user_info, 99, "%s:%s", profile.username, profile.password);
        }
        catch(std::exception e)
        {
            profile.error = e.what();
            return -2;
        }
        curl_global_init(CURL_GLOBAL_DEFAULT);                          /* In windows, this will init the winsock stuff */
        curl = curl_easy_init();                                        /* get a curl handle */
        if (curl)
        {     
            curl_easy_setopt(curl, CURLOPT_URL, taget_file);                   /*set the target file*/
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,write_callback);     /*write data into file*/
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftp_file);                /*set the ftp_file information*/
            curl_easy_setopt(curl, CURLOPT_USERPWD, user_info);                /*set the user information*/
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);                 /*open the progress*/
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,progress_func);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
            res = curl_easy_perform(curl);
            profile.error = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
        }
        else
        {
              profile.error = " init the curl error";
              return -3;
        }
        if (ftp_file.stream)
        {
            fclose(ftp_file.stream);
        }
        curl_global_cleanup();
        return is_download_success(profile.error);
    }
    int upload(ftp_file_info& profile)
    {
        CURL *curl;
        CURLcode res;
        FILE *hd_src;
        struct stat file_info;
        curl_off_t fsize;
        char * local_file = profile.client_file_path;
        struct curl_slist *headerlist=NULL;
        /*translate the text formate*/
        char taget_file[100] = { '\0' };
        char user_info[100] = { '\0' };
        char buf_1[100] = { '\0' };
        char buf_2[100] = { '\0' };
        try
        {
            sprintf_s(taget_file, 99, "ftp://%s/%s", profile.ip, profile.serve_file_path);
            printf("%s\r\n",taget_file);
            sprintf_s(user_info, 99, "%s:%s", profile.username, profile.password);
             printf("%s\r\n",user_info);
            sprintf_s(buf_1, 99, "RNFR%s", profile.client_file_path);
             printf("%s\r\n",buf_1);
            sprintf_s(buf_2, 99, "RNTO%s", profile.client_file_path);
             printf("%s\r\n",buf_2);
        }
        catch(std::exception e)
        {
            profile.error = e.what();
            return -2;
        }
        /* get the file size of the local file */
        if(stat(local_file, &file_info))
        {
          printf("Couldnt open '%s': %s\n", local_file, strerror(errno));
          return 1;
        }
        fsize = ( curl_off_t )file_info.st_size;
        printf("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);
        /* get a FILE * of the same file */
        hd_src = fopen(local_file, "rb");
        /* In windows, this will init the winsock stuff */
        curl_global_init(CURL_GLOBAL_ALL);
        /* get a curl handle */
        curl = curl_easy_init();
        if(curl)
        {
          headerlist = curl_slist_append(headerlist, buf_1);                /* build a list of commands to pass to libcurl */
          headerlist = curl_slist_append(headerlist, buf_2);
          curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);   /* we want to use our own read function */
          curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);                   /* enable uploading */
          curl_easy_setopt(curl, CURLOPT_URL, taget_file);                /* specify target */
          curl_easy_setopt(curl, CURLOPT_USERPWD, user_info);
          /* pass in that last of FTP commands to run after the transfer */
          curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
          /* now specify which file to upload */
          curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
          /* Set the size of the file to upload (optional).  If you give a *_LARGE
             option you MUST make sure that the type of the passed-in argument is a
             curl_off_t. If you use CURLOPT_INFILESIZE (without _LARGE) you must
             make sure that to pass in a type 'long' argument. */
          curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                           (curl_off_t)fsize);
          /* Now run off and do what you've been told! */
          res = curl_easy_perform(curl);
          /* Check for errors */
          if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
          /* clean up the FTP commands list */
          curl_slist_free_all(headerlist);
          /* always cleanup */
          curl_easy_cleanup(curl);
        }
        fclose(hd_src); /* close the local file */
        curl_global_cleanup();
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
    static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
    {
      curl_off_t nread;
      /* in real-world cases, this would probably get this data differently
         as this fread() stuff is exactly what the library already would do
         by default internally */
      size_t retcode = fread(ptr, size, nmemb, (FILE *)stream);
      nread = (curl_off_t)retcode;
      fprintf(stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
              " bytes from file\n", nread);
      return retcode;
    }
    /* use to write data into file */
    static size_t write_callback(void *buffer, size_t size, size_t nmemb, void *stream)
    {
        struct ftp_file *out = (struct ftp_file *)stream;
        if (out && !out->stream)
        {
            /* open file for writing */
            fopen_s(&(out->stream), out->filename, "wb");
            if (!out->stream)
            {
                return -1; /* failure, can't open file to write */
            }
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
            std::printf("\r\n[OK] Success\r\n");
            std::fflush(stdout);
            return 1;
        }
        else
        {
            std::printf("[ERROR] %s\r\n", error);
            std::fflush(stdout);
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
    std::cout<<"begin"<<std::endl;
    FTPHelper ftp;
    FTPHelper::ftp_file_info info;
    info.ip = "192.168.31.201";
    info.serve_file_path = "map.pgm";
    info.username = "wellcasa";
    info.password = " ";
    info.client_file_path = "D:/map.pgm";
    std::cout<<"begin"<<std::endl;
    std::cout<<ftp.download(info)<<std::endl;
   std::cout<<info.error<<std::endl;
    std::cout<<"end"<<std::endl;

        info.serve_file_path = "map.pgm";
        ftp.upload(info);

       std::cout<<info.error<<std::endl;
        std::cout<<"end111111"<<std::endl;
    return 0;
	
	return 0;
}