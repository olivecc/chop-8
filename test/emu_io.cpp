#include <cstdint>  //uint32_t
#include <cstdlib>  //std::malloc
#include <memory>   //std::unique_ptr, std::make_unique

#include "emu_io.h"
#include "SDL.h"

using namespace emu_io;

namespace
{
    void io_fail_init()
    {
        throw io_exception("Failed to initialise");
    }
}

void emu_io::load_rom_file(const char* path, void* buffer, size_t max_size)
{
    const char* read_binary = "rb";
    std::FILE* program_file = std::fopen(path, read_binary);

    if(!program_file)
    {
        throw io_exception("ROM file not found");
    }

    std::fseek(program_file, 0, SEEK_END);
    size_t file_size = std::ftell(program_file);
    auto program_size = std::min(file_size, max_size);

    std::fseek(program_file, 0, SEEK_SET);
    std::fread(buffer, 1, program_size, program_file);

    std::fclose(program_file);
}

class IO::IO_impl
{
private:
    static constexpr unsigned int sample_rate = 44100;
    const Uint8* kb_state_;
    float audio_off[sample_rate / 60] = { 0 };
    float audio_on [sample_rate / 60];

    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    SDL_AudioDeviceID audio_device_;
    uint32_t* canvas_;
    const unsigned int width_;
    const unsigned int height_;
    bool is_audible = false;

public:
    IO_impl(const char* title, 
        unsigned int width, unsigned int height) 
        : width_{width}, height_{height}, kb_state_{SDL_GetKeyboardState(NULL)}
    {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);


        //Video init
        enum { NO_FLAGS = 0 };

        const char* nearest_pixel_sampling = "0"; 
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 
            nearest_pixel_sampling);
        
        window_ = SDL_CreateWindow(title, 0, 0, width_, height_,
            SDL_WINDOW_FULLSCREEN_DESKTOP);
        if(window_ == NULL) io_fail_init();
        
        enum { ANY_DRIVER = -1 };
        renderer_ = SDL_CreateRenderer(window_, ANY_DRIVER, 
            SDL_RENDERER_PRESENTVSYNC);//NO_FLAGS);
        if(renderer_ == NULL) io_fail_init();
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, SDL_ALPHA_OPAQUE);

        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, width_, height_);
        if(texture_ == NULL) io_fail_init();

        canvas_ = static_cast<uint32_t*>(
                std::malloc(width * height * sizeof(uint32_t)));
        if(canvas_ == nullptr) io_fail_init();

        SDL_AudioSpec audio_spec{};
        SDL_AudioSpec dummy;
        audio_spec.freq = sample_rate;
        audio_spec.format = AUDIO_F32;
        audio_spec.channels = 1;
        audio_spec.samples = 2048;
        audio_spec.callback = NULL;
        
        audio_device_ = SDL_OpenAudioDevice(NULL, 0, &audio_spec, &dummy, 
            NO_FLAGS);
        if(audio_device_ == 0) io_fail_init();
        SDL_PauseAudioDevice(audio_device_, 0);

        for(unsigned int i = 0; i < sample_rate / 60; ++i)
        {
            audio_on[i] = (((i / 46) % 2 == 0) ? 1 : -1);
        }
    }

    ~IO_impl()
    {
        SDL_DestroyTexture(texture_);
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);

        SDL_Quit();
    }

    IO_impl& render(const uint32_t* buffer)
    {
        static_assert(sizeof(Uint32) == sizeof(uint32_t));

        SDL_UpdateTexture(texture_, NULL, buffer, width_ * sizeof(Uint32)); 
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, NULL, NULL);
        SDL_RenderPresent(renderer_);

        float (&audio_buf)[sample_rate / 60] = (is_audible ? audio_on : audio_off);
        SDL_QueueAudio(audio_device_, audio_buf, sizeof(audio_buf));

        return *this;
    }

    IO_impl& set_audible(bool val) { is_audible = val; return *this; }

    template<typename T, typename F>
    IO_impl& render(const T* buffer, F to_argb)
    {
        for(unsigned int p = 0; p < width_ * height_; p++)
        {
            canvas_[p] = to_argb(buffer[p]);
        }
        render(canvas_);

        return *this;
    }

    IO_impl& update_input() { SDL_PumpEvents(); return *this; }

    bool is_key_held(Keys key) 
    { 
        //Note that emu_io::Keys values are directly derived from SDL_Scancode
        return kb_state_[static_cast<SDL_Scancode>(key)]; 
    }
};



IO::IO(const char* title, unsigned int width, unsigned int height) 
    : pImpl_{std::make_unique<IO_impl>(title, width, height)} {update_input();}

IO::~IO() = default;

IO& IO::render(const uint32_t* buffer)
{
    pImpl_->render(buffer);
    return *this;
}

IO& IO::set_audible(bool val)
{
    pImpl_->set_audible(val);
    return *this;
}

IO& IO::update_input()
{
    pImpl_->update_input();
    return *this;
}

bool IO::is_key_held(Keys key)
{
    return pImpl_->is_key_held(key);
}
