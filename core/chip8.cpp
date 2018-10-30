#include <array>        //std::array
#include <cstdint>      //uint8_t, uint16_t
#include <cstring>      //std::memcpy, size_t
#include <exception>    //std::out_of_range

#include "chip8.h"

using namespace chip8;

namespace
{
    constexpr uint8_t font[0x10 * BYTES_PER_CHAR_SPRITE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,       0x20, 0x60, 0x20, 0x20, 0x70, //0,1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,       0xF0, 0x10, 0xF0, 0x10, 0xF0, //2,3
        0x90, 0x90, 0xF0, 0x10, 0x10,       0xF0, 0x80, 0xF0, 0x10, 0xF0, //4,5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,       0xF0, 0x10, 0x20, 0x40, 0x40, //6,7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,       0xF0, 0x90, 0xF0, 0x10, 0xF0, //8,9
        0xF0, 0x90, 0xF0, 0x90, 0x90,       0xE0, 0x90, 0xE0, 0x90, 0xE0, //A,B
        0xF0, 0x80, 0x80, 0x80, 0xF0,       0xE0, 0x90, 0x90, 0x90, 0xE0, //C,D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,       0xF0, 0x80, 0xF0, 0x80, 0x80  //E,F
    };
}

uint16_t chip8::font_address(unsigned int ch)
{
    return BYTES_PER_CHAR_SPRITE * ch;
}

CPU::CPU(const void* program, size_t size, Flags flags)
        : i_{}, delay_timer_{}, sound_timer_{}, pc_{PROGRAM_BEGIN}, opcode_{},
          sp_{}, paused_{}, is_held_{},
          argb_pixel_       {DEFAULT_ARGB_PIXEL}, 
          argb_no_pixel_    {DEFAULT_ARGB_NO_PIXEL}, 
          clock_speed_hz_   {DEFAULT_CLOCK_SPEED_HZ},
          new_8XYU_         (flags & NEW_8XYU),
          new_FXU5_         (flags & NEW_FXU5),
          key_up_FX0A_      (flags & KEY_UP_FX0A),
          old_press_FX0A_   (flags & NEW_PRESS_FX0A)
{
    if(size > PROGRAM_SIZE) 
        throw cpu_exception("CHIP-8 program too large", pc_);

    //Populate interpreter RAM with font sprites at appropriate locations
    for(int ch = 0x0; ch < 0x10; ++ch)
    {
        unsigned int addr = font_address(ch);

        for(int b = 0; b < BYTES_PER_CHAR_SPRITE; ++b)
        {
            ram_[addr + b] = font[ch * BYTES_PER_CHAR_SPRITE + b];
        }
    }

    //Copy CHIP-8 program
    std::memcpy(ram_ + PROGRAM_BEGIN, program, size * sizeof(uint8_t));

    //Clear framebuffer
    for(uint32_t& pixel : framebuffer_)
    {
        pixel = argb_no_pixel_;
    }
}

CPU& CPU::pump_input(Keys key_pressed, bool is_held)
{
    unsigned int key = static_cast<unsigned int>(key_pressed);

    if(paused_ && 
       (is_held_[key] == key_up_FX0A_) && 
       (is_held       != key_up_FX0A_))
    {
        paused_ = false;
        v_[x()] = key;
        pc_ += BYTES_PER_OPCODE;
    }

    is_held_[key] = is_held;

    return *this;
}

CPU& CPU::execute() 
{
    static constexpr double d_s_timer_tick_speed_hz = 60.0;
    double ticks = d_s_timer_tick_speed_hz / clock_speed_hz_;
    delay_timer_ -= ticks;
    sound_timer_ -= ticks;
    if(delay_timer_ < 0) delay_timer_ = 0;
    if(sound_timer_ < 0) sound_timer_ = 0;

    if(!paused_)
    {
        //Fetch instructions
        if((pc_ >= RAM_SIZE - 1) || (pc_ < PROGRAM_BEGIN))
        {
            throw cpu_exception("PC address is invalid, opcode can't be fetched", 
                                pc_);
        }
        opcode_ = (ram_[pc_] << 8) + ram_[pc_ + 1];
        pc_ += BYTES_PER_OPCODE; 

        //Jump table: a (large) switch statement in execute() would be an
        //alternative, but a jump table is chosen for consistency with
        //other emulators (with more complicated opcodes)
        using chip8_func_ptr = void(CPU::*)();
        constexpr static chip8_func_ptr func_table_[0x10] = {
            &CPU::op_0nnn_, &CPU::op_1nnn_, 
            &CPU::op_2nnn_, &CPU::op_3xkk_,
            &CPU::op_4xkk_, &CPU::op_5xy0_, 
            &CPU::op_6xkk_, &CPU::op_7xkk_,
            &CPU::op_8xyz_, &CPU::op_9xy0_,
            &CPU::op_Annn_, &CPU::op_Bnnn_,
            &CPU::op_Cxkk_, &CPU::op_Dxyz_, 
            &CPU::op_Exkk_, &CPU::op_Fxkk_
        };

        chip8_func_ptr op = func_table_[first_nibble()];
        (this->*op)();
    }

    return *this;
}




