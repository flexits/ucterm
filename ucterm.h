/*
Terminal wrapper module - provides command-line interface interaction,
parses user input and invokes callback to execute commands.

Note: non-ASCII characters are not supported (silently ignored).

The code invokes PrintChar and PrintStr callbacks to output a
single char or a null-terminated sequence of chars. See the comments below.

The code invokes Execute callback when user presses Enter key, if
there is at least one non-whitespace character in the input buffer.
The number of parsed arguments and their values are passes to the
function. See the comment below.

The three callbacks MUST be initialized beforehands! No NULL-check inside!

Commands and actions currently supported:
- enter, backspace, delete keys;
- left & right arrows as ESC-sequences or hotkeys Ctrl+B/Ctrl+F;
- home & end keys as ESC-sequences or hotkeys Ctrl+A/Ctrl+E;
- Ctrl+K: delete line contents from current position to the end;
- Ctrl+U: delete line contents from current position to the beginning.

Initialization procedure consists of 3 obligatory steps. You MUST:
- allocate an UcTerm_HandleTypeDef instance;
- call the UcTerm_Init, passing the pointer to the allocated memory;
- register the 3 callbacks: UcTerm_RegisterPrintCharCallback,
                            UcTerm_RegisterPrintStrCallback,
                            UcTerm_RegisterExecuteCallback.

After initialization, pass all incoming characters to UcTerm_IngestChar.

    Created on: Jan 22, 2026
        Author: Alexander Korostelin (4d.41.49.4c@gmail.com)
*/

#ifndef UCTERM_H_
#define UCTERM_H_

#include <stddef.h>
#include <stdint.h>

// Internal storage size, bytes.
// Must never be 0!
// Must match the internal structure size with alignment
// (i.e. 184 on Win64, 156 on STM32).
#if defined(_WIN32)
#define UCTERM_STORAGE_SIZE 184
#elif
#define UCTERM_STORAGE_SIZE 156
#endif

/// @brief UcTerm state storage.
typedef union
{
  uint8_t storage[UCTERM_STORAGE_SIZE];
  max_align_t _align;
} UcTerm_HandleTypeDef;

/// @brief Init the internal storage. This must be called
/// prior to using any functions of the package.
void UcTerm_Init(UcTerm_HandleTypeDef *self);

/// @brief Register a callback function to output a single uint8_t.
/// The function will be called when the UcTerm instance
/// needs to output a uint8_t.
/// @param self     UcTerm instance handle.
void UcTerm_RegisterPrintCharCallback(UcTerm_HandleTypeDef *self,
                                      void (*printChr)(uint8_t));

/// @brief Register a callback function to output a null-terminated string.
/// The function will be called when the UcTerm instance
/// needs to output a sequence of uint8_ts.
/// The memory location pointed to by the argument of the callback fn
/// isn't guaranteed to remain intact after the callback fn returns.
/// Copy this memory contents if you're going to process it after
/// returning from the callback fn.
/// @param self     UcTerm instance handle.
void UcTerm_RegisterPrintStrCallback(UcTerm_HandleTypeDef *self,
                                     void (*printStr)(const uint8_t *));

/// @brief Register a callback function to execute the parsed commands.
/// The function will receive an array of pointers to null-terminated
/// strings and the total count of these pointers.
/// The memory locations pointed to by these pointers will probabaly be
/// cleared after the callback fn exits. Copy the memory contents
/// if you're going to process it after returning from the callback fn.
/// @param self     UcTerm instance handle.
/// @param execute  Callback function (uint8_t argc, uint8_t *argv[]).
void UcTerm_RegisterExecuteCallback(UcTerm_HandleTypeDef *self,
                                    void (*execute)(uint8_t, uint8_t **));

/// @brief Output a standard command prompt.
void UcTerm_ShowPrompt(UcTerm_HandleTypeDef *self);

/// @brief Process a uint8_t from the input stream.
void UcTerm_IngestChar(UcTerm_HandleTypeDef *self, uint8_t c);

#endif // UCTERM_H_
