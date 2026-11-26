#pragma once

#include <Jevaing/Graphics2D.h>

#include <array>
#include <string_view>

namespace SkyDodge
{
    using Glyph = std::array<const char*, 7>;

    inline Glyph Letter(char c)
    {
        switch (c)
        {
            case 'A': return { " ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #" };
            case 'B': return { "#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### " };
            case 'C': return { " ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####" };
            case 'D': return { "#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### " };
            case 'E': return { "#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####" };
            case 'F': return { "#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    " };
            case 'G': return { " ####", "#    ", "#    ", "# ###", "#   #", "#   #", " ####" };
            case 'H': return { "#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #" };
            case 'I': return { "#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####" };
            case 'J': return { "#####", "    #", "    #", "    #", "#   #", "#   #", " ### " };
            case 'K': return { "#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #" };
            case 'L': return { "#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####" };
            case 'M': return { "#   #", "## ##", "# # #", "#   #", "#   #", "#   #", "#   #" };
            case 'N': return { "#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #" };
            case 'O': return { " ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### " };
            case 'P': return { "#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    " };
            case 'Q': return { " ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #" };
            case 'R': return { "#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #" };
            case 'S': return { " ####", "#    ", "#    ", " ### ", "    #", "    #", "#### " };
            case 'T': return { "#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  " };
            case 'U': return { "#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### " };
            case 'V': return { "#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  " };
            case 'W': return { "#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #" };
            case 'X': return { "#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #" };
            case 'Y': return { "#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  " };
            case 'Z': return { "#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####" };
            case '0': return { " ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### " };
            case '1': return { "  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### " };
            case '2': return { " ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####" };
            case '3': return { "#### ", "    #", "    #", " ### ", "    #", "    #", "#### " };
            case '4': return { "   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # " };
            case '5': return { "#####", "#    ", "#    ", "#### ", "    #", "    #", "#### " };
            case '6': return { " ####", "#    ", "#    ", "#### ", "#   #", "#   #", " ### " };
            case '7': return { "#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   " };
            case '8': return { " ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### " };
            case '9': return { " ### ", "#   #", "#   #", " ####", "    #", "    #", "#### " };
            case '/': return { "    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    " };
            case ':': return { "     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     " };
            case '-': return { "     ", "     ", "     ", "#####", "     ", "     ", "     " };
            default: return { "     ", "     ", "     ", "     ", "     ", "     ", "     " };
        }
    }

    inline void DrawText(Jevaing::Graphics2D& graphics, std::string_view text,
        float left, float top, float pixel, const Jevaing::Color& color)
    {
        for (std::size_t index = 0; index < text.size(); ++index)
        {
            const Glyph glyph = Letter(text[index]);
            for (int row = 0; row < 7; ++row)
                for (int column = 0; column < 5; ++column)
                    if (glyph[row][column] == '#')
                        graphics.DrawQuad({ left + (static_cast<float>(index * 6 + column) + 0.5f) * pixel,
                            top - (static_cast<float>(row) + 0.5f) * pixel },
                            { pixel * 0.9f, pixel * 0.9f }, color);
        }
    }

    inline void DrawCenteredText(Jevaing::Graphics2D& graphics, std::string_view text,
        float center, float top, float pixel, const Jevaing::Color& color)
    {
        DrawText(graphics, text,
            center - static_cast<float>(text.size() * 6) * pixel * 0.5f,
            top, pixel, color);
    }
}
