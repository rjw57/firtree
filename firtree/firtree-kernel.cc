/* firtree-kernel.c */

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "internal/firtree-kernel-intl.hh"
#include "firtree-sampler.h"
#include "firtree-vector.h"

#include <llvm-frontend/llvm-compiled-kernel.h>

using namespace Firtree;
using namespace Firtree::LLVM;

enum {
    PROP_0,
    PROP_COMPILE_STATUS,
    LAST_PROP
};

enum {
    ARGUMENT_CHANGED,
    MODULE_CHANGED,
    CONTENTS_CHANGED,
    LAST_SIGNAL
};

static guint _firtree_kernel_signals[LAST_SIGNAL] = { 0 };

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

static GType
_firtree_kernel_type_specifier_to_gtype(KernelTypeSpecifier type_spec)
{
    static GType type_map[TySpecVoid+1] = {
        G_TYPE_FLOAT,
        G_TYPE_INT,
        G_TYPE_BOOLEAN,
        FIRTREE_TYPE_VEC2,
        FIRTREE_TYPE_VEC3,
        FIRTREE_TYPE_VEC4,
        FIRTREE_TYPE_SAMPLER,
        FIRTREE_TYPE_VEC4,
        G_TYPE_NONE
    };

    if((type_spec >= 0) && (type_spec <= TySpecVoid)) {
        return type_map[type_spec];
    }

    g_error("Unhandled type specifier: %i\n", type_spec);

    return G_TYPE_NONE;
}

static void
firtree_kernel_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(object);
    switch (property_id) {
        case PROP_COMPILE_STATUS:
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

    klass->argument_changed = NULL;
    klass->module_changed = NULL;
    klass->contents_changed = NULL;

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
            PROP_COMPILE_STATUS,
            param_spec);

    /**
     * FirtreeKernel::argument-changed:
     * @kernel: The kernel whose argument has changed.
     * @arg_name: A string indicating the argument name.
     *
     * The ::argument-changed signal is emitted each time a @kernel 's argument
     * is modified via firtree_kernel_set_argument_value().
     */
    _firtree_kernel_signals[ARGUMENT_CHANGED] = 
        g_signal_new("argument-changed",
                G_OBJECT_CLASS_TYPE(klass),
                (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | 
                    G_SIGNAL_DETAILED),
                G_STRUCT_OFFSET(FirtreeKernelClass, argument_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__STRING,
                G_TYPE_NONE, 1, G_TYPE_STRING);
    
    /**
     * FirtreeKernel::module-changed:
     * @kernel: The kernel whose module has changed.
     *
     * The ::module-changed signal is emitted each time the @kernel 's 
     * compiled module is changed via a call to 
     * firtree_kernel_compile_from_source(). In addition, failed compilations
     * cause this signal to be emitted.
     */
    _firtree_kernel_signals[MODULE_CHANGED] = 
        g_signal_new("module-changed",
                G_OBJECT_CLASS_TYPE(klass),
                (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE),
                G_STRUCT_OFFSET(FirtreeKernelClass, module_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE, 0);
    /**
     * FirtreeKernel::contents-changed:
     * @kernel: The kernel whose argument has changed.
     *
     * The ::contents-changed signal is emitted each time the contents
     * of a kernel changes (i.e. when the contents of a dependent 
     * sampler changes).
     */
    _firtree_kernel_signals[CONTENTS_CHANGED] = 
        g_signal_new("contents-changed",
                G_OBJECT_CLASS_TYPE(klass),
                (GSignalFlags)(G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | 
                    G_SIGNAL_DETAILED),
                G_STRUCT_OFFSET(FirtreeKernelClass, contents_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE, 0);
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
        firtree_kernel_module_changed(self);
        return p->compile_status;
    }

    const KernelFunctionList& functions = p->compiled_kernel->GetKernels();

    /* Do we have any functions defined? */
    if(functions.size() == 0) {
        p->compile_status = FALSE;
        firtree_kernel_module_changed(self);
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
        g_assert(p->arg_names == NULL);
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
            spec->type = _firtree_kernel_type_specifier_to_gtype(i->Type);
            spec->is_static = i->IsStatic;

            /* add the spec to the list. */
            g_datalist_id_set_data_full(&p->arg_spec_list,
                    name_quark, spec, 
                    _firtree_kernel_arg_spec_destroy_func);
        }
    }

    firtree_kernel_module_changed(self);

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
    /* We do it this way to avoid creating a GQuark for an
     * argument name that may well not exist meaning we needlessly make
     * a copy of the argument. This way is slower but more memory 
     * efficient. */

    GQuark* args = firtree_kernel_list_arguments(self, NULL);
    while(*args)
    {
        if(0 == g_strcmp0(arg_name, g_quark_to_string(*args))) {
            return TRUE;
        }
        ++args;
    }
    return FALSE;
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

/* Free a GValue structure allocated via g_slice_{alloc,new}, etc. */
/* Used in firtree_kernel_set_argument_value(). */
static void
_firtree_kernel_value_destroy_func (gpointer value)
{
    g_value_unset((GValue*)value);
    g_slice_free(GValue, value);
}

static void
_firtree_kernel_sampler_module_changed_cb(FirtreeSampler* sampler,
        FirtreeKernel* self)
{
    if(FIRTREE_IS_KERNEL(self)) {
        firtree_kernel_module_changed(self);
    }
}

static void
_firtree_kernel_sampler_contents_changed_cb(FirtreeSampler* sampler,
        FirtreeKernel* self)
{
    if(FIRTREE_IS_KERNEL(self)) {
        firtree_kernel_contents_changed(self);
    }
}

gboolean
firtree_kernel_set_argument_value (FirtreeKernel* self,
        GQuark arg_name, GValue* value)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    FirtreeKernelArgumentSpec* spec =
        firtree_kernel_get_argument_spec(self, arg_name);

    /* If we have no spec, we assume it is because this argument doesn't
     * exist. */
    if(spec == NULL) {
        return FALSE;
    }

    /* If value is NULL, unset the value and return. */
    if(NULL == value) {
        g_datalist_id_set_data(&p->arg_value_list, arg_name, NULL);
        firtree_kernel_argument_changed(self, arg_name);
        return TRUE;
    }

    /* Check the type */
    if(spec->type != G_VALUE_TYPE(value)) {
        return FALSE;
    }

    /* Special case: for sampler arguments, we care about when their
     * module changes since we link them in. Register handlers for
     * this. */
    if(spec->type == FIRTREE_TYPE_SAMPLER) {
        /* register the new handler if necessary. */
        if(value) {
            FirtreeSampler* new_sampler = FIRTREE_SAMPLER(
                    g_value_get_object(value));
            g_signal_connect(new_sampler, "module-changed", 
                    G_CALLBACK(_firtree_kernel_sampler_module_changed_cb),
                    self);
            g_signal_connect(new_sampler, "contents-changed", 
                    G_CALLBACK(_firtree_kernel_sampler_contents_changed_cb),
                    self);
        }
    }

    /* Create a new GValue to store a copy of the passed value. */
    GValue* new_val = (GValue*)g_slice_alloc0(sizeof(GValue));
    g_value_init(new_val, spec->type);
    g_value_copy(value, new_val);

    /* Insert the GValue into the arg value list. */
    g_datalist_id_set_data_full(&p->arg_value_list, arg_name,
            new_val, _firtree_kernel_value_destroy_func);

    firtree_kernel_argument_changed(self, arg_name);

    return TRUE;
}

void
firtree_kernel_argument_changed (FirtreeKernel* self, GQuark arg_name)
{
    g_return_if_fail(FIRTREE_IS_KERNEL(self));
    g_signal_emit(self, _firtree_kernel_signals[ARGUMENT_CHANGED], arg_name, 
            g_quark_to_string(arg_name));
}

gboolean
firtree_kernel_is_valid (FirtreeKernel* self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);
    if(!p->compile_status) {
        return FALSE;
    }

    /* iterate through arguments */
    GQuark* args = firtree_kernel_list_arguments(self, NULL);
    if(!args) {
        return FALSE;
    }
    while(*args) {
        if(NULL == firtree_kernel_get_argument_value(self, *args)) {
            return FALSE;
        }
        ++args;
    }

    return TRUE;
}

void
firtree_kernel_module_changed (FirtreeKernel* self)
{
    g_return_if_fail(FIRTREE_IS_KERNEL(self));
    g_signal_emit(self, _firtree_kernel_signals[MODULE_CHANGED], 0);
}

void
firtree_kernel_contents_changed (FirtreeKernel* self)
{
    g_return_if_fail(FIRTREE_IS_KERNEL(self));
    g_signal_emit(self, _firtree_kernel_signals[CONTENTS_CHANGED], 0);
}

llvm::Function*
firtree_kernel_get_function(FirtreeKernel* self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    /* If we have no compiled kernel, we have no module. */
    if(!p->compiled_kernel || !p->compile_status) {
        return NULL;
    }

    return p->preferred_function->Function;
}

GType
firtree_kernel_get_return_type (FirtreeKernel* self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    /* If we have no compiled kernel, we have no return type. */
    if(!p->compiled_kernel || !p->compile_status) {
        return G_TYPE_NONE;
    }

    return _firtree_kernel_type_specifier_to_gtype(
            p->preferred_function->ReturnType);
}

FirtreeKernelTarget
firtree_kernel_get_target (FirtreeKernel* self)
{
    FirtreeKernelPrivate* p = GET_PRIVATE(self);

    if(!p->compiled_kernel || !p->compile_status) {
        return FIRTREE_KERNEL_TARGET_INVALID;
    }

    if( p->preferred_function->Target == LLVM::KernelFunction::Render ) {
        return FIRTREE_KERNEL_TARGET_RENDER;
    } else if( p->preferred_function->Target == LLVM::KernelFunction::Reduce ) {
        return FIRTREE_KERNEL_TARGET_REDUCE;
    }

    return FIRTREE_KERNEL_TARGET_INVALID;
}

/* vim:sw=4:ts=4:et:cindent
 */
