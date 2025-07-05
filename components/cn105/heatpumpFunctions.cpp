#include "cn105.h"
#include "heatpumpFunctions.h"

using namespace esphome;
//#region heatpump_functions fonctions clim

void CN105Climate::getFunctions() {
    ESP_LOGI(TAG, "Getting the list of functions...");

    functions.clear();

    uint8_t packet1[PACKET_LEN] = {};

    prepareInfoPacket(packet1, PACKET_LEN);
    packet1[5] = FUNCTIONS_GET_PART1;
    packet1[21] = checkSum(packet1, 21);

    ESP_LOGI(TAG, "Sending function codes request part 1 (0x%02X)", FUNCTIONS_GET_PART1);
    writePacket(packet1, PACKET_LEN);

    // Read command will issue part 2.
}

void CN105Climate::getFunctionsPart2() {
    ESP_LOGI(TAG, "Getting the list of functions part 2...");

    uint8_t packet2[PACKET_LEN] = {};

    prepareInfoPacket(packet2, PACKET_LEN);
    packet2[5] = FUNCTIONS_GET_PART2;
    packet2[21] = checkSum(packet2, 21);

    ESP_LOGI(TAG, "Sending function codes request part 2 (0x%02X)", FUNCTIONS_GET_PART2);
    writePacket(packet2, PACKET_LEN);
}

void CN105Climate::functionsArrived() {

    // Called after 2nd packet has arrived.

    ESP_LOGI(TAG, "Function codes response received");
    ESP_LOGI(TAG, "Functions valid: %s", functions.isValid() ? "true" : "false");

    char states[256];
    states[0] = '\0';  // Initialize as empty string
    size_t remaining = sizeof(states);
    char* pos = states;

    heatpumpFunctionCodes codes = functions.getAllCodes();
    ESP_LOGI(TAG, "Function codes received:");
    int validCodes = 0;
    for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
        if (codes.valid[i]) {
            int code = codes.code[i];
            int value = functions.getValue(code);
            if (value > 0) {  // only values 1, 2, 3 are valid -- 0 values mean something the device does not support
                ESP_LOGI(TAG, "  Code %i: Value %i", code, value);
                validCodes++;
                int written = snprintf(pos, remaining, "%i: %i ", code, value);
                if (written < 0 || static_cast<size_t>(written) >= remaining) {
                    // Buffer full or error
                    break;
                }
                pos += written;
                remaining -= written;
            }
        }
    }

    if (validCodes == 0) {
        ESP_LOGW(TAG, "No valid function codes found - your model may not support function codes");
        if (this->Functions_sensor_ != nullptr) {
            this->Functions_sensor_->publish_state("No function codes available");
        }
    } else {
        ESP_LOGI(TAG, "Found %d valid function codes", validCodes);
        // Publish the results of all the codes in the Functions sensor
        if (this->Functions_sensor_ != nullptr) {
            this->Functions_sensor_->publish_state(states);
        }
    }
}

bool CN105Climate::setFunctions(heatpumpFunctions const& functions) {
    if (!functions.isValid()) {
        return false;
    }

    uint8_t packet1[PACKET_LEN] = {};
    uint8_t packet2[PACKET_LEN] = {};

    prepareSetPacket(packet1, PACKET_LEN);
    packet1[5] = FUNCTIONS_SET_PART1;

    prepareSetPacket(packet2, PACKET_LEN);
    packet2[5] = FUNCTIONS_SET_PART2;

    functions.getData1(&packet1[6]);
    functions.getData2(&packet2[6]);

    // sanity check, we expect data byte 15 (index 20) to be 0
    if (packet1[20] != 0 || packet2[20] != 0)
        return false;

    // make sure all the other data bytes are set
    for (int i = 6; i < 20; ++i) {
        if (packet1[i] == 0 || packet2[i] == 0)
            return false;
    }

    packet1[21] = checkSum(packet1, 21);
    packet2[21] = checkSum(packet2, 21);
    /*
        while (!canSend(false)) {
            //esphome::CUSTOM_DELAY(10);
            CUSTOM_DELAY(10);
        }*/
    ESP_LOGD(TAG, "sending a setFunctions packet part 1");
    writePacket(packet1, PACKET_LEN);
    //readPacket();

    /*while (!canSend(false)) {
        //esphome::CUSTOM_DELAY(10);
        CUSTOM_DELAY(10);
    }*/
    ESP_LOGD(TAG, "sending a setFunctions packet part 2");
    writePacket(packet2, PACKET_LEN);
    //readPacket();

    return true;
}


heatpumpFunctions::heatpumpFunctions() {
    clear();
}

bool heatpumpFunctions::isValid() const {
    return _isValid1 && _isValid2;
}

void heatpumpFunctions::setData1(uint8_t* data) {
    ESP_LOGI(TAG, "Setting function data 1:");
    for (int i = 0; i < 15; i++) {
        ESP_LOGI(TAG, "  data1[%d] = 0x%02X", i, data[i]);
    }
    memcpy(raw, data, 15);
    _isValid1 = true;
    ESP_LOGI(TAG, "Function data 1 set, isValid1 = true");
}

void heatpumpFunctions::setData2(uint8_t* data) {
    ESP_LOGI(TAG, "Setting function data 2:");
    for (int i = 0; i < 15; i++) {
        ESP_LOGI(TAG, "  data2[%d] = 0x%02X", i, data[i]);
    }
    memcpy(raw + 15, data, 15);
    _isValid2 = true;
    ESP_LOGI(TAG, "Function data 2 set, isValid2 = true");
}

void heatpumpFunctions::getData1(uint8_t* data) const {
    memcpy(data, raw, 15);
}

void heatpumpFunctions::getData2(uint8_t* data) const {
    memcpy(data, raw + 15, 15);
}

void heatpumpFunctions::clear() {
    memset(raw, 0, sizeof(raw));
    _isValid1 = false;
    _isValid2 = false;
}

int heatpumpFunctions::getCode(uint8_t b) {
    return ((b >> 2) & 0xff) + 100;
}

int heatpumpFunctions::getValue(uint8_t b) {
    return b & 3;
}

int heatpumpFunctions::getValue(int code) {
    if (code > 128 || code < 101) {
        ESP_LOGD(TAG, "Function code %d out of range (101-128)", code);
        return 0;
    }

    for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
        if (getCode(raw[i]) == code) {
            int value = getValue(raw[i]);
            ESP_LOGD(TAG, "Found function code %d at index %d with value %d", code, i, value);
            return value;
        }
    }

    ESP_LOGD(TAG, "Function code %d not found", code);
    return 0;
}

bool heatpumpFunctions::setValue(int code, int value) {
    if (code > 128 || code < 101)
        return false;

    if (value < 1 || value > 3)
        return false;

    for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
        if (getCode(raw[i]) == code) {
            raw[i] = ((code - 100) << 2) + value;
            return true;
        }
    }

    return false;
}

heatpumpFunctionCodes heatpumpFunctions::getAllCodes() {
    heatpumpFunctionCodes result;
    ESP_LOGI(TAG, "Getting all function codes from raw data:");
    for (int i = 0; i < MAX_FUNCTION_CODE_COUNT; ++i) {
        ESP_LOGI(TAG, "  raw[%d] = 0x%02X", i, raw[i]);
        int code = getCode(raw[i]);
        result.code[i] = code;
        result.valid[i] = (code >= 101 && code <= 128);
        ESP_LOGI(TAG, "  Code %d: %d (valid: %s)", i, code, result.valid[i] ? "true" : "false");
    }

    return result;
}

bool heatpumpFunctions::operator==(const heatpumpFunctions& rhs) {
    return this->isValid() == rhs.isValid() && memcmp(this->raw, rhs.raw, MAX_FUNCTION_CODE_COUNT * sizeof(int)) == 0;
}

bool heatpumpFunctions::operator!=(const heatpumpFunctions& rhs) {
    return !(*this == rhs);
}
//#endregion heatpump_functions
