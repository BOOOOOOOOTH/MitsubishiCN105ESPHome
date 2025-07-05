#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome {

    class DiscoveryDumpButton : public button::Button, public Component {
    public:
        DiscoveryDumpButton() {}

    protected:
        void press_action() override {
            // This will be handled by the lambda in the Python code
        }
    };

} 