#include "chip8.h"
#include "emu_io.h"

#include <chrono>           //std::chrono::steady_clock, std::chrono::duration, 
                            //std::chrono::duration_cast
#include <thread>           //std::this_thread::sleep_for
#include <unordered_map>

int main(int argc, char** argv)
{
    if(argc == 1) return 1;

    namespace C8 = chip8;
    using Ck = C8::Keys;
    using Ik = emu_io::Keys;
    
    uint8_t buffer[C8::PROGRAM_SIZE];
    emu_io::load_rom_file(argv[1], buffer, C8::PROGRAM_SIZE);

    //<chip8::Keys, emu_io::Keys>
    std::unordered_map<Ck, Ik> map {
        {Ck::KEY_1, Ik::KEY_1},
        {Ck::KEY_2, Ik::KEY_2},
        {Ck::KEY_3, Ik::KEY_3},
        {Ck::KEY_C, Ik::KEY_4},
        {Ck::KEY_4, Ik::KEY_Q},
        {Ck::KEY_5, Ik::KEY_W},
        {Ck::KEY_6, Ik::KEY_E},
        {Ck::KEY_D, Ik::KEY_R},
        {Ck::KEY_7, Ik::KEY_A},
        {Ck::KEY_8, Ik::KEY_S},
        {Ck::KEY_9, Ik::KEY_D},
        {Ck::KEY_E, Ik::KEY_F},
        {Ck::KEY_A, Ik::KEY_Z},
        {Ck::KEY_0, Ik::KEY_X},
        {Ck::KEY_B, Ik::KEY_C},
        {Ck::KEY_F, Ik::KEY_V}
    };

    using clock = std::chrono::steady_clock;
    using milliseconds = std::chrono::duration<double, std::milli>;
    using std::chrono::duration_cast;

    constexpr milliseconds dt {1000.0/C8::DEFAULT_CLOCK_SPEED_HZ};
    auto previous_time = clock::now();
    auto new_time = previous_time;
    milliseconds accumulator = milliseconds(0);
    
    C8::CPU cpu(buffer, C8::PROGRAM_SIZE, C8::NEW_OPCODES);
    emu_io::IO& io = emu_io::IO::instance("CHOP-8", C8::WIDTH, C8::HEIGHT);

    do 
    {
        new_time = clock::now();
        accumulator += duration_cast<milliseconds>(new_time - previous_time);
        previous_time = new_time;

        while(accumulator >= dt)
        {
            using U = unsigned int;
            for(U u = static_cast<U>(Ck::KEY_0); 
                u < static_cast<U>(Ck::QUANTITY_OF_KEYS);
                u++)
            {
                Ck k = static_cast<Ck>(u);
                cpu.pump_input(k, io.is_key_held(map.at(k)));
            }

            cpu.execute();

            accumulator -= dt;
        }

        io.set_audible(cpu.is_sound());
        io.render(cpu.framebuffer());

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        io.update_input();
    } 
    while(!(io.is_key_held(Ik::KEY_ESCAPE)));

    return 0;
}
