/* include this first, before NO_IMPORT_PYGOBJECT is defined */
#include <Python.h>
#include <pyglib.h> 
#include <pygobject.h> 

void pyfirtree_register_classes (PyObject *d);
void pyfirtree_add_constants(PyObject *module, const gchar *strip_prefix);

extern PyMethodDef pyfirtree_functions[];

DL_EXPORT(void)
initpyfirtree(void)
{
    PyObject *m, *d;
	
    init_pygobject ();
    g_thread_init (NULL);

    m = Py_InitModule ("pyfirtree", pyfirtree_functions);
    d = PyModule_GetDict (m);
	
    pyfirtree_register_classes (d);
    pyfirtree_add_constants(m, "FIRTREE_"); 
}

/* vim:sw=4:ts=4:cindent:et
 */
