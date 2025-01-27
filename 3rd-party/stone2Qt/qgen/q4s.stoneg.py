#
# Dropbox Stone API specification converter for C++/Qt 
#
# Yuriy Khodak
# www.prokarpaty.net
#
import ssc
import datetime
import stone.data_type

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

from stone.generator import CodeGenerator

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

pretty_module = first_upper(ssc.sroot)
module_route_root = pretty_module + 'Routes'
autotest_class = pretty_module + 'Autotest'
endpoint_class4route = 'Endpoint'
api_module_root_include_path = '#include "{}/{}.h"'.format(ssc.sroot, module_route_root)
api_common_using_namespace = ssc.dns
print ('module: {} {} {} {}'.format(ssc.sroot, pretty_module, module_route_root, autotest_class))

class QtGenerator(CodeGenerator):
    def generate(self, api):
        if ssc.support_autotest:
            self._generate_autotest(api)
        for ns in api.namespaces.values():
            if ns.name != ssc.sroot:
                if self._use_namespace_in_code_generation(ns):
                    self._generate_h_files4namespace(ns)
                    self._generate_cpp_files4namespace(ns)


    def _generate_h_files4namespace(self, ns):
        for dt in ns.linearize_data_types():
            if is_struct_type(dt):
                with self.output_to_relative_path(ssc.gen_destination + h_file_name_for_data_type(ns, dt, False)):
                    self._generate_struct_h_file(ns, dt)
            if is_union_type(dt):
                with self.output_to_relative_path(ssc.gen_destination + h_file_name_for_data_type(ns, dt, False)):
                    self._generate_union_h_file(ns, dt)
        if ns.routes:
            self._generate_route_h_file(ns)
            self._generate_route_cpp_file(ns)

    def _generate_cpp_files4namespace(self, ns):
        for dt in ns.linearize_data_types():
            if is_struct_type(dt):
                cname = ssc.class_name_for_data_type(dt)
                with self.output_to_relative_path(ssc.gen_destination + cpp_file_name_for_data_type(ns, dt)):
                    self._generate_struct_cpp_file(ns, dt)
            if is_union_type(dt):
                with self.output_to_relative_path(ssc.gen_destination + cpp_file_name_for_data_type(ns, dt)):
                    self._generate_union_cpp_file(ns, dt)

    def _generate_struct_h_file(self, ns, dt):
        self._generate_header_comments(ns)
        self.emit('#pragma once')        
        self._generate_qt_h_includes()
        self._generate_class_h_dependcy(ns, dt)
        self.emit('namespace {}{{'.format(ssc.dns))
        self.emit('namespace {}{{'.format(ns.name))
        with self.indent():
            self._generate_struct_class_h(ns, dt)
        self.emit('}}//{}'.format(ns.name))
        self.emit('}}//{}'.format(ssc.dns))


    def _generate_union_h_file(self, ns, dt):
        self._generate_header_comments(ns)
        self.emit('#pragma once')
        self.emit()
        self._generate_qt_h_includes()
        self._generate_class_h_dependcy(ns, dt)
        self.emit('namespace {}{{'.format(ssc.dns))
        self.emit('namespace {}{{'.format(ns.name))
        with self.indent():
            self._generate_union_h(ns, dt)
        self.emit('}}//{}'.format(ns.name))
        self.emit('}}//{}'.format(ssc.dns))


    def _generate_route_h_file(self, ns):
        with self.output_to_relative_path(ssc.gen_destination + h_file_name_for_route(ns, False)):
            self._generate_header_comments(ns)
            self.emit('#pragma once')
            self.emit()
            self._generate_route_h_dependcy(ns)
            self.emit()
            self.emit('namespace {}{{'.format(ssc.dns))
            self.emit('namespace {}{{'.format(fmt_var(ns.name)))
            self.emit()
            self._generate_route_exceptions_h(ns)
            self.emit()
            with self.indent():
                cname = '{}Routes'.format(fmt_class(ns.name))
                self.emit('class {}: public {}{{'.format(cname, ssc.route_base_typename))
                self.emit('public:')
                with self.indent():
                    self.emit('{}({}* ep):{}(ep){{}};'.format(cname, endpoint_class4route, ssc.route_base_typename))
                    for r in ns.routes:
                        ssc.generate_route_method_h(self, ns, r)
                self.emit('protected:')
#                with self.indent():
#                    self.emit('{}* m_end_point;'.format(endpoint_class4route))
                self.emit('}};//{}'.format(cname))
            self.emit()
            self.emit('}}//{}'.format(ns.name))
            self.emit('}}//{}'.format(ssc.dns))

    def _generate_route_cpp_file(self, ns):
        cname = '{}Routes'.format(fmt_class(ns.name))
        with self.output_to_relative_path(ssc.gen_destination + cpp_file_name_for_route(ns)):
            self._generate_header_comments(ns)
            self.emit('#include "{}"'.format(h_file_name_for_route(ns)))
            self.emit(ssc.api_endpoint_include_path)
            if ssc.containes_nested_routes:
                self.emit('#include "{}/{}.h"'.format(ssc.sroot, module_route_root))
            self.emit('using namespace {};'.format(api_common_using_namespace))
            self.emit('using namespace {};'.format(ns.name))
            self.emit()
#            self.emit('namespace {}{{'.format(ssc.dns))
#            self.emit();
#            self.emit('namespace {}{{'.format(ns.name))
#            self.emit('{}::{}({}* p):m_end_point(p){{'.format(cname, cname, endpoint_class4route))
#            self.emit('}')
#            self.emit()
            for r in ns.routes:
                ssc.generate_route_method_cpp(self, ns, r)
            self._generate_route_exceptions_cpp(ns)
#            self.emit('}}//{}'.format(ns.name))
#            self.emit('}}//{}'.format(ssc.dns))

    def _generate_route_exceptions_h(self, ns):
        if not ssc.support_exceptions:
            return
        r_exceptions = set()
        for r in ns.routes:
            if not is_void_type(r.error_data_type):
                with self.indent():
                    ex_name = ssc.build_exception_load_class_name(r)
#                    ex_name = '{}Exception'.format(r.error_data_type.name)
                    if ex_name not in r_exceptions:
                        r_exceptions.add(ex_name)
                        self.emit('///exception {} for {}'.format(r.error_data_type.name, r.name))
                        self.emit('DECLARE_API_ERR_EXCEPTION({}, {}::{});'.format(ex_name, r.error_data_type.namespace.name, r.error_data_type.name))
                        self.emit()

    def _generate_route_exceptions_cpp(self, ns):
        if not ssc.support_exceptions:
            return
        r_exceptions = set()
        for r in ns.routes:
            if not is_void_type(r.error_data_type):                
                ex_name = ssc.build_exception_load_class_name(r)
#                ex_name = '{}Exception'.format(r.error_data_type.name)
                if ex_name not in r_exceptions:
                    r_exceptions.add(ex_name)
                    self.emit('IMPLEMENT_API_ERR_EXCEPTION({}, {}::{});'.format(ex_name, r.error_data_type.namespace.name, r.error_data_type.name))

    def _generate_struct_cpp_file(self, ns, dt):
        self._generate_header_comments(ns)
        cname = ssc.class_name_for_data_type(dt)
        self.emit('#include "{}"'.format(h_file_name_for_data_type(ns, dt)))
        if dt.has_enumerated_subtypes():
            for tags, subtype in dt.get_all_subtypes_with_tags():
                self.emit('#include "{}/{}/{}{}.h"'.format(ssc.sroot, ns.name, fmt_class(ns.name), fmt_class(subtype.name)))        
        self.emit('using namespace {};'.format(api_common_using_namespace))
        self.emit()
        self.emit('namespace {}{{'.format(ssc.dns))
        self.emit();
        self.emit('namespace {}{{'.format(ns.name))
        self.emit('///{}'.format(cname))
        self._generate_struct_json_converters_cpp(ns, dt)
        self.emit('}}//{}'.format(ns.name))
        self.emit('}}//{}'.format(ssc.dns))


    def _generate_union_cpp_file(self, ns, dt):
        self._generate_header_comments(ns)
        cname = ssc.class_name_for_data_type(dt)
        self.emit('#include "{}"'.format(h_file_name_for_data_type(ns, dt)))
        self.emit()
        self.emit('namespace {}{{'.format(ssc.dns))
        self.emit();    
        self.emit('namespace {}{{'.format(ns.name))
        self.emit('///{}'.format(cname))
        self._generate_union_json_converters_cpp(ns, dt)
        self.emit('}}//{}'.format(ns.name))
        self.emit('}}//{}'.format(ssc.dns))

    def _generate_struct_class_h(self, ns, dt):
        """Defines a C++ class that represents a struct in Stone."""
        self.emit(self._struct_class_declaration(ns, dt))
        self._generate_class_doc(dt)
        self._generate_struct_constructors(ns, dt)
        self._generate_struct_getters(ns, dt)
        self._generate_json_struct_converters_h(ns, dt)
        self._generate_class_members(ns, dt);
        cname = ssc.class_name_for_data_type(dt)
        self.emit('}};//{}'.format(cname))        
        self.emit()


    def _generate_union_h(self, ns, dt):
        """Defines a C++ class that represents a union in Stone."""
        cname = ssc.class_name_for_data_type(dt)
        self.emit(self._union_declaration(ns, dt))
        self._generate_class_doc(dt)
        self._generate_union_tag_enum_h(ns, dt)
        self._generate_union_getters(ns, dt)
        self._generate_json_union_converters_h(ns, dt)
        self._generate_union_members(ns, dt)
        self.emit('}};//{}'.format(cname))        
        self.emit()
        

    def _generate_union_tag_enum_h(self, ns, dt):
        cname = ssc.class_name_for_data_type(dt)
        self.emit('public:')
        with self.indent():
            self.emit('enum Tag{')
            self._generate_enum_values(ns, dt)
            self.emit('};')
            self.emit()
            self.emit('{}(){{}}'.format(cname))
            self.emit('{}(Tag v):m_tag(v){{}}'.format(cname))
            self.emit('virtual ~{}(){{}}'.format(cname))
        
    def _generate_union_members(self, ns, dt):
        self.emit()
        self.emit('protected:')
        with self.indent():
            self.emit('Tag m_tag;')
            self._generate_union_members_details(ns, dt)

    def _generate_union_members_details(self, ns, dt):
        if dt.parent_type:
            self._generate_union_members_details(ns, dt.parent_type)
        for f in dt.fields:
            if not is_void_type(f.data_type):
                field_def = ssc.qt_type_mapping(ns, f.data_type) + ' {};'.format(f2name(f))
                self.emit(field_def)


    def _generate_union_getters(self, ns, dt):
        self.emit()
        with self.indent():
            self.emit('Tag tag()const{return m_tag;}')
            self._generate_union_getters_details(ns, dt)

    def _generate_union_getters_details(self, ns, dt):
        if dt.parent_type:
            self._generate_union_getters_details(ns, dt.parent_type)
        for f in dt.fields:
            if not is_void_type(f.data_type):
                self.emit('///{}'.format(f.doc))
                field_type = member_getter_type_for_field(ns, f)
                fname = f2name(f);
                field_mem_name = name_for_member(f)
                etype = encode_enum_value(dt, f)
                func_def = field_type+' get{}()const{{API_CHECK_STATE(({} == m_tag), "expected tag: {}", m_tag);return {};}};'.format(first_upper(field_mem_name), etype, etype, fname)
                self.emit(func_def)
                self.emit()


    def _generate_json_struct_converters_h(self, ns, dt):
        cname = ssc.class_name_for_data_type(dt)
        self.emit('public:')
        with self.indent():
            self.emit('operator QJsonObject ()const;')
            self.emit('virtual void fromJson(const QJsonObject& js);')
            self.emit('virtual void toJson(QJsonObject& js)const;')
            self.emit('virtual QString toString(bool multiline = true)const;')
            self.emit()
            self.emit()
            self.emit('class factory{')
            self.emit('public:')
            with self.indent():
                self.emit('static std::unique_ptr<{}>  create(const QByteArray& data);'.format(cname))
                self.emit('static std::unique_ptr<{}>  create(const QJsonObject& js);'.format(cname))
            self.emit('};')
            self.emit()
            if ssc.support_autotest:
                self.emit()
                self.emit('#ifdef API_QT_AUTOTEST')
                self.emit('static std::unique_ptr<{}> EXAMPLE(int context_index, int parent_context_index);'.format(cname))
                self.emit('#endif //API_QT_AUTOTEST')
                
        self.emit()

    def _generate_json_union_converters_h(self, ns, dt):
        cname = ssc.class_name_for_data_type(dt)
        self.emit('public:')
        with self.indent():
            self.emit('operator QJsonObject ()const;')
            self.emit('virtual void fromJson(const QJsonObject& js);')            
            self.emit('virtual void toJson(QJsonObject& js, QString name)const;')
            self.emit('virtual QString toString(bool multiline = true)const;')
            self.emit()
            self.emit()
            self.emit('class factory{')
            self.emit('public:')
            with self.indent():
                self.emit('static std::unique_ptr<{}>  create(const QByteArray& data);'.format(cname))
                self.emit('static std::unique_ptr<{}>  create(const QJsonObject& js);'.format(cname))
            self.emit('};')
            self.emit()            
            if ssc.support_autotest:
                self.emit()
                self.emit('#ifdef API_QT_AUTOTEST')
                self.emit('static std::unique_ptr<{}> EXAMPLE(int context_index, int parent_context_index);'.format(cname))
                self.emit('#endif //API_QT_AUTOTEST')
        self.emit()
        

    def _generate_enum_serializer_h(self, ns, dt):
        ename = enum_name_for_data_type(dt)
        sname = serializer_name(ename)
        self.emit('class {}{{'.format(sname))
        self.emit('public:')
        with self.indent():
            self.emit('static void load(const QJsonObject&, {}&);'.format(ename))
            self.emit('static void store(QJsonObject&, const {}&);'.format(ename))
        self.emit('}};//{}'.format(sname))
        self.emit()


    def _struct_class_declaration(self, ns, data_type):
        assert is_user_defined_type(data_type), \
            'Expected struct, got %r' % type(data_type)
        if data_type.parent_type:
            extends = ssc.class_name_for_data_type(data_type.parent_type, ns)
        else:
            extends = ''
        cname = ssc.class_name_for_data_type(data_type)
        if not extends:
            return 'class {}{{'.format(cname)
        return 'class {} : public {}{{'.format(cname, extends)
    
    def _union_declaration(self, ns, data_type):
        """ no derivation for unions """
        assert is_user_defined_type(data_type), \
            'Expected struct, got %r' % type(data_type)
        cname = ssc.class_name_for_data_type(data_type)
        return 'class {}{{'.format(cname)

    def _generate_class_doc(self, dt):
        with self.indent():
            if dt.has_documented_type_or_fields():
                self.emit('/**')
                with self.indent():
                    if dt.doc:
                        self.emit_wrapped_text(
                            self.process_doc(dt.doc, self._docf))
                        if dt.has_documented_fields():
                            self.emit()
                    for field in dt.fields:
                        if not field.doc:
                            continue
                        self.emit_wrapped_text('field: {}: {}'.format(
                            fmt_var(field.name),
                            self.process_doc(field.doc, self._docf)),
                                               subsequent_prefix='    ')
                self.emit('*/')        

    def _generate_class_field_doc(self, f):
        with self.indent():
            if f.doc:
                self.emit('/**')
                with self.indent():
                    self.emit_wrapped_text(
                        self.process_doc(f.doc, self._docf))
                self.emit('*/')        

    
    def _generate_route_doc(self, r):
        with self.indent():
            if r.doc:
                self.emit('/**')
                self.emit('{}'.format(r))
                self.emit()
                self.emit()
                self.emit_wrapped_text(self.process_doc(r.doc, self._docf))
                ex_name = ssc.build_exception_load_class_name(r)
#                ex_name = '{}Exception'.format(r.error_data_type.name)
                self.emit()                
                if ssc.support_exceptions and not is_void_type(r.error_data_type):
                    self.emit('on error:{} throws exception {}'.format(r.error_data_type.name, ex_name))
                self.emit('*/')

    def _generate_namespace_doc(self, ns):
        with self.indent():
            if ns.doc:
                self.emit('/**')
                self.emit_wrapped_text(
                    self.process_doc(ns.doc, self._docf))
                self.emit('*/')            

    def _generate_class_members(self, ns, dt):
        self.emit()
        self.emit('protected:')
        for f in dt.fields:
            with self.indent():
                self._generate_class_field_doc(f)
                init_val = ';';
                if is_integer_type(f.data_type):
                    init_val = ' = {0};';
                field_def = ssc.qt_type_mapping(ns, f.data_type, True, False, True) + ' {}{}'.format(f2name(f), init_val)
                self.emit(field_def)        
                self.emit()

    def _generate_struct_constructors(self, ns, dt):
        cname = ssc.class_name_for_data_type(dt)
        self.emit()
        self.emit('public:')            
        has_default_fields = False
        for f in dt.fields:
            if f.has_default:
                has_default_fields = True
                break
        with self.indent():
            if has_default_fields:
                self.emit('{}():'.format(cname))
                first_in_list = True
                for f in dt.fields:
                    if f.has_default:
                        comma_prefix = ','
                        if first_in_list:
                            comma_prefix = ''
                        self.emit('{}{}({})'.format(comma_prefix, f2name(f), qt_default_value(ns, dt, f)))
                        first_in_list = False
                self.emit('{};')
            else:
                self.emit('{}(){{}};'.format(cname))
            self.emit()
            if dt.fields:
                fcount = len(dt.fields)
                if fcount > 0:
                    f = dt.fields[0]
                    field_type = member_setter_type_for_field(ns, f)
                    if not is_void_type(f):
                        if is_list_type(f.data_type) and ssc.qt_type_is_uptr_in_list(f.data_type.data_type):
                            return                        
                        pfix = 'const '                        
                        if field_type.find('const') == 0:
                            pfix = ''
                        fname = f2name(f);
                        if has_default_fields:
                            self.emit('{}({}{}& arg):'.format(cname, pfix, field_type))
                            first_in_list = True
                            for f in dt.fields:
                                if f.has_default:
                                    comma_prefix = ','
                                    if first_in_list:
                                        comma_prefix = ''
                                    self.emit('{}{}({})'.format(comma_prefix, f2name(f), qt_default_value(ns, dt, f)))
                                    first_in_list = False
                            self.emit('{{ {} = arg; }};'.format(fname))
                        else:
                            self.emit('{}({}{}& arg){{ {} = arg; }};'.format(cname, pfix, field_type, fname))
            self.emit('virtual ~{}(){{}};'.format(cname))

    def _generate_struct_getters(self, ns, dt):
        self.emit()
        self.emit('public:')
        for f in dt.fields:
            with self.indent():
                field_type = member_getter_type_for_field(ns, f)
                self._generate_class_field_doc(f)
                fname = f2name(f)
                field_mem_name = name_for_member(f)
                get_func = field_type+' {}()const{{return {};}};'.format(field_mem_name, fname)
                self.emit(get_func) 
                if True:
                    cname = ssc.class_name_for_data_type(dt)
                    if 'Result' not in cname:
                        set_field_type = member_setter_type_for_field(ns, f)
                        pfix = 'const '
                        if set_field_type.find('const') == 0:
                            pfix = ''
                        set_func = '{}& set{}({}{}& arg){{{}=arg;return *this;}};'.format(cname, field_mem_name.title(), pfix, set_field_type, fname)
                        self.emit(set_func)
                self.emit()

    def _generate_class_h_dependcy(self, ns, dt):
        incl_set = set()
        if dt.parent_type:
            if is_user_defined_type(dt.parent_type) or is_alias(dt.parent_type):
                incl = class_include_line_for_data_type(dt.parent_type)
                if incl not in incl_set and len(incl) > 0:
                    incl_set.add(incl)
                    self.emit(incl)
        for f in dt.fields:
            if dt.namespace.name != ssc.sroot:
                incl = get_field_h_dependancy(ns, f)
                if incl not in incl_set and len(incl) > 0:
                    incl_set.add(incl)
                    self.emit(incl)
        self.emit()            
    
    def _generate_route_h_dependcy(self, ns):
#        self.emit('#include "{}/{}.h"'.format(ssc.sroot, route_base_typename))
        self._generate_qt_h_includes()
        ssc.generate_qt_route_base_include(self)
        namespace_data_types = sorted(ns.get_route_io_data_types(),key=lambda dt: dt.name)
        for dt in namespace_data_types:
            if self._use_namespace_in_code_generation(dt.namespace):
                self.emit(class_include_line_for_data_type(dt))

    def _get_enum_values(self, ns, dt):
        s = ''
        if dt.parent_type:
            s = self._get_enum_values(ns, dt.parent_type)
            s += ','
        for f in dt.fields:
            with self.indent():
                s += '\n'
                s += '        /*{}*/'.format(f.doc)
                s += '\n        '
                s += encode_enum_value(dt, f)
                s += ','
        s = s[:-1]
        return s

    def _generate_enum_values(self, ns, dt):
        s = ''
        s += self._get_enum_values(ns, dt)
        s += '\n'
        self.emit_raw(s)

    def _generate_qt_h_includes(self):
        ssc.generate_qt_common_include(self)

    def _generate_struct_json_operator_cpp(self, ns, dt):
        self.emit()
        cname = ssc.class_name_for_data_type(dt)
        self.emit('{}::operator QJsonObject()const{{'.format(cname))
        with self.indent():
            self.emit('QJsonObject js;')
            self.emit('this->toJson(js);')
            self.emit('return js;')
        self.emit('}')
        self.emit()

    def _generate_union_json_operator_cpp(self, ns, dt):
        self.emit()
        cname = ssc.class_name_for_data_type(dt)
        self.emit('{}::operator QJsonObject()const{{'.format(cname))
        with self.indent():
            self.emit('QJsonObject js;')
            self.emit('this->toJson(js, "");')
            self.emit('return js;')
        self.emit('}')
        self.emit()
        
        
    def _generate_struct_json_converters_cpp(self, ns, dt):
        cname = ssc.class_name_for_data_type(dt)
        self._generate_struct_json_operator_cpp(ns, dt)
        self.emit()
        self.emit('void {}::toJson(QJsonObject& js)const{{'.format(cname))
        acceptEmptyString = False
        if(cname == 'ListFolderArg'):
            acceptEmptyString = True         
        self.emit()
        with self.indent():
            if dt.parent_type:
                pname = ssc.class_name_for_data_type(dt.parent_type)
                self.emit('{}::toJson(js);'.format(pname))
            for f in dt.fields:
                m_n = f2name(f)
                self._generate_qt_field_json_encoder(f.data_type, f.name, m_n, acceptEmptyString)
        self.emit('}')
        self.emit()
        self.emit('void {}::fromJson(const QJsonObject& js){{'.format(cname))
        self.emit()
        with self.indent():
            if dt.parent_type:
                pname = ssc.class_name_for_data_type(dt.parent_type)
                self.emit('{}::fromJson(js);'.format(pname))
            for f in dt.fields:
                m_n = f2name(f)
                self._generate_qt_field_json_decoder(ns, f.data_type, f.name, m_n)
        self.emit('}')
        self.emit()
        self.emit('QString {}::toString(bool multiline)const'.format(cname))
        self.emit('{')
        with self.indent():
            self.emit('QJsonObject js;')
            self.emit('toJson(js);')
            self.emit('QJsonDocument doc(js);')
            self.emit('QString s(doc.toJson(multiline ? QJsonDocument::Indented : QJsonDocument::Compact));')
            self.emit('return s;')
        self.emit('}')
        self.emit()
        self.emit()
        self.emit('std::unique_ptr<{}>  {}::factory::create(const QByteArray& data)'.format(cname, cname))
        self.emit('{')        
        with self.indent():            
            self.emit('QJsonDocument doc = QJsonDocument::fromJson(data);')
            self.emit('QJsonObject js = doc.object();')
            self.emit('return create(js);')
        self.emit('}')        
        self.emit()        
        self.emit()
        self.emit('std::unique_ptr<{}>  {}::factory::create(const QJsonObject& js)'.format(cname, cname))
        self.emit('{')
        with self.indent():
            self.emit('std::unique_ptr<{}> rv;'.format(cname))            
            if dt.has_enumerated_subtypes():
                for tags, subtype in dt.get_all_subtypes_with_tags():
                    self.emit("// subtype {} : {}".format(
                        tags,
                        subtype.name))                
                self.emit('QString tag = js[".tag"].toString();')
                self.emit('if(tag.isEmpty()){')
                with self.indent():
                    self.emit('rv = std::unique_ptr<{}>(new {});'.format(cname, cname))
                for tags, subtype in dt.get_all_subtypes_with_tags():
                    s = str(tags)
                    subtag = s.strip(',(\')')
                    self.emit('}}else if(tag.compare("{}") == 0){{'.format(subtag))
                    with self.indent():
                        self.emit('rv = std::unique_ptr<{}>(new {});'.format(cname, subtype.name))
                self.emit('}')
            else:
                self.emit('rv = std::unique_ptr<{}>(new {});'.format(cname, cname))
            self.emit('rv->fromJson(js);')
            self.emit('return rv;')
        self.emit('}')        
        self.emit()
        if ssc.support_autotest:
            self._generate_struct_example(ns, dt)
            
    def _generate_union_json_converters_cpp(self, ns, dt):
        self._generate_union_json_operator_cpp(ns, dt)
        self.emit()
        cname = ssc.class_name_for_data_type(dt)
        self.emit('void {}::toJson(QJsonObject& js, QString name)const{{'.format(cname))
        self.emit()
        with self.indent():
            self.emit('switch(m_tag){')
            with self.indent():
                self._generate_union_json_converters_out_switch_cases(ns, dt)
            self.emit('}//switch')
        self.emit('}')
        self.emit()
        self.emit('void {}::fromJson(const QJsonObject& js){{'.format(cname))
        self.emit()
        with self.indent():
            self.emit('QString s = js[".tag"].toString();')
            self._generate_union_json_converters_in_switch_cases(ns, dt)
        self.emit('}')
        self.emit()
        self.emit('QString {}::toString(bool multiline)const'.format(cname))
        self.emit('{')
        with self.indent():
            self.emit('QJsonObject js;')
            self.emit('toJson(js, "{}");'.format(cname))
            self.emit('QJsonDocument doc(js);')
            self.emit('QString s(doc.toJson(multiline ? QJsonDocument::Indented : QJsonDocument::Compact));')
            self.emit('return s;')
        self.emit('}')
        self.emit()
        self.emit('std::unique_ptr<{}>  {}::factory::create(const QByteArray& data)'.format(cname, cname))
        self.emit('{')
        with self.indent():
            self.emit('QJsonDocument doc = QJsonDocument::fromJson(data);')
            self.emit('QJsonObject js = doc.object();')
            self.emit('std::unique_ptr<{}> rv = std::unique_ptr<{}>(new {});'.format(cname, cname, cname))
            self.emit('rv->fromJson(js);')
            self.emit('return rv;')
        self.emit('}')
        self.emit()
        if ssc.support_autotest:
            self._generate_struct_example(ns, dt)


    def _generate_union_json_converters_out_switch_cases(self, ns, dt):
        if dt.parent_type:
            self._generate_union_json_converters_out_switch_cases(ns, dt.parent_type)
        for f in dt.fields:
            m_n = f2name(f)
            ename = encode_enum_value(dt, f)
            self.emit('case {}:{{'.format(ename))
            with self.indent():
                self.emit('if(!name.isEmpty())')
                with self.indent():
                    self.emit('js[name] = QString("{}");'.format(f.name))
                self._generate_qt_field_json_encoder(f.data_type, f.name, m_n)
            self.emit('}break;')

    def _generate_union_json_converters_in_switch_cases(self, ns, dt):
        if dt.parent_type:
            self._generate_union_json_converters_in_switch_cases(ns, dt.parent_type)        
        first_if = True
        for f in dt.fields:
            m_n = f2name(f)
            ename = encode_enum_value(dt, f)
            ename = encode_enum_value(dt, f)
            if first_if:
                self.emit('if(s.compare("{}") == 0){{'.format(f.name))
                first_if = False
            else:
                self.emit('else if(s.compare("{}") == 0){{'.format(f.name))
            with self.indent():
                self.emit('m_tag = {};'.format(ename))
                self._generate_qt_field_json_decoder(ns, f.data_type, f.name, m_n)
            self.emit('}')
            
    def _union_has_serializable_data(self, ns, dt):
        if dt.parent_type:
            if self._union_has_serializable_data(ns, dt.parent_type):
                return True
        for f in dt.fields:
            if not is_void_type(f.data_type):
                return True
        return False
                

    def _generate_qt_field_json_encoder(self, f, n, m_n, acceptEmptyString = False):
        if is_nullable_type(f) or is_alias(f):
            return self._generate_qt_field_json_encoder(f.data_type, n, m_n)
        elif is_list_type(f):
            if is_user_defined_type(f.data_type):
                if ssc.qt_type_is_uptr_in_list(f.data_type):
                    self.emit('js["{}"] = struct_list2jsonarray_uptr({});'.format(n, m_n))
                else:
                    self.emit('js["{}"] = struct_list2jsonarray({});'.format(n, m_n))
            else:
                if is_list_type(f.data_type):
                    self.emit('js["{}"] = list_of_struct_list2jsonarray({});'.format(n, m_n))
                else:
                    self.emit('js["{}"] = ingrl_list2jsonarray({});'.format(n, m_n))
        elif is_timestamp_type(f):
            self.emit('if({}.isValid())'.format(m_n))
            with self.indent():
                self.emit('js["{}"] = {}.toString({});'.format(n, m_n, ssc.time_format))
        elif is_union_type(f):
            self.emit('{}.toJson(js, "{}");'.format(m_n, n))
        elif is_struct_type(f):
            self.emit('js["{}"] = (QJsonObject){};'.format(n, m_n))
        elif is_string_type(f):
            if acceptEmptyString:
                self.emit('js["{}"] = QString({});'.format(n, m_n))
            else:
                self.emit('if(!{}.isEmpty())'.format(m_n))
                with self.indent():
                    self.emit('js["{}"] = QString({});'.format(n, m_n))
        elif is_integer_type(f):
            self.emit('js["{}"] = QString("%1").arg({});'.format(n, m_n))
        elif is_float_type(f) or is_boolean_type(f):
            self.emit('js["{}"] = {};'.format(n, m_n))
        elif is_bytes_type(f):
            self.emit('js["{}"] = {}.constData();'.format(n, m_n))

    def _generate_qt_field_json_decoder(self, ns, f, n, m_n):
        if is_nullable_type(f) or is_alias(f):
            return self._generate_qt_field_json_decoder(ns, f.data_type, n, m_n)
        elif is_list_type(f):
            if is_user_defined_type(f.data_type):
                if ssc.qt_type_is_uptr_in_list(f.data_type):
                    self.emit('jsonarray2struct_list_uptr(js["{}"].toArray(), {});'.format(n, m_n))                    
                else:
                    self.emit('jsonarray2struct_list(js["{}"].toArray(), {});'.format(n, m_n))
            else:
                if is_list_type(f.data_type):
                    self.emit('jsonarray2list_of_struct_list(js["{}"].toArray(), {});'.format(n, m_n))
                else:
                    self.emit('jsonarray2ingrl_list(js["{}"].toArray(), {});'.format(n, m_n))
        elif is_timestamp_type(f):
            self.emit('{} = QDateTime::fromString(js["{}"].toString(), {});'.format(m_n, n, ssc.time_format))
        elif is_struct_type(f):
            self.emit('{}.fromJson(js["{}"].toObject());'.format(m_n, n))
        elif is_union_type(f):
            self.emit('{}.fromJson(js["{}"].toObject());'.format(m_n, n))
        elif is_integer_type(f):
            self.emit('{} = js["{}"].toVariant().toString().toULongLong();'.format(m_n, n))
        elif is_boolean_type(f):
            self.emit('{} = js["{}"].toVariant().toBool();'.format(m_n, n))
        elif is_float_type(f):
            self.emit('{} = js["{}"].toVariant().toFloat();'.format(m_n, n))
        elif is_void_type(f):
            self.emit()
        elif is_bytes_type(f):
            self.emit('{} = js["{}"].toVariant().toByteArray();'.format(m_n, n))
        else:
            self.emit('{} = js["{}"].toString();'.format(m_n, n))



    def _docf(self, tag, val):
        """
        Callback used as the handler argument to process_docs(). This converts
        Babel doc references to Sphinx-friendly annotations.
        """
        if tag == 'type':
            return ':class:`{}`'.format(val)
        elif tag == 'route':
            return ':meth:`{}`'.format(fmt_func(val))
        elif tag == 'link':
            anchor, link = val.rsplit(' ', 1)
            return '`{} <{}>`_'.format(anchor, link)
        elif tag == 'val':
            if val == 'null':
                return 'None'
            elif val == 'true' or val == 'false':
                return '``{}``'.format(val.capitalize())
            else:
                return val
        elif tag == 'field':
            return '``{}``'.format(val)
        else:
            raise RuntimeError('Unknown doc ref tag %r' % tag)
    
    def _generate_header_comments(self, ns = None):
        self.emit('/**********************************************************')
        self.emit(' DO NOT EDIT')
        ns_name = ''
        if ns is not None:
            ns_name = '"{}"'.format(ns.name)
        self.emit(' This file was generated from stone specification {}'.format(ns_name))
        self.emit(' Part of "Ardi - the organizer" project.')
        self.emit(' osoft4ardi@gmail.com')
        self.emit(' www.prokarpaty.net')
        self.emit('***********************************************************/')
        self.emit()

    def _use_namespace_in_code_generation(self, ns):
        if(ns.name == ssc.sroot):
            return False
        return True

    def _generate_autotest(self, api):
        if ssc.support_autotest:
            self._generate_autotest_h(api)
            self._generate_autotest_cpp(api)


    def _generate_autotest_h(self, api):
        with self.output_to_relative_path(ssc.autotest_destination + '{}.h'.format(autotest_class)):
            self._generate_header_comments()
            self.emit('#pragma once')
            self._generate_qt_h_includes()
            self.emit('#include <QFile>')
            self.emit('#include <QTextStream>')
            self.emit('#include <QNetworkRequest>')
            self.emit(ssc.api_client_include_path)
            self.emit(api_module_root_include_path)
            self.emit()
            self.emit('#ifdef API_QT_AUTOTEST')
            self.emit('namespace {}{{'.format(ssc.dns))
            self.emit()
            with self.indent():
#                self.emit('class {} : public {}::ApiAutotest{{'.format(autotest_class, ssc.dns))
                self.emit('class {} {{'.format(autotest_class, ssc.dns))
                self.emit('public:')    
                with self.indent():
                    self.emit('{}({}&);'.format(autotest_class, ssc.api_client_name))
                    self.emit()
                    self.emit('void generateCalls();')
                    self.emit()
                    self.emit()
                self.emit('}};//{}'.format(autotest_class))
            self.emit()
            self.emit('}}//{}'.format(ssc.dns))
            self.emit('#endif //API_QT_AUTOTEST')

    def _generate_autotest_cpp(self, api):
        with self.output_to_relative_path(ssc.autotest_destination + '{}.cpp'.format(autotest_class)):
            self._generate_header_comments()
            self.emit('#include "{}.h"'.format(autotest_class))
            self.emit('#ifdef API_QT_AUTOTEST')
            self.emit('#include <QBuffer>')
            self.emit('#include <QByteArray>')
#            self.emit('using namespace {};'.format(ssc.dns))
            self.emit('using namespace {};'.format(ssc.dns))
            self.emit()
            self.emit('static {}* cl;'.format(module_route_root))
            self.emit()
            self.emit()
            for ns in api.namespaces.values():
                if ns.routes:                    
                    for r in ns.routes:
                        self.emit('static void {}{{'.format(autotest_name_for_route_method(ns, r)))
                        with self.indent():
                            self.emit('ApiAutotest::INSTANCE() << QString("%1/%2").arg("{}").arg("{}");'.format(fmt_class(ns.name), ssc.fmt_route(r.name)))
                            get_ns = 'get{}()'.format(fmt_class(ns.name))
                            rname = ssc.fmt_route(r.name)
                            arg_dt = r.arg_data_type
                            result_dt = r.result_data_type
                            result_type_name = ssc.qt_type_mapping(ns, result_dt, True, False, True)
                            if is_void_type(result_dt):
                               result_type_name = ''
                            if not ssc.support_exceptions:
                                if is_void_type(arg_dt):
                                    arg_dt = r.error_data_type
                            if is_void_type(arg_dt):
                                sname = ssc.build_endpoint_entry_name(ns, r)
                                io_device_arg = ''
                                if sname == 'simpleUploadStyle':
                                    self.emit('QByteArray data("Hello World! 123454321 (.) :: (b -> c) -> (a -> b) -> (a -> c)");')
                                    self.emit('QBuffer io(&data);')
                                    self.emit('io.open(QIODevice::ReadOnly);')
                                    io_device_arg = '&io'                                    
                                self.emit('cl->{}->{}({});'.format(get_ns, rname, io_device_arg))
                            else:
                                arg_name = ssc.qt_type_mapping(ns, arg_dt, True, True)
                                self.emit('std::unique_ptr<{}> arg = {}::EXAMPLE(0, 0);'.format(arg_name, arg_name))
                                sname = ssc.build_endpoint_entry_name(ns, r)
                                io_device_arg = ''
                                if sname == 'downloadStyle' or sname == 'uploadStyle' or sname == 'mpartUploadStyle' or sname == 'downloadContactPhotoStyle' or sname == 'uploadContactPhotoStyle':
                                    self.emit('QByteArray data("Hello World! 123454321 (.) :: (b -> c) -> (a -> b) -> (a -> c)");')
                                    self.emit('QBuffer io(&data);')
                                    self.emit('io.open(QIODevice::ReadOnly);')
                                    io_device_arg = ', &io'
                                if not ssc.support_exceptions:
                                    if not is_void_type(r.arg_data_type) and not is_void_type(r.error_data_type):
                                        body_name = ssc.qt_type_mapping(ns, r.error_data_type, True, True)
                                        self.emit('std::unique_ptr<{}> arg2 = {}::EXAMPLE(0, 0);'.format(body_name, body_name))
                                        io_device_arg = ', *(arg2.get())'
                                if is_void_type(result_dt):
                                   self.emit('cl->{}->{}(*(arg.get()) {});'.format(get_ns, rname, io_device_arg))
                                else:
                                   self.emit('auto res = cl->{}->{}(*(arg.get()) {});'.format(get_ns, rname, io_device_arg))
                                   self.emit('ApiAutotest::INSTANCE() << "------ RESULT ------------------";')
                                   self.emit('ApiAutotest::INSTANCE() << res->toString();')
                            self.emit('ApiAutotest::INSTANCE() << "--------------------------";')
                        self.emit('}')
                        self.emit()
            self.emit()
            for ns in api.namespaces.values():
                if ns.routes:
                    self.emit('static void {}{{'.format(autotest_name_for_routes(ns)))
                    with self.indent():
                        for r in ns.routes:
                            self.emit('{};'.format(autotest_name_for_route_method(ns, r)))
                    self.emit('}')
                    self.emit()
            self.emit()
            self.emit('void {}::generateCalls(){{'.format(autotest_class))
            with self.indent():
                self.emit('ApiAutotest::INSTANCE() << "";')
                self.emit('ApiAutotest::INSTANCE() << "============ autotest for module: {} ============";'.format(ssc.sroot))
                self.emit('cl->autotest();')
                for ns in api.namespaces.values():
                    if ns.routes:
                        self.emit(autotest_name_for_routes(ns) + ';')
            self.emit('}')
            self.emit()
            self.emit('{}::{}({}& c)'.format(autotest_class, autotest_class, ssc.api_client_name))
            self.emit('{')
            with self.indent():
                self.emit('cl = c.{}();'.format(ssc.sroot))
            self.emit('}')
            self.emit('#endif //API_QT_AUTOTEST')
            self.emit()

    def _generate_struct_example(self, ns, dt):
        cname = ssc.class_name_for_data_type(dt)
        self.emit('#ifdef API_QT_AUTOTEST')
        self.emit('std::unique_ptr<{}> {}::EXAMPLE(int context_index, int parent_context_index){{'.format(cname, cname))        
        with self.indent():
            self.emit('Q_UNUSED(context_index);')
            self.emit('Q_UNUSED(parent_context_index);')
            self.emit('static int example_idx = 0;')
            self.emit('example_idx++;')
            self.emit('std::unique_ptr<{}> rv(new {});'.format(cname, cname))
            idx = 1;
            for f in dt.fields:
                fname = f2name(f)
                self._generate_qt_field_example(ns, dt, f, idx, fname)
                idx += 1
            if is_union_type(dt):
                if dt.fields:
                    self.emit('rv->m_tag = {};'.format(encode_enum_value(dt, dt.fields[0])))
            self.emit('return rv;')
        self.emit('}')
        self.emit('#endif //API_QT_AUTOTEST')
        self.emit()


    def _generate_qt_field_example(self, ns, dt, f, idx, fname):
        context_class_name = ssc.qt_type_mapping(ns, dt, True, True)
        if is_nullable_type(f.data_type) or is_alias(f.data_type):
            return self._generate_qt_field_example(ns, dt, f.data_type, idx, fname)
        if is_string_type(f.data_type):
            if f.name == 'id':
                self.emit('rv->{} = {};'.format(fname, 'ApiAutotest::INSTANCE().getId("{}", example_idx)'.format(context_class_name)))
            else:
#                self.emit('rv->{} = {};'.format(fname, 'QString("{}_%1").arg(example_idx)'.format(f.name)))
                s_default_value = 'QString("{}_%1").arg(example_idx)'.format(f.name)
                self.emit('rv->{} = ApiAutotest::INSTANCE().getString("{}", "{}", {});'.format(fname, context_class_name, fname, s_default_value))
        elif is_user_defined_type(f.data_type):
            arg_name = ssc.qt_type_mapping(ns, f.data_type, True, True)
            self.emit('rv->{} = {};'.format(fname, '*({}::EXAMPLE(0, context_index).get())'.format(arg_name)))
        elif is_integer_type(f.data_type) or is_float_type(f.data_type):
            self.emit('rv->{} = ApiAutotest::INSTANCE().getInt("{}", "{}", {} + example_idx);'.format(fname, context_class_name, fname, idx))
#            self.emit('rv->{} = {} + example_idx;'.format(fname, idx))
        elif is_timestamp_type(f.data_type):
            self.emit('rv->{} = {};'.format(fname, 'QDateTime::currentDateTime()'))
        elif is_bytes_type(f.data_type):
            self.emit('rv->{} = ApiAutotest::INSTANCE().generateData("{}", context_index, parent_context_index);'.format(fname, context_class_name))
#            self.emit('rv->{} = {};'.format(fname, 'QByteArray("AUTOTEST-DATA").toBase64()'))
        elif is_list_type(f.data_type):
            field_mem_name = name_for_member(f)
            arg_name = ssc.qt_type_mapping(ns, f.data_type.data_type, True, True)
            self.emit('std::vector<{}> list_of_{};'.format(arg_name, f.name))
            self.emit('for(int i = 0; i < 5; i++){')
            with self.indent():
                if is_user_defined_type(f.data_type.data_type):
                    self.emit('{} p = *({}::EXAMPLE(i, context_index).get());'.format(arg_name, arg_name))
                    self.emit('ApiAutotest::INSTANCE().prepareAutoTestObj("{}", "{}", &p, i, context_index);'.format(context_class_name, arg_name))
                    self.emit('rv->{}.push_back(p);'.format(fname))
                elif is_string_type(f.data_type.data_type):
                    self.emit('rv->{}.push_back(QString("id_%1").arg(i+1));'.format(fname))
#                    self.emit('rv->{}.push_back(QString("_%1_%2").arg(i).arg(example_idx));'.format(fname))
            self.emit('}')
            if is_string_type(f.data_type.data_type):
                self.emit('QString tmp_{} = ApiAutotest::INSTANCE().getString4List("{}", "{}");'.format(fname, context_class_name, fname))
                self.emit('if(!tmp_{}.isEmpty())rv->{}.push_back(tmp_{});'.format(fname, fname, fname))


def get_field_h_dependancy(ns, f):
    if is_void_type(f):
        return ''
    if is_nullable_type(f.data_type) or is_alias(f.data_type):
        return get_field_h_dependancy(ns, f.data_type)
    if is_user_defined_type(f.data_type) or is_alias(f.data_type):
        return class_include_line_for_data_type(f.data_type)
    if is_list_type(f.data_type):
        if is_list_type(f.data_type.data_type):
            return class_include_line_for_data_type(f.data_type.data_type)
        if is_user_defined_type(f.data_type.data_type) or is_alias(f.data_type.data_type):
           return class_include_line_for_data_type(f.data_type.data_type)
    if is_union_type(f.data_type):
        return class_include_line_for_data_type(f.data_type)
    return ''


def encode_enum_value(dt, f):
    return '{}_{}'.format(dt.name, f.name.upper())


def qt_default_value(ns, dt, f):
    res = f.default
    if is_boolean_type(f.data_type):
        if f.default == 'True':
            res = 'true'
        else:
            res = 'false'
    elif is_string_type(f.data_type):
        res = '"{}"'.format(f.default)
    elif is_union_type(f.data_type):
        res = '{}::{}_{}'.format(f.data_type.name, f.data_type.name, f.default.tag_name.upper())
    return res


def f2name(f):
    return 'm_' + f.name

def first_lower(s):
   if len(s) == 0:
      return s
   else:
      return s[0].lower() + s[1:]

def first_upper(s):
   if len(s) == 0:
      return s
   else:
      return s[0].upper() + s[1:]


def name_for_member(f):
    r = first_lower(f.name.title()).replace('_', '')
    if r == 'linux':
        r = 'Linux'
    return r

def member_getter_type_for_field(ns, f):
    if is_alias(f.data_type) or is_nullable_type(f.data_type):
        return member_getter_type_for_field(ns, f.data_type)
    elif is_list_type(f.data_type):
        return 'const {}&'.format(ssc.qt_type_mapping(ns, f.data_type, True, False, True))
    elif is_user_defined_type(f.data_type):
        class_name = ssc.class_name_for_data_type(f.data_type)
        if(f.data_type.namespace.name != ns.name):
            class_name = '%s::%s' % (f.data_type.namespace.name, class_name)
        return 'const {}&'.format(class_name)
    else:
        return ssc.qt_type_mapping(ns, f.data_type)
#    else:
#        return ssc.qt_type_mapping(ns, f.data_type)

def member_setter_type_for_field(ns, f):
    if is_alias(f.data_type) or is_nullable_type(f.data_type):
        return member_setter_type_for_field(ns, f.data_type)
    else:
        return ssc.qt_type_mapping(ns, f.data_type)

    
def class_include_line_for_data_type(dt):
    if is_alias(dt) or is_nullable_type(dt) or is_list_type(dt):
        return class_include_line_for_data_type(dt.data_type)
    elif is_user_defined_type(dt):
        return '#include "{}"'.format(h_file_name_for_data_type(dt.namespace, dt))
    else:
        raise TypeError('Unknown data type %r' % dt)

def enum_name_for_data_type(dt):
    assert is_union_type(dt), 'Expected enum type, got %r' % type(dt)
    name = fmt_class(dt.name)
    return name

#def serializer_name(n):
#    name = n + 'Serializer'
#    return name


def h_file_name_for_data_type(ns, dt, addPath=True):
    assert is_user_defined_type(dt) or is_alias(dt), 'Expected composite type, got %r' % type(dt)
    stone_prefix = ''
    if addPath:
        stone_prefix = '{}/'.format(ssc.sroot)
    return '{}{}/{}{}.h'.format(stone_prefix, ns.name, fmt_class(ns.name), fmt_class(dt.name))

def cpp_file_name_for_data_type(ns, dt):
    assert is_user_defined_type(dt) or is_alias(dt), 'Expected composite type, got %r' % type(dt)
    return '{}/{}{}.cpp'.format(ns.name, fmt_class(ns.name), fmt_class(dt.name))

def h_file_name_for_route(ns, addPath=True):
    stone_prefix = ''
    if addPath:
        stone_prefix = '{}/'.format(ssc.sroot)
    return '{}{}/{}Routes.h'.format(stone_prefix,ns.name, fmt_class(ns.name))

def cpp_file_name_for_route(ns):
    return '{}/{}Routes.cpp'.format(ns.name, fmt_class(ns.name))

def autotest_name_for_routes(ns):
    rcall_name = 'test_call_{}Routes'.format(fmt_class(ns.name))
    return '{}()'.format(rcall_name)

def autotest_name_for_route_method(ns, r):
    cname = 'call_{}_from_{}'.format(ssc.fmt_route(r.name), ssc.fmt_class(ns.name))
    return '{}()'.format(cname)



#def routeStyleName(r):
#    sname = 'rpcStyle'
#    s_attr = r.attrs['style']
#    if s_attr:
#        if s_attr == 'download':
#            sname = 'downloadStyle'
#        elif s_attr == 'upload':
#            sname = 'uploadStyle'    
#        if s_attr == 'get':
#            sname = 'getStyle'
#    return sname


