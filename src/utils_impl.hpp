#ifndef UTILS_IMPL_HPP
#define UTILS_IMPL_HPP

// stl
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <ranges>
#include <algorithm>
#include <ctime>
#include <iomanip>

// stb image
#define STB_IMAGE_IMPLEMENTATION
#include "../dep//stb/stb_image.h"

// stb image write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../dep//stb/stb_image_write.h"

// // stb true type
// #define STB_TRUETYPE_IMPLEMENTATION
// #include "../../dependencies/stb/stb_true_type.h"

// local
#include "utils.hpp"

namespace utils
{

    ////////
    // definitions
    ////////
    
    ////
    // stb_image

    stb_image::stb_image() noexcept
    {
    }
    
    stb_image::~stb_image() noexcept
    {
        if (m_data != NULL)
            stbi_image_free(m_data);
    }

    [[nodiscard]] bool stb_image::load_file(const char *path, bool vertical_flip) noexcept
    {
        stbi_set_flip_vertically_on_load(vertical_flip);
        m_data = stbi_load(path, &m_width, &m_height, &m_nr_channels, 0);
        
        // stbi load does not throw exceptions
        if (m_width <= 0 || m_height <= 0)
        {
            std::cout << "[utils] Error: Failed to load texture image from " << path << ".\n";
            return false;
        }

        return true;
    }

    [[nodiscard]] std::future<bool> stb_image::load_file_async(const char *path, bool vertical_flip) noexcept
    {
        return std::async(std::launch::async, [
            path, vertical_flip,
            &data = m_data, &width = m_width, &height = m_height, &nr_channels = m_nr_channels
        ](){
            stbi_set_flip_vertically_on_load_thread(vertical_flip);
            data = stbi_load(path, &width, &height, &nr_channels, 0);
        
            // stbi load does not throw exceptions
            if (width <= 0 || height <= 0)
            {
                std::cout << "[utils] Error: Failed to load texture image from " << path << ".\n";
                return false;
            }

            return true;
        });
    }

    ////
    // stb true type

    float stb_true_type::get_string_width(const stbtt_fontinfo *info, const char *str, size_t from, size_t to, int font_height, int bitmap_width) noexcept
    {
        float width = 0.0f, scale = stbtt_ScaleForPixelHeight(info, font_height);
        int lines = 1;

        for (size_t i = from; i < to; ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(info, str[i], &ax, &lsb);
            
            float x_shift = width - (float)std::floor(width);
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBoxSubpixel(info, str[i], scale, scale, x_shift, 0, &c_x1, &c_y1, &c_x2, &c_y2);
            
            width += ax * scale;// linear movement of characters

            if (bitmap_width != -1 && std::ceil(width) >= lines * bitmap_width)
            {
                width += lines * bitmap_width - (width - ax * scale);
                ++lines;
            }
            else if (str[i + 1] != '\0')
            {
                int kern;
                kern = stbtt_GetCodepointKernAdvance(info, str[i], str[i + 1]);
                width += kern * scale; // add kerning to make words look nice.
            }
        }

        return width;
    }

    bool stb_true_type::load_file(const char *path) noexcept
    {
        m_font_file_data = utils::read_file_bytes_sync(path);

       if (!stbtt_InitFont(&m_font_info, m_font_file_data.cbegin().base(), 0))
        {
            std::cout << "[utils] Error: Failed to initialize font from file at " << path << ".\n";
            return false;
        }

        loaded = true;

        return true;
    }

    bool stb_true_type::make_bitmap(int bitmap_width, int bitmap_height, int font_height, const char *text, float line_gap_scale) noexcept
    {
        float scale = stbtt_ScaleForPixelHeight(&m_font_info, font_height);

        int ascent, descent, line_gap;

        stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, &line_gap);

        ascent = std::round(ascent * scale);
        descent = std::round(descent * scale);
        line_gap = std::round(line_gap * line_gap_scale);

        if (line_gap == 0)
        {
            line_gap = font_height * line_gap_scale;
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: No line gap value provided by font file. Setting it to font height (" << font_height << ").\n";
#endif
        }

        m_bitmap_data.resize(bitmap_width * bitmap_height);
        m_bitmap_data.clear();

        int x = 0;
        int line = 0;
        
        for (size_t i = 0; i < strlen(text); ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(&m_font_info, text[i], &ax, &lsb);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&m_font_info, text[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            if (x + c_x2 - c_x1 >= bitmap_width)
            {
                x = 0;
                ++line;
            }

            int y = ascent + c_y1 // makes characters like t and h be on the same line as e. from bottom
                    + line * line_gap; // controls height if in another line

            if (y >= bitmap_height)
                break;  

            int byte_offset = x// linear movement of characters 
                            + std::round(lsb * scale) // moves characters to top instead of being at the bottom.
                            + (y * bitmap_width); // moves characters like e to be one same line as larger ones like l. from top

            stbtt_MakeCodepointBitmap(&m_font_info, m_bitmap_data.data() + byte_offset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, text[i]);

            x += std::round(ax * scale); // linear movement of characters

            int kern;
            kern = stbtt_GetCodepointKernAdvance(&m_font_info, text[i], text[i + 1]);
            x += std::round(kern * scale); // add kerning to make words look nice.
        }

        return true;
    }

    bool stb_true_type::make_bitmap_fixed(int& bitmap_width, int& bitmap_height, int font_height, const char *text, float line_gap_scale) noexcept
    {
        if (!loaded)
        {
            std::cout << "[utils] Error: Font not loaded.\n";
            return false;
        }

        if (bitmap_width <= 0)
        {
            std::cout << "[utils] Error: Invalid bitmap width.\n";
            return false;
        }

        if (bitmap_height < font_height)
        {
            std::cout << "[utils] Error: Bitmap height provided too small.\n";
            return false;
        }
        
        if (font_height <= 0)
        {
            std::cout << "[utils] Error: Invalid font height.\n";
            return false;
        }

        int width = bitmap_width;
        
        if (width % _bitmap_width_correcter != 0)
        {
            width += _bitmap_width_correcter - (bitmap_width % _bitmap_width_correcter);
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap width provided too small, moving up to nearest multiple of " << _bitmap_width_correcter <<" (" << width << ").\n";
#endif
            bitmap_width = width;
        }

        float scale = stbtt_ScaleForPixelHeight(&m_font_info, font_height);

        int ascent, descent, line_gap;

        stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, &line_gap);

        ascent = std::round(ascent * scale);
        descent = std::round(descent * scale);
        line_gap = std::round(line_gap * line_gap_scale);

        if (line_gap == 0)
        {
            line_gap = font_height * line_gap_scale;
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: No line gap value provided by font file. Setting it to font height (" << font_height << ").\n";
#endif
        }

        m_bitmap_data.resize(bitmap_width * bitmap_height);
        m_bitmap_data.clear();

        int x = 0;
        int line = 0;
        
        for (size_t i = 0; i < strlen(text); ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(&m_font_info, text[i], &ax, &lsb);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&m_font_info, text[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

            if (x + c_x2 - c_x1 >= bitmap_width)
            {
                x = 0;
                ++line;
            }

            int y = ascent + c_y1 // makes characters like t and h be on the same line as e. from bottom
                    + line * line_gap; // controls height if in another line
            
            if (y >= bitmap_height)
                break;  
            

            int byte_offset = x// linear movement of characters 
                            + std::round(lsb * scale) // moves characters to top instead of being at the bottom.
                            + (y * bitmap_width); // moves characters like e to be one same line as larger ones like l. from top

            stbtt_MakeCodepointBitmap(&m_font_info, m_bitmap_data.data() + byte_offset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, text[i]);

            x += std::round(ax * scale); // linear movement of characters

            int kern;
            kern = stbtt_GetCodepointKernAdvance(&m_font_info, text[i], text[i + 1]);
            x += std::round(kern * scale); // add kerning to make words look nice.
        }

        return true;
    }

    bool stb_true_type::make_bitmap_line(int& bitmap_width, int& bitmap_height, int font_height, const char *text) noexcept
    {
        if (!loaded)
        {
            std::cout << "[utils] Error: Font not loaded.\n";
            return false;
        }
        
        if (font_height <= 0)
        {
            std::cout << "[utils] Error: Invalid font height.\n";
            return false;
        }

        if (bitmap_height < font_height)
        {
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap height provided too small, moving up to nearest line.\n";
#endif
            bitmap_height = font_height;
        }

        int width = get_string_width(&m_font_info, text, 0, strlen(text), font_height);

        width = std::max(width, bitmap_width);

        if (width % _bitmap_width_correcter != 0)
        {
            width += _bitmap_width_correcter - (width % _bitmap_width_correcter);
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap width calculated too small, moving up to nearest multiple of " << _bitmap_width_correcter <<" (" << width << ").\n";
#endif
        }

        if (bitmap_width != width)
        {
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap width provided too small moving up to string width (" << width << ").\n";
#endif
            bitmap_width = width;
        }
        
        float scale = stbtt_ScaleForPixelHeight(&m_font_info, font_height);

        int ascent, descent;
        stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, 0);

        ascent = std::round(ascent * scale);
        descent = std::round(descent * scale);
        
        m_bitmap_data.resize(bitmap_width * bitmap_height);
        m_bitmap_data.clear();

        float xpos = 0.0f;
        for (size_t i = 0; i < strlen(text); ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(&m_font_info, text[i], &ax, &lsb);
            
            float x_shift = xpos - (float)std::floor(xpos);
            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBoxSubpixel(&m_font_info, text[i], scale, scale, x_shift, 0, &c_x1, &c_y1, &c_x2, &c_y2);

            auto stride = width * (ascent + c_y1) + (int)xpos + c_x1;
            stbtt_MakeCodepointBitmapSubpixel(&m_font_info, m_bitmap_data.data() + stride, c_x2 - c_x1, c_y2 - c_y1, width, scale, scale, x_shift, 0, text[i]);

            xpos += ax * scale;

            if (text[i + 1] != '\0')
            {
                int kern = stbtt_GetGlyphKernAdvance(&m_font_info, text[i], text[i + 1]);
                xpos += kern * scale;
            }
        }

        return true;
    }
    
    bool stb_true_type::make_bitmap_multiline(int& bitmap_width, int& bitmap_height, int font_height, const char *text, float line_gap_scale) noexcept
    {
        if (!loaded)
        {
            std::cout << "[utils] Error: Font not loaded.\n";
            return false;
        }

        if (bitmap_width <= 0)
        {
            std::cout << "[utils] Error: Invalid bitmap width.\n";
            return false;
        }
        
        if (font_height <= 0)
        {
            std::cout << "[utils] Error: Invalid font height.\n";
            return false;
        }

        int string_width = get_string_width(&m_font_info, text, 0, strlen(text), font_height, bitmap_width);
        
        if (string_width <= bitmap_width)
        {
            return make_bitmap_line(bitmap_width,  bitmap_height, font_height, text);
        }

        float scale = stbtt_ScaleForPixelHeight(&m_font_info, font_height);
        int ascent, descent, line_gap;

        stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, &line_gap);

        ascent = std::round(ascent * scale);
        descent = std::round(descent * scale);
        line_gap = std::round(line_gap * line_gap_scale);

        if (line_gap == 0)
        {
            line_gap = font_height * line_gap_scale;
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: No line gap value provided by font file. Setting it to font height (" << font_height << ").\n";
#endif
        }

        int lines = std::ceil((float)string_width / (float)bitmap_width);
        int height = line_gap * (lines - 1) + (ascent - descent);
        height = std::max(height, bitmap_height);

        if (bitmap_height != height)
        {
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap height provided too small, moving up to fit text (" << height << ").\n";
#endif
            bitmap_height = height;
        }

        int width = bitmap_width;

        if (width % _bitmap_width_correcter != 0)
        {
            width += _bitmap_width_correcter - (width % _bitmap_width_correcter);

#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap width provided too small, moving up to nearest multiple of " << _bitmap_width_correcter <<" (" << width << ").\n";
#endif
            bitmap_width = width;
        }

        m_bitmap_data.resize(bitmap_width * bitmap_height);
        m_bitmap_data.clear();

        float x = 0;
        int line = 0;
        
        for (size_t i = 0; i < strlen(text); ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(&m_font_info, text[i], &ax, &lsb);

            float x_shift = x - (float)std::floor(x);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBoxSubpixel(&m_font_info, text[i], scale, scale, x_shift, 0, &c_x1, &c_y1, &c_x2, &c_y2);
            
            if (x + c_x2 - c_x1 >= bitmap_width)
            {
                x = 0;
                ++line;
            }

            int y = ascent + c_y1 // makes characters like t and h be on the same line as e. from bottom
                    + line * line_gap; // controls height if in another line
            
            int byte_offset = x// linear movement of characters 
                            + std::round(lsb * scale) // moves characters to top instead of being at the bottom.
                            + (y * bitmap_width); // moves characters like e to be one same line as larger ones like l. from top

            stbtt_MakeCodepointBitmapSubpixel(&m_font_info, m_bitmap_data.data() + byte_offset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, x_shift, 0, text[i]);

            x += ax * scale; // linear movement of characters

            int kern;
            kern = stbtt_GetCodepointKernAdvance(&m_font_info, text[i], text[i + 1]);
            x += kern * scale; // add kerning to make words look nice.
        }

        return true;
    }

    bool stb_true_type::make_bitmap_fit(int bitmap_width, int bitmap_height, int& font_height, const char *text, float line_gap_scale) noexcept
    {
        if (!loaded)
        {
            std::cout << "[utils] Error: Font not loaded.\n";
            return false;
        }

        if (bitmap_height <= 0)
        {
            std::cout << "[utils] Error: Invalid bitmap height.\n";
            return false;
        }

        if (bitmap_width <= 0)
        {
            std::cout << "[utils] Error: Invalid bitmap width.\n";
            return false;
        }

        int width = bitmap_width;

        if (width % _bitmap_width_correcter != 0)
        {
            width += _bitmap_width_correcter - (width % _bitmap_width_correcter);

#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Bitmap width provided too small, moving up to nearest multiple of " << _bitmap_width_correcter <<" (" << width << ").\n";
#endif
            bitmap_width = width;
        }

        // const float f10_width = get_string_width(&m_font_info, text, 0, strlen(text), 10.0, bitmap_width);
        // const float f20_width = get_string_width(&m_font_info, text, 0, strlen(text), 20.0, bitmap_width);

        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&m_font_info, &ascent, &descent, &line_gap);

        line_gap = std::round(line_gap * line_gap_scale);

        int font_height_calc = 0;

        // TODO: find a more math way to calculate the font height
        while (true)
        {
            // int lines =  std::ceil(f10_width * font_height_calc / (10.0 * bitmap_width));
            const float strw = get_string_width(&m_font_info, text, 0, strlen(text), font_height_calc, bitmap_width);
            int lines = std::ceil(strw/bitmap_width);
            int height = (line_gap == 0 ? font_height_calc * line_gap_scale : line_gap) * (lines - 1) + font_height_calc;

            if (height >= bitmap_height)
            {
                --font_height_calc;
                break;
            }

            ++font_height_calc;
        }

        if (font_height_calc <= 0)
        {
            std::cout << "[utils] Error: Failed to calculate appropriate font height.\n";
            return false;
        }

        if (font_height <= 0 || font_height > font_height_calc)
        {
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: Font height provided does not fit inside, moving to " << font_height_calc << "\n";
#endif
            font_height = font_height_calc;
        }

        // TODO: add this smh
        if (line_gap == 0)
        {
            line_gap = font_height * line_gap_scale;
#if WRAP_G_DEBUG
            std::cout << "[utils] Info: No line gap value provided by font file. Setting it to font height (" << font_height << ").\n";
#endif
        }

        float scale = stbtt_ScaleForPixelHeight(&m_font_info, font_height);

        ascent = std::round(ascent * scale);
        descent = std::round(descent * scale);
        
        m_bitmap_data.resize(bitmap_width * bitmap_height);
        m_bitmap_data.clear();

        int x = 0;
        int line = 0;
        
        for (size_t i = 0; i < strlen(text); ++i)
        {
            int ax, lsb;
            stbtt_GetCodepointHMetrics(&m_font_info, text[i], &ax, &lsb);

            int c_x1, c_y1, c_x2, c_y2;
            stbtt_GetCodepointBitmapBox(&m_font_info, text[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
            
            if (x + c_x2 - c_x1 >= bitmap_width)
            {
                x = 0;
                ++line;
            }

            int y = ascent + c_y1 // makes characters like t and h be on the same line as e. from bottom
                    + line * line_gap; // controls height if in another line
            int byte_offset = x// linear movement of characters 
                            + std::round(lsb * scale) // moves characters to top instead of being at the bottom.
                            + (y * bitmap_width); // moves characters like e to be one same line as larger ones like l. from top

            stbtt_MakeCodepointBitmap(&m_font_info, m_bitmap_data.data() + byte_offset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, text[i]);

            x += std::round(ax * scale); // linear movement of characters

            int kern;
            kern = stbtt_GetCodepointKernAdvance(&m_font_info, text[i], text[i + 1]);
            x += std::round(kern * scale); // add kerning to make words look nice.
        }

        return true;
    }
    
    // TODO: make bitmap that assigns size based on height and width

    ////
    // random

    template <class Engine>
    random<Engine>::random() noexcept
        : m_engine(m_seed())
    {
    }

    template <class Engine>
    random<Engine>::result_type random<Engine>::operator()() noexcept
    {
        return m_engine();
    }

    template <class Engine>
    std::string random<Engine>::operator()(random<Engine>::type string_type, unsigned int len) noexcept
    {
        std::stringstream ss;

        for (unsigned int i = 0; i < len; ++i)
        {
            result_type val = m_engine();
            auto rem = 0;

            switch (string_type)
            {
            case type::BIN:
                ss << val % 2;
                break;
            case type::DEC:
                ss << val % 10;
                break;
            case type::HEX:
                rem = val % 16;
                ss << (rem < 10 ? rem : (char)(rem - 10 + (int)'a'));
                break;
            case type::LETTERS:
                ss << (char)('a' + val % 26);
                break;
            case type::ALPHANUMERIC:
                rem = val % 36;
                ss << (rem < 10 ? rem : (char)(rem - 10 + (int)'a'));
                break;
            default:
                break;
            }
        }

        return ss.str();
    }

    ////
    // timer

    timer::timer(const char *tag) noexcept
        : m_tag(tag)
    {
        start();
    }

    void timer::start() noexcept
    {
        m_start = clock::now();
    }

    template <typename DurationUnit>
    requires is_one_of<DurationUnit, timer::y, timer::m, timer::d, timer::hr, timer::min, timer::s, timer::ms, timer::us, timer::ns>
    [[nodiscard]] int timer::stop() noexcept
    {
        return std::chrono::duration_cast<DurationUnit>(clock::now() - m_start).count();
    }

    ////
    // metrics

    metrics::metrics(std::ostream& out) noexcept
        : m_out(out)
    {
    }

    void metrics::start_tracking() noexcept
    {
        m_frames = 0;
        m_total_time = 0.0;
        m_last_time = 0.0;
        
        m_out << "------------------------------------------\n";
        m_out << "[metrcis] Debug: Starting tracking.\n";
    }

    void metrics::track_frame(double dt, bool output) noexcept
    {
        ++m_frames;
        m_last_time = dt;
        m_total_time += dt;

        if (output)
            m_out << "[metrcis] Debug: FPS: " << 1e3 / m_last_time << ", Frame render took " << m_last_time << " ms.\n";
    }

    void metrics::finish_tracking() noexcept
    {
        m_out << "[metrcis] Debug: Finishing tracking..\n";
        m_out << "------------------------------------------\n";
        m_out << "[metrcis] Debug: Total frames: " << m_frames << ".\n";
        m_out << "[metrcis] Debug: Average frame render time: " << m_total_time / m_frames << " ms.\n";
        m_out << "[metrcis] Debug: FPS: " << 1e3 * m_frames / m_total_time << "\n";
        m_out << "[metrcis] Debug: Total rendering code time elapsed: " << m_total_time << " ms \n";
        m_out << "------------------------------------------\n";
    }

    void metrics::save(std::string_view filename, std::vector<std::string_view> extra_fields) noexcept
    {
        try
        {
            std::fstream file(filename.data(), std::fstream::app);
            // file headers must have headers to start
            // Date, Time, Avg. Render time(ms), FPS, Total Render time (ms),
            
            std::stringstream ss{};

            auto now = std::chrono::high_resolution_clock::to_time_t(std::chrono::high_resolution_clock::now());
            auto now_tm = *std::localtime(&now);            

            ss << std::put_time(&now_tm, "%a, %d %b %Y %H:%M:%S") << ", ";

            ss << m_total_time / m_frames << ", " << 1e3 * m_frames / m_total_time << ", " << m_total_time;
            for (auto& field : extra_fields)
            {
                ss << ", " << field;
            }
            ss << ",\n";

            std::string str = ss.str();

            file.write(str.c_str(), str.size());
            file.close();
        }
        catch (const std::fstream::failure& e)
        {
            std::cout << "[metrics] Error (#" << e.code() << "): " << e.what() << "\n";
        }
    }

    ////
    // functions

    std::string read_file_sync(const char *path) noexcept
    {
        std::string str;
        std::ifstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        try
        {
            file.open(path);
            std::stringstream ss;
            ss << file.rdbuf();
            str = ss.str();
            file.close();
            return str;
        }
        catch (const std::ifstream::failure &e)
        {
            std::cout << "[utils] Error: Failed to read file " << path << ". Code: " << e.code() << ", Message: " << e.what() << ".\n";
            return str;
        }
    }

    std::future<std::string> read_file_async(const char *path) noexcept
    {
        return std::async(std::launch::async, [path](){
            return read_file_sync(path);
        });
    }
    
    std::vector<unsigned char> read_file_bytes_sync(const char *path) noexcept
    {
        std::vector<unsigned char> data;
        std::ifstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        try
        {
            file.open(path, std::ios::binary);
            data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            file.close();
            return data;
        }
        catch (const std::ifstream::failure &e)
        {
            std::cout << "[utils] Error: Failed to read file " << path << ". Code: " << e.code() << ", Message: " << e.what() << ".\n";
            return data;
        }
    }
    
    std::future<std::vector<unsigned char>> read_file_bytes_async(const char *path) noexcept
    {
        return std::async(std::launch::async, [path](){
            return read_file_bytes_sync(path);
        });
    }

    template<typename ... Ts, typename Fn>
    [[nodiscard]] std::pair<std::array<std::string, sizeof...(Ts)>, std::vector<std::tuple<Ts...>>>
    read_csv_tuple_sync(const char *path, bool has_headers, Fn&& fn) noexcept
    {
        std::vector<std::tuple<Ts...>> data;
        constexpr size_t data_size = sizeof...(Ts);
        std::array<std::string, data_size> headers;

        std::ifstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        try
        {
            file.open(path);

            std::array<std::string, data_size> row_str_arr;
            std::string row_str;

            std::getline(file, row_str);

            auto start = 0;
            bool is_header = has_headers;

            int param = 0;

            while (row_str.find_first_of(',') != std::string::npos)
            {
                auto end = row_str.find_first_of(',', start);
                if (end == std::string::npos)
                {
                    end = row_str.find_first_of('\n', start);
                }
                if (end == std::string::npos)
                {
                    end = row_str.size();
                }

                std::string str = row_str.substr(start, end - start);

                if (is_header)
                {
                    headers[param] = str;
                }
                else
                {
                    row_str_arr[param] = str;
                }

                ++param;
                
                start = row_str.find_first_of(',', start) + 1;

                if (param == std::tuple_size_v<std::tuple<Ts...>>)
                {
                    if (!is_header)
                    {
                        std::tuple<Ts ...> tup{};

                        utils::constexpr_for<0, row_str_arr.size(), 1>([&row_str_arr, &tup, fn](auto i){
                            fn(std::get<i>(tup), row_str_arr[i]);
                        });

                        data.push_back(tup);
                    }
                    else
                    {
                        is_header = false;
                    }

                    try {
                        std::getline(file, row_str);
                    }
                    catch (const std::ifstream::failure &e)
                    {
                        break;
                    }

                    start = 0;
                    param = 0;
                }
            }

            file.close();
            return std::make_pair(headers, data);
        }
        catch (const std::ifstream::failure &e)
        {
            std::cout << "[utils] Error: Failed to read file " << path << ". Code: " << e.code() << ", Message: " << e.what() << ".\n";
            return std::make_pair(headers, data);
        }
    }

    template<typename ... Ts, typename Fn>
    [[nodiscard]] std::future<std::pair<std::array<std::string, sizeof...(Ts)>, std::vector<std::tuple<Ts...>>>>
    read_csv_tuple_async(const char *path, bool has_headers, Fn&& fn) noexcept
    {
        return std::async(std::launch::async, [path, has_headers, &fn](){
            return read_csv_tuple_sync<Ts ...>(path, has_headers, fn);
        });
    }

    template<typename Struct, typename Fn>
    requires std::is_invocable_r_v<Struct, Fn, const std::vector<std::string>&>
    [[nodiscard]] std::pair<std::vector<std::string>, std::vector<Struct>>
    read_csv_struct_sync(const char *path, bool has_headers, Fn&& fn) noexcept
    {
        std::vector<std::string> headers;
        std::vector<Struct> data;

        bool is_headers = has_headers;

        std::ifstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        try
        {
            file.open(path);

            while (!file.eof())
            {
                std::string str;
                try
                {
                    std::getline(file, str);
                }
                catch (const std::ifstream::failure& e)
                {
                    continue;
                }

                std::vector<std::string> row_str;
                
                size_t start = 0;
                size_t end = str.find_first_of(',');
                while (end != std::string::npos)
                {
                    row_str.push_back(str.substr(start, end - start));

                    start = end + 1;
                    end = str.find_first_of(',', start);
                }

                if (start < str.size() - 1) {
                    end = str.size();
                    row_str.push_back(str.substr(start, end - start));
                }

                if (is_headers)
                {
                    headers = row_str;
                    is_headers = false;
                }
                else
                {
                    // utils::print_vecs<11>(row_str.data());
                    data.push_back(fn(row_str));
                }
            };

            return std::make_pair(headers, data);
        }
        catch (const std::ifstream::failure& e)
        {
            std::cout << "[utils] Error: Failed to read file " << path << ". Code: " << e.code() << ", Message: " << e.what() << ".\n";
            return std::make_pair(headers, data);
        }
    }
    
    template<typename Struct, typename Fn>
    requires std::is_invocable_r_v<Struct, Fn, const std::vector<std::string>&>
    [[nodiscard]] std::future<std::pair<std::vector<std::string>, std::vector<Struct>>>
    read_csv_struct_async(const char *path, bool has_headers, Fn&& fn) noexcept
    {
        return std::async([path, has_headers, &fn](){
            return read_csv_struct_sync<Struct>(path, has_headers, fn);
        });
    }

    template<typename DurationUnit, typename Fn>
    requires is_one_of<DurationUnit, timer::y, timer::m, timer::d, timer::hr, timer::min, timer::s, timer::ms, timer::us, timer::ns>
            && std::is_invocable_v<Fn>
    void set_timeout(int timeout, Fn&& fn) noexcept
    {
        utils::timer t1;
        t1.start();
        while (timeout > t1.stop<DurationUnit>()){};
        fn();
    }

    template<typename DurationUnit, typename Fn>
    requires is_one_of<DurationUnit, timer::y, timer::m, timer::d, timer::hr, timer::min, timer::s, timer::ms, timer::us, timer::ns>
            && std::is_invocable_v<Fn, bool&>
    void set_interval(int interval, Fn&& fn) noexcept
    {
        utils::timer t1;
        t1.start();
        int now = 0, old = 0;
        bool end = false;
        while (!end)
        {
            now = t1.stop<DurationUnit>();
            if (now - old > interval) {
                old = now;
                fn(end);
            }
        }
    }
    
    template<typename DurationUnit, typename Fn>
    requires is_one_of<DurationUnit, timer::y, timer::m, timer::d, timer::hr, timer::min, timer::s, timer::ms, timer::us, timer::ns>
            && std::is_invocable_v<Fn>
    [[nodiscard]] std::future<void> set_timeout_async(int timeout, Fn&& fn) noexcept
    {
        return std::async(std::launch::async, [timeout, &fn](){
            std::this_thread::sleep_for(DurationUnit(timeout));

            fn();
        });
    }

    template<typename DurationUnit, typename Fn>
    requires is_one_of<DurationUnit, timer::y, timer::m, timer::d, timer::hr, timer::min, timer::s, timer::ms, timer::us, timer::ns>
            && std::is_invocable_v<Fn, bool&>
    [[nodiscard]] std::future<void> set_interval_async(int interval, Fn&& fn) noexcept
    {
        return std::async(std::launch::async, [interval, &fn](){
            bool end = false;
            while (!end)
            {
                std::this_thread::sleep_for(DurationUnit(interval));
                fn(end);
            }
        });
    }

    template<size_t Width, size_t Height, typename T>
    requires std::swappable<T> && (Width > 0) && (Height > 0)
    void flip_array2d(T *ptr, bool horizontally, bool vertically) noexcept
    {
        if (!horizontally && !vertically)
            return;

        if (horizontally ^ vertically)
        {
            for (size_t i = 0; i < Height * (vertically ? 0.5 : 1.0); ++i)
            {
                for (size_t j = 0; j < Width * (horizontally ? 0.5 : 1.0); ++j)
                {
                    auto* first = ptr + j + i * Width;
                    auto* second = ptr + (horizontally ? Width - j - 1: j) + (vertically ? Height - i - 1 : i) * Width;

                    auto temp = *first;
                    *first = *second;
                    *second = temp;
                }
            }
        }
        else
        {
            // assume square
            for (size_t i = 0; i < Height; ++i)
            {
                for (size_t j = 0; j <= i; ++j)
                {
                    if (i == j && i > std::ceil(Width / 2))
                    {
                        continue;
                    }

                    auto* first = ptr + j + i * Width;
                    auto* second = ptr + (horizontally ? Width - j - 1: j) + (vertically ? Height - i - 1 : i) * Width;

                    auto temp = *first;
                    *first = *second;
                    *second = temp;
                }
            }

            // non square
            // TODO: add factor based algorithm here
        }
    }

    template<typename T>
    requires std::swappable<T>
    void flip_array2d(size_t width, size_t height, T *ptr, bool horizontally, bool vertically) noexcept
    {
        if (!horizontally && !vertically)
            return;


       if (horizontally ^ vertically)
        {
            for (size_t i = 0; i < height * (vertically ? 0.5 : 1.0); ++i)
            {
                for (size_t j = 0; j < width * (horizontally ? 0.5 : 1.0); ++j)
                {
                    auto* first = ptr + j + i * width;
                    auto* second = ptr + (horizontally ? width - j - 1: j) + (vertically ? height - i - 1 : i) * width;

                    auto temp = *first;
                    *first = *second;
                    *second = temp;
                }
            }
        }
        else
        {
            // assume square
            for (size_t i = 0; i < height; ++i)
            {
                for (size_t j = 0; j < i; ++j)
                {
                    if (i == j && i > std::ceil(width / 2))
                    {
                        continue;
                    }

                    auto* first = ptr + j + i * width;
                    auto* second = ptr + (horizontally ? width - j - 1: j) + (vertically ? height - i - 1 : i) * width;

                    auto temp = *first;
                    *first = *second;
                    *second = temp;
                }
            }

            // non square
            // TODO: add factor based algorithm here
        }
    }
    
    template <typename T>
    requires std::equality_comparable<T>
    bool one_of(const T &val, const std::initializer_list<T> &list) noexcept
    {
        for (const auto &item : list)
        {
            if (val == item)
                return true;
        }
        return false;
    }

    template<auto Start, auto End, auto Inc, typename Fn>
    constexpr void constexpr_for(Fn&& fn) noexcept
    {
        if constexpr (Start < End)
        {
            fn(std::integral_constant<decltype(Start), Start>());
            constexpr_for<Start + Inc, End, Inc>(fn);
        }
    }

    template<typename T, typename U, typename V, typename W>
    requires (std::is_floating_point_v<T> || std::is_integral_v<T>)
        && (std::is_floating_point_v<U> || std::is_integral_v<U>)
        && (std::is_floating_point_v<V> || std::is_integral_v<V>)
        && (std::is_floating_point_v<W> || std::is_integral_v<W>)
    consteval glm::vec4 rgba(T&& r, U&& g, V&& b, W&& a) noexcept
    {
        glm::vec4 col{r, g, b, a};
        if constexpr (std::is_integral_v<T>) {
            col.r /= 256.0f;
        }
        if constexpr (std::is_integral_v<U>) {
            col.g /= 256.0f;
        }
        if constexpr (std::is_integral_v<V>) {
            col.b /= 256.0f;
        }
        if constexpr (std::is_integral_v<W>) {
            col.a /= 256.0f;
        }
        return col;
    }

    consteval glm::vec4 hex(std::string_view&& code) noexcept
    {
        // const hex size
        // #RRGGBB --> 7
        // #RRGGBBAA --> 9
        // #RGBA or #RGB -- > 5 or 4

        if (code.size() > 9)
        {
            // * Compile time detects and prevents compile if this line is reached.
            // ? For use if funciton is converted to constexpr
            std::cout << "[utils] Error: Failed to create color glm::vec4 due to invalid hex code: " << code << "\n";
            return glm::vec4{0};
        }

        auto heximal = [](char c)->double{
            if ('0' <= c && c <= '9')
            {
                return c - '0';
            }
            if ('a' <= c && c <= 'f')
            {
                return c - 'a' + 10;
            }
            if ('A' <= c && c <= 'F')
            {
                return c - 'A' + 10;
            }
            return 0;
        };
        
        return code.size() > 5 ?
                    glm::vec4{
                        (heximal(code[2]) + heximal(code[1]) * 16.0) / 255.0,
                        (heximal(code[4]) + heximal(code[3]) * 16.0) / 255.0,
                        (heximal(code[6]) + heximal(code[5]) * 16.0) / 255.0,
                        code.size() > 7 ? (heximal(code[8]) + heximal(code[7]) * 16.0) / 255.0 : 1.0,
                    }
                    :
                    glm::vec4{
                        ((heximal(code[1]) + heximal(code[1])) * 16.0) / 255.0,
                        ((heximal(code[2]) + heximal(code[2])) * 16.0) / 255.0,
                        ((heximal(code[3]) + heximal(code[3])) * 16.0) / 255.0,
                        code.size() > 4 ? (heximal(code[4]) + heximal(code[4]) * 16.0) / 255.0 : 1.0,
                    };
    }

    template <size_t rows, size_t cols, typename T>
    requires Printable<T>
    void print_mat(const T *ptr) noexcept
    {
        for (size_t i = 0; i < rows; ++i)
        {
            std::cout << "[";
            for (size_t j = 0; j < cols; ++j)
            {
                std::cout << *(ptr + i * cols + j);
                // std::cout << ptr[i][j];
                if (j != cols - 1)
                    std::cout << ", ";
            }
            std::cout << "]\n";
        }
    }

    template <size_t rows, size_t cols, typename T, typename Fn>
    requires PrintableFn<T, Fn>
    void print_mat(const T *ptr, Fn&& fn) noexcept
    {
        for (size_t i = 0; i < rows; ++i)
        {
            std::cout << "[";
            for (size_t j = 0; j < cols; ++j)
            {
                std::cout << fn(*(ptr + i * cols + j));
                // std::cout << ptr[i][j];
                if (j != cols - 1)
                    std::cout << ", ";
            }
            std::cout << "]\n";
        }
    }

    template <size_t vec_size, size_t n, typename T>
    requires Printable<T>
    void print_vecs(const T *ptr) noexcept
    {
        if (n != 1)
            std::cout << "[ ";

        for (size_t i = 0; i < n; ++i)
        {
            std::cout << "[";
            for (size_t j = 0; j < vec_size; ++j)
            {
                std::cout << *(ptr + j + i * vec_size);
                if (j != vec_size - 1)
                    std::cout << ", ";
            }
            std::cout << "]";

            if (i != n - 1)
                std::cout << ", ";
        }

        if (n != 1)
            std::cout << " ]";
        
        
        std::cout << "\n";    
    }

    template <size_t vec_size, size_t n, typename T, typename Fn>
    requires PrintableFn<T, Fn>
    void print_vecs(const T *ptr, Fn&& fn) noexcept
    {
        if (n != 1)
            std::cout << "[ ";

        for (size_t i = 0; i < n; ++i)
        {
            std::cout << "[";
            for (size_t j = 0; j < vec_size; ++j)
            {
                std::cout << fn(*(ptr + j + i * vec_size));
                if (j != vec_size - 1)
                    std::cout << ", ";
            }
            std::cout << "]";

            if (i != n - 1)
                std::cout << ", ";
        }

        if (n != 1)
            std::cout << " ]";
        
        
        std::cout << "\n";    
    }

    template<typename Tuple, size_t ... Is>
    void print_tuple_impl(const Tuple& tup, std::index_sequence<Is...>)
    {
        ((std::cout << (Is == 0 ? "" : ", ") << std::get<Is>(tup)), ... );
    }

    template<typename ... Ts>
    requires (Printable<Ts> && ...)
    void print_tuple(const std::tuple<Ts...>& tup) noexcept
    {
        std::cout << "(";
        print_tuple_impl(tup, std::make_index_sequence<sizeof...(Ts)>{});
        std::cout << ")\n";
    }

    template <typename T>
    requires Printable<T>
    void print(const T *ptr) noexcept
    {
        std::cout << ptr << "\n";
    }

    template <typename T, typename Fn>
    requires PrintableFn<T, Fn>
    void print(const T* ptr, Fn fn) noexcept
    {
        std::cout << fn(ptr) << "\n";
    }

    template<typename ... Ts>
    requires (Printable<Ts> && ...)
    void print_all(const Ts&... args)
    {
        (std::cout << ... << args) << '\n';
    }

    template<size_t Dimensions>
    requires (Dimensions > 1 && Dimensions < 4)
    constexpr std::array<glm::vec<Dimensions, float>, 3> gen_tri_verts(const glm::vec<Dimensions, float> &start, const glm::vec<Dimensions, float> &end) noexcept
    {
        if constexpr (Dimensions == 2) {
            return {
                glm::vec2{start.x, start.y},
                glm::vec2{start.x + (end.x - start.x) / 2.0, end.y},
                glm::vec2{end.x, start.y},
            };
        }
        if constexpr (Dimensions == 3) {
            return {
                glm::vec3{start.x, start.y, start.z},
                glm::vec3{start.x + (end.x - start.x) / 2.0, end.y, start.z + (end.z - start.z) / 2.0},
                glm::vec3{end.x, start.y, end.z},
            };
        }
    }

    template<size_t Dimensions>
    requires (Dimensions > 1 && Dimensions < 4)
    constexpr std::array<glm::vec<Dimensions, float>, 4> gen_rect_verts(const glm::vec<Dimensions, float> &start, const glm::vec<Dimensions, float> &end) noexcept
    {
        if constexpr (Dimensions == 2) {
            return {
                glm::vec2{start.x, start.y},
                glm::vec2{start.x, end.y},
                glm::vec2{end.x, end.y},
                glm::vec2{end.x, start.y},
            };
        }
        if constexpr (Dimensions == 3) {
            return {
                glm::vec3{start.x, start.y, start.z},
                glm::vec3{start.x, end.y, start.z},
                glm::vec3{end.x, end.y, end.z},
                glm::vec3{end.x, start.y, end.z},
            };
        }
    }

    constexpr std::array<glm::uvec3, 2> gen_rect_indices() noexcept
    {
        using V = utils::GEN_RECT_FACE_VERTS;

        return std::array{
            glm::uvec3{
                V::BOTTOM_LEFT,
                V::TOP_LEFT,
                V::TOP_RIGHT
            },
            glm::uvec3{
                V::BOTTOM_LEFT,
                V::TOP_RIGHT,
                V::BOTTOM_RIGHT
            }
        };
    }

    constexpr std::array<glm::vec3, 36> gen_cube_verts(const glm::vec3& start, const glm::vec3& end) noexcept
    {
        return std::array{
            // BACK FACE
            glm::vec3{start.x, start.y, start.z},
            glm::vec3{start.x, end.y, start.z},
            glm::vec3{end.x, end.y, start.z},

            glm::vec3{start.x, start.y, start.z},
            glm::vec3{end.x, end.y, start.z},
            glm::vec3{end.x, start.y, start.z},

            // BOTTOM FACE
            glm::vec3{start.x, start.y, start.z},
            glm::vec3{start.x, start.y, end.z},
            glm::vec3{end.x, start.y, end.z},
            
            glm::vec3{start.x, start.y, start.z},
            glm::vec3{end.x, start.y, end.z},
            glm::vec3{end.x, start.y, start.z},

            // LEFT FACE
            glm::vec3{start.x, start.y, start.z},
            glm::vec3{start.x, end.y, start.z},
            glm::vec3{start.x, end.y, end.z},
            
            glm::vec3{start.x, start.y, start.z},
            glm::vec3{start.x, end.y, end.z},
            glm::vec3{start.x, start.y, end.z},

            // reflect on primary axis

            // FRONT FACE
            glm::vec3{start.x, start.y, end.z},
            glm::vec3{start.x, end.y, end.z},
            glm::vec3{end.x, end.y, end.z},

            glm::vec3{start.x, start.y, end.z},
            glm::vec3{end.x, end.y, end.z},
            glm::vec3{end.x, start.y, end.z},

            // TOP FACE
            glm::vec3{start.x, end.y, start.z},
            glm::vec3{start.x, end.y, end.z},
            glm::vec3{end.x, end.y, end.z},
            
            glm::vec3{start.x, end.y, start.z},
            glm::vec3{end.x, end.y, end.z},
            glm::vec3{end.x, end.y, start.z},

            // RIGHT FACE
            glm::vec3{end.x, start.y, start.z},
            glm::vec3{end.x, end.y, start.z},
            glm::vec3{end.x, end.y, end.z},
            
            glm::vec3{end.x, start.y, start.z},
            glm::vec3{end.x, end.y, end.z},
            glm::vec3{end.x, start.y, end.z},
        };
    }

    constexpr std::array<glm::vec2, 36> gen_cube_texcoords() noexcept
    {
        constexpr auto LEFT_START = -1.0f;
        constexpr auto LEFT_END = 0.0f;
        constexpr auto MIDDLE_START = LEFT_START;
        constexpr auto MIDDLE_END = 1.0f;
        constexpr auto RIGHT_START = MIDDLE_END;
        constexpr auto RIGHT_END = 2.0f;

        constexpr auto TOP_START = -1.0f;
        constexpr auto TOP_END = 0.0f;
        constexpr auto TOP_SECOND_START = TOP_END;
        constexpr auto TOP_SECOND_END = 1.0f;
        constexpr auto BOTTOM_SECOND_START = TOP_SECOND_END;
        constexpr auto BOTTOM_SECOND_END = 2.0f;
        constexpr auto BOTTOM_START = BOTTOM_SECOND_END;
        constexpr auto BOTTOM_END = 3.0f;

        return std::array{
            // BACK FACE
            glm::vec2{MIDDLE_START, BOTTOM_END},
            glm::vec2{MIDDLE_START, BOTTOM_START},
            glm::vec2{MIDDLE_END, BOTTOM_START},
            
            glm::vec2{MIDDLE_START, BOTTOM_END},
            glm::vec2{MIDDLE_END, BOTTOM_START},
            glm::vec2{MIDDLE_END, BOTTOM_END},
            
            // BOTTOM FACE
            glm::vec2{MIDDLE_START, BOTTOM_SECOND_END},
            glm::vec2{MIDDLE_START, BOTTOM_SECOND_START},
            glm::vec2{MIDDLE_END, BOTTOM_SECOND_START},
            
            glm::vec2{MIDDLE_START, BOTTOM_SECOND_END},
            glm::vec2{MIDDLE_END, BOTTOM_SECOND_START},
            glm::vec2{MIDDLE_END, BOTTOM_SECOND_END},

            // LEFT FACE
            glm::vec2{LEFT_START, TOP_SECOND_END},
            glm::vec2{LEFT_START, TOP_SECOND_START},
            glm::vec2{LEFT_END, TOP_SECOND_START},
            
            glm::vec2{LEFT_START, TOP_SECOND_END},
            glm::vec2{LEFT_END, TOP_SECOND_START},
            glm::vec2{LEFT_END, TOP_SECOND_END},

            // FRONT FACE
            glm::vec2{MIDDLE_START, TOP_SECOND_END},
            glm::vec2{MIDDLE_START, TOP_SECOND_START},
            glm::vec2{MIDDLE_END, TOP_SECOND_START},
            
            glm::vec2{MIDDLE_START, TOP_SECOND_END},
            glm::vec2{MIDDLE_END, TOP_SECOND_START},
            glm::vec2{MIDDLE_END, TOP_SECOND_END},

            // TOP FACE
            glm::vec2{MIDDLE_START, TOP_END},
            glm::vec2{MIDDLE_START, TOP_START},
            glm::vec2{MIDDLE_END, TOP_START},
            
            glm::vec2{MIDDLE_START, TOP_END},
            glm::vec2{MIDDLE_END, TOP_START},
            glm::vec2{MIDDLE_END, TOP_END},

            // RIGHT FACE
            glm::vec2{RIGHT_START, TOP_SECOND_END},
            glm::vec2{RIGHT_START, TOP_SECOND_START},
            glm::vec2{RIGHT_END, TOP_SECOND_START},
            
            glm::vec2{RIGHT_START, TOP_SECOND_END},
            glm::vec2{RIGHT_END, TOP_SECOND_START},
            glm::vec2{RIGHT_END, TOP_SECOND_END},
        };
    }

    constexpr std::array<glm::vec2, 36> gen_cube_texcoords_single_face(const glm::vec2& start, const glm::vec2& end) noexcept
    {
        return std::array{
            // BACK FACE
            glm::vec2{start.x, start.y},
            glm::vec2{start.x, end.y},
            glm::vec2{end.x, end.y},
            
            glm::vec2{start.x, start.y},
            glm::vec2{end.x, end.y},
            glm::vec2{end.x, start.y},

            // BOTTOM FACE
            glm::vec2{start.x, start.y},
            glm::vec2{start.x, end.y},
            glm::vec2{end.x, end.y},
            
            glm::vec2{start.x, start.y},
            glm::vec2{end.x, end.y},
            glm::vec2{end.x, start.y},

            // LEFT FACE
            glm::vec2{start.x, start.y},
            glm::vec2{start.x, end.y},
            glm::vec2{end.x, end.y},
            
            glm::vec2{start.x, start.y},
            glm::vec2{end.x, end.y},
            glm::vec2{end.x, start.y},

            // reflect on primary axis
            
            // FRONT FACE
            glm::vec2{start.x, start.y},
            glm::vec2{start.x, end.y},
            glm::vec2{end.x, end.y},
            
            glm::vec2{start.x, start.y},
            glm::vec2{end.x, end.y},
            glm::vec2{end.x, start.y},

            // TOP FACE
            glm::vec2{start.x, start.y},
            glm::vec2{start.x, end.y},
            glm::vec2{end.x, end.y},
            
            glm::vec2{start.x, start.y},
            glm::vec2{end.x, end.y},
            glm::vec2{end.x, start.y},

            // RIGHT FACE
            glm::vec2{start.x, start.y},
            glm::vec2{start.x, end.y},
            glm::vec2{end.x, end.y},
            
            glm::vec2{start.x, start.y},
            glm::vec2{end.x, end.y},
            glm::vec2{end.x, start.y},
        };
    }

    constexpr std::array<glm::vec3, 36> gen_cube_normals(const glm::vec3& start, const glm::vec3& end) noexcept
    {
        glm::vec3 dir = end - start;
        glm::vec3 right, up, forward;
        right = glm::vec3{dir.x, 0.0f, 0.0f};
        up = glm::vec3{0.0f, dir.y, 0.0f};
        forward = glm::vec3{0.0f, 0.0f, dir.z};

        return std::array{
            // BACK FACE
            -forward, -forward, -forward, -forward, -forward, -forward,

            // BOTTOM FACE
            -up, -up, -up, -up, -up, -up,

            // LEFT FACE
            -right, -right, -right, -right, -right, -right,

            // reflect on primary axis

            // FRONT FACE
            forward, forward, forward, forward, forward, forward,

            // TOP FACE
            up, up, up, up, up, up,

            // RIGHT FACE
            right, right, right, right, right, right,
        };
    }
} // namespace utils

#endif