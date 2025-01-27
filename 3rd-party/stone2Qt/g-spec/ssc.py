## messages ##

import os
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

def first_upper(s):
   if len(s) == 0:
      return s
   else:
      return s[0].upper() + s[1:]


sroot = os.getenv("STONE_MODULE_ROOT", "")
if len(sroot) < 1:
    raise Exception('Expected environment variable STONE_MODULE_ROOT')

dns = 'googleQt'
gen_destination =      '../../{}/src/{}/'.format(dns, sroot)
autotest_destination = '../../{}/src/{}/AUTOTEST/'.format(dns, sroot)
route_base_typename = 'GoogleRouteBase'

#time_format = '"yyyy-MM-ddThh:mm:ssZ"'
time_format = 'Qt::ISODate'
support_exceptions = False
support_autotest = True
containes_nested_routes = True
api_client_name = 'GoogleClient'
api_client_include_path = '#include "GoogleClient.h"'
api_endpoint_include_path = '#include "Endpoint.h"'
enable_async_CB = True


def generate_qt_common_include(self):
    common_include = 'google/endpoint/ApiUtil.h'
    request_arg = first_upper(sroot) + 'RequestArg'
    self.emit('#include "{}"'.format(common_include))
    self.emit('#include "{}/{}.h"'.format(sroot, request_arg))

def generate_qt_route_base_include(self):
    common_include = 'GoogleRouteBase.h'
    self.emit('#include "{}"'.format(common_include))
    
    
def generate_route_method_h(self, ns, r):
    arg_dt = r.arg_data_type
    result_dt = r.result_data_type
    error_dt = r.error_data_type
    sname = build_endpoint_entry_name(ns, r)
    io_device_arg = ''
    io_device_arg_typename = ''
    ex_arg = ''
    arg_body_typename = build_arg_body_class_name(r)
    if not is_void_type(r.error_data_type):
        ex_arg = 'const {}& body'.format(build_arg_body_class_name(r))
    if sname in set(['downloadStyle', 'downloadContactPhotoStyle']):
        io_device_arg = ', QIODevice* writeTo'
        io_device_arg_typename = 'QIODevice*'
    elif sname == 'mpartUploadStyle':
        io_device_arg = ', QIODevice* readFrom'
        io_device_arg_typename = 'QIODevice*'
    elif sname in set(['simpleUploadStyle']):
        io_device_arg = 'QIODevice* readFrom'
        io_device_arg_typename = 'QIODevice*'
    elif sname in set(['uploadContactPhotoStyle']):
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
    async_failed_param_decl = 'std::function<void(std::unique_ptr<{}>)> failed_callback = nullptr'.format('GoogleException')
    arg_decl = qt_type_mapping(ns, arg_dt)
    self._generate_route_doc(r)
    if is_void_type(result_dt):
        if is_void_type(arg_dt):
            self.emit('void {}({});'.format(fmt_route(r.name), qt_type_mapping(ns, arg_dt)))
        else:
            self.emit('void {}(const {}& {});'.format(fmt_route(r.name), qt_type_mapping(ns, arg_dt), io_device_arg))
    else:
        if is_void_type(arg_dt):
            if len(ex_arg) > 0:
                arg_decl = ex_arg
            else:
                arg_decl = ''
            self.emit('std::unique_ptr<{}> {}({}{});'.format(qt_type_mapping(ns, result_dt), fmt_route(r.name), arg_decl, io_device_arg))
        else:
            ex_decl = ''
            if len(ex_arg) > 0:
                ex_decl = ', ' + ex_arg
            if is_list_type(result_dt):
                self.emit('{} {}(const {}& {}{});'.format(qt_type_mapping(ns, result_dt, True, False, True), fmt_route(r.name), qt_type_mapping(ns, arg_dt), io_device_arg, ex_decl))
            else:
                self.emit('std::unique_ptr<{}> {}(const {}& arg{}{});'.format(qt_type_mapping(ns, result_dt, True, False), fmt_route(r.name), qt_type_mapping(ns, arg_dt), io_device_arg, ex_decl))
    generate_async_task_declaration(self, route_function_name, arg_dt,result_dt, arg_type_name, arg_body_typename, io_device_arg_typename, result_type_name)
    if enable_async_CB:
        generate_async_CB_declaration(self, route_function_name, arg_type_name, arg_body_typename, io_device_arg_typename, async_completed_param_decl, async_failed_param_decl)
    self.emit()

def generate_route_method_cpp(self, ns, r):
    arg_dt = r.arg_data_type
    result_dt = r.result_data_type
    error_dt = r.error_data_type
    cname = '{}Routes'.format(fmt_class(ns.name))
    sname = 'rpcStyle'
    sname = build_endpoint_entry_name(ns, r)
    io_device_arg = ''
#    sarg_list = 'arg'
    ex_arg_name = ''
    if sname in set(['downloadStyle', 'downloadContactPhotoStyle']):
        io_device_arg = ', QIODevice* data'
        sarg_list = 'arg, writeTo'
        ex_arg_name += ', writeTo'
    elif sname == 'mpartUploadStyle':
        io_device_arg = ', QIODevice* data'
        sarg_list = 'arg, readFrom'
        ex_arg_name += ', readFrom'
    elif sname in set(['simpleUploadStyle']):
        io_device_arg = 'QIODevice* data'
        sarg_list = 'readFrom'
        ex_arg_name += 'readFrom'
    elif sname in set(['uploadContactPhotoStyle']):
        io_device_arg = ', QIODevice* data'
        sarg_list = 'arg, readFrom'
        ex_arg_name += ', readFrom'

#    ex_name = 'NotAnException'
    arg_body_typename = build_arg_body_class_name(r)
    ex_decl = ''
    if not is_void_type(r.error_data_type):
        ex_decl = 'const {}& body'.format(build_arg_body_class_name(r))
        ex_arg_name += ', body'
#    if not is_void_type(r.error_data_type):
#        ex_name = build_arg_body_class_name(r)
    rpath = build_route_request_string(ns, r, arg_dt)
    res_name = qt_type_mapping(ns, result_dt, True, False, True)
    arg_name = qt_type_mapping(ns, arg_dt)
    route_function_name = fmt_route(r.name)    
    if is_void_type(result_dt):
        if is_void_type(arg_dt):
            self.emit('void {}::{}(){{'.format(cname, fmt_route(r.name)))
            with self.indent():
                self.emit('m_end_point->{}<std::unique_ptr<VoidType>, VoidType, {}>({} {});'.format(sname, arg_body_typename, rpath, ex_arg))
        else:
            self.emit('void {}::{}(const {}& arg {}){{'.format(cname, fmt_route(r.name), arg_name, io_device_arg))
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
            if len(arg_body_typename) > 0:
                self.emit('std::unique_ptr<{}> {}::{}(const {}& body{}){{'.format(res_name, cname, fmt_route(r.name), arg_body_typename, io_device_arg))
            else:
                if len(io_device_arg) > 0:
                    self.emit('std::unique_ptr<{}> {}::{}({}){{'.format(res_name, cname, fmt_route(r.name), io_device_arg))
                else:
                    self.emit('std::unique_ptr<{}> {}::{}({}){{'.format(res_name, cname, fmt_route(r.name), arg_name))
            with self.indent():
               if len(arg_body_typename) > 0:
                   if len(io_device_arg) > 0:
                      self.emit('return {}_Async(body, data)->waitForResultAndRelease();'.format(route_function_name))
                   else:
                      self.emit('return {}_Async(body)->waitForResultAndRelease();'.format(route_function_name))
               else:
                   if len(io_device_arg) > 0:
                       self.emit('return {}_Async(data)->waitForResultAndRelease();'.format(route_function_name))
                   else:
                       self.emit('return {}_Async()->waitForResultAndRelease();'.format(route_function_name))
        else:
            arg_decl = 'const {}& arg'.format(arg_name)
            if len(ex_decl) > 0:
                arg_decl += ', ' + ex_decl
            uptr_type = 'std::unique_ptr<{}>'.format(res_name)
            if is_list_type(result_dt):
                uptr_type = res_name;
            self.emit('{} {}::{}({}{}){{'.format(uptr_type, cname, fmt_route(r.name), arg_decl,  io_device_arg))
            with self.indent():                
                if len(io_device_arg) > 0:
                    self.emit('return {}_Async(arg, data)->waitForResultAndRelease();'.format(route_function_name))
#                    self.emit('DATA_GBC({}_AsyncCB, {}, arg, data);'.format(route_function_name, res_name))
                else:
                   if len(arg_body_typename) > 0:
                       self.emit('return {}_Async(arg, body)->waitForResultAndRelease();'.format(route_function_name))
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
    if sname in set(['downloadStyle', 'downloadContactPhotoStyle']):
        io_device_arg_typename = 'QIODevice*'
    elif sname in set(['mpartUploadStyle', 'simpleUploadStyle', 'uploadContactPhotoStyle']):
       io_device_arg_typename = 'QIODevice*'
    arg_body_typename = build_arg_body_class_name(r)
    body_template_comma = ','
    if is_void_type(r.error_data_type):
#       body_type_name = ''
       body_template_comma = ''
    rpath = build_route_request_string(ns, r, arg_dt)
    result_type_name = qt_type_mapping(ns, result_dt, True, False, True)
    if is_void_type(result_dt):
        result_type_name = ''
    arg_type_name = qt_type_mapping(ns, arg_dt)
    if is_void_type(arg_dt):
        arg_type_name = ''
    async_completed_param_decl = build_async_completed_param(ns, r, False)
    async_failed_param_decl = 'std::function<void(std::unique_ptr<{}>)> failed_callback'.format('GoogleException')
    self.emit('void {}::{}_AsyncCB('.format(cname, fmt_route(r.name)))
    with self.indent():
        if len(arg_type_name) > 0:
            self.emit('const {}& arg,'.format(arg_type_name))
        if len(arg_body_typename) > 0:
            self.emit('const {}& body,'.format(arg_body_typename))
        if len(io_device_arg_typename) > 0:
            self.emit('{} data,'.format(io_device_arg_typename))
        self.emit('{},'.format(async_completed_param_decl))
        self.emit('{})'.format(async_failed_param_decl))
    self.emit('{')
    is_template_call = False
    if len(result_type_name) > 0 or len(arg_body_typename) > 0:
       is_template_call = True
    if sname in ['deleteContactStyleB', 'deleteContactGroupStyleB', 'deleteContactPhotoStyleB']:
       is_template_call = True
#    print('sname= =' + sname + ' body_template_comma=' + body_template_comma + ' result_type_name=' +result_type_name)
    with self.indent():
        self.emit('m_end_point->{}'.format(sname))
        with self.indent():
            if is_template_call:
                self.emit('<'.format(sname))
            if len(result_type_name) > 0:
                self.emit('{},'.format(result_type_name))
                if is_list_type(result_dt):
                    self.emit('ListFromJsonLoader<{}, {}>,'.format(result_type_name, result_dt.data_type.name))
                else:
                    self.emit('{}::factory{}'.format(result_type_name, body_template_comma))
            if len(arg_body_typename) > 0:
               self.emit('{}'.format(arg_body_typename))
            if sname in ['updateStyle', 'mpartUploadStyle', 'postStyleB', 'rfc822UploadStyle', 'postStyleGmailB', 'postContactStyleB', 'putContactStyleB', 'postContactGroupStyleB', 'putContactGroupStyleB']:
               self.emit(', {}'.format(arg_type_name))
            elif sname in ['deleteContactStyleB', 'deleteContactGroupStyleB', 'deleteContactPhotoStyleB']:
               self.emit('{}'.format(arg_type_name))
            if is_template_call:
               self.emit('>'.format(sname))
#            self.emit('>')
        with self.indent():
            self.emit('({},'.format(rpath))
            if sname in ['updateStyle', 'mpartUploadStyle', 'postStyleB', 'rfc822UploadStyle', 'postStyleGmailB', 'postContactStyleB', 'deleteContactStyleB', 'putContactStyleB', 'postContactGroupStyleB', 'putContactGroupStyleB', 'deleteContactGroupStyleB', 'deleteContactPhotoStyleB', 'postStyleB2Empty']:
                self.emit('arg,')            
            if len(arg_body_typename) > 0:
               self.emit('body,')
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
    if sname in set([ 'downloadStyle', 'downloadContactPhotoStyle']):
        io_device_arg_typename = 'QIODevice*'
    elif sname in set(['mpartUploadStyle', 'simpleUploadStyle', 'uploadContactPhotoStyle']):
       io_device_arg_typename = 'QIODevice*'
#    ex_name = build_exception_load_class_name(r)
    arg_body_typename = build_arg_body_class_name(r)
    rpath = build_route_request_string(ns, r, arg_dt)
    result_type_name = qt_type_mapping(ns, result_dt, True, False, True)
    if is_void_type(result_dt):
        result_type_name = ''
    arg_type_name = qt_type_mapping(ns, arg_dt)
    if is_void_type(arg_dt):
        arg_type_name = ''
    route_function_name = fmt_route(r.name)
    arg = ''
    body_arg_comma = ','
    if len(arg_body_typename) == 0:
       body_arg_comma = ''
    if len(arg_type_name) > 0:
        arg += 'const {}& arg'.format(arg_type_name)
    if len(arg_body_typename) > 0:
        if len(arg_type_name) > 0:
            arg += ', '
        arg += 'const {}& body'.format(arg_body_typename)
    if len(io_device_arg_typename) > 0:
        if len(arg_type_name) > 0 or len(arg_body_typename) > 0:
            arg += ', '
        arg += '{} data'.format(io_device_arg_typename)

    fname = '';
    if is_void_type(result_dt):
        fname = 'GoogleVoidTask* {}::{}_Async('.format(cname, route_function_name)
    else:
        fname = 'GoogleTask<{}>* {}::{}_Async('.format(result_type_name, cname, route_function_name)

    has_template = (len(result_type_name) > 0) or (len(arg_body_typename) > 0)

    self.emit('{}{})'.format(fname, arg));
    self.emit('{')
    with self.indent():
        if is_void_type(result_dt):
            self.emit('GoogleVoidTask* t = m_end_point->produceVoidTask();')
        else:
            self.emit('GoogleTask<{}>* t = m_end_point->produceTask<{}>();'.format(result_type_name,result_type_name))
        if has_template:
           self.emit('m_end_point->{}<'.format(sname))
           with self.indent():
               if len(result_type_name) > 0:
                   self.emit('{},'.format(result_type_name))
                   if is_list_type(result_dt):
                       self.emit('ListFromJsonLoader<{}, {}>,'.format(result_type_name, result_dt.data_type.name))
                   else:
                       self.emit('{}::factory{}'.format(result_type_name, body_arg_comma))
               arg_body_ex = arg_body_typename
               if sname in ['updateStyle', 'mpartUploadStyle', 'postStyleB', 'rfc822UploadStyle', 'postStyleGmailB', 'postContactStyleB', 'deleteContactStyleB', 'putContactStyleB', 'postContactGroupStyleB', 'putContactGroupStyleB', 'deleteContactGroupStyleB', 'deleteContactPhotoStyleB', 'postStyleB2Empty']:
                   arg_body_ex = ',' + arg_type_name
               self.emit('{}>'.format(arg_body_ex))
        else:
            self.emit('m_end_point->{}'.format(sname))
        with self.indent():
            self.emit('({},'.format(rpath))
            if sname in ['updateStyle', 'mpartUploadStyle', 'postStyleB', 'rfc822UploadStyle', 'postStyleGmailB', 'postContactStyleB', 'deleteContactStyleB', 'putContactStyleB', 'postContactGroupStyleB', 'putContactGroupStyleB', 'deleteContactGroupStyleB', 'deleteContactPhotoStyleB', 'postStyleB2Empty']:
                self.emit('arg,')
            if len(arg_body_typename) > 0:
                self.emit('body,')
            if len(io_device_arg_typename) > 0:
                self.emit('data,')
            self.emit('t);')
        self.emit('return t;')
    self.emit('}')
    self.emit()
##...


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


def generate_async_CB_declaration(self, route_function_name, arg_type_name, arg_body_typename, io_device_arg_typename, async_completed_param_decl, async_failed_param_decl):
    self.emit('void {}_AsyncCB('.format(route_function_name))
    with self.indent():
        if len(arg_type_name) > 0:
            self.emit('const {}&,'.format(arg_type_name))
        if len(arg_body_typename) > 0:
            self.emit('const {}& body,'.format(arg_body_typename))
        if len(io_device_arg_typename) > 0:
            self.emit('{} data,'.format(io_device_arg_typename))
        self.emit('{},'.format(async_completed_param_decl))
        self.emit('{});'.format(async_failed_param_decl))
    

def generate_async_task_declaration(self, route_function_name, arg_dt, result_dt, arg_type_name, arg_body_typename, io_device_arg_typename, result_type_name):
    arg = ''
    if len(arg_type_name) > 0:
        arg += 'const {}& arg'.format(arg_type_name)
    if len(arg_body_typename) > 0:
        if len(arg_type_name) > 0:
            arg += ', '
        arg += 'const {}& body'.format(arg_body_typename)
    if len(io_device_arg_typename) > 0:
        if len(arg_type_name) > 0 or len(arg_body_typename) > 0:
            arg += ', '
        arg += '{} data'.format(io_device_arg_typename)

    fname = '';
    if is_void_type(result_dt):
        fname = 'GoogleVoidTask* {}_Async('.format(route_function_name)
    else:
        fname = 'GoogleTask<{}>* {}_Async('.format(result_type_name, route_function_name)

    self.emit('{}{});'.format(fname, arg));


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
        return 'quint64'
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
            return 'std::vector<std::unique_ptr<{}> >'.format(qt_type_mapping(
                ns, data_type.data_type, append_ns, force_ns, as_unique_ptr))
        else:
            return 'std::vector<{}>'.format(qt_type_mapping(
                ns, data_type.data_type, append_ns, force_ns, as_unique_ptr))
    elif is_nullable_type(data_type):
        return '{}'.format(
                qt_type_mapping(ns, data_type.data_type, append_ns, force_ns, as_unique_ptr))
    else:
        raise TypeError('Unknown data type %r' % data_type)

def build_exception_load_class_name(r):
    return build_arg_body_class_name(r)

def build_arg_body_class_name(r):
    ex_name = '{}'.format(r.error_data_type.name)
    if is_void_type(r.error_data_type):
       ex_name = ''
#       ex_name = 'VoidType'
    return ex_name

#def build_route_request_string(ns, r):
#   return '{}'.format(r.name)
#    return '{}/{}/{}'.format('2', ns.name, r.name)

def build_route_request_string(ns, r, arg_dt):
    arg_param = ', arg'
    if is_void_type(arg_dt):
       arg_param = ', VoidType::instance()'
    rpath = '{}'.format(ns.name)
    rlink = 'm_end_point->buildGmailUrl("{}"{})'.format(rpath, arg_param)
    s_attr = r.attrs['style']
    if s_attr:
       if s_attr == 'rfc822Upload':
          rlink = 'm_end_point->buildGmailUploadlUrl("{}"{})'.format(rpath, arg_param)
       elif s_attr in set(['getTaskList', 'postTaskList', 'putTaskList', 'deleteTaskList']):
          rlink = 'm_end_point->buildGtasklistUrl("{}"{})'.format(rpath, arg_param)
       elif s_attr in set(['getTask', 'postTask', 'putTask', 'deleteTask']):
          rlink = 'm_end_point->buildGtaskUrl("{}"{})'.format(rpath, arg_param)
       elif s_attr in set(['getDrive', 'postDrive', 'postDriveB', 'putDrive', 'deleteDrive', 'downloadDrive', 'updateDrive']):
          rlink = 'm_end_point->buildGdriveUrl("{}"{})'.format(rpath, arg_param)
       elif s_attr in set(['mpartUploadDrive']):
          rlink = 'm_end_point->buildGdriveMPartUploadUrl("{}"{})'.format(rpath, arg_param)
       elif s_attr in set(['simpleUploadDrive']):
          rlink = 'm_end_point->buildGdriveSimpleMediaUploadUrl("{}"{})'.format(rpath, arg_param)
       elif s_attr in set(['getAttachment']):
          rlink = 'm_end_point->buildGmailAttachmentUrl(arg)'
       elif s_attr in set(['getContact', 'postContactB', 'deleteContactB', 'putContactB']):
          rlink = 'm_end_point->buildContactUrl(arg)'
       elif s_attr in set(['postContactBatchB']):
          rlink = 'm_end_point->buildContactBatchUrl(arg)'
       elif s_attr in set(['postContactGroupBatchB']):
          rlink = 'm_end_point->buildContactGroupBatchUrl(arg)'
       elif s_attr in set(['getContactGroup', 'postContactGroupB', 'deleteContactGroupB', 'putContactGroupB']):
          rlink = 'm_end_point->buildContactGroupUrl(arg)'
       elif s_attr in set(['downloadContactPhoto', 'deleteContactPhotoB', 'uploadContactPhoto']):
          rlink = 'm_end_point->buildContactPhotoUrl(arg)'
    return rlink

def build_endpoint_entry_name(ns, r):
    ename = 'rpcStyle'
    s_attr = r.attrs['style']
    if s_attr:
        if s_attr == 'downloadDrive':
            ename = 'downloadStyle'
        elif s_attr == 'mpartUploadDrive':
            ename = 'mpartUploadStyle'
        elif s_attr == 'simpleUploadDrive':
            ename = 'simpleUploadStyle'
        if s_attr == 'downloadContactPhoto':
            ename = 'downloadContactPhotoStyle'
        if s_attr == 'uploadContactPhoto':
            ename = 'uploadContactPhotoStyle'
        elif s_attr in set(['get', 'getAttachment', 'getTaskList', 'getTask', 'getDrive']):
            ename = 'getStyle'
        elif s_attr in set(['getContact', 'getContactGroup']):
           ename = 'getContactStyle'
        elif s_attr in set(['delete', 'deleteTask', 'deleteTaskList', 'deleteDrive']):
            ename = 'deleteStyle'
        elif s_attr in set(['post', 'postTask', 'postTaskList', 'postDrive']):
            ename = 'postStyle'
        elif s_attr in set(['put', 'putTask', 'putTaskList', 'putDrive']):
            ename = 'putStyle'
        elif s_attr == 'updateDrive':
            ename = 'updateStyle'
        elif s_attr in set(['postDriveB', 'postGmailB']):
            ename = 'postStyleB'
        elif s_attr in set(['postGmailB2Empty']):
            ename = 'postStyleB2Empty'
        elif s_attr in set(['postContactB', 'postContactBatchB', 'postContactGroupBatchB']):
            ename = 'postContactStyleB'
        elif s_attr in set(['postContactGroupB']):
            ename = 'postContactGroupStyleB'
        elif s_attr in set(['deleteContactB']):
            ename = 'deleteContactStyleB'
        elif s_attr in set(['deleteContactGroupB']):
            ename = 'deleteContactGroupStyleB'
        elif s_attr in set(['deleteContactPhotoB']):
            ename = 'deleteContactPhotoStyleB'
        elif s_attr in set(['putContactB']):
            ename = 'putContactStyleB'
        elif s_attr in set(['putContactGroupB']):
            ename = 'putContactGroupStyleB'
#        elif s_attr == 'postGmailB':
#            ename = 'postStyleGmailB'
#        elif s_attr == 'simpleUpload':
#            ename = 'simpleUploadStyle'
        elif s_attr == 'rfc822Upload':
            ename = 'rfc822UploadStyle'
    return ename

def build_endpoint_link_builder(ns, r):
    ename = 'rpcStyle'
    s_attr = r.attrs['style']
#    if s_attr:


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


