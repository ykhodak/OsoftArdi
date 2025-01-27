from stone.data_type import (
    is_alias,
    is_boolean_type,
    is_bytes_type,
    is_float_type,
    is_integer_type,
    is_list_type,
    is_nullable_type,
    is_numeric_type,
    is_string_type,
    is_struct_type,
    is_tag_ref,
    is_timestamp_type,
    is_union_type,
    is_user_defined_type,
    is_void_type,
    unwrap_aliases,
    unwrap_nullable,
)

from stone.target.python_helpers import (
    fmt_class,
    fmt_func,
    fmt_obj,
    fmt_var,
)


sroot = "dropbox"
dns = 'dropboxQt'
gen_destination = '../../dropboxQt/src/{}/'.format(sroot)
autotest_destination = '../../dropboxQt/src/{}/AUTOTEST/'.format(sroot)
time_format = '"yyyy-MM-ddThh:mm:ssZ"'
support_exceptions = True
support_autotest = False
containes_nested_routes = False
api_client_name = 'DropboxClient'
api_client_include_path = '#include "GoogleClient.h"'
api_endpoint_include_path = '#include "dropbox/endpoint/Endpoint.h"'
route_base_typename = 'DropboxRouteBase'
enable_async_CB = True

def generate_qt_common_include(self):
    common_include = '{}/endpoint/ApiUtil.h'.format(sroot)
    self.emit('#include "{}"'.format(common_include))

def generate_qt_route_base_include(self):
    common_include = 'DropboxRouteBase.h'
    self.emit('#include "dropbox/{}"'.format(common_include))

def generate_route_method_h(self, ns, r):
    arg_dt = r.arg_data_type
    result_dt = r.result_data_type
    error_dt = r.error_data_type
    sname = build_endpoint_entry_name(ns, r)
    io_device_arg = ''
    io_device_arg_typename = ''
    if sname == 'downloadStyle':
        io_device_arg = ', QIODevice* writeTo'
        io_device_arg_typename = 'QIODevice*'
    elif sname == 'uploadStyle':
        io_device_arg = ', QIODevice* readFrom'
        io_device_arg_typename = 'QIODevice*'
    arg_type_name = qt_type_mapping(ns, arg_dt)
    if is_void_type(arg_dt):
        arg_type_name = ''
    route_function_name = fmt_route(r.name)
    result_type_name = qt_type_mapping(ns, result_dt, True, False, True)
    if is_void_type(result_dt):
        result_type_name = ''
    async_completed_param_decl = build_async_completed_param(ns, r)
    async_failed_param_decl = 'std::function<void(std::unique_ptr<{}>)> failed_callback = nullptr'.format('DropboxException')
    self._generate_route_doc(r)
    if is_void_type(result_dt):
        if is_void_type(arg_dt):
            self.emit('void {}({});'.format(fmt_route(r.name), qt_type_mapping(ns, arg_dt)))
        else:
            self.emit('void {}(const {}& {});'.format(fmt_route(r.name), qt_type_mapping(ns, arg_dt), io_device_arg))
    else:
        if is_void_type(arg_dt):
            res_type_name = qt_type_mapping(ns, result_dt)
            self.emit('std::unique_ptr<{}> {}({});'.format(res_type_name, fmt_route(r.name), arg_type_name))
        else:
            if is_list_type(result_dt):
                res_type_name = qt_type_mapping(ns, result_dt, True, False, True)
                self.emit('std::unique_ptr<{}> {}(const {}& {});'.format(res_type_name, route_function_name, arg_type_name, io_device_arg))
            else:
                res_type_name = qt_type_mapping(ns, result_dt, True, False)
                self.emit('std::unique_ptr<{}> {}(const {}& {});'.format(res_type_name, route_function_name, arg_type_name, io_device_arg))
    generate_async_task_declaration(self, route_function_name, arg_dt,result_dt, arg_type_name, io_device_arg_typename, result_type_name)
    if enable_async_CB:
        generate_async_CB_declaration(self, route_function_name, arg_type_name, io_device_arg_typename, async_completed_param_decl, async_failed_param_decl)
    self.emit()

def build_async_completed_param(ns, r, forHdecl = True ): 
    defValue = '= nullptr'
    if not forHdecl:
        defValue = ''
    result_dt = r.result_data_type
    if is_void_type(result_dt):
        return 'std::function<void()> completed_callback {}'.format(defValue)
    else:
        res_type_name = qt_type_mapping(ns, result_dt, True, False)
        if is_list_type(result_dt):
            res_type_name = qt_type_mapping(ns, result_dt, True, False, True)
        return 'std::function<void(std::unique_ptr<{}>)> completed_callback {}'.format(res_type_name, defValue)



def generate_async_CB_declaration(self, route_function_name, arg_type_name, io_device_arg_typename, async_completed_param_decl, async_failed_param_decl):
    self.emit('void {}_AsyncCB('.format(route_function_name))
    with self.indent():
        if len(arg_type_name) > 0:
            self.emit('const {}&,'.format(arg_type_name))
        if len(io_device_arg_typename) > 0:
            self.emit('{} data,'.format(io_device_arg_typename))
        self.emit('{},'.format(async_completed_param_decl))
        self.emit('{});'.format(async_failed_param_decl))

def generate_async_task_declaration(self, route_function_name, arg_dt, result_dt, arg_type_name, io_device_arg_typename, result_type_name):
    arg = ''
    if len(arg_type_name) > 0:
        arg += 'const {}&'.format(arg_type_name)
    if len(io_device_arg_typename) > 0:
        arg += ', {} data'.format(io_device_arg_typename)

    fname = '';
    if is_void_type(result_dt):
        fname = 'DropboxVoidTask* {}_Async('.format(route_function_name)
    else:
        fname = 'DropboxTask<{}>* {}_Async('.format(result_type_name, route_function_name)

    self.emit('{}{});'.format(fname, arg));


def generate_route_method_cpp(self, ns, r):
    arg_dt = r.arg_data_type
    result_dt = r.result_data_type
    error_dt = r.error_data_type
    cname = '{}Routes'.format(fmt_class(ns.name))
    sname = 'rpcStyle'
    sname = build_endpoint_entry_name(ns, r)
    io_device_arg = ''
    sarg_list = 'arg'
    if sname == 'downloadStyle':
        io_device_arg = ', QIODevice* data'
        sarg_list = 'arg, writeTo'
    elif sname == 'uploadStyle':
        io_device_arg = ', QIODevice* data'
        sarg_list = 'arg, readFrom'
    ex_name = build_exception_load_class_name(r)
    rpath = build_route_request_string(ns, r)
    res_name = qt_type_mapping(ns, result_dt, True, False, True)
    arg_name = qt_type_mapping(ns, arg_dt)
    route_function_name = fmt_route(r.name)
    if is_void_type(result_dt):
        if is_void_type(arg_dt):
            self.emit('void {}::{}(){{'.format(cname, route_function_name))
            with self.indent():
                self.emit('{}_Async()->waitForResultAndRelease();'.format(route_function_name))
        else:
            self.emit('void {}::{}(const {}& arg {}){{'.format(cname, route_function_name, arg_name, io_device_arg))
            with self.indent():
                if is_list_type(result_dt):
                    self.emit('//skipped route: {}'.format(r.name))
                else:
                    if len(io_device_arg) > 0:
                        self.emit('{}_Async(arg, data)->waitForResultAndRelease();'.format(route_function_name))
                    else:
                        self.emit('{}_Async(arg)->waitForResultAndRelease();'.format(route_function_name))
        self.emit('}')
    else:
        if is_void_type(arg_dt):
            self.emit('std::unique_ptr<{}> {}::{}({}){{'.format(res_name, cname, route_function_name, arg_name))
            with self.indent():
                self.emit('return {}_Async()->waitForResultAndRelease();'.format(route_function_name))
        else:
            uptr_type = 'std::unique_ptr<{}>'.format(res_name)
            self.emit('{} {}::{}(const {}& arg {}){{'.format(uptr_type, cname, route_function_name, arg_name,  io_device_arg))
            with self.indent():                
                if len(io_device_arg) > 0:
                    self.emit('return {}_Async(arg, data)->waitForResultAndRelease();'.format(route_function_name))
                else:
                    self.emit('return {}_Async(arg)->waitForResultAndRelease();'.format(route_function_name))
        self.emit('}')
    self.emit()
    generate_route_async_task_method_cpp(self, ns, r)
    if enable_async_CB:
        generate_route_async_CB_method_cpp(self, ns, r)

def generate_route_async_CB_method_cpp(self, ns, r):
    arg_dt = r.arg_data_type
    result_dt = r.result_data_type
    error_dt = r.error_data_type
    cname = '{}Routes'.format(fmt_class(ns.name))
    sname = 'rpcStyle'
    sname = build_endpoint_entry_name(ns, r)
    io_device_arg_typename = ''
    sarg_list = 'arg'
    if sname == 'downloadStyle':
        io_device_arg_typename = 'QIODevice*'
    elif sname == 'uploadStyle':
       io_device_arg_typename = 'QIODevice*'
    ex_name = build_exception_load_class_name(r)
    rpath = build_route_request_string(ns, r)
    result_type_name = qt_type_mapping(ns, result_dt, True, False, True)
    if is_void_type(result_dt):
        result_type_name = ''
    arg_type_name = qt_type_mapping(ns, arg_dt)
    if is_void_type(arg_dt):
        arg_type_name = ''
    async_completed_param_decl = build_async_completed_param(ns, r, False)
    async_failed_param_decl = 'std::function<void(std::unique_ptr<{}>)> failed_callback'.format('DropboxException')
    self.emit('void {}::{}_AsyncCB('.format(cname, fmt_route(r.name)))
    with self.indent():
        if len(arg_type_name) > 0:
            self.emit('const {}& arg,'.format(arg_type_name))
        if len(io_device_arg_typename) > 0:
            self.emit('{} data,'.format(io_device_arg_typename))
        self.emit('{},'.format(async_completed_param_decl))
        self.emit('{})'.format(async_failed_param_decl))
    self.emit('{')
    with self.indent():
        self.emit('m_end_point->{}<'.format(sname))
        with self.indent():
            if len(arg_type_name) > 0:
                self.emit('{},'.format(arg_type_name))
            if len(result_type_name) > 0:
#                self.emit('std::unique_ptr<{}>,'.format(result_type_name))
                self.emit('{},'.format(result_type_name))
                if is_list_type(result_dt):
                    self.emit('ListFromJsonLoader<{}, {}>,'.format(result_type_name, result_dt.data_type.name))
                else:
                    self.emit('{}::factory,'.format(result_type_name))
            self.emit('{}>'.format(ex_name))
        with self.indent():
            self.emit('("{}",'.format(rpath))
            if len(arg_type_name) > 0:
                self.emit('arg,')
            if len(io_device_arg_typename) > 0:
                self.emit('data,')
            self.emit('completed_callback,')
            self.emit('failed_callback);')
    self.emit('}')
    self.emit()

###...
def generate_route_async_task_method_cpp(self, ns, r):
    arg_dt = r.arg_data_type
    result_dt = r.result_data_type
    error_dt = r.error_data_type
    cname = '{}Routes'.format(fmt_class(ns.name))
    sname = 'rpcStyle'
    sname = build_endpoint_entry_name(ns, r)
    io_device_arg_typename = ''
    sarg_list = 'arg'
    if sname == 'downloadStyle':
        io_device_arg_typename = 'QIODevice*'
    elif sname == 'uploadStyle':
       io_device_arg_typename = 'QIODevice*'
    ex_name = build_exception_load_class_name(r)
    rpath = build_route_request_string(ns, r)
    result_type_name = qt_type_mapping(ns, result_dt, True, False, True)
    if is_void_type(result_dt):
        result_type_name = ''
    arg_type_name = qt_type_mapping(ns, arg_dt)
    if is_void_type(arg_dt):
        arg_type_name = ''
    route_function_name = fmt_route(r.name)
    arg = ''
    if len(arg_type_name) > 0:
        arg += 'const {}& arg'.format(arg_type_name)
    if len(io_device_arg_typename) > 0:
        arg += ', {} data'.format(io_device_arg_typename)

    fname = '';
    if is_void_type(result_dt):
        fname = 'DropboxVoidTask* {}::{}_Async('.format(cname, route_function_name)
    else:
        fname = 'DropboxTask<{}>* {}::{}_Async('.format(result_type_name, cname, route_function_name)
    self.emit('{}{})'.format(fname, arg));
    self.emit('{')
    with self.indent():
        if is_void_type(result_dt):
            self.emit('DropboxVoidTask* t = m_end_point->produceVoidTask();')
        else:
            self.emit('DropboxTask<{}>* t = m_end_point->produceTask<{}>();'.format(result_type_name,result_type_name))
        self.emit('m_end_point->{}<'.format(sname))
        with self.indent():
            if len(arg_type_name) > 0:
                self.emit('{},'.format(arg_type_name))
            if len(result_type_name) > 0:
#                self.emit('std::unique_ptr<{}>,'.format(result_type_name))
                self.emit('{},'.format(result_type_name))
                if is_list_type(result_dt):
                    self.emit('ListFromJsonLoader<{}, {}>,'.format(result_type_name, result_dt.data_type.name))
                else:
                    self.emit('{}::factory,'.format(result_type_name))
            self.emit('{}>'.format(ex_name))
        with self.indent():
            self.emit('("{}",'.format(rpath))
            if len(arg_type_name) > 0:
                self.emit('arg,')
            if len(io_device_arg_typename) > 0:
                self.emit('data,')
            self.emit('t);')
        self.emit('return t;')
    self.emit('}')
    self.emit()
##...


def build_exception_load_class_name(r):
    ex_name = 'DropboxException'
    if not is_void_type(r.error_data_type):
        ex_name = '{}Exception'.format(r.error_data_type.name)
    return ex_name


def build_route_request_string(ns, r):
    return '{}/{}/{}'.format('2', ns.name, r.name)

def build_endpoint_entry_name(ns, r):
    ename = 'rpcStyle'
    s_attr = r.attrs['style']
    if s_attr:
        if s_attr == 'download':
            ename = 'downloadStyle'
        elif s_attr == 'upload':
            ename = 'uploadStyle'
    return ename
    

def fmt_route(name):
    s = fmt_class(name)
    if len(s) > 0:
        s = s[0].lower() + s[1:]
    if s == 'delete':
        s = 'deleteOperation'
    return s

def qt_type_mapping(ns, data_type, append_ns=True, force_ns=False, as_unique_ptr=False):
    """Map Stone data types to their most natural equivalent in Python
        for documentation purposes."""
    if is_string_type(data_type):
        return 'QString'
    elif is_bytes_type(data_type):
        return 'QByteArray'
    elif is_boolean_type(data_type):
        return 'bool'
    elif is_float_type(data_type):
        return 'float'
    elif is_integer_type(data_type):
        return 'int'
    elif is_void_type(data_type):
        return 'void'
    elif is_timestamp_type(data_type):
        return 'QDateTime'
    elif is_alias(data_type):
        return qt_type_mapping(ns, data_type.data_type, append_ns, force_ns, as_unique_ptr)
    elif is_user_defined_type(data_type):
        class_name = class_name_for_data_type(data_type)
        if force_ns or (append_ns and data_type.namespace.name != ns.name):
            return '%s::%s' % (data_type.namespace.name, class_name)
        else:
            return class_name
    elif is_list_type(data_type):
        if as_unique_ptr and is_user_defined_type(data_type.data_type) and qt_type_is_uptr_in_list(data_type.data_type):
            return 'std::list <std::unique_ptr<{}> >'.format(qt_type_mapping(
                ns, data_type.data_type, append_ns, force_ns, as_unique_ptr))
        else:
            return 'std::list <{}>'.format(qt_type_mapping(
                ns, data_type.data_type, append_ns, force_ns, as_unique_ptr))
    elif is_nullable_type(data_type):
        return '{}'.format(
                qt_type_mapping(ns, data_type.data_type, append_ns, force_ns, as_unique_ptr))
    else:
        raise TypeError('Unknown data type %r' % data_type)



def class_name_for_data_type(data_type, ns=None):
    """
    Returns the name of the Python class that maps to a user-defined type.
    The name is identical to the name in the spec.

    If ``ns`` is set to a Namespace and the namespace of `data_type` does
    not match, then a namespace prefix is added to the returned name.
    For example, ``foreign_ns.TypeName``.
    """
    assert is_user_defined_type(data_type) or is_alias(data_type), \
        'Expected composite type, got %r' % type(data_type)
    name = fmt_class(data_type.name)
    if ns and data_type.namespace != ns:
        # If from an imported namespace, add a namespace prefix.
        name = '{}::{}'.format(data_type.namespace.name, name)
    return name


def qt_type_is_uptr_in_list(dt):
    res = False
    if dt.name == 'Metadata':
        res = True
    return res
    
