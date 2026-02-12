# UcTerm

[![License](https://img.shields.io/github/license/flexits/ucterm)](LICENSE) [![C](https://img.shields.io/badge/language-C-blue)]() [![Embedded](https://img.shields.io/badge/platform-embedded-lightgrey)]()

Ever considered adding a command-line interface to your microcontroller-based project, but didn't want to deal with cursor movement and ESC-sequence boilerplate? **UcTerm** is a lean and simple solution.

![ucterm](https://github.com/user-attachments/assets/e5b94dd1-94d6-485e-902b-359b5948ea63)


With just two files — `ucterm.h` and `ucterm.c` — containing the public API and implementation respectively, UcTerm provides an easy-to-use CLI engine. Out of the box, it supports:

- Enter, Backspace, and Delete keys
- Left & Right arrows (as ESC sequences or via hotkeys Ctrl+B / Ctrl+F)
- Home & End keys (as ESC sequences or via hotkeys Ctrl+A / Ctrl+E)
- Ctrl+K — delete from the cursor position to the end of the line
- Ctrl+U — delete from the cursor position to the beginning of the line

Key characteristics of UcTerm:

- Clean modular design that simplifies integration and extension
- No dynamic memory allocation
- Designed for constrained environments: `ucterm.c` uses only _memset_ and _memmove_ and `cli.c` uses only _strcmp_; no division, printf, strtok, etc.
- No non-standard (“private”) ESC sequences

UcTerm is completely hardware-independent. It relies on three callback functions that you implement.

An optional wrapper (`cli.h` & `cli.c`) further abstracts UcTerm usage and improves modularity and separation of concerns - if you need to.

## Usage - bare minimum

UcTerm relies on three callbacks.

`PrintChar` and `PrintStr` are used for outputting a single character or a null-terminated string. For example, if you use UART for communication (a common case in bare-metal microcontroller systems), these functions should transmit data via UART.

The third callback, `Execute`, is called when the user presses Enter, provided there is at least one non-whitespace character in the input buffer. The parsed argument count and values are passed as `(uint8_t argc, uint8_t *argv[])`. You are responsible for implementing the command parser and executing the desired actions.

> ⚠️ **Important**: UcTerm does not perform NULL checks on callbacks. All three callbacks must be registered before use.

Finally, UcTerm provides the `UcTerm_IngestChar` function. Pass every incoming character to it, and UcTerm handles the rest.

So, the usage of UcTerm is as simple as:

```c
// Copy the two files, `ucterm.h` and `ucterm.c`,
// to your project, and include the header:
#include "ucterm.h"

// Statically init memory for the
// UcTerm inner state storage.
static UcTerm_HandleTypeDef hucterm;

// Implement the callback functions:
void printChar(uint8_t ch)
{
    // TODO send ch via UART or
    // whatever interface you use
}

void printString(const uint8_t *str)
{
    // TODO send *str via UART or
    // whatever interface you use
}

void execute(uint8_t argc, uint8_t *argv[])
{
    // TODO parse the arguments in
    // an if-else ladder with strcmp or other:
    if (argc == 0)
    {
        return;
    }
    if (strcmp((char *)argv[0], "uname") == 0)
    {
        // TODO: handle "uname" command
        return;
    } else if (/* ... */)
    {
        // TODO other commands
    }
}

void main(void)
{
    // Initialize the UcTerm instance and register the callbacks:
    UcTerm_Init(&hucterm);
    UcTerm_RegisterPrintCharCallback(&hucterm, &printChar);
    UcTerm_RegisterPrintStrCallback(&hucterm, &printString);
    UcTerm_RegisterExecuteCallback(&hucterm, &execute);

    for (;;)
    {
        // Pass the incoming chars from UART
        // or whatever interface you use
        // in a loop or a timer interrupt:
        uint8_t ch = _read_uart();
        UcTerm_IngestChar(&hucterm, ch);
    }
}
```

## Usage with Cli wrapper

A more structured and convenient approach is to use the optional wrapper (`cli.h` & `cli.c`). Its main purpose is to encapsulate the setup described above.

You define commands and provide hardware-specific callbacks inside `cli.c`. Then, using the module becomes as simple as the two lines:

```c
// in the initialization section
CliInit();

// in the main loop or timer ISR
CliUpdate();
```

Inside `cli.c`, commands are defined using the `CliCommand_t` type and stored in a command table. To add a command, implement its handler and add a corresponding entry to the `_commands` array (see the source for details).

The CLI wrapper automatically supports `<command> -h`, `<command> --help` or `help <command>` syntax for help. You don't need to handle help by yourself, it's already there based on the `_commands` array contents.

The wrapper also includes the required UcTerm callbacks - you'll have to provide your hardware-specific implementations. Look for the **TODO** labels in the `cli.c` file.

You may want to modify the `CliInit()` signature to pass global state or hardware handles, for example: `void CliInit(UART_HandleTypeDef *huart, AppState_t *state)`. This explicitly provides an interface to use and allows command handlers inside `cli.c` to access application state cleanly.

## Testing

Core UcTerm functionality is covered by unit tests using the [Unity](https://www.throwtheswitch.org/unity) framework.

To run tests on Windows:

```
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build . && ctest -V
```

## License

UcTerm is licensed under the MIT License. See [LICENSE](LICENSE) for details.
