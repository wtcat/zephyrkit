/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: time.proto */

#ifndef PROTOBUF_C_time_2eproto__INCLUDED
#define PROTOBUF_C_time_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003003 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Proto__Time__UnixTimestamp Proto__Time__UnixTimestamp;


/* --- enums --- */


/* --- messages --- */

struct  _Proto__Time__UnixTimestamp
{
  ProtobufCMessage base;
  uint32_t time;
};
#define PROTO__TIME__UNIX_TIMESTAMP__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&proto__time__unix_timestamp__descriptor) \
    , 0 }


/* Proto__Time__UnixTimestamp methods */
void   proto__time__unix_timestamp__init
                     (Proto__Time__UnixTimestamp         *message);
size_t proto__time__unix_timestamp__get_packed_size
                     (const Proto__Time__UnixTimestamp   *message);
size_t proto__time__unix_timestamp__pack
                     (const Proto__Time__UnixTimestamp   *message,
                      uint8_t             *out);
size_t proto__time__unix_timestamp__pack_to_buffer
                     (const Proto__Time__UnixTimestamp   *message,
                      ProtobufCBuffer     *buffer);
Proto__Time__UnixTimestamp *
       proto__time__unix_timestamp__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   proto__time__unix_timestamp__free_unpacked
                     (Proto__Time__UnixTimestamp *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Proto__Time__UnixTimestamp_Closure)
                 (const Proto__Time__UnixTimestamp *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor proto__time__unix_timestamp__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_time_2eproto__INCLUDED */
