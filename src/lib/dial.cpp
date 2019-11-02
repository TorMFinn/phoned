#include "phoned/dial.hpp"
#include <linux/gpio.h>
#include <fcntl.h>
#include <sys/types.h>

using namespace phoned;

const int GPIO_DIAL_ENABLE = 17;
const int GPIO_DIAL_COUNT = 22;

struct dial::Data {
    Data() {

    } 
    ~Data() {

    }
};

dial::dial() 
: m_data(new Data()) {
}

dial::~dial() {

}

void dial::set_number_entered_callback(number_entered_callback callback) {

}

void dial::set_digit_entered_callback(digit_entered_callback callback) {

}