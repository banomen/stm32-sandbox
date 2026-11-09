// I2C-слейв на I2C2 (PB10 = SCL, PB11 = SDA), общий для обеих плат.
//
// Одно сообщение = одна транзакция записи мастера (START, адрес+W, байты, STOP).
// Чтение со слейва не поддерживается: мастер получит 0xFF.
#pragma once

#include "common.hpp"

void i2c_slave_init(uint8_t address);

// Забрать принятое сообщение. false — пока ничего не пришло.
bool i2c_slave_take(Msg& out);
