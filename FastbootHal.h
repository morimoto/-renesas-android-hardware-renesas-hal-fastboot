/*
 * Copyright (C) 2019 GlobalLogic
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
#pragma once

#include <android/hardware/fastboot/1.0/IFastboot.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace renesas {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

enum RetVal {
    SUCCESS = 0,
    ACCESS_ERR,
    FORK_ERR,
    NORET_ERR,
    ERASE_ERR,
    OPDIR_ERR,
    OPFILE_ERR,
    PART_ERR,
    FIND_ERR,
};

class FastbootHal : public IFastboot {
public:
// ===== Commands follow.
    static constexpr char ERASE_COMMAND[] = "erase";

    // Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
    Return<void> getPartitionType(const hidl_string& partitionName, getPartitionType_cb _hidl_cb) override;
    Return<void> doOemCommand(const hidl_string& oemCmd, doOemCommand_cb _hidl_cb) override;
    Return<void> getVariant(getVariant_cb _hidl_cb) override;
    Return<void> getOffModeChargeState(getOffModeChargeState_cb _hidl_cb) override;
    Return<void> getBatteryVoltageFlashingThreshold(getBatteryVoltageFlashingThreshold_cb _hidl_cb) override;

private:
    RetVal eraseSSData();
    RetVal readPartitionType(const char *part_name, FileSystemType &type);
};

    // Methods from ::android::hidl::base::V1_0::IBase follow.
extern "C" IFastboot* HIDL_FETCH_IFastboot(const char* name);

}  // namespace renesas
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android
