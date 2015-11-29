/*
 * open_clientdf_ts - Thread safe wrapper for Open_clientfd
 */
int open_clientfd_ts(char *hostname, int port)
{
    int result;
    printf("Hostname: %s, port: %d", hostname, port);
    P(&sem);
    result = Open_clientfd(hostname, port);
    V(&sem);
    return result;
}

/*-----------Wrappers for robust I/O routines rewritten---------*/
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
    printf("Rio_readn error");
    return n;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
    printf("Rio_writen error");
}

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
    printf("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
    printf("Rio_readlineb error");
    return rc;
}
