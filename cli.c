#include "cli.h"
#include "ucterm.h"
#include <stdint.h>
#include <string.h>

// Number of avaiable commands
#define MAX_CLI_COMMANDS 2
// TODO modify according to your needs

// CLI command handler function type
typedef void (*CliCommandHandle_t)(uint8_t argc, const uint8_t *argv[]);

// CLI command definition type
typedef struct
{
    const char *name;           // command name
    CliCommandHandle_t handler; // function to execute
    const char *help;           // short help string
} CliCommand_t;

/* Command handler fuction prototypes (internal) */
static void cmd_help(uint8_t argc, const uint8_t *argv[]);
static void cmd_uname(uint8_t argc, const uint8_t *argv[]);
// TODO add yours

/* Command handler fuction prototypes (external) */
// TODO add your definitions to cli.h and implement outside

/* Ucterm callback fuction prototypes - interface-specific */
static inline uint8_t _uart_get_char(void);            // TODO your implementation
static inline void _uart_send_char(uint8_t c);         // TODO your implementation
static inline void _uart_send_str(const uint8_t *str); // TODO your implementation
static inline void _execute(uint8_t argc, uint8_t *argv[]);

/* Internal state storage */

static UcTerm_HandleTypeDef _hterm;

static const CliCommand_t _commands[MAX_CLI_COMMANDS] = {
    {
        "help",
        cmd_help,
        "List available commands or show details with \x1B[1mhelp <command>\x1B[0m.",
    },
    {
		"uname",
		cmd_uname,
		"Display system info."
	},
    // TODO add your commands here
};

/* Public interface implementation */

void CliInit(void)
{
    UcTerm_Init(&_hterm);
    UcTerm_RegisterPrintCharCallback(&_hterm, &_uart_send_char);
    UcTerm_RegisterPrintStrCallback(&_hterm, &_uart_send_str);
    UcTerm_RegisterExecuteCallback(&_hterm, &_execute);
    UcTerm_ShowPrompt(&_hterm);
}

void CliUpdate(void)
{
    // read char from UART RX buffer and pass it to Ucterm
    uint8_t c = _uart_get_char();
    if ('\0' == c)
    {
        return;
    }
    UcTerm_IngestChar(&_hterm, c);
}

/* Private functions implementation */

static inline void _execute(uint8_t argc, uint8_t *argv[])
{
    if (argc == 0)
    {
        return;
    }
    for (uint8_t i = 0; i < MAX_CLI_COMMANDS; i++)
    {
        // first symbol pre-filter
        if (argv[0][0] != _commands[i].name[0])
        {
            continue;
        }
        // call handler on command name match
        // or show help if requested
        if (strcmp((char *)argv[0], _commands[i].name) == 0)
        {
            if (argc == 2 && argv[1][0] == '-')
            {
                if (strcmp((char *)argv[1], "-h") == 0 ||
                    strcmp((char *)argv[1], "--help") == 0)
                {
                    _uart_send_str(_commands[i].help);
                    return;
                }
            }
            _commands[i].handler(argc, (const uint8_t **)argv);
            return;
        }
    }
    _uart_send_str("Unknown command!");
}

static inline uint8_t _uart_get_char(void)
{
    // TODO read a char from UART or
    // whatever interface you use
}

static inline void _uart_send_char(uint8_t c)
{
    // TODO send ch via UART or
    // whatever interface you use
}

static inline void _uart_send_str(const uint8_t *str)
{
    // TODO send *str via UART or
    // whatever interface you use
}

/* Command handlers implementation */

static void cmd_help(uint8_t argc, const uint8_t *argv[])
{
    // if "help <command>" - print help of a particular command;
    // else list available commands
    if (argc == 2)
    {
        for (uint8_t i = 0; i < MAX_CLI_COMMANDS; i++)
        {
            if (strcmp((char *)argv[1], _commands[i].name) == 0)
            {
                _uart_send_str(_commands[i].help);
                return;
            }
        }
    }
    _uart_send_str("Available commands:\x1B[1m");
    for (uint8_t i = 0; i < MAX_CLI_COMMANDS; i++)
    {
        _uart_send_char('\t');
        _uart_send_str(_commands[i].name);
    }
    _uart_send_str("\x1B[0m\r\nTry \x1B[1m-h\x1B[0m, \x1B[1m--help\x1B[0m or \x1B[1mhelp <command>\x1B[0m for details.");
}

static void cmd_uname(uint8_t argc, const uint8_t *argv[])
{
    _uart_send_str("Hello world!\r\n");
}
