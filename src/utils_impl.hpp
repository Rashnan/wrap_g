#ifndef UTILS_IMPL_HPP
#define UTILS_IMPL_HPP

// stl
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <ranges>
#include <algorithm>

// stb image
#define STB_IMAGE_IMPLEMENTATION
#include "../../dependencies/stb/stb_image.h"

// stb image write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../dependencies/stb/stb_image_write.h"

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

    stb_image::stb_image(const char *path, bool vertical_flip) noexcept
    {
        load_file(path, vertical_flip);
    }

    stb_image::~stb_image() noexcept
    {
        if (m_data != NULL)
            stbi_image_free(m_data);
    }

    bool stb_image::load_file(const char *path, bool vertical_flip) noexcept
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

    std::future<bool> stb_image::load_file_async(const char *path, bool vertical_flip) noexcept
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
    // requires ClockUnit<DurationUnit>
    [[nodiscard]] int timer::stop() noexcept
    {
        return std::chrono::duration_cast<DurationUnit>(clock::now() - m_start).count();
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

    consteval glm::vec4 rgba(int&& r, int&& g, int&& b, float&& a) noexcept
    {
        return glm::vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, a);
    }

    consteval glm::vec4 hex(std::string_view&& code) noexcept
    {
        // const hex size
        // #RRGGBB
        // TODO add alpha #rrggbbaa
        // TODO add 3&4 size #rgba or #rgb

        if (code.size() != 7)
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

        return glm::vec4{
            (heximal(code[2]) + heximal(code[1]) * 16.0)/255.0,
            (heximal(code[4]) + heximal(code[3]) * 16.0)/255.0,
            (heximal(code[6]) + heximal(code[5]) * 16.0)/255.0,
        1.0};
    }

    template <size_t rows, size_t cols, typename T>
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

    template <size_t vec_size, size_t n, typename T>
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

    template<size_t Dimensions>
    requires (Dimensions > 1 && Dimensions < 4)
    constexpr std::array<glm::vec<Dimensions, float>, 3> gen_tri_face(const glm::vec<Dimensions, float> &start, const glm::vec<Dimensions, float> &end) noexcept
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
    constexpr std::array<glm::vec<Dimensions, float>, 4> gen_rect_face(const glm::vec<Dimensions, float> &start, const glm::vec<Dimensions, float> &end) noexcept
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

} // namespace utils

#endif