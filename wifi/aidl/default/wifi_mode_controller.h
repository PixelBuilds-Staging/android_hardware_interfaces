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

#ifndef WIFI_MODE_CONTROLLER_H_
#define WIFI_MODE_CONTROLLER_H_

#include <aidl/android/hardware/wifi/IWifi.h>
#include <wifi_hal/driver_tool.h>

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace mode_controller {

/**
 * Class that encapsulates all firmware mode configuration.
 * This class will perform the necessary firmware reloads to put the chip in the
 * required state (essentially a wrapper over DriverTool).
 */
class WifiModeController {
  public:
    WifiModeController();
    virtual ~WifiModeController() = default;

    // Checks if a firmware mode change is necessary to support the specified
    // iface type operations.
    virtual bool isFirmwareModeChangeNeeded(IfaceType type);
    virtual bool initialize();
    // Change the firmware mode to support the specified iface type operations.
    virtual bool changeFirmwareMode(IfaceType type);
    // Unload the driver. This should be invoked whenever |IWifi.stop()| is
    // invoked.
    virtual bool deinitialize();

  private:
    std::unique_ptr<::android::wifi_hal::DriverTool> driver_tool_;
};

}  // namespace mode_controller
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_MODE_CONTROLLER_H_
