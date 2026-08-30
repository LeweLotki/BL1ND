#include "board_page.hpp"

#include <cctype>
#include <cstdarg>
#include <cstdio>

namespace {

class HtmlWriter {
public:
    HtmlWriter(char* output, size_t output_size)
        : output_(output)
        , output_size_(output_size)
        , length_(0)
        , valid_(output != nullptr && output_size > 0)
    {
        if (valid_) {
            output_[0] = '\0';
        }
    }

    void append(const char* format, ...)
    {
        if (!valid_) {
            return;
        }

        va_list arguments;
        va_start(arguments, format);
        const int written = vsnprintf(
            output_ + length_,
            output_size_ - length_,
            format,
            arguments
        );
        va_end(arguments);

        if (written < 0
            || static_cast<size_t>(written) >= output_size_ - length_) {
            valid_ = false;
            output_[0] = '\0';
            return;
        }

        length_ += static_cast<size_t>(written);
    }

    size_t size() const
    {
        return valid_ ? length_ : 0;
    }

private:
    char* output_;
    size_t output_size_;
    size_t length_;
    bool valid_;
};

constexpr const char* DOCUMENT_HEAD =
    "<!DOCTYPE html><html><head>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<meta http-equiv=\"refresh\" content=\"2\">"
    "<title>Chessboard</title><style>"
    "*{box-sizing:border-box}"
    "body{margin:0;background:#222;color:#eee;font-family:sans-serif;"
    "display:flex;justify-content:center;align-items:center;min-height:100vh}"
    ".board{border-collapse:collapse;table-layout:fixed}"
    "td{width:10.5vmin;height:10.5vmin;text-align:center;vertical-align:middle;"
    "font-size:7.2vmin;font-weight:800;line-height:1}"
    ".l{background:#f0d9b5}.d{background:#b58863}"
    ".w{color:#fff;-webkit-text-stroke:1px #000;"
    "text-shadow:-1px -1px 0 #000,1px -1px 0 #000,"
    "-1px 1px 0 #000,1px 1px 0 #000}"
    ".b{color:#000;-webkit-text-stroke:1px #fff;"
    "text-shadow:-1px -1px 0 #fff,1px -1px 0 #fff,"
    "-1px 1px 0 #fff,1px 1px 0 #fff}"
    ".rank,.file{font-size:3vmin;font-weight:bold;text-align:center;"
    "width:4vmin;height:4vmin}"
    "</style></head><body><table class=\"board\">";

} // namespace

size_t renderBoardPage(
    const char cells[64],
    char* output,
    size_t output_size
)
{
    if (cells == nullptr) {
        if (output != nullptr && output_size > 0) {
            output[0] = '\0';
        }
        return 0;
    }

    HtmlWriter writer(output, output_size);
    writer.append("%s", DOCUMENT_HEAD);

    for (int rank = 7; rank >= 0; --rank) {
        writer.append("<tr><th class=\"rank\">%d</th>", rank + 1);
        for (int file = 0; file < 8; ++file) {
            const char piece = cells[rank * 8 + file];
            const char square_class = (rank + file) % 2 == 0 ? 'd' : 'l';
            writer.append(
                "<td class=%c id=%c%d>",
                square_class,
                'a' + file,
                rank + 1
            );
            if (piece != ' ') {
                const bool white = std::isupper(
                    static_cast<unsigned char>(piece)
                ) != 0;
                writer.append(
                    "<b class=%c>%c</b>",
                    white ? 'w' : 'b',
                    std::toupper(static_cast<unsigned char>(piece))
                );
            }
            writer.append("</td>");
        }
        writer.append("</tr>");
    }

    writer.append("<tr><th></th>");
    for (int file = 0; file < 8; ++file) {
        writer.append("<th class=\"file\">%c</th>", 'a' + file);
    }
    writer.append("</tr></table></body></html>");

    return writer.size();
}
