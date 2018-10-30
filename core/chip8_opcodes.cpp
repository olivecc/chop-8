#include <cmath>        //std::ceil
#include <cstdint>      //uint16_t
#include <random>       //std::mt19937, std::random_device
#include "chip8.h"

using namespace chip8;

namespace
{
    int mod(int a, int b)
    {
        return (b + (a % b)) % b;
    }

    void opcode_throw(const char* m, int pc)
    {
        //pc is decremented due to previously being incremented in execute()
        throw cpu_exception(m, pc - BYTES_PER_OPCODE);
    }

    void bad_opcode(uint16_t pc)
    {
        opcode_throw("Invalid opcode", pc);
    }

    void bad_ram_access(uint16_t pc)
    {
        opcode_throw("Illegal RAM access", pc);
    }
}

void CPU::op_0nnn_()
{
    switch(nnn())
    {
    case(0x0E0):
        for(uint32_t& byte : framebuffer_) byte = argb_no_pixel_;
        break;

    case(0x0EE):
        if(sp_ == 0x0) 
            opcode_throw("00EE: Call stack underflow", pc_);
        pc_ =  stack_[--sp_];
        break;

    default:
        bad_opcode(pc_);
        break;
    }
}

void CPU::op_1nnn_()     
{
    pc_ = nnn();
}

void CPU::op_2nnn_()    
{
    if(sp_ >= STACK_MAX_SIZE) 
        opcode_throw("2nnn: Call stack overflow", pc_);
    stack_[sp_++] = pc_; 
    pc_ = nnn();
}

void CPU::op_3xkk_()   
{ 
    if(v_[x()] == kk()) 
    {
        pc_ += BYTES_PER_OPCODE; 
    }
}   

void CPU::op_4xkk_()  
{
    if(v_[x()] != kk())
    {
        pc_ += BYTES_PER_OPCODE;
    }
}
void CPU::op_5xy0_() 
{
    if(z() != 0x0) bad_opcode(pc_);

    if(v_[x()] == v_[y()])
    {
        pc_ += BYTES_PER_OPCODE;
    }
}

void CPU::op_6xkk_()
{ 
    v_[x()]  = kk(); 
}

void CPU::op_7xkk_() 
{ 
    v_[x()] += kk(); 
}

void CPU::op_8xyz_()
{
    const uint8_t old_Vx = v_[x()];
    const uint8_t pre_shift = v_[((new_8XYU_) ? x() : y())];

    switch(z())
    {
    case(0x0):
        v_[x()]  = v_[y()];
        break;

    case(0x1):
        v_[x()] |= v_[y()];
        break;

    case(0x2):
        v_[x()] &= v_[y()];
        break;

    case(0x3):
        v_[x()] ^= v_[y()];
        break;

    case(0x4):
        v_[x()] += v_[y()];
        v_[0xF] = ((old_Vx > v_[x()]) ? 1 : 0);
        break;

    case(0x5):
        v_[x()] -= v_[y()];
        v_[0xF] = ((old_Vx < v_[x()]) ? 0 : 1);
        break;

    case(0x6):
        v_[x()] = pre_shift >> 1; 
        v_[0xF] = ((pre_shift == (v_[x()] << 1)) ? 0 : 1);
        break;

    case(0x7):
        v_[x()] = (v_[y()] - v_[x()]);
        v_[0xF] = ((v_[y()] < v_[x()]) ? 0 : 1);
        break;

    case(0xE):
        v_[x()] = pre_shift << 1;
        v_[0xF] = ((pre_shift == (v_[x()] >> 1)) ? 0 : 1);
        break;
    }
}

void CPU::op_9xy0_()  
{
    if(z() != 0x0) bad_opcode(pc_); 
    
    if(v_[x()] != v_[y()])
    {
        pc_ += BYTES_PER_OPCODE;
    }
}

void CPU::op_Annn_()   
{
    i_ = nnn(); 
}

void CPU::op_Bnnn_() 
{
    pc_ = nnn() + v_[0x0];
}

void CPU::op_Cxkk_()
{
    //Random Number Generator
    //https://channel9.msdn.com/Events/GoingNative/2013/rand-Considered-Harmful
    //TODO thread-safe?
    static std::mt19937 rng{(std::random_device{})()};

    v_[x()] = (rng() & 0xFF) & kk();
}

void CPU::op_Dxyz_()
{
    v_[0xF] = 0;
       
    if((i_ + z() - 1 >= RAM_SIZE) && (z() > 0))
        bad_ram_access(pc_);

    for(unsigned int line_num = 0, 
                     pix_y = mod(v_[y()] + line_num, HEIGHT);
        line_num < z(); 
        ++line_num, pix_y = mod(pix_y + 1, HEIGHT))
    {
        uint8_t new_line = ram_[i_ + line_num];
        
        for(int col_num = 8-1, 
                     pix_x = mod(v_[x()] + col_num, WIDTH);
            col_num >= 0;
            --col_num, pix_x = mod(pix_x - 1, WIDTH))
        {
            uint32_t& dest =
                framebuffer_[pix_x + pix_y * WIDTH];

            uint32_t draw_pixel = 
                (new_line & 0x1) ? argb_pixel_ : argb_no_pixel_;

            //VF |= dest AND draw_pixel
            v_[0xF] |= ( ((dest == argb_pixel_) && (draw_pixel == argb_pixel_))
                         ? 1 : 0);

            //dest ^= draw_pixel;
            dest = ( ((dest == argb_pixel_) + (draw_pixel == argb_pixel_) == 1) 
                     ? argb_pixel_ : argb_no_pixel_);

            new_line >>= 1;
        }
    }
}

void CPU::op_Exkk_()
{
    if(!(v_[x()] < 0x10)) 
        opcode_throw("Exkk: non-nibble Vx (no equivalent key)", pc_);

    switch(kk())
    {
    case(0x9E):
        if(is_held_[v_[x()]])
        {
            pc_ += BYTES_PER_OPCODE;
        }
        break;

    case(0xA1):
        if(!is_held_[v_[x()]])
        {
            pc_ += BYTES_PER_OPCODE;
        }
        break;

    default:
        bad_opcode(pc_);
        break;
    }
}

void CPU::op_Fxkk_()
{
    bool key_pressed = false;

    switch(kk())
    {
    case(0x07):
        v_[x()] = static_cast<uint8_t>(std::ceil(delay_timer_));
        break;

    case(0x0A): 
        if(old_press_FX0A_)
        {
            for(unsigned int key = static_cast<unsigned int>(Keys::KEY_0); 
                key < static_cast<unsigned int>(Keys::QUANTITY_OF_KEYS); 
                ++key)
            {
                if(is_held_[key] == !key_up_FX0A_)
                {
                    key_pressed = true;
                    v_[x()] = key;
                    break;
                }
            }
            if(!key_pressed) pc_ -= BYTES_PER_OPCODE;
        }
        else
        {
            paused_ = true;
            pc_ -= BYTES_PER_OPCODE;
        }
        break;

    case(0x15):
        delay_timer_ = v_[x()];
        break;

    case(0x18):
        sound_timer_ = v_[x()];
        break;

    case(0x1E):
        i_ += v_[x()];
        break;

    case(0x29):
        i_ = font_address(v_[x()]);
        break;

    case(0x33):
        if((i_ < PROGRAM_BEGIN) || (i_ + 2 >= RAM_SIZE))
            bad_ram_access(pc_);
        ram_[i_ + 0] = (v_[x()] / 100) % 10;
        ram_[i_ + 1] = (v_[x()] /  10) % 10;
        ram_[i_ + 2] = (v_[x()] /   1) % 10;
        break;

    case(0x55): 
        if(((i_ < PROGRAM_BEGIN) || (i_ + x() >= RAM_SIZE)) && (x() > 0))
            bad_ram_access(pc_);
        for(unsigned int iter = 0; iter <= x(); ++iter)
        {
            ram_[i_ + iter] = v_[iter];
        }
        if(!new_FXU5_) i_ += x() + 1;
        break;

    case(0x65): 
        if((i_ + x() >= RAM_SIZE) && (x() > 0))
            bad_ram_access(pc_);
        for(unsigned int iter = 0; iter <= x(); ++iter)
        {
            v_[iter] = ram_[i_ + iter];
        }
        if(!new_FXU5_) i_ += x() + 1;
        break;

    default:
        bad_opcode(pc_);
        break;
    }
}
