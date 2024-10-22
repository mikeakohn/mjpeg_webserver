#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

/*************************************************************

mjpeg_webserver_plugin_init() - This function will get called when
the server starts up.

*************************************************************/
void mjpeg_webserver_plugin_init()
{
  printf("mjpeg_webserver_plugin_init() called\n");
}

/*************************************************************

mjpeg_webserver_plugin_get() - This function is called when a user
requests the plugin. Querystring holds everything in the
URL after the ? null terminated so for example if the
user typed:

http://localhost/test_plugin.cgi?user_id=1&name=mike%20kohn

then querystring="user_id=1&name=mike%20kohn";

fd_out is used to output html or whatever through webserver
to the user's screen.

The return value should be 0 for a keep-alive connection
and there is no problem (content-length must be sent then)
or -1 to tell the server to close the connection.

*************************************************************/

int mjpeg_webserver_plugin_get(int fd_out, char *querystring)
{
  printf("mjpeg_webserver_plugin_get() called %s\n",querystring);

  send(fd_out, "Content-Type: text/html\r\n\r\n", sizeof("Content-Type: text/html\r\n\r\n") - 1, 0);
  send(fd_out, "Get Querystring is ", sizeof("Get Querystring is ") - 1, 0);
  send(fd_out, querystring, strlen(querystring), 0);

  return -1;
}

/*************************************************************

mjpeg_webserver_plugin_get() - This function is called when a user
submits a form using the post method. Querystring here is
the same as it is for get. The server expects this function
to read content_length bytes from fd_in (which will be the
submitted data). Otherwise it's identical to mjpeg_webserver_plugin_get().

*************************************************************/

int mjpeg_webserver_plugin_post(int fd, char *querystring, int content_length)
{
  char buffer[1024];
  int c, n;

  printf("mjpeg_webserver_plugin_post() called %s\n", querystring);

  c = 0;

  while (c < content_length)
  {
    if (content_length - c < 1023)
    {
      n = recv(fd_in, buffer, content_length, 0);
    }
      else
    {
      n = recv(fd_in, buffer, 1023, 0);
    }

    if (n < 1) { break; }
    buffer[n] = 0;
    printf("recv'd %s\n", buffer);
    c = c + n;
  }

  send(fd_out, "Content-Type: text/html\r\n\r\n", sizeof("Content-Type: text/html\r\n\r\n") - 1, 0);
  send(fd_out, "Post Querystring is ", sizeof("Post Querystring is ") - 1, 0);
  send(fd_out, querystring, strlen(querystring), 0);

  return -1;
}

