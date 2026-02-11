#include "./unity/unity.h"
#include "../ucterm.h"
#include <stdint.h>
#include <string.h>

// keyboard special keys
#define KEY_ENTER '\n'
#define KEY_SPACE 0x20
#define KEY_BACKSPACE 0x08
#define KEY_DELETE 0x7F

// ESC-sequence characters
#define ESC_HEADER 0x1B
#define ESC_SEPRTR '['

// Ctrl+ sequences
#define CTRL_J 0x0A // Line Feed
#define CTRL_M 0x0D // Carriage Return
#define CTRL_A 0x01 // Home
#define CTRL_E 0x05 // End
#define CTRL_K 0x0B // Delete to end of line
#define CTRL_U 0x15 // Delete to beginning of line
#define CTRL_W 0x17 // Delete previous word

static UcTerm_HandleTypeDef hucterm;

/* Output emulation */

#define MAX_ARG_COUNT 4
#define MAX_STR_LEN 120

uint8_t buff[MAX_STR_LEN];
uint8_t buff_index = 0;
uint8_t argc = 0;
uint8_t *argv[MAX_ARG_COUNT];

/* Callbacks */

void printChar(uint8_t c)
{
    if (buff_index < MAX_STR_LEN - 1)
    {
        buff[buff_index++] = c;
    }
}

void printStr(const uint8_t *s)
{
    strcpy_s(buff, MAX_STR_LEN, s);
}

void execute(uint8_t ac, uint8_t *av[])
{
    static uint8_t _buff[MAX_STR_LEN];
    uint8_t _index = 0;
    argc = ac;
    // copy *argv contents to _buff because the original
    // memory may be zeroed out after return from here
    for (uint8_t arg_index = 0; arg_index < argc && arg_index < MAX_ARG_COUNT; arg_index++)
    {
        size_t s_len = strlen(av[arg_index]) + 1;
        if (MAX_STR_LEN <= _index + s_len)
        {
            break;
        }
        argv[arg_index] = &_buff[_index];
        strcpy(&_buff[_index], av[arg_index]);
        _index += (uint8_t)s_len;
    }
}

/* Private helpers */

static inline void _ingest_string(uint8_t *s)
{
    for (uint8_t c = '\0'; (c = *(s++)) != '\0';)
    {
        UcTerm_IngestChar(&hucterm, c);
    }
}

/* Test section */

void setUp(void)
{
    buff_index = 0;
    argc = 0;
    memset(buff, '\0', MAX_STR_LEN);
    memset(argv, '\0', MAX_ARG_COUNT * sizeof(uint8_t *));
    UcTerm_Init(&hucterm);
    UcTerm_RegisterPrintCharCallback(&hucterm, &printChar);
    UcTerm_RegisterPrintStrCallback(&hucterm, &printStr);
    UcTerm_RegisterExecuteCallback(&hucterm, &execute);
}

void tearDown(void)
{
    // clean stuff up here
}

/*
void test_should_register_printChar_callback(void)
{
    hucterm.printChr = NULL;
    UcTerm_RegisterPrintCharCallback(&hucterm, &printChar);
    TEST_ASSERT_NOT_NULL(hucterm.printChr);
}

void test_should_register_printStr_callback(void)
{
    hucterm.printStr = NULL;
    UcTerm_RegisterPrintStrCallback(&hucterm, &printStr);
    TEST_ASSERT_NOT_NULL(hucterm.printStr);
}

void test_should_register_execute_callback(void)
{
    hucterm.exec = NULL;
    UcTerm_RegisterExecuteCallback(&hucterm, &execute);
    TEST_ASSERT_NOT_NULL(hucterm.exec);
}
*/

void test_should_echo_letter_char(void)
{
    uint8_t c = 'A';
    UcTerm_IngestChar(&hucterm, c);

    TEST_ASSERT_EQUAL_CHAR(c, buff[0]);
}

void test_should_echo_sign_char(void)
{
    uint8_t c = '}';
    UcTerm_IngestChar(&hucterm, c);

    TEST_ASSERT_EQUAL_CHAR(c, buff[0]);
}

void test_should_not_echo_control_char(void)
{
    uint8_t c = 0x15;
    UcTerm_IngestChar(&hucterm, c);

    TEST_ASSERT_EQUAL_CHAR('\0', buff[0]);
}

void test_should_echo_char_sequence(void)
{
    uint8_t *input = "ad[c]def";
    _ingest_string(input);

    TEST_ASSERT_EQUAL_STRING(input, buff);
}

void test_should_echo_space_separated_sequence(void)
{
    uint8_t *input = "ab c";
    _ingest_string(input);

    TEST_ASSERT_EQUAL_STRING(input, buff);
}

void test_should_tokenize_single_word(void)
{
    uint8_t *input = "comm";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_should_tokenize_two_words(void)
{
    uint8_t *input = "comm arg";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(2, argc);
    TEST_ASSERT_EQUAL_STRING("comm", argv[0]);
    TEST_ASSERT_EQUAL_STRING("arg", argv[1]);
}

void test_should_tokenize_max_words(void)
{
    uint8_t str[MAX_STR_LEN];
    uint8_t str_index = 4;
    strcpy(str, "comm");
    for (int i = 0; i < MAX_ARG_COUNT + 2; i++)
    {
        uint8_t s[MAX_STR_LEN];
        sprintf(s, " arg%d", i);
        strcpy(&str[str_index], s);
        str_index += strlen(s);
    }
    str[str_index] = '\0';

    _ingest_string(str);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(MAX_ARG_COUNT, argc);
    TEST_ASSERT_EQUAL_STRING(strtok(str, " "), argv[0]);
    for (int i = 1; MAX_ARG_COUNT > i; i++)
    {
        TEST_ASSERT_EQUAL_STRING(strtok(NULL, " "), argv[i]);
    }
}

void test_should_tokenize_two_words_with_mutiple_spaces(void)
{
    uint8_t *input = "comm      arg";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(2, argc);
    TEST_ASSERT_EQUAL_STRING("comm", argv[0]);
    TEST_ASSERT_EQUAL_STRING("arg", argv[1]);
}

void test_should_tokenize_two_words_and_trim_spaces(void)
{
    uint8_t *input = "   comm      arg ";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(2, argc);
    TEST_ASSERT_EQUAL_STRING("comm", argv[0]);
    TEST_ASSERT_EQUAL_STRING("arg", argv[1]);
}

void test_should_not_tokenize_blank_line(void)
{
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(0, argc);
}

void test_should_not_tokenize_spaces(void)
{
    UcTerm_IngestChar(&hucterm, KEY_SPACE);
    UcTerm_IngestChar(&hucterm, KEY_SPACE);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(0, argc);
}

void test_should_process_ctrl_j(void)
{
    uint8_t *input = "comm";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, CTRL_J);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_should_process_ctrl_m(void)
{
    uint8_t *input = "comm";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, CTRL_M);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_backspace_blank_line(void)
{
    UcTerm_IngestChar(&hucterm, KEY_BACKSPACE);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(0, argc);
}

void test_backspace_last_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, 'd');

    // press Backspace
    UcTerm_IngestChar(&hucterm, KEY_BACKSPACE);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_repeat_backspace_last_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);
    UcTerm_IngestChar(&hucterm, 'd');
    UcTerm_IngestChar(&hucterm, 'e');

    // press Backspace
    UcTerm_IngestChar(&hucterm, KEY_BACKSPACE);
    UcTerm_IngestChar(&hucterm, KEY_BACKSPACE);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_backspace_first_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);

    // move the cursor to position 0:
    // arrow left *3
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    // press Backspace
    UcTerm_IngestChar(&hucterm, KEY_BACKSPACE);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_backspace_middle_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');

    // arrow left
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    // press Backspace
    UcTerm_IngestChar(&hucterm, KEY_BACKSPACE);
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("ac", argv[0]);
}

void test_delete_last_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);

    // press Delete
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '3');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_delete_middle_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, 'd');

    // arrow left *2
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    // press Delete
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '3');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abd", argv[0]);
}

void test_delete_first_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');

    // arrow left *3
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    // press Delete
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '3');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("bc", argv[0]);
}

void test_insert_one_at_last_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'd');

    // arrow left
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abcd", argv[0]);
}

void test_insert_one_at_middle_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'f');

    // arrow left
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, 'd');
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("adbf", argv[0]);
}

void test_insert_one_at_first_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'e');

    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abe", argv[0]);
}

void test_insert_one_at_first_pos_excess_arr(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'e');

    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abe", argv[0]);
}

void test_insert_two_at_last_pos(void)
{
    UcTerm_IngestChar(&hucterm, 'a');
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'd');

    // arrow left
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, 'e');
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abced", argv[0]);
}

void test_insert_one_left_right_arr(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'e');
    UcTerm_IngestChar(&hucterm, 'f');

    // left arrow *2
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, 'a');

    // right arrow
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'C');

    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("baecf", argv[0]);
}

void test_home_insert(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, 'd');

    // press Home
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '1');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, 'a');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abcd", argv[0]);
}

void test_ctrl_a_insert(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, 'd');

    // press Ctrl+A
    UcTerm_IngestChar(&hucterm, CTRL_A);

    UcTerm_IngestChar(&hucterm, 'a');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abcd", argv[0]);
}

void test_home_end_insert(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, 'd');

    // press Home
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '1');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, 'a');

    // press End
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '4');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, 'e');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abcde", argv[0]);
}

void test_home_ctrl_e_insert(void)
{
    UcTerm_IngestChar(&hucterm, 'b');
    UcTerm_IngestChar(&hucterm, 'c');
    UcTerm_IngestChar(&hucterm, 'd');

    // press Home
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '1');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, 'a');

    // press Ctrl+E
    UcTerm_IngestChar(&hucterm, CTRL_E);

    UcTerm_IngestChar(&hucterm, 'e');

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("abcde", argv[0]);
}

/* CTRL_U should delete to beginning of line */

void test_ctrl_u_last_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);

    UcTerm_IngestChar(&hucterm, CTRL_U);

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(0, argc);
}

void test_ctrl_u_first_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);

    // press Home
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '1');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, CTRL_U);

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_ctrl_u_middle_pos(void)
{
    uint8_t *input = "abcd";
    _ingest_string(input);

    // left arrow *2
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, CTRL_U);

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("cd", argv[0]);
}

/* CTRL_K should delete to end of line */

void test_ctrl_k_last_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);

    UcTerm_IngestChar(&hucterm, CTRL_K);

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING(input, argv[0]);
}

void test_ctrl_k_first_pos(void)
{
    uint8_t *input = "abc";
    _ingest_string(input);

    // press Home
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, '1');
    UcTerm_IngestChar(&hucterm, '~');

    UcTerm_IngestChar(&hucterm, CTRL_K);

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(0, argc);
}

void test_ctrl_k_middle_pos(void)
{
    uint8_t *input = "abcd";
    _ingest_string(input);

    // left arrow *2
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');
    UcTerm_IngestChar(&hucterm, ESC_HEADER);
    UcTerm_IngestChar(&hucterm, ESC_SEPRTR);
    UcTerm_IngestChar(&hucterm, 'D');

    UcTerm_IngestChar(&hucterm, CTRL_K);

    UcTerm_IngestChar(&hucterm, KEY_ENTER);

    TEST_ASSERT_EQUAL_UINT8(1, argc);
    TEST_ASSERT_EQUAL_STRING("ab", argv[0]);
}

int main(void)
{
    UNITY_BEGIN();
    // RUN_TEST(test_should_register_printChar_callback);
    // RUN_TEST(test_should_register_printStr_callback);
    // RUN_TEST(test_should_register_execute_callback);
    RUN_TEST(test_should_echo_letter_char);
    RUN_TEST(test_should_echo_sign_char);
    RUN_TEST(test_should_echo_char_sequence);
    RUN_TEST(test_should_not_echo_control_char);
    RUN_TEST(test_should_echo_space_separated_sequence);
    RUN_TEST(test_should_tokenize_single_word);
    RUN_TEST(test_should_tokenize_two_words);
    RUN_TEST(test_should_tokenize_max_words);
    RUN_TEST(test_should_tokenize_two_words_with_mutiple_spaces);
    RUN_TEST(test_should_tokenize_two_words_and_trim_spaces);
    RUN_TEST(test_should_not_tokenize_blank_line);
    RUN_TEST(test_should_not_tokenize_spaces);
    RUN_TEST(test_should_process_ctrl_j);
    RUN_TEST(test_should_process_ctrl_m);
    RUN_TEST(test_backspace_blank_line);
    RUN_TEST(test_backspace_last_pos);
    RUN_TEST(test_repeat_backspace_last_pos);
    RUN_TEST(test_backspace_first_pos);
    RUN_TEST(test_backspace_middle_pos);
    RUN_TEST(test_delete_first_pos);
    RUN_TEST(test_delete_middle_pos);
    RUN_TEST(test_delete_last_pos);
    RUN_TEST(test_insert_one_at_last_pos);
    RUN_TEST(test_insert_one_at_middle_pos);
    RUN_TEST(test_insert_one_at_first_pos);
    RUN_TEST(test_insert_one_at_first_pos_excess_arr);
    RUN_TEST(test_insert_one_left_right_arr);
    RUN_TEST(test_insert_two_at_last_pos);
    RUN_TEST(test_home_insert);
    RUN_TEST(test_ctrl_a_insert);
    RUN_TEST(test_home_end_insert);
    RUN_TEST(test_home_ctrl_e_insert);
    RUN_TEST(test_ctrl_u_last_pos);
    RUN_TEST(test_ctrl_u_first_pos);
    RUN_TEST(test_ctrl_u_middle_pos);
    RUN_TEST(test_ctrl_k_last_pos);
    RUN_TEST(test_ctrl_k_first_pos);
    RUN_TEST(test_ctrl_k_middle_pos);
    return UNITY_END();
}
