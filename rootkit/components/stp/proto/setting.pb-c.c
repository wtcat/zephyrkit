/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: setting.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "setting.pb-c.h"
void   setting__time__init
                     (Setting__Time         *message)
{
  static const Setting__Time init_value = SETTING__TIME__INIT;
  *message = init_value;
}
size_t setting__time__get_packed_size
                     (const Setting__Time *message)
{
  assert(message->base.descriptor == &setting__time__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t setting__time__pack
                     (const Setting__Time *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &setting__time__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t setting__time__pack_to_buffer
                     (const Setting__Time *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &setting__time__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Setting__Time *
       setting__time__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Setting__Time *)
     protobuf_c_message_unpack (&setting__time__descriptor,
                                allocator, len, data);
}
void   setting__time__free_unpacked
                     (Setting__Time *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &setting__time__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   setting__alarm__body__init
                     (Setting__Alarm__Body         *message)
{
  static const Setting__Alarm__Body init_value = SETTING__ALARM__BODY__INIT;
  *message = init_value;
}
void   setting__alarm__init
                     (Setting__Alarm         *message)
{
  static const Setting__Alarm init_value = SETTING__ALARM__INIT;
  *message = init_value;
}
size_t setting__alarm__get_packed_size
                     (const Setting__Alarm *message)
{
  assert(message->base.descriptor == &setting__alarm__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t setting__alarm__pack
                     (const Setting__Alarm *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &setting__alarm__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t setting__alarm__pack_to_buffer
                     (const Setting__Alarm *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &setting__alarm__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Setting__Alarm *
       setting__alarm__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Setting__Alarm *)
     protobuf_c_message_unpack (&setting__alarm__descriptor,
                                allocator, len, data);
}
void   setting__alarm__free_unpacked
                     (Setting__Alarm *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &setting__alarm__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   setting__silence__time__init
                     (Setting__Silence__Time         *message)
{
  static const Setting__Silence__Time init_value = SETTING__SILENCE__TIME__INIT;
  *message = init_value;
}
void   setting__silence__init
                     (Setting__Silence         *message)
{
  static const Setting__Silence init_value = SETTING__SILENCE__INIT;
  *message = init_value;
}
size_t setting__silence__get_packed_size
                     (const Setting__Silence *message)
{
  assert(message->base.descriptor == &setting__silence__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t setting__silence__pack
                     (const Setting__Silence *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &setting__silence__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t setting__silence__pack_to_buffer
                     (const Setting__Silence *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &setting__silence__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Setting__Silence *
       setting__silence__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Setting__Silence *)
     protobuf_c_message_unpack (&setting__silence__descriptor,
                                allocator, len, data);
}
void   setting__silence__free_unpacked
                     (Setting__Silence *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &setting__silence__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   setting__remind_mode__init
                     (Setting__RemindMode         *message)
{
  static const Setting__RemindMode init_value = SETTING__REMIND_MODE__INIT;
  *message = init_value;
}
size_t setting__remind_mode__get_packed_size
                     (const Setting__RemindMode *message)
{
  assert(message->base.descriptor == &setting__remind_mode__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t setting__remind_mode__pack
                     (const Setting__RemindMode *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &setting__remind_mode__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t setting__remind_mode__pack_to_buffer
                     (const Setting__RemindMode *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &setting__remind_mode__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Setting__RemindMode *
       setting__remind_mode__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Setting__RemindMode *)
     protobuf_c_message_unpack (&setting__remind_mode__descriptor,
                                allocator, len, data);
}
void   setting__remind_mode__free_unpacked
                     (Setting__RemindMode *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &setting__remind_mode__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor setting__time__field_descriptors[1] =
{
  {
    "unix_timestamp",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_FIXED32,
    0,   /* quantifier_offset */
    offsetof(Setting__Time, unix_timestamp),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned setting__time__field_indices_by_name[] = {
  0,   /* field[0] = unix_timestamp */
};
static const ProtobufCIntRange setting__time__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor setting__time__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "setting.Time",
  "Time",
  "Setting__Time",
  "setting",
  sizeof(Setting__Time),
  1,
  setting__time__field_descriptors,
  setting__time__field_indices_by_name,
  1,  setting__time__number_ranges,
  (ProtobufCMessageInit) setting__time__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue setting__alarm__body__week_day__enum_values_by_number[7] =
{
  { "MONDAY", "SETTING__ALARM__BODY__WEEK_DAY__MONDAY", 1 },
  { "TUESDAY", "SETTING__ALARM__BODY__WEEK_DAY__TUESDAY", 2 },
  { "WEDNESDAY", "SETTING__ALARM__BODY__WEEK_DAY__WEDNESDAY", 4 },
  { "THURSDAY", "SETTING__ALARM__BODY__WEEK_DAY__THURSDAY", 8 },
  { "FRIDAY", "SETTING__ALARM__BODY__WEEK_DAY__FRIDAY", 16 },
  { "SATURDAY", "SETTING__ALARM__BODY__WEEK_DAY__SATURDAY", 32 },
  { "SUNDAY", "SETTING__ALARM__BODY__WEEK_DAY__SUNDAY", 64 },
};
static const ProtobufCIntRange setting__alarm__body__week_day__value_ranges[] = {
{1, 0},{4, 2},{8, 3},{16, 4},{32, 5},{64, 6},{0, 7}
};
static const ProtobufCEnumValueIndex setting__alarm__body__week_day__enum_values_by_name[7] =
{
  { "FRIDAY", 4 },
  { "MONDAY", 0 },
  { "SATURDAY", 5 },
  { "SUNDAY", 6 },
  { "THURSDAY", 3 },
  { "TUESDAY", 1 },
  { "WEDNESDAY", 2 },
};
const ProtobufCEnumDescriptor setting__alarm__body__week_day__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "setting.Alarm.Body.WeekDay",
  "WeekDay",
  "Setting__Alarm__Body__WeekDay",
  "setting",
  7,
  setting__alarm__body__week_day__enum_values_by_number,
  7,
  setting__alarm__body__week_day__enum_values_by_name,
  6,
  setting__alarm__body__week_day__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const ProtobufCFieldDescriptor setting__alarm__body__field_descriptors[5] =
{
  {
    "hour",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Alarm__Body, hour),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "min",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Alarm__Body, min),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "sec",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Alarm__Body, sec),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "weekdays",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Alarm__Body, weekdays),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "repeat",
    5,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_BOOL,
    0,   /* quantifier_offset */
    offsetof(Setting__Alarm__Body, repeat),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned setting__alarm__body__field_indices_by_name[] = {
  0,   /* field[0] = hour */
  1,   /* field[1] = min */
  4,   /* field[4] = repeat */
  2,   /* field[2] = sec */
  3,   /* field[3] = weekdays */
};
static const ProtobufCIntRange setting__alarm__body__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 5 }
};
const ProtobufCMessageDescriptor setting__alarm__body__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "setting.Alarm.Body",
  "Body",
  "Setting__Alarm__Body",
  "setting",
  sizeof(Setting__Alarm__Body),
  5,
  setting__alarm__body__field_descriptors,
  setting__alarm__body__field_indices_by_name,
  1,  setting__alarm__body__number_ranges,
  (ProtobufCMessageInit) setting__alarm__body__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor setting__alarm__field_descriptors[1] =
{
  {
    "entities",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Setting__Alarm, n_entities),
    offsetof(Setting__Alarm, entities),
    &setting__alarm__body__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned setting__alarm__field_indices_by_name[] = {
  0,   /* field[0] = entities */
};
static const ProtobufCIntRange setting__alarm__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor setting__alarm__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "setting.Alarm",
  "Alarm",
  "Setting__Alarm",
  "setting",
  sizeof(Setting__Alarm),
  1,
  setting__alarm__field_descriptors,
  setting__alarm__field_indices_by_name,
  1,  setting__alarm__number_ranges,
  (ProtobufCMessageInit) setting__alarm__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor setting__silence__time__field_descriptors[4] =
{
  {
    "hour_start",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Silence__Time, hour_start),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "min_start",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Silence__Time, min_start),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "hour_end",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Silence__Time, hour_end),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "min_end",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Setting__Silence__Time, min_end),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned setting__silence__time__field_indices_by_name[] = {
  2,   /* field[2] = hour_end */
  0,   /* field[0] = hour_start */
  3,   /* field[3] = min_end */
  1,   /* field[1] = min_start */
};
static const ProtobufCIntRange setting__silence__time__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor setting__silence__time__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "setting.Silence.Time",
  "Time",
  "Setting__Silence__Time",
  "setting",
  sizeof(Setting__Silence__Time),
  4,
  setting__silence__time__field_descriptors,
  setting__silence__time__field_indices_by_name,
  1,  setting__silence__time__number_ranges,
  (ProtobufCMessageInit) setting__silence__time__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue setting__silence__state__enum_values_by_number[2] =
{
  { "DISABLE", "SETTING__SILENCE__STATE__DISABLE", 0 },
  { "ENABLE", "SETTING__SILENCE__STATE__ENABLE", 1 },
};
static const ProtobufCIntRange setting__silence__state__value_ranges[] = {
{0, 0},{0, 2}
};
static const ProtobufCEnumValueIndex setting__silence__state__enum_values_by_name[2] =
{
  { "DISABLE", 0 },
  { "ENABLE", 1 },
};
const ProtobufCEnumDescriptor setting__silence__state__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "setting.Silence.State",
  "State",
  "Setting__Silence__State",
  "setting",
  2,
  setting__silence__state__enum_values_by_number,
  2,
  setting__silence__state__enum_values_by_name,
  1,
  setting__silence__state__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const Setting__Silence__State setting__silence__enable__default_value = SETTING__SILENCE__STATE__DISABLE;
static const ProtobufCFieldDescriptor setting__silence__field_descriptors[2] =
{
  {
    "enable",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(Setting__Silence, enable),
    &setting__silence__state__descriptor,
    &setting__silence__enable__default_value,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "time",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Setting__Silence, n_time),
    offsetof(Setting__Silence, time),
    &setting__silence__time__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned setting__silence__field_indices_by_name[] = {
  0,   /* field[0] = enable */
  1,   /* field[1] = time */
};
static const ProtobufCIntRange setting__silence__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor setting__silence__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "setting.Silence",
  "Silence",
  "Setting__Silence",
  "setting",
  sizeof(Setting__Silence),
  2,
  setting__silence__field_descriptors,
  setting__silence__field_indices_by_name,
  1,  setting__silence__number_ranges,
  (ProtobufCMessageInit) setting__silence__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue setting__remind_mode__mode__enum_values_by_number[3] =
{
  { "DEFAULT", "SETTING__REMIND_MODE__MODE__DEFAULT", 0 },
  { "SHAKE", "SETTING__REMIND_MODE__MODE__SHAKE", 1 },
  { "DISPLAY", "SETTING__REMIND_MODE__MODE__DISPLAY", 2 },
};
static const ProtobufCIntRange setting__remind_mode__mode__value_ranges[] = {
{0, 0},{0, 3}
};
static const ProtobufCEnumValueIndex setting__remind_mode__mode__enum_values_by_name[3] =
{
  { "DEFAULT", 0 },
  { "DISPLAY", 2 },
  { "SHAKE", 1 },
};
const ProtobufCEnumDescriptor setting__remind_mode__mode__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "setting.RemindMode.Mode",
  "Mode",
  "Setting__RemindMode__Mode",
  "setting",
  3,
  setting__remind_mode__mode__enum_values_by_number,
  3,
  setting__remind_mode__mode__enum_values_by_name,
  1,
  setting__remind_mode__mode__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const Setting__RemindMode__Mode setting__remind_mode__mode__default_value = SETTING__REMIND_MODE__MODE__DEFAULT;
static const ProtobufCFieldDescriptor setting__remind_mode__field_descriptors[1] =
{
  {
    "mode",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(Setting__RemindMode, mode),
    &setting__remind_mode__mode__descriptor,
    &setting__remind_mode__mode__default_value,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned setting__remind_mode__field_indices_by_name[] = {
  0,   /* field[0] = mode */
};
static const ProtobufCIntRange setting__remind_mode__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor setting__remind_mode__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "setting.RemindMode",
  "RemindMode",
  "Setting__RemindMode",
  "setting",
  sizeof(Setting__RemindMode),
  1,
  setting__remind_mode__field_descriptors,
  setting__remind_mode__field_indices_by_name,
  1,  setting__remind_mode__number_ranges,
  (ProtobufCMessageInit) setting__remind_mode__init,
  NULL,NULL,NULL    /* reserved[123] */
};
