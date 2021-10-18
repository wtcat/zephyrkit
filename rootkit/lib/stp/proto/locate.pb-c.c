/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: locate.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "locate.pb-c.h"
void   locate__locate_buddy__init
                     (Locate__LocateBuddy         *message)
{
  static const Locate__LocateBuddy init_value = LOCATE__LOCATE_BUDDY__INIT;
  *message = init_value;
}
size_t locate__locate_buddy__get_packed_size
                     (const Locate__LocateBuddy *message)
{
  assert(message->base.descriptor == &locate__locate_buddy__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t locate__locate_buddy__pack
                     (const Locate__LocateBuddy *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &locate__locate_buddy__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t locate__locate_buddy__pack_to_buffer
                     (const Locate__LocateBuddy *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &locate__locate_buddy__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Locate__LocateBuddy *
       locate__locate_buddy__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Locate__LocateBuddy *)
     protobuf_c_message_unpack (&locate__locate_buddy__descriptor,
                                allocator, len, data);
}
void   locate__locate_buddy__free_unpacked
                     (Locate__LocateBuddy *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &locate__locate_buddy__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCEnumValue locate__locate_buddy__mode__enum_values_by_number[2] =
{
  { "SHAKING", "LOCATE__LOCATE_BUDDY__MODE__SHAKING", 1 },
  { "RINGING", "LOCATE__LOCATE_BUDDY__MODE__RINGING", 2 },
};
static const ProtobufCIntRange locate__locate_buddy__mode__value_ranges[] = {
{1, 0},{0, 2}
};
static const ProtobufCEnumValueIndex locate__locate_buddy__mode__enum_values_by_name[2] =
{
  { "RINGING", 1 },
  { "SHAKING", 0 },
};
const ProtobufCEnumDescriptor locate__locate_buddy__mode__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "locate.LocateBuddy.Mode",
  "Mode",
  "Locate__LocateBuddy__Mode",
  "locate",
  2,
  locate__locate_buddy__mode__enum_values_by_number,
  2,
  locate__locate_buddy__mode__enum_values_by_name,
  1,
  locate__locate_buddy__mode__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const ProtobufCFieldDescriptor locate__locate_buddy__field_descriptors[1] =
{
  {
    "mode",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Locate__LocateBuddy, mode),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned locate__locate_buddy__field_indices_by_name[] = {
  0,   /* field[0] = mode */
};
static const ProtobufCIntRange locate__locate_buddy__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor locate__locate_buddy__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "locate.LocateBuddy",
  "LocateBuddy",
  "Locate__LocateBuddy",
  "locate",
  sizeof(Locate__LocateBuddy),
  1,
  locate__locate_buddy__field_descriptors,
  locate__locate_buddy__field_indices_by_name,
  1,  locate__locate_buddy__number_ranges,
  (ProtobufCMessageInit) locate__locate_buddy__init,
  NULL,NULL,NULL    /* reserved[123] */
};