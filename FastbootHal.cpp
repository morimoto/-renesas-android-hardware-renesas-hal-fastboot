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
#include <android-base/strings.h>
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <string>

#include "FastbootHal.h"

#define LOG_TAG "fastboot-hal"
#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace renesas {


// ===== Constants for C functions follow.
static constexpr char HYPER_CA[] = "hyper_ca";
static constexpr char HYPER_CA_PATH[] = "/bin/hyper_ca";
static constexpr char FDT_PARTTABLE_PATH[] = "/proc/device-tree/android/partitions";
static constexpr auto PARTITION_NAME_LEN = 32; // Taken from u-boot

static constexpr auto IPL_ERASE_TIMEOUT = 15;

// ========== Private methods ==========

/* Erases Secure Storage. Creates child process and replaces it with hyper_ca.
 * hyper_ca is an app which erases Secure Storage with corresponding TA
 */
RetVal FastbootHal::eraseSSData() {
    LOG(INFO) << "Erasing Secure Storage..";
    if (access(HYPER_CA_PATH, X_OK)) {
        LOG(INFO) << "File doesn't exist or can't be executed";
        return RetVal::ACCESS_ERR;
    }

    pid_t child = fork();
    if (child == -1) {
        LOG(INFO) << "Process can't be forked";
        return RetVal::FORK_ERR;
    } else if (child == 0) {
        execl(HYPER_CA_PATH, HYPER_CA, "-e", "SSTDATA", NULL);
        /* Should not return */
        return RetVal::NORET_ERR;
    }

    int pid_status;
    for (int i = 0; i < IPL_ERASE_TIMEOUT; ++i) {
        if (waitpid(child, &pid_status, WNOHANG) != 0) {
            break;
        }
        sleep(1);
    }

    if (!WIFEXITED(pid_status) || WEXITSTATUS(pid_status) != 0) {
        LOG(INFO) << "Erase Error (" << WEXITSTATUS(pid_status) << ")";
        return RetVal::ERASE_ERR;
    }

    LOG(INFO) << "SSTDATA Erased";
    return RetVal::SUCCESS;
}

/* Returns negative on error, and 0 on success.
 * type will contain result.
 */
RetVal FastbootHal::readPartitionType(const char *part_name, FileSystemType &type) {
    DIR *s_dir;
    struct dirent *dir;
    RetVal res = RetVal::FIND_ERR;

    LOG(INFO) << part_name;
    s_dir = opendir(FDT_PARTTABLE_PATH);
    if (!s_dir) {
        LOG(INFO) << "Can't open fdt dir";
        return RetVal::OPDIR_ERR;
    }

    while ((dir = readdir(s_dir)) != NULL) {
        if (!strncmp(dir->d_name, part_name, PARTITION_NAME_LEN)) {
            std::string f_name(FDT_PARTTABLE_PATH);
            f_name = f_name + '/' + dir->d_name + '/' + "type";
            LOG(INFO) << f_name << "|";

            std::ifstream i_strm(f_name, std::ios::in);
            if (i_strm.is_open()) {
                std::string s_type;
                std::getline(i_strm, s_type, '\0');
                LOG(INFO) << s_type << "|";

                if (s_type == "raw") {
                    type = FileSystemType::RAW;
                    res = RetVal::SUCCESS;
                } else if (s_type == "ext4") {
                    type = FileSystemType::EXT4;
                    res = RetVal::SUCCESS;
                } else if (s_type == "f2fs") {
                    type = FileSystemType::F2FS;
                    res = RetVal::SUCCESS;
                } else {
                    LOG(INFO) << "Unknown partition type";
                    res = RetVal::PART_ERR;
                }
            } else {
                LOG(INFO) << "Can't open fdt-file with partition type";
                res = RetVal::OPFILE_ERR;
            }
            break;
        }
    }
    closedir(s_dir);
    return res;
}

// ========== Public methods ==========

// Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
Return<void> FastbootHal::getPartitionType(const hidl_string& partitionName,
                                           getPartitionType_cb _hidl_cb) {
    Result res;
    FileSystemType res_type;
    res_type = FileSystemType::RAW;
    if (readPartitionType(partitionName.c_str(), res_type)) {
        res.message = "Partition unknown or doesn't require reformat";
        res.status = Status::FAILURE_UNKNOWN;
    } else {
        res.message = "Success";
        res.status = Status::SUCCESS;
    }
    _hidl_cb(res_type, res);
    return Void();
}

Return<void> FastbootHal::doOemCommand(const hidl_string& oemCmd,
                                       doOemCommand_cb _hidl_cb) {
    Result res;
    std::string cmd(oemCmd.c_str());

    LOG(INFO) << "oem command: " << cmd;
    if (!android::base::StartsWith(cmd, "oem ")) {
        // This branch is never used during normal workflow
	res.status = Status::INVALID_ARGUMENT;
        res.message = "Command is not oem command. Abort.";
        _hidl_cb(res);
        return Void();
    }
    cmd.erase(0, 4); // remove "oem " indent
    if (cmd == ERASE_COMMAND) {
        if (!eraseSSData()) {
            res.status = Status::SUCCESS;
            res.message = "Optee Secure Storage erased.";
        } else {
            res.status = Status::FAILURE_UNKNOWN;
            res.message = "Erasing error occured!";
        }
    } else {
        res.status = Status::INVALID_ARGUMENT;
        res.message = "Unknown command";
    }
    _hidl_cb(res);
    return Void();
}

Return<void> FastbootHal::getVariant(getVariant_cb _hidl_cb) {
    // FIXME: I don't even know what the OEM-defined string is.
    hidl_string variant = "OEM-defined string";
    Result res;
    res.status = Status::SUCCESS;
    res.message = "getVariant stub";
    _hidl_cb(variant, res);
    return Void();
}

Return<void> FastbootHal::getOffModeChargeState(getOffModeChargeState_cb
                 _hidl_cb) {
    // FIXME: this function is a stub for off-mode-charging. It seems, that we
    // don't have off-mode-charge because we don't have a battery.
    bool offModeCharge = false;
    Result res;
    res.status = Status::SUCCESS;
    res.message = "Off-mode-charging stub";
    _hidl_cb(offModeCharge, res);
    return Void();
}

Return<void> FastbootHal::getBatteryVoltageFlashingThreshold(
                getBatteryVoltageFlashingThreshold_cb _hidl_cb) {
    // FIXME: this function is a stub for battery voltage. Renesas devices
    // don't have battery at all. Minimum posible threshold is 0.
    uint32_t batteryVoltage = 0;
    Result res;
    res.status = Status::SUCCESS;
    res.message = "Battery voltage stub";
    _hidl_cb(batteryVoltage, res);
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IFastboot* HIDL_FETCH_IFastboot(const char* /* name */) {
    return new FastbootHal();
}

}  // namespace renesas
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android
