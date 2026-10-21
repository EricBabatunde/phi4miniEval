#include <stdint.h>

void configure_motor_channel(uint8_t *control_reg, uint8_t mode, uint8_t enable) {
    if (control_reg == NULL || control_reg == NULL) {
        return; // Check for NULL pointer
    }

    // Set bits [3:2] for mode (values 0-3)
    // Clear bits [7:4] and bit [1] if they are not zero.
    control_reg[0] = (control_reg[0] & 0xC7) | (mode << 3);  // Clear 7th, 6th, and 1st bits if not zero
    
    // Set bit [0] to enable (0 or 1)
    control_reg[0] = (control_reg[0] & 0x7F) | (enable << 0);
}

int main() {
    uint8_t control_reg = 0x00; // Initialize control register to 0

    // Example usage
    configure_motor_channel(&control_reg, 2, 1);  // Set mode to 2 and enable
    std::cout << "Control Register after configuring mode 2 and enabling: " << std::hex << control_reg << std::endl;

    return 0;
}
