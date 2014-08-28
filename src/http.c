#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rio.h"
#include "http.h"

static void handle_get(int fd, char *url);
static void handle_head(int fd, char *url);
static void ignore_other_headers(rio_t * rp);
static int  parse_url(char *url, char *filename, char *cgiargs);
static void serve_static(int fd, char *filename, int filesize);
static void serve_dynamic(int fd, char *filename, char *cgiargs);
static void get_filetype(char *filename, char *filetype);
static void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg);


extern char **environ;

void *handle_http(int fd)
{
	char 	buf[MAXLINE];
	char 	method[MAXLINE];
	char 	url[MAXLINE];
	char 	version[MAXLINE];
	rio_t 	rio_buf;

	rio_readinit(&rio_buf, fd);
	rio_readline(&rio_buf, buf, MAXLINE);
	ignore_other_headers(&rio_buf);

	sscanf(buf, "%s %s %s", method, url, version);

	if (strcasecmp(method, "GET") == 0) {
		handle_get(fd, url);
	} else if (strcasecmp(method, "HEAD") == 0) {
		handle_head(fd, url);
	} else {
		clienterror(fd, method, "501", "Not Implemented",
			    "lynx does not implement this method");
	}
	close(fd);
	return NULL;
}

static void
handle_get(int fd, char *url)
{
	int 		is_static;
	struct stat 	sbuf;
	char 		filename[MAXLINE];
	char 		cgiargs[MAXLINE];

	is_static = parse_url(url, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) {
		clienterror(fd, filename, "404", "Not found",
			    "lynx couldn't find this file");
		return;
	}

	if (is_static) {
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden",
				    "lynx couldn't read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size);
	} else {
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(fd, filename, "403", "Forbidden",
				    "lynx couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs);
	}
	printf
	    ("\n--------------------------handle_http finish------------\n");
	return;
}

static void
handle_head(int fd, char *url)
{
	struct stat 	sbuf;
	char 		filename[MAXLINE];
	char		filetype[MAXLINE];
	char		cgiargs[MAXLINE];
	char		buf[MAXBUF];

	parse_url(url, filename, cgiargs);

	if (stat(filename, &sbuf) < 0) {
		clienterror(fd, filename, "404", "Not found",
			    "lynx couldn't find this file");
		return;
	}
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
		clienterror(fd, filename, "403", "Forbidden",
			    "lynx couldn't read the file");
		return;
	}

	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: lynx Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %ld\r\n", buf, sbuf.st_size);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	writen(fd, buf, strlen(buf));
	return;
}

static void
ignore_other_headers(rio_t * rp)
{
	char 	buf[MAXLINE];
	FILE 	*record = fopen(RECORD_FILE, "a");

	rio_readline(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {
		rio_readline(rp, buf, MAXLINE);
		fprintf(record, "%s", buf);
	}
	fclose(record);

	return;
}

static int
parse_url(char *url, char *filename, char *cgiargs)
{
	char 	*ptr;

	if (!strstr(url, "cgi-bin")) {
		strcpy(cgiargs, "");
		strcpy(filename, DATA_PATH);
		strcat(filename, url);
		if (url[strlen(url) - 1] == '/')
			strcat(filename, "index.html");
		return 1;
	} else {
		ptr = index(url, '?');
		if (ptr) {
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		} else
			strcpy(cgiargs, "");
		strcpy(filename, DATA_PATH);
		strcat(filename, url);
		return 0;
	}
}

static void
serve_static(int fd, char *filename, int filesize)
{
	int 	srcfd;
	char 	*srcp;
	char	filetype[MAXLINE];
	char	buf[MAXBUF];

	get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: lynx Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	writen(fd, buf, strlen(buf));

	srcfd = open(filename, O_RDONLY, 0);
	srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	close(srcfd);
	writen(fd, srcp, filesize);
	munmap(srcp, filesize);
}

static void
serve_dynamic(int fd, char *filename, char *cgiargs)
{
	char 	buf[MAXLINE];
	char	*emptylist[] = { NULL };

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Lynx Web Server\r\n");
	writen(fd, buf, strlen(buf));

	if (fork() == 0) {
		setenv("QUERY_STRING", cgiargs, 1);
		dup2(fd, STDOUT_FILENO);
		execve(filename, emptylist, environ);
		printf("exece fail!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		sprintf(buf, "Bad argument\r\n");
		writen(fd, buf, strlen(buf));
		exit(0);	/*important!!, for execve fail */
	}
}

static void
get_filetype(char *filename, char *filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else if (strstr(filename, ".css"))
		strcpy(filetype, "text/css");
	else if (strstr(filename, ".mp4"))
		strcpy(filetype, "video/mpeg4");
	else
		strcpy(filetype, "text/plain");
}

static void
clienterror(int fd, char *cause, char *errnum,
	    char *shortmsg, char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];

	sprintf(body, "<html><title>Lynx Error</title>");
	sprintf(body, "%s<body bgcolor=" "ffffff" ">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The lnyx Web server</em>\r\n", body);

	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int) strlen(body));
	writen(fd, buf, strlen(buf));
	writen(fd, body, strlen(body));
}
