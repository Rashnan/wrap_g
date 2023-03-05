#ifndef UTILS_HPP
#define UTILS_HPP

// stl

#include <string>
#include <string_view>
#include <array>
#include <initializer_list>
#include <chrono>
#include <random>
#include <concepts>
#include <future>
#include <utility>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb true type
#define STB_TRUETYPE_IMPLEMENTATION
#include "../dep/stb/stb_true_type.h"

namespace utils
{

    ////////
    // forward declarations
    ////////

    class stb_image;
    class stb_true_type;

    template <class Engine>
    class random;

    class timer;
    class metrics;

    ////////
    // concepts
    ////////

    template <typename T, typename... Ts>
    concept is_one_of = (std::is_same_v<T, Ts> || ...);

    template<typename T>
    concept CString = std::is_same_v<std::remove_cv_t<T>, char *>;

    template <typename T>
    concept Stringable = std::convertible_to<T, std::string_view>;

    template <typename T>
    concept Printable = requires(std::ostream& out, T t)
    {
        out << t;
    };

    template <typename T, typename Fn>
    concept PrintableFn = requires(std::ostream& out, T t, Fn fn)
    {
        out << fn(t);
    };

    ////////
    // declarations
    ////////

    ////
    // stb_image

    /**
     * @brief Create an image loader using STB_IMAGE.
     *
     */
    class stb_image
    {
    private:
        unsigned char *m_data = nullptr;
        int m_width;
        int m_height;
        int m_nr_channels;

    public:
        /**
         * @brief Construct a new stb image object.
         *
         */
        stb_image() noexcept;

        /**
         * @brief Construct a new stb image object. Also calls load_file using
         * the path and the vertical_flip.
         *
         * @param path The path to the image to be loaded.
         * @param vertical_flip Whether the image should be flipped vertically.
         */
        stb_image(const char *path, bool vertical_flip = false) noexcept;

        /**
         * @brief Destroy the stb image object.
         *
         */
        ~stb_image() noexcept;

        [[nodiscard]] inline constexpr unsigned char *data() noexcept { return m_data; }
        [[nodiscard]] inline constexpr int width() const noexcept { return m_width; }
        [[nodiscard]] inline constexpr int height() const noexcept { return m_height; }
        [[nodiscard]] inline constexpr int nr_channels() const noexcept { return m_nr_channels; }

        /**
         * @brief Load an image.
         *
         * @param path The path to the image to be loaded.
         * @param vertical_flip Whether the image should be flipped vertically.
         * @return true Loading successful.
         * @return false Loading failed.
         */
        bool load_file(const char *path, bool vertical_flip = false) noexcept;

        /**
         * @brief Load an image async.
         * * Call future.get() or future.wait() before accessing any of the data variables.
         *
         * @param path The path to the image to be loaded.
         * @param vertical_flip Whether the image should be flipped vertically.
         * @return std::future<bool>.
         */
        [[nodiscard]] std::future<bool> load_file_async(const char *path, bool vertical_flip = false) noexcept;
    };

    ////
    // stb true type

    class stb_true_type
    {
    private:
        std::vector<unsigned char> m_font_file_data;
        stbtt_fontinfo m_font_info;
        bool loaded = false;

        std::vector<unsigned char> m_bitmap_data;

        // ? a garbage value
        // ? for some reasons the widths have to be multiples of 8
        // ? originally was 4 but changed to 8 when used with times new roman w:50, h:100, fh:16
        // ? maybe only for arial ttfont
        // ! 4 is giving error fix it later
        static const int _bitmap_width_correcter = 8;

    public:
        inline unsigned char *data() noexcept { return m_bitmap_data.data(); }

        static float get_string_width(const stbtt_fontinfo *info, const char *str, size_t from, size_t to, int font_height, int bitmap_width = -1) noexcept;

        bool load_file(const char *path) noexcept;
        
        ////
        // make bitmap
        
        //? make bitmap for specific width, height, font size and line gap ... even if full text does not appear in bitmap
        //? no error checking at [utils] level
        bool make_bitmap(int bitmap_width, int bitmap_height, int font_height, const char *text, float line_gap_scale = 1.0f) noexcept;

        //? make bitmap for specific width, height, font size and line gap ... even if full text does not appear in bitmap
        bool make_bitmap_fixed(int& bitmap_width, int& bitmap_height, int font_height, const char *text, float line_gap_scale = 1.0f) noexcept;
        
        //? make bitmap in one line based of font size, bitmap height determines whether full text is shown
        bool make_bitmap_line(int& bitmap_width, int& bitmap_height, int font_height, const char *text) noexcept;
        
        //? make a bitmap within a specific bitmap width and font size for a text and assign bitmap height accordingly
        //? to fit the entire text
        bool make_bitmap_multiline(int& bitmap_width, int& bitmap_height, int font_height, const char *text, float line_gap_scale = 1.0f) noexcept;
        
        //? make a bitmap within specific sizes and auto get the best font size to fit the entire text within the bitmap
        bool make_bitmap_fit(int bitmap_width, int bitmap_height, int& font_height, const char *text, float line_gap_scale = 1.0f) noexcept;
    };

    ////
    // random

    /**
     * @brief Create a random number generator.
     *
     * @tparam Engine The generator engine to be used. Defaults to std mnist_rand.
     */
    template <class Engine = std::minstd_rand>
    class random
    {
    public:
        using default_engine = Engine;
        using result_type = default_engine::result_type;

        enum class type
        {
            BIN,
            DEC,
            HEX,
            LETTERS,
            ALPHANUMERIC,
            // not sure what ALL implies
            // ALL
        };

    private:
        std::random_device m_seed;
        default_engine m_engine;

    public:
        /**
         * @brief Construct a new random object.
         *
         */
        random() noexcept;

        /**
         * @brief Generate a random number.
         *
         * @return result_type
         */
        result_type operator()() noexcept;

        /**
         * @brief Generate a random string.
         *
         * @param string_type The type of string. Must be of type enum class 'type'. Ex: LETTERS only for letters.
         * @param len The length of the string.
         * @return std::string
         */
        std::string operator()(type string_type = type::ALPHANUMERIC, unsigned int len = 16) noexcept;
    };

    ////
    // timer

    class timer
    {
    public:
        using clock = std::chrono::high_resolution_clock;

        using y = std::chrono::year;
        using m = std::chrono::month;
        using d = std::chrono::day;
        using hr = std::chrono::hours;
        using min = std::chrono::minutes;
        using s = std::chrono::seconds;
        using ms = std::chrono::milliseconds;
        using us = std::chrono::microseconds;
        using ns = std::chrono::nanoseconds;

    private:
        clock::time_point m_start;
        const char *m_tag;

    public:
        timer(const char *tag = "") noexcept;

        void start() noexcept;

        template <typename DurationUnit = ms>
        [[nodiscard]] int stop() noexcept;
    };

    ////
    // metrics

    class metrics
    {
    private:
        std::ostream& m_out;// console

        // default tracking...
        unsigned int m_frames = 0;
        double m_total_time = 0.0;
        double m_last_time = 0.0;

    public:
        metrics(std::ostream& out = std::cout) noexcept;

        void start_tracking() noexcept;
        void track_frame(double dt,  bool output = false) noexcept;
        void finish_tracking() noexcept;

        void save(std::string_view filename, std::vector<std::string_view> extra_fields = {}) noexcept;
    };

    ///
    // functions

    // just in case functions for each variant

    std::string read_file_sync(const char *path) noexcept;
    std::future<std::string> read_file_async(const char *path) noexcept;
    std::vector<unsigned char> read_file_bytes_sync(const char *path) noexcept;
    std::future<std::vector<unsigned char>> read_file_bytes_async(const char *path) noexcept;

    template<typename ... Ts>
    std::vector<std::tuple<Ts...>> read_csv_sync(const char *path, bool has_headers = true) noexcept;
    
    template<size_t Width, size_t Height, typename T>
    requires std::swappable<T> && (Width > 0) && (Height > 0)
    void flip_array2d(T *ptr, bool horizontally = false, bool vertically = false) noexcept;

    template<typename T>
    requires std::swappable<T>
    void flip_array2d(size_t width, size_t height, T *ptr, bool horizontally = false, bool vertically = false) noexcept;

    template <typename T>
    requires std::equality_comparable<T>
    bool one_of(const T &val, const std::initializer_list<T> &list) noexcept;
    
    template<auto Start, auto End, auto Inc, typename Fn>
    constexpr void constexpr_for(Fn&& fn) noexcept;

    template<typename T, typename U, typename V, typename W = float>
    requires (std::is_floating_point_v<T> || std::is_integral_v<T>)
        && (std::is_floating_point_v<U> || std::is_integral_v<U>)
        && (std::is_floating_point_v<V> || std::is_integral_v<V>)
        && (std::is_floating_point_v<W> || std::is_integral_v<W>)
    consteval glm::vec4 rgba(T&& r, U&& g, V&& b, W&& a = 1.0f) noexcept;
    
    // hex code with #AAAAAA to rgba floats
    // TODO add alpha #rrggbbaa
    // TODO add 3&4 size #rgba or #rgb
    consteval glm::vec4 hex(std::string_view&& code) noexcept;

    // TODO: change T to printable
    template <size_t rows, size_t cols = rows, typename T = float>
    void print_mat(const T *ptr) noexcept;

    template <size_t vec_size, size_t n = 1, typename T = float>
    void print_vecs(const T *ptr) noexcept;

    // TODO: print maps

    template<typename ... Ts>
    requires (Printable<Ts> && ...)
    void print_tuple(const std::tuple<Ts...>& tup) noexcept;

    template <typename T>
    requires Printable<T>
    void print(const T *ptr) noexcept;

    template <typename T, typename Fn>
    requires PrintableFn<T, Fn>
    void print(const T* ptr, Fn fn) noexcept;

    // https://en.cppreference.com/w/cpp/language/fold
    template<typename ... Ts>
    requires (Printable<Ts> && ...)
    void print_all(const Ts&... args);

    /////
    // 2d & 3d world stuff

    enum class GEN_TRI_FACE_VERTS {
        BOTTOM_LEFT,
        TOP_CENTER,
        BOTTOM_RIGHT
    };

    /**
     * @brief Creates the vertices for a triangle like below in compile time. Sort of creates a rect face with the two
     * points given and draws a triangle in the middle of that face.
     *          |      *      | (end)
     *          |     ***     |
     *          |    *****    |
     *          |   *******   |
     *          |  *********  |
     *  (start) | *********** |
     * 
     * @param start the bottom left position
     * @param end the top right position
     * @return constexpr std::array<glm::vec3, 3> 
     */
    template<size_t Dimensions = 3>
    requires (Dimensions > 1 && Dimensions < 4)
    constexpr std::array<glm::vec<Dimensions, float> , 3> gen_tri_verts(const glm::vec<Dimensions, float> &start, const glm::vec<Dimensions, float> &end) noexcept;

    enum class GEN_RECT_FACE_VERTS {
        BOTTOM_LEFT,
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_RIGHT
    };

    /**
     * @brief Creates the vertices for a rectangle face using the two given points.
     * * NOTE: Needs an element array buffer whem generating the rectangle. 
     * creates a rectangle like below in compile time.
     *          | *********** | (end)
     *          | *********** |
     *          | *********** |
     *          | *********** |
     *          | *********** |
     *  (start) | *********** |
     * 
     * @param start the bottom left point
     * @param end the top right point
     * @return constexpr std::array<glm::vec3, 4> 
     */
    template<size_t Dimensions = 3>
    requires (Dimensions > 1 && Dimensions < 4)
    constexpr std::array<glm::vec<Dimensions, float>, 4> gen_rect_verts(const glm::vec<Dimensions, float> &start, const glm::vec<Dimensions, float> &end) noexcept;

    /**
     * @brief Creates the indices for a rectangle face. Assuming verts were provided by gen_rect_verts()
     * Indices should be provided to the element array buffer.
     * 
     * @return constexpr std::array<glm::uvec3, 2> 
     */
    constexpr std::array<glm::uvec3, 2> gen_rect_indices() noexcept;

    // TODO: maybe add gen_rect_normals() sideways normals for 2d and outward normals for 3d

    /**
     * @brief Creates the full vertices for a cube using the two points given.
     * * NOTE: These vertices are expected to be used with glDrawArrays or without an element array buffer.
     * Use this function with gen_cube_texcoords_single_face if all sides of the cube will have the same texture face. If any side
     * will have a different texture use gen_cube_verts, gen_cube_indices and gen_cube_texcoords with a cubemap.
     * 
     * @param start the bottom left back point
     * @param end the front right top point
     * @return constexpr std::array<glm::vec3, 36>
     */
    constexpr std::array<glm::vec3, 36> gen_cube_verts(const glm::vec3& start, const glm::vec3& end) noexcept;

    /**
     * @brief Creates the texture coordinates for a cube. Assuming verts were provided by gen_cube_verts()
     * and that a cubemap is being used. Assummed  that the cubemap texture is similar to a uv unwrapped
     * and uncompressed cube.
     * * NOTE: If same texture is on all sides of the cube and gen_cube_texcoords_single_face
     * 
     * @return constexpr std::array<glm::vec2, 36>
     */
    constexpr std::array<glm::vec2, 36> gen_cube_texcoords() noexcept;

    /**
     * @brief Creates the texture coordinates for a cube. Assuming verts were provided by gen_cube_verts()
     * and the same texture is used for all sides of the cube.
     * * NOTE: If even one side of the cube uses a different texture switch to gen_cube_texcoords()
     * 
     * @param start The bottom left coordinate. Default is (0.0f, 0.0f).
     * @param end The top right coordinate. Default is (1.0f, 1.0f).
     * @return constexpr std::array<glm::vec2, 36> 
     */
    constexpr std::array<glm::vec2, 36> gen_cube_texcoords_single_face(const glm::vec2& start = glm::vec2{0.0f}, const glm::vec2& end = glm::vec2{1.0f}) noexcept;

    /**
     * @brief Creates the normals for each vertex on the vertices of a cube created by gen_cube_verts().
     * Can be used to calculate diffuse lighting.
     * ! For constexpr the normals generated are not normalized (distance = 1)
     * ! use start-end distance = 1 and scale to keep normals normalized
     * ! or normalize them in a shader etc.
     * TODO: Allow a parameter to modify whether normals should be flipped?
     * 
     * @param start the bottom left back point
     * @param end the front right top point
     * @return constexpr std::array<glm::vec3, 36> 
     */
    constexpr std::array<glm::vec3, 36> gen_cube_normals(const glm::vec3& start, const glm::vec3& end) noexcept;

} // namespace utils

#include "utils_impl.hpp"

#endif