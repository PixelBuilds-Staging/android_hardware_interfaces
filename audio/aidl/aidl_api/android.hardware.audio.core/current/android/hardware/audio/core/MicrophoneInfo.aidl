/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.audio.core;
@JavaDerive(equals=true, toString=true) @VintfStability
parcelable MicrophoneInfo {
  @utf8InCpp String id;
  android.media.audio.common.AudioDevice device;
  android.hardware.audio.core.MicrophoneInfo.Location location = android.hardware.audio.core.MicrophoneInfo.Location.UNKNOWN;
  int group = -1;
  int indexInTheGroup = -1;
  @nullable android.hardware.audio.core.MicrophoneInfo.Sensitivity sensitivity;
  android.hardware.audio.core.MicrophoneInfo.Directionality directionality = android.hardware.audio.core.MicrophoneInfo.Directionality.UNKNOWN;
  android.hardware.audio.core.MicrophoneInfo.FrequencyResponsePoint[] frequencyResponse;
  @nullable android.hardware.audio.core.MicrophoneInfo.Coordinate position;
  @nullable android.hardware.audio.core.MicrophoneInfo.Coordinate orientation;
  const int GROUP_UNKNOWN = -1;
  const int INDEX_IN_THE_GROUP_UNKNOWN = -1;
  @Backing(type="int") @VintfStability
  enum Location {
    UNKNOWN = 0,
    MAINBODY = 1,
    MAINBODY_MOVABLE = 2,
    PERIPHERAL = 3,
  }
  @VintfStability
  parcelable Sensitivity {
    float leveldBFS;
    float maxSpldB;
    float minSpldB;
  }
  @Backing(type="int") @VintfStability
  enum Directionality {
    UNKNOWN = 0,
    OMNI = 1,
    BI_DIRECTIONAL = 2,
    CARDIOID = 3,
    HYPER_CARDIOID = 4,
    SUPER_CARDIOID = 5,
  }
  @VintfStability
  parcelable FrequencyResponsePoint {
    float frequencyHz;
    float leveldB;
  }
  @VintfStability
  parcelable Coordinate {
    float x;
    float y;
    float z;
  }
}