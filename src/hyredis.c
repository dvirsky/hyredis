
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <python2.6/Python.h>
#include <python2.6/structmember.h>
#include "hiredis.h"
#include "hyredis.h"
#include "sds.h"
#include "reader.h"
#include <assert.h>


static PyMemberDef Redis_members[] = {

    {NULL} /* Sentinel */
};

static void Redis_dealloc(Redis *self);
static PyObject *Redis_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Redis_init(Redis *self, PyObject *args, PyObject *kwds);
static PyObject *Redis_pipeline(Redis* self, PyObject *args);
static PyObject *Redis_execute(Redis* self, PyObject *args);
static PyObject *Redis_command(Redis* self, PyObject *args);



static PyMethodDef Redis_methods[] = {
    {
        "execute", (PyCFunction) Redis_execute, METH_VARARGS,
        "Execute a pipeline context's out buffer"
    },
    {
        "command", (PyCFunction) Redis_command, METH_VARARGS,
        "generic command executor"
    },
    {NULL} /* Sentinel */
};





static PyTypeObject RedisType = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "hyredis.Redis", /*tp_name*/
    sizeof (Redis), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor) Redis_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    0, /*tp_getattr*/
    0, /*tp_setattr*/
    0, /*tp_compare*/
    0, /*tp_repr*/
    0, /*tp_as_number*/
    0, /*tp_as_sequence*/
    0, /*tp_as_mapping*/
    0, /*tp_hash */
    0, /*tp_call*/
    0, /*tp_str*/
    0, /*tp_getattro*/
    0, /*tp_setattro*/
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Redis client object", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    Redis_methods, /* tp_methods */
    Redis_members, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    (initproc) Redis_init, /* tp_init */
    0, /* tp_alloc */
    Redis_new, /* tp_new */
};

static PyMethodDef module_methods[] = {
    {NULL} /* Sentinel */
};


static void
Redis_dealloc(Redis* self) {
    //_INFO("Deallocating conn");
    //redisReplyReaderFree(self->reader);
    if (self->encoding)
        free(self->encoding);

    redisFree(self->conn);
    Py_DECREF(self);
    
    self->ob_type->tp_free((PyObject*) self);
}



static PyObject *
Redis_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {

    Redis *self;

    char *host = "redis";
    int port = 6379;
    int db = 0;
    int pipeline = 0;
    int transaction = 0;
     
    static char *kwdlist[] = {"host", "port", "db", "pipeline", "transaction", NULL};



    if (args && kwds) {
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|siibb", kwdlist,
                &host, &port, &db, pipeline, transaction)) {


            Py_RETURN_NONE;
        }
    }
    _INFO("Creating redis client. host: %s, port %d, db: %d", host, port, db)

    self = (Redis *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->pipeline_mode = pipeline;
        //self->transaction
        self->buffersize = 0;
        self->db = 0;

        //create custom reader and connect it to hiredis
        self->reader = redisReplyReaderCreate();
        redisReplyReaderSetReplyObjectFunctions(self->reader, &hyredis_ObjectFunctions);
        redisReplyReaderSetPrivdata(self->reader, self);

        //connect
        self->conn = redisConnect((char*)host, port);

        //put the functions pointer and the reader into the connection
        //FIXME: this is a bit of a hack but the "formal" commands didn't work. fix this
        self->conn->fn = &hyredis_ObjectFunctions;
        self->conn->reader = self->reader;

        //init exceptions
        self->protocolErrorClass = HiErr_ProtocolError;
        self->replyErrorClass = HiErr_ReplyError;
        Py_INCREF(self->protocolErrorClass);
        Py_INCREF(self->replyErrorClass);

        self->error.ptype = NULL;
        self->error.pvalue = NULL;
        self->error.ptraceback = NULL;

        //no connection
        if (self->conn->err) {

            Py_DECREF(self);
            PyErr_SetString(self->protocolErrorClass, "Could not connect to redis server");
            return NULL;
            //Py_RETURN_NONE;

        }
        if (self->conn == NULL) {
            PyErr_SetString(self->protocolErrorClass, "Could not create redis client");
            Py_DECREF(self);
            return NULL;
        }
        

        PyObject *reply = redisCommand(self->conn, "SELECT %d", db);
        //TODO: Make sure the selection went fine
        Py_DECREF(reply);
        
    }
    
    _INFO("Finished creating hyredis object!")
    return (PyObject *) self;
}


static int
Redis_init(Redis *self, PyObject *args, PyObject *kwds) {

    _INFO("Created new hyredis client!");
    return 0;
}



static PyObject *
Redis_execute(Redis* self, PyObject *args) {

    PyObject *ret = PyList_New(0);
    redisReply *reply = NULL;
    int i = 0;
    while (self->buffersize > 0 && redisGetReply(self->conn, (void **) &reply) == REDIS_OK) {
        --self->buffersize;
        PyObject *obj = reply->replyObj;
        
        PyList_Append(ret, obj);
        //Py_DECREF(obj);
        //freeReplyObject(reply);
        
    }
    return ret;

}


static PyObject *
Redis_command(Redis* self, PyObject *args) {
    
    char *command = NULL;
    PyObject *argList = NULL;
    if (!PyArg_ParseTuple(args, "s|O", &command, &argList)) {
        Py_RETURN_NONE;
    }

    if (!PyObject_IsInstance(argList, &PyTuple_Type)) {
        //TODO: Raise exception
        Py_DECREF(argList);
        Py_RETURN_NONE;
    }


    //make room for a string array for all commands and args
    size_t listLen = PyTuple_Size(argList) + 1;
    size_t i = 0;

    //create c string arg list
    char **argStrings = calloc(listLen, sizeof(char *));
    
    argStrings[0] = command;
    for(i = 1; i < listLen; i++) {
          PyObject *strObj = PyObject_Str(PyTuple_GetItem(argList, i - 1));

          argStrings[i] = strdup(PyString_AsString(strObj));
          Py_DECREF(strObj);
          //_INFO("Adding argument %s", argStrings[i]);

    }
    Py_DECREF(argList);
    
    PyObject *obj = NULL;
    char *err = NULL;

    
    //first, add the command to the command buffer
    redisAppendCommandArgv(self->conn, listLen, argStrings, NULL);

    //clean up the args strings
    for(i = 1; i < listLen; i++)
         free(argStrings[i]);
    free(argStrings);

    //in pipeline mode, simply append the command at the end of the buffer
    if(self->pipeline_mode) {

        
        self->buffersize++;

    }
    //in normal mode, get the response as well
    else {

        
        //get the response
        if (redisGetReply(self->conn, &obj) != REDIS_OK) {

            //mmm..something went wrong
            err = redisReplyReaderGetError(self->reader);
            PyErr_SetString(self->protocolErrorClass, err);
            return NULL;
        }
        
        if (obj == NULL) {
            Py_RETURN_FALSE;
        }else {

            /* Restore error when there is one. */
            if (self->error.ptype != NULL) {
                Py_DECREF(obj);
                PyErr_Restore(self->error.ptype, self->error.pvalue,
                              self->error.ptraceback);
                self->error.ptype = NULL;
                self->error.pvalue = NULL;
                self->error.ptraceback = NULL;
                return NULL;
            }
            return obj;
        }
    }


    Py_RETURN_NONE;

}


#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif


PyObject *HiErr_Base;
PyObject *HiErr_ProtocolError;
PyObject *HiErr_ReplyError;

PyMODINIT_FUNC
inithyredis(void) {
    PyObject* m;

    if (PyType_Ready(&RedisType) < 0)
        return;

    m = Py_InitModule3("hyredis", module_methods,
            "Binary Redis Client For Python");

    if (m == NULL)
        return;

    Py_INCREF(&RedisType);
    PyModule_AddObject(m, "Redis", (PyObject *) & RedisType);


    HiErr_Base = PyErr_NewException("hyredis.HyredisError", PyExc_Exception, NULL);
    HiErr_ProtocolError = PyErr_NewException("hyredis.ProtocolError", HiErr_Base, NULL);
    HiErr_ReplyError = PyErr_NewException("hyredis.ReplyError", HiErr_Base, NULL);

    PyModule_AddObject(m, "HyredisError", HiErr_Base);
    PyModule_AddObject(m, "ProtocolError", HiErr_ProtocolError);
    PyModule_AddObject(m, "ReplyError", HiErr_ReplyError);






}
