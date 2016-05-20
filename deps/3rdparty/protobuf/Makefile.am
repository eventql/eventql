MAINTAINERCLEANFILES = Makefile.in

noinst_LIBRARIES = libprotobuf.a

if HAVE_PTHREAD
PTHREAD_DEF = -DHAVE_PTHREAD=1 -DHAVE_PTHREAD_H=1
else
PTHREAD_DEF =
endif

AM_CXXFLAGS = $(PTHREAD_CFLAGS) $(PTHREAD_DEF)
AM_LDFLAGS = $(PTHREAD_CFLAGS)

libprotobuf_a_SOURCES = \
    google/protobuf/descriptor.h \
    google/protobuf/descriptor.pb.h \
    google/protobuf/descriptor_database.h \
    google/protobuf/dynamic_message.h \
    google/protobuf/extension_set.h \
    google/protobuf/generated_enum_reflection.h \
    google/protobuf/generated_message_reflection.h \
    google/protobuf/generated_message_util.h \
    google/protobuf/io/coded_stream.h \
    google/protobuf/io/coded_stream_inl.h \
    google/protobuf/io/gzip_stream.h \
    google/protobuf/io/gzip_stream_unittest.sh \
    google/protobuf/io/package_info.h \
    google/protobuf/io/printer.h \
    google/protobuf/io/tokenizer.h \
    google/protobuf/io/zero_copy_stream.h \
    google/protobuf/io/zero_copy_stream_impl.h \
    google/protobuf/io/zero_copy_stream_impl_lite.h \
    google/protobuf/message.h \
    google/protobuf/message_lite.h \
    google/protobuf/package_info.h \
    google/protobuf/reflection_ops.h \
    google/protobuf/repeated_field.h \
    google/protobuf/service.h \
    google/protobuf/stubs/atomicops.h \
    google/protobuf/stubs/atomicops_internals_arm_gcc.h \
    google/protobuf/stubs/atomicops_internals_arm_qnx.h \
    google/protobuf/stubs/atomicops_internals_atomicword_compat.h \
    google/protobuf/stubs/atomicops_internals_macosx.h \
    google/protobuf/stubs/atomicops_internals_mips_gcc.h \
    google/protobuf/stubs/atomicops_internals_pnacl.h \
    google/protobuf/stubs/atomicops_internals_x86_gcc.h \
    google/protobuf/stubs/atomicops_internals_x86_msvc.h \
    google/protobuf/stubs/common.h \
    google/protobuf/stubs/hash.h \
    google/protobuf/stubs/map-util.h \
    google/protobuf/stubs/once.h \
    google/protobuf/stubs/platform_macros.h \
    google/protobuf/stubs/stl_util.h \
    google/protobuf/stubs/stringprintf.h \
    google/protobuf/stubs/strutil.h \
    google/protobuf/stubs/substitute.h \
    google/protobuf/stubs/template_util.h \
    google/protobuf/stubs/type_traits.h \
    google/protobuf/test_util.h \
    google/protobuf/test_util_lite.h \
    google/protobuf/testing/file.h \
    google/protobuf/testing/googletest.h \
    google/protobuf/text_format.h \
    google/protobuf/unknown_field_set.h \
    google/protobuf/wire_format.h \
    google/protobuf/wire_format_lite.h \
    google/protobuf/wire_format_lite_inl.h \
    google/protobuf/descriptor.cc \
    google/protobuf/descriptor.pb.cc \
    google/protobuf/descriptor_database.cc \
    google/protobuf/dynamic_message.cc \
    google/protobuf/extension_set.cc \
    google/protobuf/extension_set_heavy.cc \
    google/protobuf/generated_message_reflection.cc \
    google/protobuf/generated_message_util.cc \
    google/protobuf/io/coded_stream.cc \
    google/protobuf/io/gzip_stream.cc \
    google/protobuf/io/printer.cc \
    google/protobuf/io/tokenizer.cc \
    google/protobuf/io/zero_copy_stream.cc \
    google/protobuf/io/zero_copy_stream_impl.cc \
    google/protobuf/io/zero_copy_stream_impl_lite.cc \
    google/protobuf/message.cc \
    google/protobuf/message_lite.cc \
    google/protobuf/reflection_ops.cc \
    google/protobuf/repeated_field.cc \
    google/protobuf/service.cc \
    google/protobuf/stubs/atomicops_internals_x86_gcc.cc \
    google/protobuf/stubs/atomicops_internals_x86_msvc.cc \
    google/protobuf/stubs/common.cc \
    google/protobuf/stubs/once.cc \
    google/protobuf/stubs/stringprintf.cc \
    google/protobuf/stubs/structurally_valid.cc \
    google/protobuf/stubs/strutil.cc \
    google/protobuf/stubs/substitute.cc \
    google/protobuf/text_format.cc \
    google/protobuf/unknown_field_set.cc \
    google/protobuf/wire_format.cc \
    google/protobuf/wire_format_lite.cc

protoc_srcs = \
    $(abs_srcdir)/google/protobuf/descriptor.cc \
    $(abs_srcdir)/google/protobuf/descriptor.pb.cc \
    $(abs_srcdir)/google/protobuf/descriptor_database.cc \
    $(abs_srcdir)/google/protobuf/dynamic_message.cc \
    $(abs_srcdir)/google/protobuf/extension_set.cc \
    $(abs_srcdir)/google/protobuf/extension_set_heavy.cc \
    $(abs_srcdir)/google/protobuf/generated_message_reflection.cc \
    $(abs_srcdir)/google/protobuf/generated_message_util.cc \
    $(abs_srcdir)/google/protobuf/io/coded_stream.cc \
    $(abs_srcdir)/google/protobuf/io/gzip_stream.cc \
    $(abs_srcdir)/google/protobuf/io/printer.cc \
    $(abs_srcdir)/google/protobuf/io/tokenizer.cc \
    $(abs_srcdir)/google/protobuf/io/zero_copy_stream.cc \
    $(abs_srcdir)/google/protobuf/io/zero_copy_stream_impl.cc \
    $(abs_srcdir)/google/protobuf/io/zero_copy_stream_impl_lite.cc \
    $(abs_srcdir)/google/protobuf/message.cc \
    $(abs_srcdir)/google/protobuf/message_lite.cc \
    $(abs_srcdir)/google/protobuf/reflection_ops.cc \
    $(abs_srcdir)/google/protobuf/repeated_field.cc \
    $(abs_srcdir)/google/protobuf/service.cc \
    $(abs_srcdir)/google/protobuf/stubs/atomicops_internals_x86_gcc.cc \
    $(abs_srcdir)/google/protobuf/stubs/atomicops_internals_x86_msvc.cc \
    $(abs_srcdir)/google/protobuf/stubs/common.cc \
    $(abs_srcdir)/google/protobuf/stubs/once.cc \
    $(abs_srcdir)/google/protobuf/stubs/stringprintf.cc \
    $(abs_srcdir)/google/protobuf/stubs/structurally_valid.cc \
    $(abs_srcdir)/google/protobuf/stubs/strutil.cc \
    $(abs_srcdir)/google/protobuf/stubs/substitute.cc \
    $(abs_srcdir)/google/protobuf/text_format.cc \
    $(abs_srcdir)/google/protobuf/unknown_field_set.cc \
    $(abs_srcdir)/google/protobuf/wire_format.cc \
    $(abs_srcdir)/google/protobuf/wire_format_lite.cc \
    $(abs_srcdir)/google/protobuf/compiler/command_line_interface.cc \
    $(abs_srcdir)/google/protobuf/compiler/main.cc \
    $(abs_srcdir)/google/protobuf/compiler/parser.cc \
    $(abs_srcdir)/google/protobuf/compiler/subprocess.cc \
    $(abs_srcdir)/google/protobuf/compiler/importer.cc \
    $(abs_srcdir)/google/protobuf/compiler/code_generator.cc \
    $(abs_srcdir)/google/protobuf/compiler/plugin.cc \
    $(abs_srcdir)/google/protobuf/compiler/plugin.pb.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_enum.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_enum_field.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_extension.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_field.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_file.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_generator.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_helpers.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_message.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_message_field.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_primitive_field.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_service.cc \
    $(abs_srcdir)/google/protobuf/compiler/cpp/cpp_string_field.cc \
    $(abs_srcdir)/google/protobuf/compiler/zip_writer.cc

EXTRA_DIST = \
    google/protobuf/compiler/command_line_interface.cc \
    google/protobuf/compiler/main.cc \
    google/protobuf/compiler/parser.cc \
    google/protobuf/compiler/subprocess.cc \
    google/protobuf/compiler/importer.cc \
    google/protobuf/compiler/code_generator.cc \
    google/protobuf/compiler/plugin.cc \
    google/protobuf/compiler/plugin.pb.cc \
    google/protobuf/compiler/cpp/cpp_enum.cc \
    google/protobuf/compiler/cpp/cpp_enum_field.cc \
    google/protobuf/compiler/cpp/cpp_extension.cc \
    google/protobuf/compiler/cpp/cpp_field.cc \
    google/protobuf/compiler/cpp/cpp_file.cc \
    google/protobuf/compiler/cpp/cpp_generator.cc \
    google/protobuf/compiler/cpp/cpp_helpers.cc \
    google/protobuf/compiler/cpp/cpp_message.cc \
    google/protobuf/compiler/cpp/cpp_message_field.cc \
    google/protobuf/compiler/cpp/cpp_primitive_field.cc \
    google/protobuf/compiler/cpp/cpp_service.cc \
    google/protobuf/compiler/cpp/cpp_string_field.cc \
    google/protobuf/compiler/zip_writer.cc \
    google/protobuf/compiler/code_generator.h \
    google/protobuf/compiler/command_line_interface.h \
    google/protobuf/compiler/cpp/cpp_enum.h \
    google/protobuf/compiler/cpp/cpp_enum_field.h \
    google/protobuf/compiler/cpp/cpp_extension.h \
    google/protobuf/compiler/cpp/cpp_field.h \
    google/protobuf/compiler/cpp/cpp_file.h \
    google/protobuf/compiler/cpp/cpp_generator.h \
    google/protobuf/compiler/cpp/cpp_helpers.h \
    google/protobuf/compiler/cpp/cpp_message.h \
    google/protobuf/compiler/cpp/cpp_message_field.h \
    google/protobuf/compiler/cpp/cpp_options.h \
    google/protobuf/compiler/cpp/cpp_primitive_field.h \
    google/protobuf/compiler/cpp/cpp_service.h \
    google/protobuf/compiler/cpp/cpp_string_field.h \
    google/protobuf/compiler/cpp/cpp_unittest.h \
    google/protobuf/compiler/importer.h \
    google/protobuf/compiler/mock_code_generator.h \
    google/protobuf/compiler/package_info.h \
    google/protobuf/compiler/parser.h \
    google/protobuf/compiler/plugin.h \
    google/protobuf/compiler/plugin.pb.h \
    google/protobuf/compiler/subprocess.h \
    google/protobuf/compiler/zip_output_unittest.sh \
    google/protobuf/compiler/zip_writer.h

protoc:
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) $(CPPFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $(TARGET_ARCH_FOR_BUILD) -DHAVE_PTHREAD_H=1 -I$(abs_srcdir) $(protoc_srcs) $(LOADLIBES_FOR_BUILD) $(LDLIBS_FOR_BUILD) -o protoc

all-local: protoc

