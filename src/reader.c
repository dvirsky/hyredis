#include <assert.h>
#include "reader.h"
#include "hyredis.h"


static void *tryParentize(const redisReadTask *task, PyObject *obj) {
    _INFO(__FUNCTION__);
    PyObject *parent;
    if (task && task->parent) {
        parent = (PyObject*)task->parent->obj;
        //assert(parent->ob_type == PyList_Type);
        PyList_SET_ITEM(parent, task->idx, obj);
    }
    return obj;
}

static PyObject *createDecodedString(Redis *self, const char *str, size_t len) {
    _INFO("Created decoded string");
    PyObject *obj;


    if (self->encoding == NULL) {

        obj = PyString_FromStringAndSize(str, len);
    } else {
        obj = PyUnicode_Decode(str, len, self->encoding, NULL);
        if (obj == NULL) {
            if (PyErr_ExceptionMatches(PyExc_ValueError)) {
                /* Ignore encoding and simply return plain string. */
                obj = PyString_FromStringAndSize(str, len);
            } else {
                assert(PyErr_ExceptionMatches(PyExc_LookupError));

                /* Store error when this is the first. */
                if (self->error.ptype == NULL)
                    PyErr_Fetch(&(self->error.ptype), &(self->error.pvalue),
                            &(self->error.ptraceback));

                /* Return Py_None as placeholder to let the error bubble up and
                 * be used when a full reply in Reader#gets(). */
                obj = Py_None;
                Py_INCREF(obj);
            }

            PyErr_Clear();
        }
    }

    assert(obj != NULL);
    return obj;
}

static void *hyredis_createStringObject(const redisReadTask *task, char *str, size_t len) {
    Redis *self = (Redis*)task->privdata;
    PyObject *obj;

    if (task->type == REDIS_REPLY_ERROR) {
        PyObject *args = Py_BuildValue("(s#)", str, len);
        assert(args != NULL); /* TODO: properly handle OOM etc */

        
        obj = PyObject_CallObject(self->replyErrorClass, args);
        assert(obj != NULL);
        Py_DECREF(args);
    } else {

        obj = createDecodedString(self, str, len);
    }

    return tryParentize(task, obj);
}

static void *hyredis_createArrayObject(const redisReadTask *task, int elements) {
    PyObject *obj;
    obj = PyList_New(elements);
    return tryParentize(task, obj);
}

static void *hyredis_createIntegerObject(const redisReadTask *task, long long value) {
    PyObject *obj;
    obj = PyLong_FromLongLong(value);
    return tryParentize(task, obj);
}

static void *hyredis_createNilObject(const redisReadTask *task) {
    PyObject *obj = Py_None;
    Py_INCREF(obj);
    return tryParentize(task, obj);
}


static void hyredis_freeObject(void *obj) {
    Py_XDECREF(obj);
}


redisReplyObjectFunctions hyredis_ObjectFunctions = {
    hyredis_createStringObject,  // void *(*createString)(const redisReadTask*, char*, size_t);
    hyredis_createArrayObject,   // void *(*createArray)(const redisReadTask*, int);
    hyredis_createIntegerObject, // void *(*createInteger)(const redisReadTask*, long long);
    hyredis_createNilObject,     // void *(*createNil)(const redisReadTask*);
    hyredis_freeObject           // void (*freeObject)(void*);
};

