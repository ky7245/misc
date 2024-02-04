#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <curl/curl.h>

#define DIST_URL "https://curl.se/libcurl/c/getinmemory.html"

struct BodyStruct
{
    char *body;
    size_t size;
};

struct HeaderStruct
{
    char location[256];
    char contentType[256];
};

static size_t
WriteBodyCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct BodyStruct *mem = (struct BodyStruct *)userp;

    char *ptr = realloc(mem->body, mem->size + realsize + 1);
    if (!ptr)
    {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return CURLE_OUT_OF_MEMORY;
    }

    mem->body = ptr;
    memcpy(&(mem->body[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->body[mem->size] = 0;

    return realsize;
}

/* 最期に入る改行を取り除く関数 */
void lntrim(char *str)
{
    char *p;
    p = strstr(str, "\r\n");
    if (p != NULL)
    {
        *p = '\0';
    }
}
/* 先頭に入る空白を取り除く関数 */
void delHeadSpace(char *str)
{
    if (str != NULL && str[0] == ' ')
    {
        memmove(str, str + 1, strlen(str));
    }
}

static size_t
WriteHeaderCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct HeaderStruct *head = (struct HeaderStruct *)userp;

    char *hp = NULL;

    hp = strstr((char *)contents, "Location:");
    if (hp != NULL)
    {
        strncpy(head->location, (char *)contents + strlen("Location:"), sizeof(head->location));
    }

    hp = strstr((char *)contents, "Content-Type:");
    if (hp != NULL)
    {
        strncpy(head->contentType, (char *)contents + strlen("Content-Type:"), sizeof(head->contentType));
    }

    return realsize;
}

int main(void)
{
    CURL *curl_handle;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, DIST_URL);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);

    struct HeaderStruct header;
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&header);

    struct BodyStruct bmem;

    bmem.body = malloc(1); /* will be grown as needed by the realloc above */
    bmem.size = 0;         /* no data at this point */

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteBodyCallback);

    /* we pass our 'bmem' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&bmem);

    /* some servers do not like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);

    /* get it! */
    res = curl_easy_perform(curl_handle);

    /* check for errors */
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
    }
    else
    {
        lntrim(header.contentType);
        lntrim(header.location);
        delHeadSpace(header.contentType);
        delHeadSpace(header.location);
        printf("%ld Content-Type:[%s]\n", strlen(header.contentType), header.contentType);
        printf("%ld Location:[%s]\n", strlen(header.location), header.location);

        FILE *fp;
        fp = fopen("bodyData.html", "w");
        int writeBytes = 0;
        writeBytes = fprintf(fp, "%s", bmem.body);
        printf("%d byte wrote.\n", writeBytes);
        fclose(fp);
    }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    /* we are done with libcurl, so clean it up */
    curl_global_cleanup();

    return 0;
}