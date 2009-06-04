/* firtree-kernel.h */

/* Firtree - A generic image processing library
 * Copyright (C) 2009 Rich Wareham <richwareham@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License verstion as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef _FIRTREE_KERNEL
#define _FIRTREE_KERNEL

#include <glib-object.h>

/**
 * SECTION:firtree-kernel
 * @short_description: Compile Firtree kernels from source.
 * @include: firtree/firtree-kernel.h
 *
 * A FirtreeKernel encapsulates a kernel compiled from source code in the
 * Firtree kernel language.
 */

G_BEGIN_DECLS

#define FIRTREE_TYPE_KERNEL firtree_kernel_get_type()

#define FIRTREE_KERNEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), FIRTREE_TYPE_KERNEL, FirtreeKernel))

#define FIRTREE_KERNEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), FIRTREE_TYPE_KERNEL, FirtreeKernelClass))

#define FIRTREE_IS_KERNEL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIRTREE_TYPE_KERNEL))

#define FIRTREE_IS_KERNEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), FIRTREE_TYPE_KERNEL))

#define FIRTREE_KERNEL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), FIRTREE_TYPE_KERNEL, FirtreeKernelClass))

/**
 * FirtreeKernelArgumentSpec:
 * @name_quark: The name of the argument.
 * @type: The type of the argument.
 * @is_static: Flag indicating if the argument is static.
 *
 * A structure which holds a record of a kernel argument.
 */
typedef struct {
    GQuark      name_quark;
    GType       type;
    gboolean    is_static;
} FirtreeKernelArgumentSpec;

/**
 * FirtreeKernel:
 * @parent: The parent GObject.
 *
 * A FirtreeKernel encapsulates a kernel compiled from source code in the
 * Firtree kernel language.
 */
typedef struct {
    GObject parent;
} FirtreeKernel;

typedef struct {
    GObjectClass parent_class;

    void (* argument_changed) (FirtreeKernel *kernel);
} FirtreeKernelClass;

GType firtree_kernel_get_type (void);

/**
 * firtree_kernel_new:
 *
 * Create a firtree kernel. Call firtree_kernel_compile_from_source() to
 * actually compile a kernel.
 *
 * Returns: A new FirtreeKernel instance.
 */
FirtreeKernel* firtree_kernel_new (void);

/**
 * firtree_kernel_compile_from_source:
 * @self: A FirtreeKernel instance.
 * @lines: An array of string containing the source lines. If @n_lines is
 * negative, this should be NULL terminated.
 * @n_lines: The number of source lines in @lines or negative if the array
 * @lines is NULL terminated.
 * @kernel_name: NULL or the name of a kernel function within the source.
 *
 * Attempts to compile the passed kernel. If compilation fails, FALSE is
 * returned. TRUE is returned if compilation succeeds. The 
 * firtree_kernel_get_compile_log() method may be used to retrieve any
 * error messages.
 *
 * If @kernel_name is non-NULL, it specifies the name of a kernel function
 * which should be used for this instance. If this kernel function is 
 * not defined in the source code passed, FALSE is returned. If @kernel_name
 * is NULL, the first kernel function defined is used. If no kernels are 
 * defined in the source, compilation is deemed unsuccessful.
 *
 * FIXME: Currently, failed compilation due to missing kernel functions is
 * not adequately reported.
 *
 * Note: The source lines are simply concatenated, no implicit newline
 * characters are inserted.
 *
 * Returns: a flag indicating whether the compilation was successful.
 */
gboolean
firtree_kernel_compile_from_source (FirtreeKernel* self, 
        gchar** lines, gint n_lines,
        gchar* kernel_name);

/**
 * firtree_kernel_get_compile_log:
 * @self: A FirtreeKernel instance.
 * @n_log_lines: If non-NULL, this is updated to contain the number of lines
 * in the log array.
 *
 * Retrieves the compilation log from the last call to 
 * firtree_kernel_compile_from_source(). If there is no such log, NULL
 * is returned.
 * 
 * Returns: an array of log lines terminated by NULL or NULL if there is
 * no log to return.
 */
gchar**
firtree_kernel_get_compile_log (FirtreeKernel* self, guint* n_log_lines);

/**
 * firtree_kernel_get_compile_status:
 * @self: A FirtreeKernel instance.
 *
 * Returns: The return value from the last call to 
 * firtree_kernel_compile_from_source() or FALSE if this method
 * has not yet been called.
 */
gboolean
firtree_kernel_get_compile_status (FirtreeKernel* self);

/**
 * firtree_kernel_list_arguments:
 * @self: A FirtreeKernel instance.
 * @n_arguments: NULL or a pointer to write the number of arguments.
 *
 * Obtain a pointer to a 0-terminated array of argument name quarks.
 *
 * Should there not be a compiled kernel, this method returns NULL.
 *
 * Returns: NULL or an array of GQuark-s.
 */
GQuark*
firtree_kernel_list_arguments (FirtreeKernel* self, guint* n_arguments);

/**
 * firtree_kernel_has_argument_named:
 * @self: A FirtreeKernel instance.
 * @arg_name: A string containing an argument name.
 *
 * Checks to see if ther kernel posesses an argument named @arg_name.
 * If no such argument is present, or the kernel is not compiled,
 * FALSE is returned. Otherwise, TRUE is returned.
 *
 * Returns: A boolean indicating the presence of an argument named 
 * @arg_name.
 */
gboolean
firtree_kernel_has_argument_named (FirtreeKernel* self, gchar* arg_name);

/**
 * firtree_kernel_get_argument_spec:
 * @self: A FirtreeKernel instance.
 * @arg_name: A quark corresponding to an argument name.
 *
 * Return a pointer to a FirtreeKernelArgumentSpec containing details
 * on the argument with name @arg_name. If no such argument exists,
 * return NULL.
 *
 * Returns: A pointer to a FirtreeKernelArgumentSpec structure describing
 * the argument.
 */
FirtreeKernelArgumentSpec*
firtree_kernel_get_argument_spec (FirtreeKernel* self, GQuark arg_name);

/**
 * firtree_kernel_get_argument_value:
 * @self: A FirtreeKernel instance.
 * @arg_name: A quark corresponding to an argument name.
 *
 * Retrieves the value of the argument referred to by @arg_name. If 
 * the kernel is not compiled, there is no argument named @arg_name, or
 * the argument is unset, NULL is returned.
 *
 * Returns: The value of the specified argument.
 */
GValue*
firtree_kernel_get_argument_value (FirtreeKernel* self, GQuark arg_name);

/**
 * firtree_kernel_set_argument_value:
 * @self: A FirtreeKernel instance.
 * @arg_name: A quark corresponding to an argument name.
 * @value: A GValue containing the new value of the argument.
 *
 * Sets the value of @arg_name to @value. It is an error for the type of
 * @value to be a mis-match with that of @arg_name. Use 
 * firtree_kernel_get_argument_spec() to query what type is expected.
 *
 * If NULL is passed in @value, the argument is unset.
 *
 * Returns: A flag indicating if the argument was set.
 */
gboolean
firtree_kernel_set_argument_value (FirtreeKernel* self,
        GQuark arg_name, GValue* value);

/**
 * firtree_kernel_argument_changed:
 * @self: A FirtreeKernel instance.
 * @arg_name: A quark corresponding to an argument name.
 *
 * Emit an ::argument-changed signal indicating that the value of
 * @arg_name has changed.
 */
void
firtree_kernel_argument_changed (FirtreeKernel* self, GQuark arg_name);

G_END_DECLS

#endif /* _FIRTREE_KERNEL */

/* vim:sw=4:ts=4:et:cindent
 */
