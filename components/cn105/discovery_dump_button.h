#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

namespace esphome {

    class DiscoveryDumpButton : public button::Button, public Component {
    public:
        DiscoveryDumpButton() {}

        void add_press_action(std::function<void()>&& action) {
            this->press_action_ = std::move(action);
        }

    protected:
        void press_action() override {
            if (this->press_action_) {
                this->press_action_();
            }
        }

    private:
        std::function<void()> press_action_;
    };

} 