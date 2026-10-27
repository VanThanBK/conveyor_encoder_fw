#pragma once

#include <stddef.h>
#include <stdint.h>

// UART text framing only; command parsing stays in communication::execute().
class UartLineReceiver
{
public:
    static const size_t MaxLineLength = 128;

    enum Result
    {
        NoLine,
        LineReady,
        LineTooLong,
    };

    UartLineReceiver() { reset(); }

    void reset()
    {
        length = 0;
        buffer[0] = '\0';
        discarding = false;
    }

    Result push(uint8_t byte)
    {
        if (byte == '\n')
        {
            if (discarding)
            {
                reset();
                return LineTooLong;
            }
            while (length > 0 && buffer[length - 1] == ' ')
            {
                --length;
            }
            buffer[length] = '\0';
            return length > 0 ? LineReady : NoLine;
        }

        if (discarding)
        {
            return NoLine;
        }

        if (byte == '\t')
        {
            byte = ' ';
        }
        // CH340 startup/open noise must never insert NULs into Arduino String.
        // Preserve printable ASCII, including command punctuation and signs.
        if (byte < 0x20 || byte > 0x7E)
        {
            return NoLine;
        }
        if (length == 0 && byte == ' ')
        {
            return NoLine;
        }
        if (length == MaxLineLength)
        {
            // Never execute a truncated command. Recover at the next newline.
            length = 0;
            buffer[0] = '\0';
            discarding = true;
            return NoLine;
        }
        buffer[length++] = static_cast<char>(byte);
        buffer[length] = '\0';
        return NoLine;
    }

    // Consume a LineReady result before pushing more bytes, then call reset().
    const char *line() const { return buffer; }

private:
    char buffer[MaxLineLength + 1];
    size_t length;
    bool discarding;
};
