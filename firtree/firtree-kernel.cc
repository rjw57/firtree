/* firtree-kernel.c */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "firtree-kernel-priv.hpp"

#include <llvm-frontend/llvm-compiled-kernel.h>

using namespace Firtree;
using namespace Firtree::LLVM;

enum
{
  FIRTREE_KERNEL_PROP_0,

  FIRTREE_KERNEL_PROP_COMPILE_STATUS
};

G_DEFINE_TYPE (FirtreeKernel, firtree_kernel, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), FIRTREE_TYPE_KERNEL, FirtreeKernelPrivate))

typedef struct _FirtreeKernelPrivate FirtreeKernelPrivate;

struct _FirtreeKernelPrivate {
    CompiledKernel*                     compiled_kernel;
    gboolean                            compile_status;
    KernelFunctionList::const_iterator  preferred_function;

    GArray*                             arg_names;
    GData*                              arg_spec_list;
    GData*                              arg_value_list;
};

static void
firtree_kernel_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(object);
    switch (property_id) {
        case FIRTREE_KERNEL_PROP_COMPILE_STATUS:
            g_value_set_boolean(value, p->compile_status);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
firtree_kernel_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
    switch (property_id) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_firtree_kernel_reset_compile_status (FirtreeKernel *self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    p->compile_status = FALSE;
    if(p->arg_names) {
        g_array_free(p->arg_names, TRUE);
        p->arg_names = NULL;
    }
    if(p->arg_spec_list) {
        g_datalist_clear(&(p->arg_spec_list));
    }
    if(p->arg_value_list) {
        g_datalist_clear(&(p->arg_value_list));
    }
}

static void
firtree_kernel_dispose (GObject *object)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(object);

    _firtree_kernel_reset_compile_status(FIRTREE_KERNEL(object));

    if(p->compiled_kernel) {
        FIRTREE_SAFE_RELEASE(p->compiled_kernel);
        p->compiled_kernel = NULL;
    }
    
    G_OBJECT_CLASS (firtree_kernel_parent_class)->dispose (object);
}

static void
firtree_kernel_finalize (GObject *object)
{
    G_OBJECT_CLASS (firtree_kernel_parent_class)->finalize (object);
}

static void
firtree_kernel_class_init (FirtreeKernelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GParamSpec* param_spec;

    g_type_class_add_private (klass, sizeof (FirtreeKernelPrivate));

    object_class->get_property = firtree_kernel_get_property;
    object_class->set_property = firtree_kernel_set_property;
    object_class->dispose = firtree_kernel_dispose;
    object_class->finalize = firtree_kernel_finalize;

    param_spec = g_param_spec_boolean(
            "compile-status",
            "A flag indicating the success of the last compilation.",
            "Get the compilation status.",
            FALSE,
            (GParamFlags)(
                G_PARAM_READABLE | 
                G_PARAM_STATIC_NAME | 
                G_PARAM_STATIC_NICK |
                G_PARAM_STATIC_BLURB));
    /**
     * FirtreeKernel:compile-status:
     *
     * The return value from the last call to 
     * firtree_kernel_compile_from_source() or FALSE if this method
     * has not yet been called.
     */
    g_object_class_install_property (object_class,
            FIRTREE_KERNEL_PROP_COMPILE_STATUS,
            param_spec);
}

static void
firtree_kernel_init (FirtreeKernel *self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    /* initialise the compiled kernel to NULL (i.e. we have none) */
    p->compiled_kernel = NULL;
    p->compile_status = FALSE;

    p->arg_names = NULL;
    g_datalist_init(&(p->arg_spec_list));
    g_datalist_init(&(p->arg_value_list));

    _firtree_kernel_reset_compile_status(self);
}

FirtreeKernel*
firtree_kernel_new (void)
{
    return (FirtreeKernel*) g_object_new (FIRTREE_TYPE_KERNEL, NULL);
}

static void
_firtree_kernel_arg_spec_destroy_func(gpointer data)
{
    g_slice_free(FirtreeKernelArgumentSpec, data);
}

gboolean
firtree_kernel_compile_from_source (FirtreeKernel* self, 
        gchar** lines, gint n_lines,
        gchar* kernel_name)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    /* Create a CompiledKernel object if necessary. */
    if(!p->compiled_kernel) {
        p->compiled_kernel = CompiledKernel::Create();
    }

    _firtree_kernel_reset_compile_status(self);

    p->compile_status = p->compiled_kernel->Compile(lines, n_lines);
    if(!p->compile_status) {
        return p->compile_status;
    }

    const KernelFunctionList& functions = p->compiled_kernel->GetKernels();

    /* Do we have any functions defined? */
    if(functions.size() == 0) {
        p->compile_status = FALSE;
        return p->compile_status;
    }

    /* By default, use the first kernel function */
    p->preferred_function = functions.begin();

    /* Do we have a preferred function */
    if(NULL != kernel_name) {
        for(;   
            (p->preferred_function != functions.end()) && 
            (p->preferred_function->Name != kernel_name);
            ++(p->preferred_function)) { }
        p->compile_status = (p->preferred_function != functions.end());
    }

    /* If the compile status is good, let's create a list of our arguments. */
    if(p->compile_status) {
        /* Create an array to store the argument name quarks. */
        p->arg_names = g_array_sized_new(TRUE, FALSE, 
                sizeof(GQuark), p->preferred_function->Parameters.size());

        KernelParameterList::const_iterator i;
        for(i = p->preferred_function->Parameters.begin();
                i != p->preferred_function->Parameters.end();
                ++i)
        {
            GQuark name_quark = g_quark_from_string(i->Name.c_str());
            g_array_append_val(p->arg_names, name_quark);

            /* Create a FirtreeKernelArgumentSpec struct. */
            FirtreeKernelArgumentSpec* spec =
                g_slice_new(FirtreeKernelArgumentSpec);

            spec->name_quark = name_quark;
            spec->type = 0; // FIXME
            spec->is_static = i->IsStatic;

            /* add the spec to the list. */
            g_datalist_id_set_data_full(&p->arg_spec_list,
                    name_quark, spec, 
                    _firtree_kernel_arg_spec_destroy_func);
        }
    }

    return p->compile_status;
}

gchar**
firtree_kernel_get_compile_log (FirtreeKernel* self, guint* n_log_lines)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    /* If we have no compiled kernel, we have no log. */
    if(!p->compiled_kernel) {
        return NULL;
    }

    return const_cast<gchar**>(p->compiled_kernel->GetCompileLog(n_log_lines));
}

gboolean
firtree_kernel_get_compile_status (FirtreeKernel* self)
{
    gboolean ret_val = FALSE;
    g_object_get(self, "compile-status", &ret_val, NULL);
    return ret_val;
}

GQuark*
firtree_kernel_list_arguments (FirtreeKernel* self, guint* n_arguments)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    if(p->compiled_kernel && p->compile_status)
    {
        if(n_arguments) {
            *n_arguments = p->arg_names->len;
        }
        return (GQuark*)p->arg_names->data;
    }
    return NULL;
}

gboolean
firtree_kernel_has_argument_named (FirtreeKernel* self, gchar* arg_name)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    return (NULL != g_datalist_id_get_data(&p->arg_spec_list, 
                g_quark_from_string(arg_name)));
}

FirtreeKernelArgumentSpec*
firtree_kernel_get_argument_spec (FirtreeKernel* self, GQuark arg_name)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    return (FirtreeKernelArgumentSpec*)
        g_datalist_id_get_data(&p->arg_spec_list, arg_name);
}

GValue*
firtree_kernel_get_argument_value (FirtreeKernel* self, GQuark arg_name)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    return (GValue*)g_datalist_id_get_data(&p->arg_value_list, arg_name);
}

gboolean
firtree_kernel_set_argument_value (FirtreeKernel* self,
        GQuark arg_name, GValue* value)
{
    return FALSE;
}

llvm::Module*
firtree_kernel_get_llvm_module(FirtreeKernel* self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    /* If we have no compiled kernel, we have no module. */
    if(!p->compiled_kernel || !p->compile_status) {
        return NULL;
    }

    return p->compiled_kernel->GetCompiledModule();
}

/* vim:sw=4:ts=4:et:cindent
 */
