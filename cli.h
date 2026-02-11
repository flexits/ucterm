/*
Command-line interface module - defines CLI commands
in an inner command table (add your own commands in the
implementation) and wraps the UcTerm terminal module,
coupling it with a physical interface of your choice
(provide your own implementation inside).

Supports -h, --help or "help <command>" to show info
on a specific command.

You MUST call CliInit before usage.

To ensure smooth CLI behavior without lags, make sure
that you call CliUpdate frequently enough (at least 3-5 Hz
for normal typing and 50Hz for press-and-hold key).

    Created on: Jan 22, 2026
        Author: Alexander Korostelin (4d.41.49.4c@gmail.com)
*/

#ifndef CLI_H_
#define CLI_H_

/// @brief Init the UcTerm wrapper.
/// This must be called prior to using any functions
/// of the package.
void CliInit(void);

/// @brief Consume the input character from the UART buffer
/// and pass it to the UcTerm wrapper.
/// Call this in a loop or a timer interrupt.
void CliUpdate(void);

#endif // CLI_H_
