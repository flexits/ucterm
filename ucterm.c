#include "ucterm.h"
#include <string.h>

/* Output strings to be printed */
#define OUT_NEWLINE_STR ((uint8_t *)"\r\n")
#define OUT_UNKNOWN_STR ((uint8_t *)"\r\n?\r\n>")
#define OUT_PROMPT_STR ((uint8_t *)"\x1B[0m\r\n>")
// Note: the prompt char is '>' and you may use another.
// Keep in mind that the prompt width of the OUT_PROMPT_STR is one visible char.
// If you increase this, you also need to make corrections to
// PROMPT_WIDTH and OUT_CHA_2.

// how many visible characters does the cli prompt contain
#define PROMPT_WIDTH 1

/* Terminal interaction commands */
#define OUT_CHA_2 ((uint8_t *)"\x1B[2G") // move cursor to the 2nd column
#define OUT_L_ARROW ((uint8_t *)"\x1B[D")
#define OUT_R_ARROW ((uint8_t *)"\x1B[C")
#define OUT_ERASE_END ((uint8_t *)"\x1B[K")

/* ESC-sequence characters */
#define ESC_HEADER 0x1B
#define ESC_SEPRTR '['

/* Special characters */
#define KEY_ENTER_LF '\n'
#define KEY_ENTER_CR '\r'
#define KEY_BACKSPACE 0x08
#define KEY_DELETE 0x7F

/* Ctrl+ sequences */
#define CTRL_J 0x0A // Line Feed
#define CTRL_M 0x0D // Carriage Return
#define CTRL_H 0x08 // Backspace
#define CTRL_A 0x01 // Home
#define CTRL_E 0x05 // End
#define Ctrl_B 0x02 // Left arrow
#define Ctrl_F 0x06 // Right arrow
#define CTRL_K 0x0B // Delete to end of line
#define CTRL_U 0x15 // Delete to beginning of line

/* State storage */

// Maximum input line length
// (one byte is always reserved for termination).
#define MAX_STR_LEN 120

// Maximum ESC code length
// (without the ESC symbol itself).
#define MAX_ESC_LEN 4

// Maximum number of cli arguments
// (including the command itself).
#define MAX_ARG_COUNT 4

typedef struct
{
  uint8_t *argv[MAX_ARG_COUNT]; // pointers to arguments
  void (*printChr)(uint8_t);
  void (*printStr)(const uint8_t *);
  void (*exec)(uint8_t, uint8_t **);
  uint8_t buf[MAX_STR_LEN];     // input characters buffer
  uint8_t esc_buf[MAX_ESC_LEN]; // ESC-sequence buffer
  uint8_t esc_index;            // ESC-sequence buffer write index
  uint8_t index;                // input buffer write index
  uint8_t length;               // length of the input buffer w/o terminator
  uint8_t argc;                 // count of parsed arguments

} UcTermState_t;

_Static_assert(sizeof(UcTermState_t) <= UCTERM_STORAGE_SIZE,
               "UCTERM_STORAGE_SIZE too small");

/* Internal storage function prototypes */

// Cast externally allocated storage to internal struct.
static inline UcTermState_t *ucterm_internal(UcTerm_HandleTypeDef *self);

// Reset the input buffer index, length, and terminate it.
static inline void _reset_buf(UcTermState_t *self);

// Reset the ESC-sequence buffer and index.
static inline void _reset_esc_buf(UcTermState_t *self);

// Shift the input buffer to the left, overwriting the buf[position] symbol
// and decrease the buffer length by 1.
static inline void _shift_buf_left(UcTermState_t *self, uint8_t position);

// Shift the input buffer to the right, creating a new symbol at buf[position]
// and increase the buffer length by 1.
// Returns the count of new symbols (1 on success, 0 on length limit).
static inline uint8_t _shift_buf_right(UcTermState_t *self, uint8_t position);

/* Strings helper function prototypes */

// Split string into whitespace-separated tokens.
//
// Returns the number of tokens found.
//
// Modifies the supplied argv buffer,
// filling it with pointers to buf contents.
//
// Modifies the original buffer buf,
// replacing whitespaces with '\0'.
//
// Relies on MAX_ARG_COUNT internally,
// argv must be of sufficient capacity.
//
// If there's less tokens then argv capacity,
// the unused argv elements aren't modified.
static inline uint8_t _tokenize(uint8_t *buf, uint8_t *argv[]);

/* Terminal interaction function prototypes */

// Overwrite the current line on the terminal starting with current index
// and move the cursor back to match the index.
static inline void _overwrite_terminal_line(UcTermState_t *self);

// Generate ESC-sequence to move the terminal cursor to
// match the specified index in the buffer.
static inline uint8_t *_get_move_command(uint8_t index);

/* Input handlers */

// Move the cli cursor and the buffer index to the starting position.
static inline void _process_home(UcTermState_t *self);

// Move the cli cursor and the buffer index to the last position.
static inline void _process_end(UcTermState_t *self);

// Move the cli cursor and the buffer index one char back.
static inline void _process_left_arrow(UcTermState_t *self);

// Move the cli cursor and the buffer index one char forth.
static inline void _process_right_arrow(UcTermState_t *self);

// Delete the symbol before cursor and display changes.
static inline void _process_delete(UcTermState_t *self);

/* Public interface implementation */

void UcTerm_Init(UcTerm_HandleTypeDef *self)
{
  UcTermState_t *ctx = ucterm_internal(self);
  memset(ctx, 0, sizeof(UcTermState_t));
  ctx->printChr = NULL;
  ctx->printStr = NULL;
  ctx->exec = NULL;
}

void UcTerm_RegisterPrintCharCallback(UcTerm_HandleTypeDef *self,
                                      void (*printChr)(uint8_t))
{
  UcTermState_t *ctx = ucterm_internal(self);
  ctx->printChr = printChr;
}

void UcTerm_RegisterPrintStrCallback(UcTerm_HandleTypeDef *self,
                                     void (*printStr)(const uint8_t *))
{
  UcTermState_t *ctx = ucterm_internal(self);
  ctx->printStr = printStr;
}

void UcTerm_RegisterExecuteCallback(UcTerm_HandleTypeDef *self,
                                    void (*execute)(uint8_t, uint8_t **))
{
  UcTermState_t *ctx = ucterm_internal(self);
  ctx->exec = execute;
}

void UcTerm_ShowPrompt(UcTerm_HandleTypeDef *self)
{
  UcTermState_t *ctx = ucterm_internal(self);
  ctx->printStr(OUT_PROMPT_STR);
}

void UcTerm_IngestChar(UcTerm_HandleTypeDef *self, uint8_t c)
{
  UcTermState_t *ctx = ucterm_internal(self);

  // check if this is an ESC sequence
  // (the first element of the esc_buf is used as a state switch):
  if (ESC_HEADER == c)
  {
    ctx->esc_buf[0] = ESC_HEADER;
    return;
  }
  if (ESC_SEPRTR == c)
  {
    if (ESC_HEADER == ctx->esc_buf[0])
    {
      ctx->esc_buf[0] = ESC_SEPRTR;
      ctx->esc_index = 1;
      return;
    }
    else
    {
      _reset_esc_buf(ctx);
    }
  }
  if (ESC_SEPRTR == ctx->esc_buf[0])
  {
    // ESC sequence detected:
    // check total length
    if (MAX_ESC_LEN <= ctx->esc_index)
    {
      // sequence is too long, discard the buffer
      ctx->printStr(OUT_UNKNOWN_STR);
      _reset_esc_buf(ctx);
      return;
    }
    // ingest the symbol
    ctx->esc_buf[ctx->esc_index++] = c;
    // if the last byte is in the range 0x40–0x7E
    // then the sequence is terminated, process it
    if (0x40 <= c && 0x7E >= c)
    {
      // [D  Arrow left
      // [C  Arrow right
      // [1~ Home key
      // [4~ End key
      // [3~ Delete key
      if (2 == ctx->esc_index)
      {
        if ('D' == c)
        {
          _process_left_arrow(ctx);
        }
        else if ('C' == c)
        {
          _process_right_arrow(ctx);
        }
      }
      else if (3 == ctx->esc_index && '~' == c)
      {
        if ('1' == ctx->esc_buf[1])
        {
          _process_home(ctx);
        }
        else if ('4' == ctx->esc_buf[1])
        {
          _process_end(ctx);
        }
        else if ('3' == ctx->esc_buf[1])
        {
          _process_delete(ctx);
        }
      }
      _reset_esc_buf(ctx);
    }
    return;
  }

  // process Enter
  if (KEY_ENTER_CR == c || KEY_ENTER_LF == c)
  {
    // early return if no input
    if (0 == ctx->length)
    {
      ctx->printStr(OUT_PROMPT_STR); // TODO proper prompt
      return;
    }
    // terminate the string
    ctx->buf[ctx->length] = '\0';
    // find tokens and invoke callback if any
    memset(ctx->argv, '\0', MAX_ARG_COUNT * sizeof(uint8_t *));
    ctx->argc = _tokenize(ctx->buf, ctx->argv);
    if (ctx->argc > 0)
    {
      ctx->printStr(OUT_NEWLINE_STR);
      ctx->exec(ctx->argc, ctx->argv);
    }
    // reset the buffer - get ready for a new input line
    _reset_buf(ctx);
    ctx->printStr(OUT_PROMPT_STR); // TODO proper prompt
    return;
  }

  // process Backspace
  if (KEY_BACKSPACE == c || KEY_DELETE == c)
  {
    if (0 == ctx->index)
    {
      return;
    }
    _shift_buf_left(ctx, ctx->index - 1);
    ctx->index--;
    ctx->printChr(c);
    if (ctx->index < ctx->length)
    {
      _overwrite_terminal_line(ctx);
    }
    return;
  }

  // Home
  if (CTRL_A == c)
  {
    _process_home(ctx);
    return;
  }

  // End
  if (CTRL_E == c)
  {
    _process_end(ctx);
    return;
  }

  // Left Arrow
  if (Ctrl_B == c)
  {
    _process_left_arrow(ctx);
  }

  // Right Arrow
  if (Ctrl_F == c)
  {
    _process_right_arrow(ctx);
  }

  // process Ctrl+U: Delete to the beginning of the line
  if (CTRL_U == c)
  {
    if (0 == ctx->index)
    {
      return;
    }
    memmove(&ctx->buf[0], &ctx->buf[ctx->index], ctx->length - ctx->index + 1);
    ctx->length -= ctx->index;
    ctx->index = 0;
    _process_home(ctx);
    _overwrite_terminal_line(ctx);
    return;
  }

  // process Ctrl+K: Delete to the end of the line
  if (CTRL_K == c)
  {
    if (ctx->index < ctx->length)
    {
      ctx->buf[ctx->index] = '\0';
      ctx->length = ctx->index;
      _overwrite_terminal_line(ctx);
    }
    return;
  }

  // check buffer length (one char is reserved for termination)
  if ((MAX_STR_LEN - 2) < ctx->index)
  {
    // input too long, show error
    ctx->printStr(OUT_UNKNOWN_STR); // TODO more meaningful error
    _reset_buf(ctx);
    return;
  }

  // store and echo printable characters
  if (0x20 <= c && 0x7E >= c)
  {
    if (ctx->index < ctx->length)
    {
      if (_shift_buf_right(ctx, ctx->index))
      {
        _overwrite_terminal_line(ctx);
        ctx->length--; // will be increased later TODO ugly
      }
      else
      {
        // buffer overflow, discard the character
        return;
      }
    }
    ctx->buf[ctx->index] = c;
    ctx->index++;
    ctx->length++;
    ctx->printChr(c);
    return;
  }
}

/* Private functions implementation */

static inline UcTermState_t *ucterm_internal(UcTerm_HandleTypeDef *self)
{
  return (UcTermState_t *)(self->storage);
}

static inline void _reset_buf(UcTermState_t *self)
{
  self->length = 0;
  self->index = 0;
  self->buf[0] = '\0';
  self->esc_buf[0] = '\0';
  self->esc_index = 0;
}

static inline void _reset_esc_buf(UcTermState_t *self)
{
  self->esc_buf[0] = '\0';
  self->esc_index = 0;
}

static inline void _shift_buf_left(UcTermState_t *self, uint8_t position)
{
  if (position >= self->length)
  {
    return;
  }
  memmove(&self->buf[position], &self->buf[position + 1],
          self->length - position);
  self->length--;
  self->buf[self->length] = '\0';
}

static inline uint8_t _shift_buf_right(UcTermState_t *self, uint8_t position)
{
  if ((MAX_STR_LEN - 2) < self->length)
  {
    return 0;
  }
  memmove(&self->buf[position + 1], &self->buf[position],
          self->length - position + 1);
  self->length++;
  self->buf[self->length] = '\0';
  return 1;
}

static inline uint8_t *_get_move_command(uint8_t index)
{
  // buffer to contain command sequence
  static uint8_t buffer[8] = {
      0x1B,
      '[',
  };
  // buffer write position
  uint8_t i = 2;
  // shortcut for the first column
  if (index == 0)
  {
    return OUT_CHA_2;
  }
  // prevent wrap and out-of-range columns
  if (index >= (255 - PROMPT_WIDTH))
  {
    index = 255 - PROMPT_WIDTH;
  }
  // column numbers start with 1, then
  // we reserve some space for the cli prompt
  index += (1 + PROMPT_WIDTH);
  // calculate hundreds
  if (index >= 100)
  {
    for (buffer[i] = '0'; index >= 100; index -= 100)
    {
      buffer[i]++;
    }
    i++;
  }
  // calculate tens
  if (index >= 10)
  {
    for (buffer[i] = '0'; index >= 10; index -= 10)
    {
      buffer[i]++;
    }
    i++;
  }
  // calculate ones
  buffer[i++] = '0' + index;
  // finalize the command sequence
  buffer[i++] = 'G';
  buffer[i++] = '\0';
  return buffer;
}

static inline uint8_t _tokenize(uint8_t *buf, uint8_t *argv[])
{
  uint8_t argc = 0;
  uint8_t isSubstrFound = 0;
  uint8_t isWhitespace = 0;

  for (uint8_t *p = buf; *p != '\0'; p++)
  {
    isWhitespace = *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
                   *p == '\v' || *p == '\f';
    if (isWhitespace)
    {
      if (isSubstrFound)
      {
        *p = '\0';
        isSubstrFound = 0;
      }
    }
    else
    {
      if (!isSubstrFound)
      {
        if (argc < MAX_ARG_COUNT)
        {
          argv[argc++] = p;
          isSubstrFound = 1;
        }
        else
        {
          break;
        }
      }
    }
  }
  return argc;
}

static inline void _overwrite_terminal_line(UcTermState_t *self)
{
  self->printStr(OUT_ERASE_END);
  self->printStr(&self->buf[self->index]);
  self->printStr(_get_move_command(self->index));
}

static inline void _process_home(UcTermState_t *self)
{
  self->index = 0;
  self->printStr(_get_move_command(self->index));
}

static inline void _process_end(UcTermState_t *self)
{
  self->index = self->length;
  self->printStr(_get_move_command(self->index));
}

static inline void _process_left_arrow(UcTermState_t *self)
{
  if (0 < self->index)
  {
    self->index--;
    self->printStr(OUT_L_ARROW);
  }
  else
  {
    self->printStr(OUT_CHA_2);
  }
}

static inline void _process_right_arrow(UcTermState_t *self)
{
  if (self->index < self->length)
  {
    self->index++;
    self->printStr(OUT_R_ARROW);
  }
}

static inline void _process_delete(UcTermState_t *self)
{
  if (self->index < self->length)
  {
    _shift_buf_left(self, self->index);
    _overwrite_terminal_line(self);
  }
}
