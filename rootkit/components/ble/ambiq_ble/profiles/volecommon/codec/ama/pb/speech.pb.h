//*****************************************************************************
//
//  speech.pb.h
//! @file
//!
//! @brief Auto-generated (see below).
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 2.5.1 of the AmbiqSuite Development Package.
//
//*****************************************************************************
/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.1 at Fri Nov 09 16:58:28 2018. */

#ifndef PB_SPEECH_PB_H_INCLUDED
#define PB_SPEECH_PB_H_INCLUDED
#include <pb.h>

#include "common.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _AudioProfile
{
    AudioProfile_CLOSE_TALK = 0,
    AudioProfile_NEAR_FIELD = 1,
    AudioProfile_FAR_FIELD = 2
} AudioProfile;
#define _AudioProfile_MIN AudioProfile_CLOSE_TALK
#define _AudioProfile_MAX AudioProfile_FAR_FIELD
#define _AudioProfile_ARRAYSIZE ((AudioProfile)(AudioProfile_FAR_FIELD + 1))

typedef enum _AudioFormat
{
    AudioFormat_PCM_L16_16KHZ_MONO = 0,
    AudioFormat_OPUS_16KHZ_32KBPS_CBR_0_20MS = 1,
    AudioFormat_OPUS_16KHZ_16KBPS_CBR_0_20MS = 2,
    AudioFormat_MSBC = 3
} AudioFormat;
#define _AudioFormat_MIN AudioFormat_PCM_L16_16KHZ_MONO
#define _AudioFormat_MAX AudioFormat_MSBC
#define _AudioFormat_ARRAYSIZE ((AudioFormat)(AudioFormat_MSBC + 1))

typedef enum _AudioSource
{
    AudioSource_STREAM = 0,
    AudioSource_BLUETOOTH_SCO = 1
} AudioSource;
#define _AudioSource_MIN AudioSource_STREAM
#define _AudioSource_MAX AudioSource_BLUETOOTH_SCO
#define _AudioSource_ARRAYSIZE ((AudioSource)(AudioSource_BLUETOOTH_SCO + 1))

typedef enum _SpeechState
{
    SpeechState_IDLE = 0,
    SpeechState_LISTENING = 1,
    SpeechState_PROCESSING = 2,
    SpeechState_SPEAKING = 3
} SpeechState;
#define _SpeechState_MIN SpeechState_IDLE
#define _SpeechState_MAX SpeechState_SPEAKING
#define _SpeechState_ARRAYSIZE ((SpeechState)(SpeechState_SPEAKING + 1))

typedef enum _SpeechInitiator_Type
{
    SpeechInitiator_Type_NONE = 0,
    SpeechInitiator_Type_PRESS_AND_HOLD = 1,
    SpeechInitiator_Type_TAP = 3,
    SpeechInitiator_Type_WAKEWORD = 4
} SpeechInitiator_Type;
#define _SpeechInitiator_Type_MIN SpeechInitiator_Type_NONE
#define _SpeechInitiator_Type_MAX SpeechInitiator_Type_WAKEWORD
#define _SpeechInitiator_Type_ARRAYSIZE ((SpeechInitiator_Type)(SpeechInitiator_Type_WAKEWORD + 1))

/* Struct definitions */
typedef struct _Dialog
{
    uint32_t id;
/* @@protoc_insertion_point(struct:Dialog) */
} Dialog;

typedef struct _NotifySpeechState
{
    SpeechState state;
/* @@protoc_insertion_point(struct:NotifySpeechState) */
} NotifySpeechState;

typedef PB_BYTES_ARRAY_T(256) SpeechInitiator_WakeWord_metadata_t;
typedef struct _SpeechInitiator_WakeWord
{
    uint32_t start_index_in_samples;
    uint32_t end_index_in_samples;
    bool near_miss;
    SpeechInitiator_WakeWord_metadata_t metadata;
/* @@protoc_insertion_point(struct:SpeechInitiator_WakeWord) */
} SpeechInitiator_WakeWord;

typedef struct _SpeechSettings
{
    AudioProfile audio_profile;
    AudioFormat audio_format;
    AudioSource audio_source;
/* @@protoc_insertion_point(struct:SpeechSettings) */
} SpeechSettings;

typedef struct _EndpointSpeech
{
    Dialog dialog;
/* @@protoc_insertion_point(struct:EndpointSpeech) */
} EndpointSpeech;

typedef struct _ProvideSpeech
{
    Dialog dialog;
/* @@protoc_insertion_point(struct:ProvideSpeech) */
} ProvideSpeech;

typedef struct _SpeechInitiator
{
    SpeechInitiator_Type type;
    SpeechInitiator_WakeWord wake_word;
/* @@protoc_insertion_point(struct:SpeechInitiator) */
} SpeechInitiator;

typedef struct _SpeechProvider
{
    SpeechSettings speech_settings;
    Dialog dialog;
/* @@protoc_insertion_point(struct:SpeechProvider) */
} SpeechProvider;

typedef struct _StopSpeech
{
    ErrorCode error_code;
    Dialog dialog;
/* @@protoc_insertion_point(struct:StopSpeech) */
} StopSpeech;

typedef struct _StartSpeech
{
    SpeechSettings settings;
    SpeechInitiator initiator;
    Dialog dialog;
    bool suppressEarcon;
/* @@protoc_insertion_point(struct:StartSpeech) */
} StartSpeech;

/* Default values for struct fields */

/* Initializer values for message structs */
#define Dialog_init_default                      {0}
#define SpeechSettings_init_default              {_AudioProfile_MIN, _AudioFormat_MIN, _AudioSource_MIN}
#define SpeechInitiator_init_default             {_SpeechInitiator_Type_MIN, SpeechInitiator_WakeWord_init_default}
#define SpeechInitiator_WakeWord_init_default    {0, 0, 0, {0, {0}}}
#define StartSpeech_init_default                 {SpeechSettings_init_default, SpeechInitiator_init_default, Dialog_init_default, 0}
#define SpeechProvider_init_default              {SpeechSettings_init_default, Dialog_init_default}
#define ProvideSpeech_init_default               {Dialog_init_default}
#define StopSpeech_init_default                  {_ErrorCode_MIN, Dialog_init_default}
#define EndpointSpeech_init_default              {Dialog_init_default}
#define NotifySpeechState_init_default           {_SpeechState_MIN}
#define Dialog_init_zero                         {0}
#define SpeechSettings_init_zero                 {_AudioProfile_MIN, _AudioFormat_MIN, _AudioSource_MIN}
#define SpeechInitiator_init_zero                {_SpeechInitiator_Type_MIN, SpeechInitiator_WakeWord_init_zero}
#define SpeechInitiator_WakeWord_init_zero       {0, 0, 0, {0, {0}}}
#define StartSpeech_init_zero                    {SpeechSettings_init_zero, SpeechInitiator_init_zero, Dialog_init_zero, 0}
#define SpeechProvider_init_zero                 {SpeechSettings_init_zero, Dialog_init_zero}
#define ProvideSpeech_init_zero                  {Dialog_init_zero}
#define StopSpeech_init_zero                     {_ErrorCode_MIN, Dialog_init_zero}
#define EndpointSpeech_init_zero                 {Dialog_init_zero}
#define NotifySpeechState_init_zero              {_SpeechState_MIN}

/* Field tags (for use in manual encoding/decoding) */
#define Dialog_id_tag                            1
#define NotifySpeechState_state_tag              1
#define SpeechInitiator_WakeWord_start_index_in_samples_tag 1
#define SpeechInitiator_WakeWord_end_index_in_samples_tag 2
#define SpeechInitiator_WakeWord_near_miss_tag   3
#define SpeechInitiator_WakeWord_metadata_tag    4
#define SpeechSettings_audio_profile_tag         1
#define SpeechSettings_audio_format_tag          2
#define SpeechSettings_audio_source_tag          3
#define EndpointSpeech_dialog_tag                1
#define ProvideSpeech_dialog_tag                 1
#define SpeechInitiator_type_tag                 1
#define SpeechInitiator_wake_word_tag            2
#define SpeechProvider_speech_settings_tag       1
#define SpeechProvider_dialog_tag                2
#define StopSpeech_error_code_tag                1
#define StopSpeech_dialog_tag                    2
#define StartSpeech_settings_tag                 1
#define StartSpeech_initiator_tag                2
#define StartSpeech_dialog_tag                   3
#define StartSpeech_suppressEarcon_tag           4

/* Struct field encoding specification for nanopb */
extern const pb_field_t Dialog_fields[2];
extern const pb_field_t SpeechSettings_fields[4];
extern const pb_field_t SpeechInitiator_fields[3];
extern const pb_field_t SpeechInitiator_WakeWord_fields[5];
extern const pb_field_t StartSpeech_fields[5];
extern const pb_field_t SpeechProvider_fields[3];
extern const pb_field_t ProvideSpeech_fields[2];
extern const pb_field_t StopSpeech_fields[3];
extern const pb_field_t EndpointSpeech_fields[2];
extern const pb_field_t NotifySpeechState_fields[2];

/* Maximum encoded size of messages (where known) */
#define Dialog_size                              6
#define SpeechSettings_size                      6
#define SpeechInitiator_size                     278
#define SpeechInitiator_WakeWord_size            273
#define StartSpeech_size                         299
#define SpeechProvider_size                      16
#define ProvideSpeech_size                       8
#define StopSpeech_size                          10
#define EndpointSpeech_size                      8
#define NotifySpeechState_size                   2

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define SPEECH_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
