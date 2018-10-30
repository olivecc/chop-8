#ifndef CHIP8_H_OLIVECC
#define CHIP8_H_OLIVECC

#include <cstdint>      //uint8_t, uint16_t, uint32_t
#include <stdexcept>    //std::runtime_error
//TODO README note must support cstdint types listed above
//TODO reference laurence, cowgod, mastering, etc
//TODO note focus on compatibility, not accuracy
//TODO note timers tied to CPU cycles
//TODO driver sound
//TODO thread safety

namespace chip8
{
    class cpu_exception : public std::runtime_error 
    {
    public:
        const int last_address;    
        cpu_exception(const char* what_arg, int pc = -1) 
            : std::runtime_error{what_arg}, last_address{pc} {}
        cpu_exception(const std::string& what_arg, int pc = -1)  
            : cpu_exception(what_arg.c_str(), pc) {}
        ~cpu_exception() = default;
    };

    enum class Keys : unsigned int
    {
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7,
        KEY_8, KEY_9, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F,
        QUANTITY_OF_KEYS
    };
    
    enum : unsigned int //CHIP-8 constants
    {
        //Dimensions
        WIDTH = 64,
        HEIGHT = 32,
        //CPU quantities
        BYTES_PER_OPCODE = 2,
        BYTES_PER_CHAR_SPRITE = 5,
        STACK_MAX_SIZE = 16,        //as per original; sometimes 16? TODO
        //Addresses
        PROGRAM_BEGIN   = 0x200, 
        RAM_SIZE        = 0x1000,
        PROGRAM_SIZE    = RAM_SIZE - PROGRAM_BEGIN,
        //Miscellaneous
        DEFAULT_CLOCK_SPEED_HZ = 500,
        DEFAULT_ARGB_PIXEL    = 0xFFFFFFFF,
        DEFAULT_ARGB_NO_PIXEL = 0xFF000000,
    };

    enum Flags : unsigned int
    {
        KEY_UP_FX0A         = 1U << 0,
        OLD_PRESS_FX0A      = 1U << 1,
        NEW_8XYU            = 1U << 2,
        NEW_FXU5            = 1U << 3,
        NEW_OPCODES         = NEW_8XYU | NEW_FXU5,
        KEY_DOWN_FX0A       = 0,
        NEW_PRESS_FX0A      = 0,
        OLD_8XYU            = 0,
        OLD_FXU5            = 0,
        OLD_OPCODES         = OLD_8XYU | OLD_FXU5,
        NO_FLAGS            = OLD_OPCODES | KEY_DOWN_FX0A | NEW_PRESS_FX0A
    };

    class CPU
    {
    private:
        //Random Access Memory: [0x0, PROGRAM_BEGIN) reserved for 
        //interpreter, [PROGRAM_BEGIN, RAM_SIZE) reserved for CHIP-8 program
        uint8_t ram_[RAM_SIZE];

        //Framebuffer
        uint32_t framebuffer_[WIDTH * HEIGHT];

        //Registers
        uint8_t v_[0x10];           //Addressable by a nibble
        uint16_t i_;
        double delay_timer_;
        double sound_timer_;

        //Program Counter/Current opcode
        uint16_t pc_;
        uint16_t opcode_;

        //Stack (Pointer)
        uint16_t stack_[STACK_MAX_SIZE];
        uint8_t sp_;


        //Operations: for the specification of nnn etc., two alternatives are
        //macro substitutions, and calculation of their values in execute() 
        //followed by passing each value into each opcode function
        uint8_t first_nibble()  {return 0xF   & (opcode_ >> 12);}
        uint32_t nnn()          {return 0xFFF &  opcode_;}
        uint8_t x()             {return 0xF   & (opcode_ >>  8);}
        uint8_t y()             {return 0xF   & (opcode_ >>  4);}
        uint8_t z()             {return 0xF   &  opcode_;}
        uint16_t kk()           {return 0xFF  &  opcode_;}


        //Opcodes
        void op_0nnn_(void);    
              //00E0              CLS:  Clear display
              //00EE              RET:  Return from subroutine
        void op_1nnn_(void);    //JP:   Jump to location nnn
        void op_2nnn_(void);    //CALL: Call subroutine at nnn
        void op_3xkk_(void);    //SE:   Skip next instruction iff Vx == kk
        void op_4xkk_(void);    //SNE:  Skip next instruction iff Vx != kk
        void op_5xy0_(void);    //SE:   Skip next instruction iff Vx == Vy 
        void op_6xkk_(void);    //LD:   Vx := kk
        void op_7xkk_(void);    //ADD:  Vx := Vx + kk
        void op_8xyz_(void);    
              //8xy0              LD:   Vx := Vy
              //8xy1              OR:   Vx := Vx OR Vy
              //8xy2              AND:  Vx := Vx AND Vy
              //8xy3              XOR:  Vx := Vx XOR Vy
              //8xy4              ADD:  Vx := Vx + Vy, VF = carry flag
              //8xy5              SUB:  Vx := Vx - Vy, VF = NOT borrow flag
              //8xy6              SHR:  Right-shift Vu, VF = truncated bit 
              //                        (u == ((NEW_8XYU flag set) ? x : y))
              //8xy7              SUBN: Vx := Vy - Vx, VF = NOT borrow flag
              //8xyE              SHL:  Left-shift Vu, VF = truncated bit
              //                        (u == ((NEW_8XYU flag set) ? x : y))
        void op_9xy0_(void);    //SNE:  Skip next instruction iff Vx != Vy
        void op_Annn_(void);    //LD:   I := nnn
        void op_Bnnn_(void);    //JP:   Jump to location nnn + V0
        void op_Cxkk_(void);    //RND:  Vx = random byte AND kk
        void op_Dxyz_(void);    //DRW:  Draw z-byte sprite from I at 
              //                        (Vx,Vy), VF := collision. Each byte is 
              //                        a horizontal line of bit-pixels.
        void op_Exkk_(void);     
              //Ex9E              SKP:  Skip next instruction iff key Vx held
              //ExA1              SKNP: Skip next instruction iff key Vx not held
        void op_Fxkk_(void);    
              //Fx07              LD:   Vx := delay timer value
              //Fx0A              LD:   'Await' keypress, store value in Vx
              //                        (Event queried == (KEY_UP_FX0A flag set)
              //                         ? key up : key down)
              //                        ('Await' == (OLD_PRESS_FX0A flag set)
              //                         ? check if key event currently applied
              //                         : wait for new key event)
              //Fx15              LD:   delay timer := Vx
              //Fx18              LD:   sound timer := Vx
              //Fx1E              ADD:  I := I + Vx
              //Fx29              LD:   I := location of sprite for digit Vx
              //Fx33              LD:   Stores decreasing decimal digits of Vx 
              //                        in [I], [I+1], [I+2]
              //Fx55              LD:   Load V0-Vx into [I]-[I+x]
              //                        (I := I + x + 1 iff NEW_FXU5 flag set)
              //Fx65              LD:   Load [I]-[I+x] into V0-Vx
              //                        (I := I + x + 1 iff NEW_FXU5 flag set)
       
        bool is_held_[static_cast<unsigned int>(Keys::QUANTITY_OF_KEYS)];
         
        bool paused_;

        //Option flags: for changing how opcodes work (see above for specifics).
        //Required due to ambiguities in/between CHIP-8 specification(s)
        //available online. I am aware that the canonical solution would be to
        //follow the original COSMAC VIP implementation, however this would 
        //break compatibility with many CHIP-8 programs available online, so the
        //choice is presented to the front-end implementer.
        bool key_up_FX0A_;
        bool old_press_FX0A_;
        bool new_8XYU_;
        bool new_FXU5_;

        //Settings
        unsigned int clock_speed_hz_;
        uint32_t argb_pixel_;
        uint32_t argb_no_pixel_;

    public:
        CPU(const void* program, size_t size, Flags flags = NO_FLAGS);
        CPU& execute();
        CPU& pump_input(Keys, bool);
        uint32_t* framebuffer() { return framebuffer_; }
        bool is_sound() { return sound_timer_ > 0; }


        //Getters/setters for settings
        unsigned int get_clock_speed_hz() { return clock_speed_hz_; }
        CPU& set_clock_speed_hz(unsigned int set)
        { 
            clock_speed_hz_ = (set > 0) ? 
                set : throw cpu_exception("Clock speed set to zero"); 
            return *this; 
        }

        uint32_t get_argb_pixel() { return argb_pixel_; }
        CPU& set_argb_pixel(uint32_t set)
        { argb_pixel_ = set; return *this; }

        uint32_t get_argb_no_pixel() { return argb_no_pixel_; }
        CPU& set_argb_no_pixel(uint32_t set)
        { argb_no_pixel_ = set; return *this; }
    };

    uint16_t font_address(unsigned int ch);
}
#endif //CHIP8_H_OLIVECC
