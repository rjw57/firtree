// FIRTREE
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License version 2 for more details.
//
// You should have received a copy of the GNU General Public License version 2
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

%module Firtree
%{
#include <firtree/blob.h>
#include <firtree/image.h>
#include <firtree/main.h>
#include <firtree/math.h>
#include <firtree/kernel.h>
#include <firtree/glsl-runtime.h>

using namespace Firtree;

%}

%include <firtree/main.h>
%include <firtree/math.h>
%include <firtree/image.h>
%include <firtree/kernel.h>
%include <firtree/glsl-runtime.h>

#ifdef SWIGPYTHON

/* Convert from Python --> C */
%typemap(in) uint32_t {
    $1 = PyInt_AsLong($input);
}

/* Convert from C --> Python */
%typemap(out) uint32_t {
    $result = PyInt_FromLong($1);
}

%exception {
    try {
        $action
    } catch ( Firtree::Exception& e ) {
        std::string message = e.GetFunction();
        message += ": ";
        message += e.GetMessage();
        PyErr_SetString(PyExc_RuntimeError, const_cast<char*>(message.c_str()));
        return NULL;
    }
}
#endif

/* So that VIM does the "right thing"
 * vim:cindent:sw=4:ts=4:et
 */
