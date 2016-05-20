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
    google/protobuf/compiler/zip_writer.cc

protoc:
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) $(CPPFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) $(TARGET_ARCH_FOR_BUILD) -DHAVE_PTHREAD_H=1 -I. ${libprotobuf_a_SOURCES} $(protoc_srcs) $(LOADLIBES_FOR_BUILD) $(LDLIBS_FOR_BUILD) -o protoc

all-local: protoc

