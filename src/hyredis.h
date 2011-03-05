/* 
 * File:   hyredis.h
 * Author: dvirsky
 *
 * Created on March 5, 2011, 3:44 PM
 */

#ifndef HYREDIS_H
#define	HYREDIS_H
#include <python2.6/Python.h>

typedef struct {

    PyObject_HEAD
    redisContext *conn;
    int pipeline_mode;
    int buffersize;
    int db;
    char *encoding;
    void *reader;
    struct {
        PyObject *ptype;
        PyObject *pvalue;
        PyObject *ptraceback;
    } error;
    PyObject *protocolErrorClass;
    PyObject *replyErrorClass;
    


} Redis;


extern PyObject *HiErr_Base;
extern PyObject *HiErr_ProtocolError;
extern PyObject *HiErr_ReplyError;


extern redisReplyObjectFunctions hiredis_ObjectFunctions;

//#define HYREDIS_LOGGING 1
#ifdef HYREDIS_LOGGING
#define _INFO(...) fprintf(stderr, "[INFO In %s:%d]", __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
#define _ERR(...) fprintf(stderr, "[ERROR In %s:%d]", __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
#define _WRN(...) fprintf(stderr, "[WARNING In %s:%d]", __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");
#else
#define _INFO(...) void;
#define _ERR(...) void;
#define _WRN(...) void;
#endif

#endif	/* HYREDIS_H */

